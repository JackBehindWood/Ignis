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
- Shader system — `ShaderCache` (disk + memory, FNV-1a content-hash invalidation), `AssetShader`, `ShaderLoader`, `AssetShaderCompiler`; supports file-based and inline string compilation; cache root set in `Application` ctor and overridden in `EditorLayer::attach()`

## Branch layout
- `main` — stable baseline
- `asset-system` — integration branch for all asset subsystems; `shader-system` merged in
- `mesh-system` — current active branch, branched from `asset-system`

## Current focus — `mesh-system` branch
**Goal:** Mesh asset pipeline — import, cook, load static meshes for draw calls.
Status: not started.

## Up next (not started)
- Render graph / frame graph
- Material system
- Scene / ECS
- Renderer (draw call submission, passes)
