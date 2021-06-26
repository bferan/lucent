#include "Basic.hpp"

#include <iostream>
#include <fstream>

#include "GLFW/glfw3.h"

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

    Vector3 vertices[] = {
        Vector3{ 0.0f, -1.0f, 0.0f },
        Vector3{ 1.0f, 0.0f, 0.0f },
        Vector3{ 1.0f, 1.0f, 0.0f },
    };

    uint32_t indices[] = { 0, 1, 2 };

    m_VertexBuffer = m_Device.CreateBuffer(BufferType::Vertex, sizeof(vertices));
    m_VertexBuffer->Upload(vertices, sizeof(vertices));

    m_IndexBuffer = m_Device.CreateBuffer(BufferType::Index, sizeof(indices));
    m_IndexBuffer->Upload(indices, sizeof(indices));

    m_Context = m_Device.CreateContext();
}

void BasicDemo::Draw()
{
    // Draw
    auto& ctx = *m_Context;
    auto& fb = m_Device.AcquireFramebuffer();

    ctx.BeginRenderPass(fb);

    ctx.Bind(*m_Pipeline);
    ctx.Bind(*m_VertexBuffer, *m_IndexBuffer);
    ctx.Draw(3);

    ctx.EndRenderPass();

    m_Device.Submit(&ctx);
    m_Device.Present();
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
