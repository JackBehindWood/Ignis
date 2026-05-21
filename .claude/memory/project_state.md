---
name: project-state
description: Ignis engine — milestone-level status and active focus area
metadata:
  type: project
---

## Standing systems (functional baseline, will grow)
- Metal GRI backend — device, command context, viewport, textures, buffers, pipeline state
- GRI abstraction layer — full RHI interface + resource type hierarchy
- Asset pipeline — manager, registry, binary stream, cook infrastructure
- Editor host — EditorLayer, EditorAssetManager, import UI
- SPIRV toolchain — DXC (HLSL→SPIR-V) + SPIRV-Cross (SPIR-V→MSL)
- Shader system — `ShaderCache` (disk + memory, FNV-1a content-hash invalidation), `AssetShader`, `ShaderLoader`, `AssetShaderCompiler`; per-stage `RenderShader` (GRIShaderPtr + GRIShaderStage + ShaderReflection); `get_or_compile(path, stage)` returns one stage at a time, compiles both in one pass internally; `GRIShader` base with `GRIVertexShader`/`GRIPixelShader` inheriting from it

## Branch layout
- `main` — stable baseline
- `asset-system` — integration branch for all asset subsystems; `shader-system` and `mesh-system` merged in
- `material-system` — DONE; `Material`, `AssetMaterial`, `AssetMaterialCompiler`, `MaterialLoader`, `Renderer` submit API
- `texture-system` — current active branch (cut from `material-system`)

## Standing systems (continued)
- Material system — `Material` (PSO + VS + PS), `AssetMaterial`, `AssetMaterialCompiler` (.igmat → IGMT v1), `MaterialLoader` (reads IGMT, builds PSO with hardcoded standard VD pos+nrm+uv stride 32)
- Renderer — static `Renderer`: `begin(viewport, clear)` / `submit(mesh, material, transform_ubo)` / `end()`; `EditorLayer::update()` is 3 lines
- Shader bug fixes — `ShaderCompiler::compile` now stores resolved entry point (not empty string); `ShaderLoader`/`MaterialLoader` now explicitly build `ShaderCompilerOptions` with count=2 before calling `get_or_compile`

## Current focus — `texture-system` branch
**DONE.** Texture2D asset pipeline fully implemented and building clean.

Deliverables:
- `engine/vendor/stb/stb_image.h` — vendored
- `GRITexture2DDesc` — `initial_data` + `initial_data_size` fields added
- `MetalTexture.cpp` — `replaceRegion` upload on creation
- `Ignis/Rendering/RenderTexture2D.h` — GPU texture wrapper
- `Ignis/Asset/AssetTexture2D.h` — asset class
- `Ignis/Asset/AssetTexture2DCompiler.h/.cpp` — PNG → IGTX v1
- `Ignis/Asset/TextureLoader.h/.cpp` — IGTX → GPU → AssetTexture2D
- Registered in `AssetManager`, `EditorAssetManager`, `Ignis.h`

## Texture binding (on texture-system branch)
- `set_texture(GRITexture2D*, slot, stage)` added to `GRICommandContext`, `MetalCommandContext`, `GRICommandList`
- Metal default linear+repeat sampler created in `MetalCommandContext` ctor, auto-bound at matching slot in `set_texture`
- `triangle.hlsl` now samples `g_texture`/`g_sampler` at `t0`/`s0`
- `EditorLayer` imports `test.png`, loads as `AssetTexture2D`, binds before `Renderer::submit`

## Up next
- `Renderer` architecture design — general rendering pipeline: Renderer, Scene/draw-list, Render/Frame graph interconnect
- Scene / draw list — feeds the Renderer each frame
- Render graph / Frame graph
- Vertex declaration registry (TODO in MaterialLoader) — materials declare required input layout; Renderer matches against mesh VBs
- Per-material parameter buffer (TODO in Material) — blocked on vertex decl registry