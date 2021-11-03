
target_include_directories(lucent PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_precompile_headers(lucent PUBLIC "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/core/Core.hpp>")

target_sources(lucent PRIVATE
        core/Array.hpp
        core/Color.hpp
        core/Hash.hpp
        core/Log.cpp
        core/Log.hpp
        core/Lucent.hpp
        core/Math.hpp
        core/Matrix4.hpp
        core/Pool.hpp
        core/Quaternion.hpp
        core/Vector3.hpp
        core/Vector4.hpp
        core/Utility.cpp
        core/Utility.hpp

        debug/DebugConsole.cpp
        debug/DebugConsole.hpp
        debug/Font.cpp
        debug/Font.hpp
        debug/TextMesh.cpp
        debug/TextMesh.hpp
        debug/Input.cpp
        debug/Input.hpp
        debug/Keys.hpp

        device/Context.cpp
        device/Context.hpp
        device/Descriptor.hpp
        device/Device.hpp
        device/Framebuffer.hpp
        device/Texture.hpp
        device/Pipeline.hpp

        device/vulkan/ResourceLimits.hpp
        device/vulkan/ShaderCache.cpp
        device/vulkan/ShaderCache.hpp
        device/vulkan/VulkanBuffer.cpp
        device/vulkan/VulkanBuffer.hpp
        device/vulkan/VulkanContext.cpp
        device/vulkan/VulkanContext.hpp
        device/vulkan/VulkanDevice.hpp
        device/vulkan/VulkanDevice.cpp
        device/vulkan/VulkanFramebuffer.cpp
        device/vulkan/VulkanFramebuffer.hpp
        device/vulkan/VulkanPipeline.hpp
        device/vulkan/VulkanShader.cpp
        device/vulkan/VulkanShader.hpp
        device/vulkan/VulkanSwapchain.cpp
        device/vulkan/VulkanSwapchain.hpp
        device/vulkan/VulkanTexture.cpp
        device/vulkan/VulkanTexture.hpp

        features/AmbientOcclusionPass.cpp
        features/AmbientOcclusionPass.hpp
        features/DebugOverlayPass.cpp
        features/DebugOverlayPass.hpp
        features/GeometryPass.cpp
        features/GeometryPass.hpp
        features/LightingPass.cpp
        features/LightingPass.hpp
        features/MomentShadowPass.cpp
        features/MomentShadowPass.hpp
        features/PostProcessPass.cpp
        features/PostProcessPass.hpp
        features/ScreenSpaceReflectionsPass.cpp
        features/ScreenSpaceReflectionsPass.hpp

        rendering/Geometry.cpp
        rendering/Geometry.hpp
        rendering/Renderer.cpp
        rendering/Renderer.hpp
        rendering/Engine.cpp
        rendering/Engine.hpp
        rendering/RenderSettings.hpp
        rendering/RenderSettings.cpp
        rendering/View.cpp
        rendering/View.hpp

        scene/Components.cpp
        scene/Components.hpp
        scene/ComponentPool.hpp
        scene/EntityIDPool.hpp
        scene/HdrImporter.cpp
        scene/HdrImporter.hpp
        scene/Importer.cpp
        scene/Importer.hpp
        scene/Lights.hpp
        scene/Scene.hpp
        scene/Scene.cpp
        scene/Entity.hpp
        )
