
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

        device/Context.cpp
        device/Context.hpp
        device/DescriptorCache.cpp
        device/DescriptorCache.hpp
        device/Device.cpp
        device/Device.hpp
        device/Input.cpp
        device/Input.hpp
        device/Keys.hpp
        device/ResourceLimits.hpp
        device/Shader.cpp
        device/Shader.hpp
        device/ShaderCache.cpp
        device/ShaderCache.hpp

        rendering/SceneRenderer.cpp
        rendering/SceneRenderer.hpp

        scene/ComponentPool.hpp
        scene/EntityPool.hpp
        scene/HdrImporter.cpp
        scene/HdrImporter.hpp
        scene/Importer.cpp
        scene/Importer.hpp
        scene/Scene.hpp
        scene/Scene.cpp
        )
