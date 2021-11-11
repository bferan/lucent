<p align="center">
<img width="400" src="https://user-images.githubusercontent.com/25774113/140318368-10ecdd0a-0ea6-4a96-b3b5-aa7a540c84b5.png" alt="Lucent logo">
</p>

<p align="center">
<img alt="GitHub" src="https://img.shields.io/github/license/bferan/lucent">
<img alt="GitHub release (latest by date including pre-releases)" src="https://img.shields.io/github/v/release/bferan/lucent?include_prereleases">
<img alt="Status: experimental" src="https://img.shields.io/badge/status-experimental-orange">
<img alt="Vulkan" src="https://img.shields.io/badge/-Vulkan-red?logo=Vulkan&logoColor=white">
<img alt="C++" src="https://img.shields.io/badge/-C++-blue?logo=cplusplus&logoColor=white">
</p>

# Lucent Rendering Engine

Lucent is my hobby project rendering engine built with C++ and Vulkan. It implements many modern realtime rendering
techniques atop an extensible hardware layer which can accommodate additional backends in the future.

## Features

- [x] Vulkan backend
- [x] gLTF support
- [x] Physically based shading
- [x] Image-based environment lighting
- [x] Analytical point and directional lights
- [x] Cascaded, moment-based shadow mapping
- [x] Screen-space reflections
- [x] Screen-space ground truth ambient occlusion (GTAO)
- [x] Postprocessing: HDR tonemapping, vignette and bloom
- [ ] Temporal antialiasing (WIP)

## Screenshots

![Bust](https://raw.githubusercontent.com/bferan/bferan.github.io/master/assets/img/LucentBustThumb.png)
![Hall](https://raw.githubusercontent.com/bferan/bferan.github.io/master/assets/img/LucentCorridorThumb.png)
![Robot](https://raw.githubusercontent.com/bferan/bferan.github.io/master/assets/img/LucentRobotThumb.png)

Model Credits:

"Marble Bust 01" & "Classic Console 01" (CC0 https://polyhaven.com/) </br>
"Spaceship Corridor" (https://skfb.ly/6GHqT) by the_table </br>
"Robo_OBJ_pose4" (https://skfb.ly/m4ifca) by Artem Shupa-Dubrova </br>

<br/>

## Examples

#### High-level usage

```c++
// Initialize the engine
Engine* engine = Engine::Init();

// Create an empty scene
Scene* scene = engine->CreateScene();

// Import a gLTF model into the scene
Importer importer(engine->GetDevice());
Entity robot = importer.Import(scene, "path/to/file.gltf");

robot.SetPosition(Vector3(1.0f, 2.0, 3.0));

// Add a custom component to an entity
struct MyComponent { ... };

robot.Assign(MyComponent(...));

MyComponent& myComponent = robot.Get<MyComponent>()
....

// Render frames in a loop
while(engine->Update());
```

#### Custom render pass
```c++
// Create a custom texture to render to
auto target = renderer.AddRenderTarget(TextureSettings{...});

auto myFramebuffer = renderer.AddFramebuffer(FramebufferSettings{ 
    .colorTextures = { target }
});

// Set up a pipeline (shader + pipeline state) to use in the pass
auto myPipeline = renderer.AddPipeline(PipelineSettings{ 
    .shaderName = "MyShader.shader",
    .shaderDefines = { "MY_SHADER_DEFINE1", "MY_SHADER_DEFINE2" },
    .framebuffer = myFramebuffer
});

// Register a render pass function to be executed on each rendering of the scene 
renderer.AddPass("My Custom Pass", [=](Context& ctx, View& view))
{
    ctx.BeginRenderPass(myFramebuffer);

    ctx.BindPipeline(myPipeline);
    
    // Iterate over all entities in the scene with both Transform and MyComponent
    view.GetScene().Each<Transform, MyComponent>([&](auto& transform, auto& myComponent))
    {
        // Set a uniform value in the shader with given ID
        ctx.Uniform("u_MyValue"_id, myComponent.value);
        
        ctx.Draw(...);
    }
    
    ctx.EndRenderPass();
}
```

## Building

Requires a recent version of CMAKE (>= 3.17) and the [Vulkan SDK](https://www.lunarg.com/vulkan-sdk/) installed.
Additional dependencies are bundled with the repo as git submodules. Uses mostly features of C++17 but requires a 
compiler that supports C++20 designated initializers.

To build:

```shell
git clone --recursive https://github.com/bferan/lucent
```

When launching a Lucent application, runtime shaders are loaded from the `LC_SHADER_ROOT` environment variable. Set this
to the `./src/shaders` source directory to allow proper compilation of shaders.


