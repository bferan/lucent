#pragma once

#include "VulkanCommon.hpp"
#include "device/vulkan/VulkanShader.hpp"
#include "device/Pipeline.hpp"

namespace lucent
{

class VulkanPipeline : public Pipeline
{
public:
    VulkanPipeline(VulkanDevice* device, VulkanShader* shader, const PipelineSettings& settings);
    ~VulkanPipeline();

    VulkanPipeline& operator=(VulkanPipeline&& other) noexcept;

    Descriptor* Lookup(DescriptorID id) const override;

private:
    void InitGraphics();
    void InitCompute();

public:
    VulkanDevice* device;
    VulkanShader* shader;
    VkPipeline handle{};
};

}
