#include "Importer.hpp"

#include "stb_image.h"
#include "tiny_gltf.h"
#include "mikktspace.h"
#include "weldmesh.h"

namespace gltf = tinygltf;

namespace lucent
{

Importer::Importer(Device* device)
    : m_Device(device)
{
}

Entity Importer::Import(Scene& scene, const std::string& modelFile)
{
    Clear();
    m_ModelFile = modelFile;

    LC_INFO("Importing {}", modelFile);

    gltf::Model model;
    gltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    auto result = loader.LoadBinaryFromFile(&model, &err, &warn, modelFile);
    LC_ASSERT(result);

    ImportMaterials(scene, model);
    ImportMeshes(scene, model);

    Entity lastRoot;
    for (auto& node : model.nodes)
    {
        lastRoot = ImportEntities(scene, model, node, Entity{});
    }
    return lastRoot;
}

static Texture* ImportTexture(Device* device, const gltf::Model& model, const gltf::TextureInfo& textureInfo,
    Texture* defaultTexture, bool linear = true)
{
    if (textureInfo.index < 0)
        return defaultTexture;

    auto& tex = model.textures[textureInfo.index];
    auto& img = model.images[tex.source];

    LC_ASSERT(img.bits == 8);
    LC_ASSERT(img.component == 4);

    auto imported = device->CreateTexture(TextureSettings{
        .width = uint32(img.width),
        .height = uint32(img.height),
        .format = linear ? TextureFormat::kRGBA8 : TextureFormat::kRGBA8_SRGB
    }, img.image.size(), img.image.data());

    return imported;
}

static Texture* ImportTexture(Device* device, const gltf::Model& model, const gltf::NormalTextureInfo& textureInfo,
    Texture* defaultTexture)
{
    gltf::TextureInfo info;
    info.index = textureInfo.index;
    info.texCoord = textureInfo.texCoord;
    return ImportTexture(device, model, info, defaultTexture, true);
}

static Texture* ImportTexture(Device* device, const gltf::Model& model, const gltf::OcclusionTextureInfo& textureInfo,
    Texture* defaultTexture)
{
    gltf::TextureInfo info;
    info.index = textureInfo.index;
    info.texCoord = textureInfo.texCoord;
    return ImportTexture(device, model, info, defaultTexture, true);
}

void Importer::ImportMaterials(Scene& scene, const gltf::Model& model)
{
    for (auto& data : model.materials)
    {
        Material material{};

        auto& pbr = data.pbrMetallicRoughness;

        // Create textures
        material.baseColorMap = ImportTexture(m_Device, model, pbr.baseColorTexture, m_Device->m_BlackTexture, false);
        material.metalRough = ImportTexture(m_Device, model, pbr.metallicRoughnessTexture, m_Device->m_GreenTexture);
        material.normalMap = ImportTexture(m_Device, model, data.normalTexture, m_Device->m_NormalTexture);
        material.aoMap = ImportTexture(m_Device, model, data.occlusionTexture, m_Device->m_WhiteTexture);
        material.emissive = ImportTexture(m_Device, model, data.emissiveTexture, m_Device->m_BlackTexture, false);

        // Material parameters
        auto& col = pbr.baseColorFactor;
        material.baseColorFactor = Color((float)col[0], (float)col[1], (float)col[2], (float)col[3]);
        material.metallicFactor = (float)pbr.metallicFactor;
        material.roughnessFactor = (float)pbr.roughnessFactor;

        m_MaterialIndices.push_back(scene.materials.size());
        scene.materials.emplace_back(material);
    }
}

// Helper to access data from glTF buffers
struct Accessor
{
public:
    Accessor(const gltf::Model& model, int index)
    {
        if (index >= 0)
        {
            auto& access = model.accessors[index];
            auto& view = model.bufferViews[access.bufferView];
            auto& buff = model.buffers[view.buffer];

            size = gltf::GetNumComponentsInType(access.type) * gltf::GetComponentSizeInBytes(access.componentType);
            total = access.count;
            count = 0;

            data = buff.data.data();
            data += view.byteOffset;
            data += access.byteOffset;

            stride = view.byteStride ? view.byteStride : size;
        }
    }

    template<typename T>
    T Next()
    {
        if (count < total)
        {
            LC_ASSERT(size <= sizeof(T));
            T value{};
            memcpy(&value, data + stride * count, size);
            ++count;
            return value;
        }
        else return {};
    }

public:
    size_t count{ 0 };
    size_t total{ 0 };
    size_t size{ 0 };
    size_t stride{ 0 };
    const uint8* data;
};

// http://www.mikktspace.com/
static void CalculateTangents(std::vector<Vertex>& vertices, std::vector<uint32>& indices)
{
    std::vector<Vertex> unindexed(indices.size());

    std::vector<Vertex> newVertices(indices.size());
    std::vector<uint32> newIndices(indices.size());

    struct TangentData
    {
        std::vector<Vertex>& vertices;
        std::vector<uint32>& indices;
        std::vector<Vertex>& unindexed;

        Vertex& GetVertex(int face, int vert)
        {
            auto idx = indices[3 * face + vert];
            return vertices[idx];
        }
    };

    auto data = TangentData{
        .vertices = vertices,
        .indices = indices,
        .unindexed = unindexed
    };

    SMikkTSpaceInterface interface{};
    interface.m_getNumFaces = [](auto context)
    {
        auto& data = *((TangentData*)context->m_pUserData);
        return (int)data.indices.size() / 3;
    };

    interface.m_getNumVerticesOfFace = [](auto context, auto face)
    {
        return 3; // Always triangles
    };

    interface.m_getPosition = [](auto context, auto out, auto face, auto vert)
    {
        auto& data = *((TangentData*)context->m_pUserData);
        *(Vector3*)out = data.GetVertex(face, vert).position;
    };

    interface.m_getNormal = [](auto context, auto out, auto face, auto vert)
    {
        auto& data = *((TangentData*)context->m_pUserData);
        *(Vector3*)out = data.GetVertex(face, vert).normal;
    };

    interface.m_getTexCoord = [](auto context, auto out, auto face, auto vert)
    {
        auto& data = *((TangentData*)context->m_pUserData);
        *(Vector2*)out = data.GetVertex(face, vert).texCoord0;
    };

    interface.m_setTSpaceBasic = [](auto context, auto tang, auto sign, auto face, auto vert)
    {
        auto& data = *((TangentData*)context->m_pUserData);
        auto v = data.GetVertex(face, vert);

        v.tangent = Vector4(tang[0], tang[1], tang[2], sign);

        data.unindexed[3 * face + vert] = v;
    };

    SMikkTSpaceContext context;
    context.m_pInterface = &interface;
    context.m_pUserData = &data;

    auto result = genTangSpaceDefault(&context);
    LC_ASSERT(result);

    auto vertexCount = WeldMesh((int*)newIndices.data(), (float*)newVertices.data(),
        (const float*)unindexed.data(), (int)unindexed.size(), sizeof(Vertex) / sizeof(float));

    newVertices.resize(vertexCount);

    vertices = std::move(newVertices);
    indices = std::move(newIndices);
}

void Importer::ImportMeshes(Scene& scene, const gltf::Model& model)
{
    std::vector<Vertex> vertices;
    std::vector<uint32> indices;

    for (auto& data : model.meshes)
    {
        MeshInstance instance;
        for (auto& primitive : data.primitives)
        {
            vertices.clear();
            indices.clear();

            auto getAttribute = [&](const char* attr)
            {
                auto it = primitive.attributes.find(attr);
                return it == primitive.attributes.end() ? -1 : it->second;
            };

            Accessor positions(model, getAttribute("POSITION"));
            Accessor normals(model, getAttribute("NORMAL"));
            Accessor tangents(model, getAttribute("TANGENT"));
            Accessor texCoords(model, getAttribute("TEXCOORD_0"));
            Accessor colors(model, getAttribute("COLOR_0"));

            vertices.reserve(positions.total);
            for (int v = 0; v < positions.total; ++v)
            {
                vertices.emplace_back(Vertex{
                    .position = positions.Next<Vector3>(),
                    .normal = normals.Next<Vector3>(),
                    .tangent = tangents.Next<Vector4>(),
                    .texCoord0 = texCoords.Next<Vector2>(),
                    .color = colors.Next<Color>()
                });
            }

            Accessor indexes(model, primitive.indices);
            indices.reserve(indexes.total);
            for (int i = 0; i < indexes.total; ++i)
            {
                indices.push_back(indexes.Next<uint32>());
            }

            // Calculate tangent space if missing tangents
            if (tangents.total == 0)
            {
                CalculateTangents(vertices, indices);
            }

            Mesh mesh{ .numIndices = static_cast<uint32>(indices.size()) };

            VkDeviceSize vertSize = vertices.size() * sizeof(Vertex);
            VkDeviceSize indexSize = indices.size() * sizeof(uint32);

            mesh.vertexBuffer = m_Device->CreateBuffer(BufferType::Vertex, vertSize);
            mesh.indexBuffer = m_Device->CreateBuffer(BufferType::Index, indexSize);

            mesh.vertexBuffer->Upload(vertices.data(), vertSize);
            mesh.indexBuffer->Upload(indices.data(), indexSize);

            if (primitive.material >= 0)
            {
                mesh.materialIdx = m_MaterialIndices[primitive.material];
            }
            else
            {
                // Create default material
                Material material{};
                material.baseColorMap = m_Device->m_WhiteTexture;
                material.metalRough = m_Device->m_GreenTexture;
                material.normalMap = m_Device->m_NormalTexture;
                material.aoMap = m_Device->m_WhiteTexture;
                material.emissive = m_Device->m_BlackTexture;

                material.baseColorFactor = Color::Gray();
                material.metallicFactor = 0.0f;
                material.roughnessFactor = 1.0f;

                mesh.materialIdx = scene.materials.size();
                scene.materials.emplace_back(material);
            }

            instance.meshes.push_back(scene.meshes.size());
            scene.meshes.push_back(mesh);
        }
        m_MeshInstances.push_back(std::move(instance));
    }
}

Entity Importer::ImportEntities(Scene& scene, const gltf::Model& model, const gltf::Node& node, Entity parent)
{
    auto entity = scene.CreateEntity();

    // Local transform
    Transform transform{};
    transform.parent = parent.id;

    if (node.matrix.empty())
    {
        // From separate translation, rotation & scale
        if (!node.translation.empty())
            transform.position = { (float)node.translation[0], (float)node.translation[1], (float)node.translation[2] };

        if (!node.rotation.empty())
            transform.rotation =
                { (float)node.rotation[0], (float)node.rotation[1], (float)node.rotation[2], (float)node.rotation[3] };

        if (!node.scale.empty())
            transform.scale = (float)node.scale[0];
    }
    else
    {
        // From TRS matrix
        auto& m = node.matrix;
        auto matrix = Matrix4{
            { (float)m[0], (float)m[1], (float)m[2], (float)m[3] },
            { (float)m[4], (float)m[5], (float)m[6], (float)m[7] },
            { (float)m[8], (float)m[9], (float)m[10], (float)m[11] },
            { (float)m[12], (float)m[13], (float)m[14], (float)m[15] }
        };
        Vector3 scale;
        matrix.Decompose(transform.position, transform.rotation, scale);
        transform.scale = scale.x;
    }

    entity.Assign(transform);

    if (node.mesh >= 0)
    {
        entity.Assign(m_MeshInstances[node.mesh]);
    }

    if (!node.children.empty())
    {
        Parent parentComponent;
        parentComponent.children.reserve(node.children.size());

        for (auto& i : node.children)
        {
            auto& child = model.nodes[i];
            parentComponent.children.push_back(ImportEntities(scene, model, child, entity).id);
        }
        entity.Assign(std::move(parentComponent));
    }

    return entity;
}

void Importer::Clear()
{
    m_ModelFile = {};
    m_MeshInstances.clear();
    m_MaterialIndices.clear();
}

}
