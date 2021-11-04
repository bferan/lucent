#include "Importer.hpp"

#include "stb_image.h"
#include "tiny_gltf.h"
#include "mikktspace.h"
#include "weldmesh.h"

#include "rendering/Geometry.hpp"
#include "rendering/PbrMaterial.hpp"
#include "scene/ModelInstance.hpp"
#include "scene/Transform.hpp"

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

    bool result = false;
    if (modelFile.ends_with(".glb"))
    {
        result = loader.LoadBinaryFromFile(&model, &err, &warn, modelFile);
    }
    else if (modelFile.ends_with(".gltf"))
    {
        result = loader.LoadASCIIFromFile(&model, &err, &warn, modelFile);
    }
    LC_ASSERT(result);

    ImportMaterials(scene, model);
    ImportMeshes(scene, model);

    // Load all entities from the default scene
    std::vector<Entity> rootEntities;
    if (model.defaultScene >= 0)
    {
        auto& nodeIndices = model.scenes[model.defaultScene].nodes;
        rootEntities.reserve(nodeIndices.size());
        for (auto nodeIndex: nodeIndices)
        {
            auto& node = model.nodes[nodeIndex];
            rootEntities.push_back(ImportEntities(scene, model, node, Entity{}));
        }

        const Quaternion flip = Quaternion::AxisAngle(Vector3::Up(), kPi);
        for (auto entity: rootEntities)
        {
            // Flip Z-axis to have -Z facing forward
            entity.SetRotation(flip * entity.GetRotation());
        }
    }
    return rootEntities.empty() ? Entity{} : rootEntities.back();
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
        .format = linear ? TextureFormat::kRGBA8 : TextureFormat::kRGBA8_sRGB,
        .generateMips = true
    });
    imported->Upload(img.image.size(), img.image.data());

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
    for (auto& data: model.materials)
    {
        auto material = std::make_unique<PbrMaterial>();

        auto& pbr = data.pbrMetallicRoughness;

        // Create textures
        material->baseColorMap = ImportTexture(m_Device, model, pbr.baseColorTexture, g_BlackTexture, false);
        material->metalRough = ImportTexture(m_Device, model, pbr.metallicRoughnessTexture, g_GreenTexture);
        material->normalMap = ImportTexture(m_Device, model, data.normalTexture, g_NormalTexture);
        material->aoMap = ImportTexture(m_Device, model, data.occlusionTexture, g_WhiteTexture);
        material->emissive = ImportTexture(m_Device, model, data.emissiveTexture, g_BlackTexture, false);

        // Material parameters
        auto& col = pbr.baseColorFactor;
        material->baseColorFactor = Color((float)col[0], (float)col[1], (float)col[2], (float)col[3]);
        material->metallicFactor = (float)pbr.metallicFactor;
        material->roughnessFactor = (float)pbr.roughnessFactor;

        m_ImportedMaterials.push_back(scene.AddMaterial(std::move(material)));
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
static void CalculateTangents(std::vector<Mesh::Vertex>& vertices, std::vector<uint32>& indices)
{
    std::vector<Mesh::Vertex> unindexed(indices.size());

    std::vector<Mesh::Vertex> newVertices(indices.size());
    std::vector<uint32> newIndices(indices.size());

    struct TangentData
    {
        std::vector<Mesh::Vertex>& vertices;
        std::vector<uint32>& indices;
        std::vector<Mesh::Vertex>& unindexed;

        Mesh::Vertex& GetVertex(int face, int vert)
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
        (const float*)unindexed.data(), (int)unindexed.size(), sizeof(Mesh::Vertex) / sizeof(float));

    newVertices.resize(vertexCount);

    vertices = std::move(newVertices);
    indices = std::move(newIndices);
}

void Importer::ImportMeshes(Scene& scene, const gltf::Model& gltfModel)
{
    Mesh mesh;
    for (auto& data: gltfModel.meshes)
    {
        auto model = std::make_unique<Model>();
        for (auto& primitive: data.primitives)
        {
            mesh.Clear();

            auto getAttribute = [&](const char* attr)
            {
                auto it = primitive.attributes.find(attr);
                return it == primitive.attributes.end() ? -1 : it->second;
            };

            Accessor positions(gltfModel, getAttribute("POSITION"));
            Accessor normals(gltfModel, getAttribute("NORMAL"));
            Accessor tangents(gltfModel, getAttribute("TANGENT"));
            Accessor texCoords(gltfModel, getAttribute("TEXCOORD_0"));
            Accessor colors(gltfModel, getAttribute("COLOR_0"));

            Accessor indexes(gltfModel, primitive.indices);

            mesh.Reserve(positions.total, indexes.total);

            for (int v = 0; v < positions.total; ++v)
            {
                mesh.AddVertex(Mesh::Vertex{
                    .position = positions.Next<Vector3>(),
                    .normal = normals.Next<Vector3>(),
                    .tangent = tangents.Next<Vector4>(),
                    .texCoord0 = texCoords.Next<Vector2>(),
                    .color = colors.Next<Color>()
                });
            }
            for (int i = 0; i < indexes.total; ++i)
            {
                mesh.AddIndex(indexes.Next<uint32>());
            }

            // Calculate tangent space if missing tangents
            if (tangents.total == 0)
            {
                CalculateTangents(mesh.vertices, mesh.indices);
            }

            auto material = (primitive.material >= 0) ?
                m_ImportedMaterials[primitive.material] :
                scene.GetDefaultMaterial();

            model->AddMesh(StaticMesh(m_Device, mesh), material);
        }
        m_ImportedMeshes.push_back(scene.AddModel(std::move(model)));
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
        entity.Assign(ModelInstance{ .model = m_ImportedMeshes[node.mesh] });
    }

    if (!node.children.empty())
    {
        Parent parentComponent;
        parentComponent.children.reserve(node.children.size());

        for (auto& i: node.children)
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
    m_ImportedMeshes.clear();
    m_ImportedMaterials.clear();
    m_ModelFile = {};
}

}
