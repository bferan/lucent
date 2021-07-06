
target_include_directories(lucent PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_sources(lucent PRIVATE
        core/Hash.hpp
        core/Lucent.hpp
        core/Math.hpp
        core/Matrix4.hpp
        core/Quaternion.hpp
        core/Vector3.hpp
        core/Vector4.hpp

        device/Device.cpp
        device/Device.hpp
        device/Input.cpp
        device/Input.hpp
        device/Keys.hpp
        device/ResourceLimits.hpp
        device/ShaderCache.cpp
        device/ShaderCache.hpp

        rendering/SceneRenderer.cpp
        rendering/SceneRenderer.hpp

        scene/ComponentPool.hpp
        scene/EntityPool.hpp
        scene/Importer.cpp
        scene/Importer.hpp
        scene/Scene.hpp
        scene/Scene.cpp
        )