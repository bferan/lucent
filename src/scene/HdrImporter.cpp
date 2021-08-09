#include "HdrImporter.hpp"

#include "stb_image.h"

#include "core/Utility.hpp"
#include "device/Context.hpp"

namespace lucent
{

constexpr int kCubeSize = 1024;
constexpr int kIrradianceSize = 32;
constexpr int kSpecularSize = 256;
constexpr int kSpecularLevels = 6;
constexpr int kBRDFSize = 512;

HdrImporter::HdrImporter(Device* device)
    : m_Device(device)
{
    m_Context = m_Device->CreateContext();

    m_OffscreenColor = m_Device->CreateTexture(TextureSettings{
        .width = kCubeSize,
        .height = kCubeSize,
        .format = TextureFormat::kRGBA32F });

    m_OffscreenDepth = m_Device->CreateTexture(TextureSettings{
        .width = kCubeSize,
        .height = kCubeSize,
        .format = TextureFormat::kDepth
    });

    m_Offscreen = m_Device->CreateFramebuffer(FramebufferSettings{
        .colorTexture = m_OffscreenColor,
        .depthTexture = m_OffscreenDepth
    });

    m_RectToCube = m_Device->CreatePipeline(PipelineSettings{
        .shaderName = "RectToCube.shader",
        .framebuffer = m_Offscreen
    });

    m_GenIrradiance = m_Device->CreatePipeline(PipelineSettings{
        .shaderName = "IrradianceCube.shader",
        .framebuffer = m_Offscreen
    });

    m_GenSpecular = m_Device->CreatePipeline(PipelineSettings{
        .shaderName = "SpecularCube.shader",
        .framebuffer = m_Offscreen
    });

    m_GenBRDF = m_Device->CreatePipeline(PipelineSettings{
        .shaderName = "BRDF.shader",
        .framebuffer = m_Offscreen
    });

    m_UniformBuffer = m_Device->CreateBuffer(BufferType::UniformDynamic, 65535);
}

void HdrImporter::RenderToCube(Pipeline* pipeline, Texture* src, Texture* dst, int dstLevel, uint32 size)
{
    auto& ctx = *m_Context;
    auto proj = Matrix4::Perspective(kHalfPi, 1.0f, 0.001, 10000);

    Matrix4 views[] = {
        Matrix4::RotationY(kHalfPi),   // +x
        Matrix4::RotationY(-kHalfPi),  // -x
        Matrix4::RotationX(-kHalfPi),  // +y
        Matrix4::RotationX(kHalfPi),   // -y
        Matrix4::Identity(),           // +z
        Matrix4::RotationY(kPi)        // -z
    };

    const size_t uniformAlignment = m_Device->m_DeviceProperties.limits.minUniformBufferOffsetAlignment;
    uint32 uniformOffset = 0;

    ctx.Begin();
    for (int face = 0; face < LC_ARRAY_SIZE(views); ++face)
    {
        m_UBO.view = views[face];
        m_UBO.proj = proj;
        m_UniformBuffer->Upload(&m_UBO, sizeof(HdrUBO), uniformOffset);

        ctx.BeginRenderPass(*m_Offscreen, VkExtent2D{ size, size });
        ctx.Bind(pipeline);

        ctx.Bind(m_Device->m_Cube.indices);
        ctx.Bind(m_Device->m_Cube.vertices);

        ctx.Bind(0, 0, m_UniformBuffer, uniformOffset);
        ctx.Bind(0, 1, src);

        ctx.Draw(m_Device->m_Cube.numIndices);
        ctx.EndRenderPass();

        ctx.CopyTexture(
            m_OffscreenColor, 0, 0,
            dst, face, dstLevel,
            size, size);

        uniformOffset += sizeof(HdrUBO);
        // Align up
        uniformOffset += (uniformAlignment - (uniformOffset % uniformAlignment)) % uniformAlignment;
    }
    ctx.End();

    m_Device->Submit(m_Context);
}

void HdrImporter::RenderToQuad(Pipeline* pipeline, Texture* dst, uint32 size)
{
    auto& ctx = *m_Context;

    ctx.Begin();
    ctx.BeginRenderPass(*m_Offscreen, VkExtent2D{ size, size });
    ctx.Bind(pipeline);

    ctx.Bind(m_Device->m_Quad.indices);
    ctx.Bind(m_Device->m_Quad.vertices);

    ctx.Bind(0, 0, m_UniformBuffer, 0);

    ctx.Draw(m_Device->m_Quad.numIndices);
    ctx.EndRenderPass();

    ctx.CopyTexture(
        m_OffscreenColor, 0, 0,
        dst, 0, 0,
        size, size);

    ctx.End();

    m_Device->Submit(m_Context);
}

Environment HdrImporter::Import(const std::string& hdrFile)
{
    Environment env{};

    // Import rectangle HDR
    const int kDesiredChannels = 4;
    int x, y, numChannels;
    float* data = stbi_loadf(hdrFile.c_str(), &x, &y, &numChannels, kDesiredChannels);

    auto envRectangle = m_Device->CreateTexture(TextureSettings{
        .width = static_cast<uint32>(x),
        .height = static_cast<uint32>(y),
        .format = TextureFormat::kRGBA32F
    }, x * y * kDesiredChannels * sizeof(float), data);

    stbi_image_free(data);

    // Convert to cubemap
    env.cubeMap = m_Device->CreateTexture(TextureSettings{
        .width = kCubeSize,
        .height = kCubeSize,
        .format = TextureFormat::kRGBA32F,
        .shape = TextureShape::kCube
    });

    RenderToCube(m_RectToCube, envRectangle, env.cubeMap, 0, kCubeSize);

    // Generate irradiance map
    env.irradianceMap = m_Device->CreateTexture(TextureSettings{
        .width = kIrradianceSize,
        .height = kIrradianceSize,
        .format = TextureFormat::kRGBA32F,
        .shape = TextureShape::kCube
    });

    RenderToCube(m_GenIrradiance, env.cubeMap, env.irradianceMap, 0, kIrradianceSize);

    // Generate specular map
    env.specularMap = m_Device->CreateTexture(TextureSettings{
        .width = kSpecularSize,
        .height = kSpecularSize,
        .levels = kSpecularLevels,
        .format = TextureFormat::kRGBA32F,
        .shape = TextureShape::kCube
    });

    int dim = kSpecularSize;
    for (int level = 0; level < kSpecularLevels; ++level)
    {
        float rough = (float)level / (kSpecularLevels - 1);
        m_UBO.rough = rough;

        RenderToCube(m_GenSpecular, env.cubeMap, env.specularMap, level, dim);
        dim /= 2;
    }

    // Generate BRDF LUT
    env.BRDF = m_Device->CreateTexture(TextureSettings{
        .width = kBRDFSize,
        .height = kBRDFSize,
        .format = TextureFormat::kRGBA32F,
        .addressMode = TextureAddressMode::kClampToEdge
    });
    RenderToQuad(m_GenBRDF, env.BRDF, kBRDFSize);

    return env;
}

}
