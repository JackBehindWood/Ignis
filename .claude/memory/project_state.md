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
- `material-system` — current active branch

## Current focus — `material-system` branch
**Goal:** Material abstraction — own PSO + shader, decouple draw call setup from `EditorLayer`.

**Step 1 DONE:** `GRIShader` base; per-stage `RenderShader`; stage-dispatching `ShaderCache::get_or_compile`; `AssetShader` holds m_vs + m_ps separately.

**Step 2 DONE:** `Material` type.
- `Material` (Rendering): `GRIPipelineStatePtr` + `m_vs` + `m_ps`. Parameter buffer stubbed (TODO).
- `AssetMaterial` (Asset): wraps `SharedPtr<Material>`.
- `AssetMaterialCompiler`: reads `.igmat` (single line = shader filename) → writes IGMT v1 (absolute shader source path string).
- `MaterialLoader`: reads IGMT, compiles via `ShaderCache`, builds PSO with hardcoded standard VD (pos+nrm+uv stride 32). TODO vertex decl registry left in code.
- `.igmat` source files → `resources/assets/materials/`. `EditorAssetManager::import_material`/`load_material` added.
- `EditorLayer` now loads `AssetMaterial`; inline PSO/VD setup removed.

**Step 3 DONE:** `Renderer` submit API.
- Static `Renderer` class: `begin(GRIViewport*, GRIClearValue)` + `submit(RenderMesh*, Material*, GRIBuffer* transform_ubo)` + `end()`.
- `begin` builds `GRIRenderPassInfo` internally and calls `begin_frame` / `begin_drawing_viewport` / `begin_render_pass`.
- `submit` issues PSO + VB + IB + UBO + `draw_indexed_primitives`.
- `end` calls `end_render_pass` / `end_frame` / `RenderSystem::submit()`.
- `EditorLayer::update()` reduced to 3 lines; no direct cmd_list or GRI calls remain.
- `transform_ubo` param is nullable; currently EditorLayer owns the identity UBO and passes it. TODO: move to scene/draw-list layer when that exists.

## Up next (not started)
- `Texture System`
- `Renderer` architecture design — general rendering pipeline design: how Renderer, Scene/draw-list, and Render/Frame graph interconnect; what the submit API should evolve into (transforms, draw lists, passes)
- Scene / draw list — feeds the Renderer each frame
- Render graph / Frame graph