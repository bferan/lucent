#include "Basic.hpp"

#include <iostream>
#include <fstream>

#include "GLFW/glfw3.h"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "assimp/pbrmaterial.h"

#include "device/Device.hpp"
#include "core/Vector3.hpp"

namespace lucent::demos
{

static std::string ReadFile(const std::string& path)
{
    std::ifstream file(path);
    std::string buf;

    file.seekg(0, std::ios::end);
    buf.reserve(file.tellg());
    file.seekg(0, std::ios::beg);

    buf.assign((std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());

    return buf;
}

void BasicDemo::Init()
{
    m_Pipeline = m_Device.CreatePipeline(PipelineInfo{
        .vertexShader = ReadFile("C:/Code/lucent/src/rendering/vert.glsl"),
        .fragmentShader = ReadFile("C:/Code/lucent/src/rendering/frag.glsl")
    });

    ImportModel();

    m_Context = m_Device.CreateContext();

    // Create UBO
    m_UniformBuffer = m_Device.CreateBuffer(BufferType::Uniform, sizeof(UBO));

    UBO ubo = {
        .model = Matrix4::Identity(),
        .view = Matrix4::Translation(Vector3{0.0f, 0.0f, -5.0f}),
        .proj = Matrix4::Perspective(1, 1600.0f/900.0f, 0.01, 10000),
        .col = Vector3{0.0f, 0.0f, 1.0f}
    };

    m_UniformBuffer->Upload(&ubo, sizeof(UBO));

    m_DescSet = m_Context->CreateDescriptorSet(*m_Pipeline, 0);
    m_Context->WriteSet(m_DescSet, 0, *m_UniformBuffer);
}

void BasicDemo::Draw()
{
    // Draw
    auto& ctx = *m_Context;
    auto& fb = m_Device.AcquireFramebuffer();

    ctx.BeginRenderPass(fb);

    ctx.Bind(*m_Pipeline);
    ctx.BindSet(m_DescSet);
    ctx.Bind(*m_VertexBuffer, *m_IndexBuffer);
    ctx.Draw(m_numIndices);

    ctx.EndRenderPass();

    m_Device.Submit(&ctx);
    m_Device.Present();
}

void BasicDemo::ImportModel()
{
    Assimp::Importer importer;
    importer.ReadFile("models/DamagedHelmet.glb", aiProcess_Triangulate | aiProcess_EmbedTextures);

    auto model = importer.GetScene();
    auto& mesh = *model->mMeshes[0];

    std::vector<uint32_t> indices;
    indices.reserve(mesh.mNumFaces * 3);
    for (int i = 0; i < mesh.mNumFaces; ++i)
    {
        auto& face = mesh.mFaces[i];
        indices.push_back(face.mIndices[0]);
        indices.push_back(face.mIndices[1]);
        indices.push_back(face.mIndices[2]);
    }

    m_numIndices = indices.size();
    VkDeviceSize vertSize = mesh.mNumVertices * 3 * sizeof(float);
    VkDeviceSize indexSize = indices.size() * sizeof(uint32_t);

    m_VertexBuffer = m_Device.CreateBuffer(BufferType::Vertex, vertSize);
    m_IndexBuffer = m_Device.CreateBuffer(BufferType::Index, indexSize);

    m_VertexBuffer->Upload(mesh.mVertices, vertSize);
    m_IndexBuffer->Upload(indices.data(), indexSize);
}

}

int main()
{
    lucent::demos::BasicDemo demo;

    demo.Init();
    demo.Draw();

    while (!glfwWindowShouldClose(demo.m_Device.m_Window))
        glfwPollEvents();

    return 0;
}
