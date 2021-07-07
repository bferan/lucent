#pragma once

#include <string>

#include "device/Device.hpp"
#include "core/Matrix4.hpp"
#include "scene/Scene.hpp"

namespace lucent
{

struct HdrUBO
{
    Matrix4 view;
    Matrix4 proj;
    float rough;
};

class HdrImporter
{
public:
    explicit HdrImporter(Device* device);

    Environment Import(const std::string& hdrFile);

private:
    void RenderToCube(Pipeline* pipeline, Texture* src, Texture* dst, int dstLevel, uint32_t size);
    void RenderToQuad(Pipeline* pipeline, Texture* dst, uint32_t size);

private:
    Device* m_Device;
    Context* m_Context;

    Texture* m_OffscreenColor;
    Texture* m_OffscreenDepth;
    Framebuffer* m_Offscreen;

    HdrUBO m_UBO;
    Buffer* m_UniformBuffer;
    DescriptorSet* m_DescSet;

    Pipeline* m_RectToCube;
    Pipeline* m_GenIrradiance;
    Pipeline* m_GenSpecular;
    Pipeline* m_GenBRDF;
};

}