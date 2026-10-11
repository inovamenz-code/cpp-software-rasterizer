# C++ Software Rasterizer

[English](README.md) · [简体中文](README.zh-CN.md)

A Windows C++ rendering demo that turns 3D triangles into a shaded image on the CPU. It shows how transforms, clipping, triangle filling, lighting and a depth buffer work together, with a row-sliced multithreaded rendering path.

![Actual rasterizer output: shaded cubes and a foreground sphere](docs/images/rasterizer.jpg)

[Implementation](Rasterizer/raster.cpp) · [Rendering classes](Rasterizer) · [Sources and attribution](THIRD_PARTY_NOTICES.md)

## What it does

- Renders procedural cube and sphere scenes through perspective and camera transforms.
- Clips triangles against the supported depth range and fills their screen-space pixels.
- Uses a Z-buffer so nearer surfaces cover surfaces behind them.
- Applies lighting and rotates objects in the animated scenes.
- Divides screen rows between thread-pool jobs, alongside a serial rendering path for comparison.

## My contribution and background

This project is the rasterizer portion of my WM9M4 Games Engineering coursework. My coursework implementation connects scene construction, transformation, triangle processing, shading, visibility and thread-pool rendering in a working demonstration. It builds on the course's `GamesEngineeringBase` teaching framework and supporting material; the framework is not independently authored code.

The repository was separated from a combined rasterizer/chatroom submission to make this rendering work easier to inspect. The original snapshot is recorded in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). Source filenames, project settings and the solution's relative paths are retained.

## How it works

| Technology or system | Role | Start reading |
| --- | --- | --- |
| C++ and procedural meshes | Build cubes, spheres and animated scenes | [`raster.cpp`](Rasterizer/raster.cpp), [`mesh.h`](Rasterizer/mesh.h) |
| Matrix transforms and clipping | Project world-space triangles into the image | [`matrix.h`](Rasterizer/matrix.h), [`raster.cpp`](Rasterizer/raster.cpp) |
| Triangle filling and Z-buffer | Decide pixel coverage and visible depth | [`triangle.h`](Rasterizer/triangle.h), [`zbuffer.h`](Rasterizer/zbuffer.h) |
| Lighting | Calculate shaded surface colour | [`light.h`](Rasterizer/light.h) |
| Thread pool | Schedule rendering over separate row slices | [`Tpool.h`](Rasterizer/Tpool.h) |
| Teaching window framework | Present the CPU-rendered image | [`renderer.h`](Rasterizer/renderer.h), [`GamesEngineeringBase.h`](Rasterizer/GamesEngineeringBase.h) |

## Build and run

Requirements: Windows x64, Visual Studio 2022 with Desktop development with C++, the MSVC v143 toolset and a Windows SDK.

Open `Rasterizer.sln`, select **Release / x64**, and build. Alternatively, run this from a Visual Studio Developer PowerShell at the repository root:

```powershell
msbuild .\Rasterizer.sln /m /p:Configuration=Release /p:Platform=x64
.\x64\Release\Rasterizer.exe
```

The current executable opens the `Raster` window and runs the scene selected by `funcnum` in `raster.cpp`. The default is scene 2. Press **Escape** to exit. Scene selection is currently a source-level setting; there is no scene-selection menu. The optional `sceneTest()` route contains keyboard translation controls but is not the default scene.

## Status and limitations

The split **Release x64** project was built on 11 October 2026, and its cube/sphere output was visually inspected; the image above comes from that run. Existing compiler warnings remain. This is a coursework rendering demonstration, not a complete game or a general-purpose graphics engine.

The original scene loops include serial and multithreaded rendering calls. Their presence is not evidence of a measured speedup. No new benchmark or performance comparison is claimed by this repository. The original code is retained so that these implementation choices remain visible.

## Credits

The teaching framework's MIT notice is preserved in `GamesEngineeringBase.h`. No licence is newly applied to the combined project. See [sources and attribution](THIRD_PARTY_NOTICES.md) for the coursework origin and redistribution boundary.
