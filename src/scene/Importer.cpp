#include "Importer.hpp"

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/pbrmaterial.h"
#include "stb_image.h"

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

    Assimp::Importer importer;
    importer.ReadFile(modelFile,
        aiProcess_Triangulate |
            aiProcess_EmbedTextures |
            aiProcess_FlipUVs |
            aiProcess_CalcTangentSpace);

    auto& modelScene = *importer.GetScene();

    ImportMaterials(scene, modelScene);
    ImportMeshes(scene, modelScene);
    auto root = ImportEntities(scene, modelScene, *modelScene.mRootNode, Entity{});

    return root;
}

static Texture* ImportTexture(Device* device, const aiTexture* texture,
    aiReturn result, Texture* defaultTex, bool linear = true)
{
    if (result != aiReturn_SUCCESS || !texture)
    {
        return defaultTex;
    }

    int reqChannels = 4;
    int x, y, n;
    auto imgData = stbi_load_from_memory(
        reinterpret_cast<const stbi_uc*>(texture->pcData),
        static_cast<int>(texture->mWidth), &x, &y, &n, reqChannels);
    LC_ASSERT(imgData);

    auto importedTexture = device->CreateTexture(TextureSettings{
        .width = static_cast<uint32>(x),
        .height = static_cast<uint32>(y),
        .format = linear ? TextureFormat::kRGBA8 : TextureFormat::kRGBA8_SRGB
    }, x * y * reqChannels, imgData);

    stbi_image_free(imgData);

    return importedTexture;
}

static Texture* ImportTexture(Device* device, std::string_view rootPath, const char* file,
    aiReturn result, Texture* defaultTex, bool linear = true)
{
    if (result != aiReturn_SUCCESS)
    {
        return defaultTex;
    }

    rootPath = rootPath.substr(0, rootPath.find_last_of("\\/") + 1);

    std::string path(rootPath);
    path += file;

    int reqChannels = 4;
    int x, y, n;
    auto imgData = stbi_load(path.c_str(), &x, &y, &n, reqChannels);
    LC_ASSERT(imgData);

    auto importedTexture = device->CreateTexture(TextureSettings{
        .width = static_cast<uint32>(x),
        .height = static_cast<uint32>(y),
        .format = linear ? TextureFormat::kRGBA8 : TextureFormat::kRGBA8_SRGB
    }, x * y * reqChannels, imgData);

    stbi_image_free(imgData);

    return importedTexture;
}

void Importer::ImportMaterials(Scene& scene, const aiScene& model)
{
    aiString path;

    for (int i = 0; i < model.mNumMaterials; ++i)
    {
        Material material{};
        auto& data = *model.mMaterials[i];

        // Create textures
        auto result = data.GetTexture(aiTextureType_DIFFUSE, 0, &path);
        material.baseColor = ImportTexture(m_Device, model.GetEmbeddedTexture(path.C_Str()),
            result, m_Device->m_BlackTexture, false);

        // 'Unknown' texture type needs special handling as it is excluded from embed post-processing
        result = data.GetTexture(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE, &path);
        auto unknown = model.GetEmbeddedTexture(path.C_Str());
        material.metalRough = unknown ?
            ImportTexture(m_Device, unknown, result, m_Device->m_GreenTexture) :
            ImportTexture(m_Device, m_ModelFile, path.C_Str(), result, m_Device->m_GreenTexture);

        result = data.GetTexture(aiTextureType_NORMALS, 0, &path);
        material.normalMap = ImportTexture(m_Device, model.GetEmbeddedTexture(path.C_Str()),
            result, m_Device->m_GrayTexture);

        result = data.GetTexture(aiTextureType_LIGHTMAP, 0, &path);
        material.aoMap = ImportTexture(m_Device, model.GetEmbeddedTexture(path.C_Str()), result,
            m_Device->m_WhiteTexture);

        result = data.GetTexture(aiTextureType_EMISSIVE, 0, &path);
        material.emissive = ImportTexture(m_Device, model.GetEmbeddedTexture(path.C_Str()), result,
            m_Device->m_BlackTexture, false);

        // GLTF material parameters
        if (aiColor4D c; data.Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_BASE_COLOR_FACTOR, c) == aiReturn_SUCCESS)
        {
            material.baseColorFactor = { c.r, c.g, c.b, c.a };
        }
        data.Get(AI_MATKEY_METALLIC_FACTOR, material.metallicFactor);
        data.Get(AI_MATKEY_ROUGHNESS_FACTOR, material.roughnessFactor);

        m_MaterialIndices.push_back(scene.materials.size());
        scene.materials.emplace_back(material);
    }
}

void Importer::ImportMeshes(Scene& scene, const aiScene& model)
{
    std::vector<Vertex> vertices;
    std::vector<uint32> indices;

    for (int i = 0; i < model.mNumMeshes; ++i)
    {
        vertices.clear();
        indices.clear();

        auto& mesh = *model.mMeshes[i];

        vertices.reserve(mesh.mNumVertices);
        for (int v = 0; v < mesh.mNumVertices; ++v)
        {
            auto pos = mesh.mVertices[v];
            auto norm = mesh.mNormals[v];
            auto tan = mesh.mTangents[v];
            auto bit = mesh.mBitangents[v];
            auto uv = mesh.mTextureCoords[0][v];

            vertices.emplace_back(Vertex{
                .position = { pos.x, pos.y, pos.z },
                .normal = { norm.x, norm.y, norm.z },
                .tangent = { tan.x, tan.y, tan.z },
                .bitangent = { bit.x, bit.y, bit.z },
                .texCoord0 = { uv.x, uv.y }
            });
        }

        indices.reserve(3 * mesh.mNumFaces);
        for (int f = 0; f < mesh.mNumFaces; ++f)
        {
            auto& face = mesh.mFaces[f];
            indices.push_back(face.mIndices[0]);
            indices.push_back(face.mIndices[1]);
            indices.push_back(face.mIndices[2]);
        }

        Mesh newMesh{ .numIndices = static_cast<uint32>(indices.size()) };

        VkDeviceSize vertSize = vertices.size() * sizeof(Vertex);
        VkDeviceSize indexSize = indices.size() * sizeof(uint32);

        newMesh.vertexBuffer = m_Device->CreateBuffer(BufferType::Vertex, vertSize);
        newMesh.indexBuffer = m_Device->CreateBuffer(BufferType::Index, indexSize);

        newMesh.vertexBuffer->Upload(vertices.data(), vertSize);
        newMesh.indexBuffer->Upload(indices.data(), indexSize);

        newMesh.materialIdx = m_MaterialIndices[mesh.mMaterialIndex];

        m_MeshIndices.push_back(scene.meshes.size());
        scene.meshes.push_back(newMesh);
    }
}

Entity Importer::ImportEntities(Scene& scene, const aiScene& model, const aiNode& node, Entity parent)
{
    auto entity = scene.entities.Create();

    aiVector3D pos;
    aiQuaternion rot;
    aiVector3D scale;
    node.mTransformation.Decompose(scale, rot, pos);

    scene.transforms.Assign(entity, Transform{
        .rotation = { rot.x, rot.y, rot.z, rot.w },
        .position = { pos.x, pos.y, pos.z },
        .scale = scale.x,
        .parent = parent
    });

    if (node.mNumMeshes > 0)
    {
        std::vector<uint32> meshes(node.mNumMeshes);
        for (int i = 0; i < meshes.size(); ++i)
            meshes[i] = m_MeshIndices[node.mMeshes[i]];

        scene.meshInstances.Assign(entity, MeshInstance{ .meshes = std::move(meshes) });
    }

    if (node.mNumChildren > 0 || node.mNumMeshes > 1)
    {
        Parent parentComponent(node.mNumChildren);

        // Import actual children
        for (int i = 0; i < node.mNumChildren; ++i)
        {
            parentComponent.children[i] = ImportEntities(scene, model, *node.mChildren[i], entity);
        }

        scene.parents.Assign(entity, std::move(parentComponent));
    }

    return entity;
}

void Importer::Clear()
{
    m_ModelFile = {};
    m_MeshIndices.clear();
    m_MaterialIndices.clear();
}

}
