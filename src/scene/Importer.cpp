#include "Importer.hpp"

#include <iostream>

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "stb_image.h"

namespace lucent
{

Importer::Importer(Device* device, Pipeline* pipeline)
    : m_Device(device), m_Pipeline(pipeline)
{
}

Entity Importer::Import(Scene& scene, const std::string& modelFile)
{
    Clear();

    std::cout << "Importing " << modelFile << "\n";

    Assimp::Importer importer;
    importer.ReadFile(modelFile,
        aiProcess_Triangulate |
            aiProcess_EmbedTextures |
            aiProcess_FlipUVs |
            aiProcess_CalcTangentSpace);

    auto& modelScene = *importer.GetScene();

    ImportMaterials(scene, modelScene);
    ImportMeshes(scene, modelScene);
    auto root = ImportEntities(scene, modelScene);

    return root;
}

static Texture* ImportTexture(Device* device, const aiTexture* texture)
{
    if (!texture)
    {
        return device->m_DefaultTexture;
    }

    int reqChannels = 4;
    int x, y, n;
    auto imgData = stbi_load_from_memory(
        reinterpret_cast<const stbi_uc*>(texture->pcData),
        static_cast<int>(texture->mWidth), &x, &y, &n, reqChannels);

    auto importedTexture = device->CreateTexture(TextureInfo{
        .width = static_cast<uint32_t>(x),
        .height = static_cast<uint32_t>(y),
    }, x * y * reqChannels, imgData);

    stbi_image_free(imgData);

    return importedTexture;
}

void Importer::ImportMaterials(Scene& scene, const aiScene& model)
{
    aiString path;

    for (int i = 0; i < model.mNumMaterials; ++i)
    {
        auto& mat = *model.mMaterials[i];

        std::cout << "Model " << model.mName.C_Str() << " material " << i << ":\n";

        for (int p = 0; p < mat.mNumProperties; ++p)
        {
            auto& prop = *mat.mProperties[p];
            std::cout << "\t"
                      << prop.mKey.C_Str()
                      << ":  Semantic: "
                      << prop.mSemantic
                      << ";  Index: "
                      << prop.mIndex
                      << "; Type: "
                      << prop.mType;

            if (prop.mType == aiPTI_String)
            {
                aiString string;
                aiGetMaterialString(&mat, prop.mKey.C_Str(), prop.mSemantic, prop.mIndex, &string);
                std::cout << " S: " << string.C_Str();
            }
            std::cout << "\n";
        }

        // Create textures
        mat.GetTexture(aiTextureType_DIFFUSE, 0, &path);
        auto baseColTex = ImportTexture(m_Device, model.GetEmbeddedTexture(path.C_Str()));

        // Create descriptor set for material textures
        auto descSet = m_Device->CreateDescriptorSet(*m_Pipeline, 1);
        m_Device->WriteSet(descSet, 0, *baseColTex);

        m_MaterialSets.push_back(descSet);

    }
}

void Importer::ImportMeshes(Scene& scene, const aiScene& model)
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

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
            auto uv = mesh.mTextureCoords[0][v];

            vertices.emplace_back(Vertex{
                .position = { pos.x, pos.y, pos.z },
                .normal = { norm.x, norm.y, norm.z },
                .tangent = { tan.x, tan.y, tan.z },
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

        Mesh newMesh{ .numIndices = static_cast<uint32_t>(indices.size()) };

        VkDeviceSize vertSize = vertices.size() * sizeof(Vertex);
        VkDeviceSize indexSize = indices.size() * sizeof(uint32_t);

        newMesh.vertexBuffer = m_Device->CreateBuffer(BufferType::Vertex, vertSize);
        newMesh.indexBuffer = m_Device->CreateBuffer(BufferType::Index, indexSize);

        newMesh.vertexBuffer->Upload(vertices.data(), vertSize);
        newMesh.indexBuffer->Upload(indices.data(), indexSize);

        newMesh.descSet = m_MaterialSets[mesh.mMaterialIndex];

        m_MeshIndices.push_back(scene.meshes.size());
        scene.meshes.push_back(newMesh);
    }
}

Entity Importer::ImportEntities(Scene& scene, const aiScene& model)
{
    auto& node = *model.mRootNode;
    auto entity = scene.entities.Create();

    aiVector3D pos;
    aiQuaternion rot;
    aiVector3D scale;
    node.mTransformation.Decompose(scale, rot, pos);

    scene.transforms.Assign(entity, Transform{
        .rotation = { rot.x, rot.y, rot.z, rot.w },
        .position = { pos.x, pos.y, pos.z },
        .scale = scale.x
    });

    if (node.mNumMeshes > 0)
    {
        scene.meshInstances.Assign(entity, MeshInstance{ .meshIndex = m_MeshIndices[node.mMeshes[0]] });
    }

    return entity;
}

void Importer::Clear()
{
    m_MaterialSets.clear();
    m_MeshIndices.clear();
}

}
