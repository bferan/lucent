#pragma once

#include "device/Pipeline.hpp"
#include "VulkanCommon.hpp"

namespace lucent
{

class VulkanPipeline : public Pipeline
{
public:
    explicit VulkanPipeline(PipelineSettings settings)
    {
        this->settings = std::move(settings);
    }
    VkPipeline handle{};
};

}
