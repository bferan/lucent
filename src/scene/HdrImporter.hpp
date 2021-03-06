#pragma once

#include "device/Device.hpp"
#include "scene/Scene.hpp"
#include "scene/Lighting.hpp"

namespace lucent
{

//! Processes HDRi images for use as environment lighting
class HdrImporter
{
public:
    explicit HdrImporter(Device* device);

    Environment Import(const std::string& hdrFile);

private:
    void RenderToCube(Pipeline* pipeline, Texture* src, Texture* dst, int dstLevel, uint32 size, float rough = 0.0f);
    void RenderToQuad(Pipeline* pipeline, Texture* dst, uint32 size);

private:
    Device* m_Device;
    Context* m_Context;

    Texture* m_OffscreenColor;
    Texture* m_OffscreenDepth;
    Framebuffer* m_Offscreen;

    Pipeline* m_RectToCube;
    Pipeline* m_GenIrradiance;
    Pipeline* m_GenSpecular;
    Pipeline* m_GenBRDF;
};

}