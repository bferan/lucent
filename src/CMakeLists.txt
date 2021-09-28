
target_include_directories(lucent PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_precompile_headers(lucent PUBLIC "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/core/Core.hpp>")

target_sources(lucent PRIVATE
        Engine.cpp
        Engine.hpp

        core/Array.hpp
        core/Color.hpp
        core/Hash.hpp
        core/Log.cpp
        core/Log.hpp
        core/Lucent.hpp
        core/Math.hpp
        core/Matrix4.hpp
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
        device/Device.hpp
        device/Framebuffer.hpp
        device/Shader.cpp
        device/Shader.hpp
        device/Texture.hpp

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
        device/vulkan/VulkanPipeline.cpp
        device/vulkan/VulkanPipeline.hpp
        device/vulkan/VulkanTexture.cpp
        device/vulkan/VulkanTexture.hpp

        rendering/AmbientOcclusionPass.cpp
        rendering/AmbientOcclusionPass.hpp
        rendering/DebugOverlayPass.cpp
        rendering/DebugOverlayPass.hpp
        rendering/Geometry.cpp
        rendering/Geometry.hpp
        rendering/GeometryPass.cpp
        rendering/GeometryPass.hpp
        rendering/LightingPass.cpp
        rendering/LightingPass.hpp
        rendering/MomentShadowPass.cpp
        rendering/MomentShadowPass.hpp
        rendering/PostProcessPass.cpp
        rendering/PostProcessPass.hpp
        rendering/Renderer.cpp
        rendering/Renderer.hpp
        rendering/ScreenSpaceReflectionsPass.cpp
        rendering/ScreenSpaceReflectionsPass.hpp
        rendering/RenderSettings.hpp
        rendering/RenderSettings.cpp

        scene/Components.cpp
        scene/Components.hpp
        scene/ComponentPool.hpp
        scene/EntityIDPool.hpp
        scene/HdrImporter.cpp
        scene/HdrImporter.hpp
        scene/Importer.cpp
        scene/Importer.hpp
        scene/Scene.hpp
        scene/Scene.cpp
        )
