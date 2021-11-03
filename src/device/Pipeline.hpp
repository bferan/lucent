#pragma once

#include "device/Framebuffer.hpp"
#include "device/Descriptor.hpp"

namespace lucent
{

enum class PipelineType
{
    kGraphics,
    kCompute
};

struct PipelineSettings
{
    std::string shaderName;
    PipelineType type = PipelineType::kGraphics;
    Framebuffer* framebuffer = nullptr;
    bool depthTestEnable = true;
    bool depthClampEnable = false;
};

class Pipeline
{
public:
    const PipelineSettings& GetSettings() const { return m_Settings; }

    PipelineType GetType() const { return GetSettings().type; };

    virtual Descriptor* Lookup(DescriptorID id) const = 0;

protected:
    PipelineSettings m_Settings;
};

}
