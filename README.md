
<p align="center">
<img width="400" src="https://user-images.githubusercontent.
com/25774113/140317991-d574013e-2761-4c99-a546-5272fb8645d3.png" alt="Vue logo">
</p>

<p align="center">
<img alt="GitHub" src="https://img.shields.io/github/license/bferan/lucent">
<img alt="GitHub release (latest by date)" src="https://img.shields.io/github/v/release/bferan/lucent?display_name=tag">
<img alt="Status: experimental" src="https://img.shields.io/badge/status-experimental-orange">
<img alt="Vulkan" src="https://img.shields.io/badge/-Vulkan-red?logo=Vulkan&logoColor=white">
<img alt="C++" src="https://img.shields.io/badge/-C++-blue?logo=cplusplus&logoColor=white">
</p>

# Lucent Rendering Engine

Lucent is my hobby project rendering engine built with C++ and Vulkan. It implements many modern realtime rendering
techniques atop an extensible hardware layer which will accommodate additional backends in the future.

## Features

- [x] Vulkan backend
- [x] gLTF support
- [x] Physically based shading
- [x] Image-based environment lighting
- [x] Analytical point and directional lights
- [x] Cascaded, moment-based shadow mapping
- [x] Screen-space reflections
- [x] Screen-space ground truth ambient occlusion (GTAO)
- [x] Postprocessing: HDR tonemapping, vignette
- [ ] Temporal antialiasing (WIP)
- [ ] Additional postprocessing effects (bloom, film grain...) (WIP)

## Screenshots

## Examples

#### High-level usage

```c++
```

#### Custom render pass

```c++
```

## Building

Requires a recent version of CMAKE (>= 3.17) and the [Vulkan SDK](https://www.lunarg.com/vulkan-sdk/) installed.
Additional dependencies are bundled with the repo as git submodules.

To build:

```shell
git clone --recursive https://github.com/bferan/lucent
```

When launching a Lucent application, runtime shaders are loaded from the `LC_SHADER_ROOT` environment variable. Set this
to the `./src/shaders` source directory to allow proper compilation of shaders.


