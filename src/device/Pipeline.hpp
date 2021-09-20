#pragma once

#include "device/Shader.hpp"
#include "device/Framebuffer.hpp"

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

struct Pipeline
{
    PipelineType type;
    PipelineSettings settings;
    Shader* shader{};
};

}
