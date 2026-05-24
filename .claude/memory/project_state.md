---
name: project-state
description: Ignis engine — milestone-level status and active focus area
metadata:
  type: project
---

## Standing systems (functional baseline, will grow)
* **Metal GRI backend** — device, command context, viewport, textures, buffers, pipeline state
* **GRI abstraction layer** — full RHI interface + resource type hierarchy
* **Asset pipeline (v1 Complete)** — manager, registry, binary stream, and cook infrastructure. Fully integrated with complete subsystems for **Shaders, Meshes, Materials, and Textures**.
* **Editor host** — EditorLayer, EditorAssetManager, import UI
* **SPIRV toolchain** — DXC (HLSL→SPIR-V) + SPIRV-Cross (SPIR-V→MSL)
* **Renderer architecture (Complete, v2)** — `Renderer` is a **pass-agnostic** coordinator.
* **Math Library** — in Math namespace.
* **Scene System (Complete, v1)** — ECS scene layer utilizing EnTT with a dedicated, asset-decoupled `SceneRenderer` that builds sorted draw-lists and emits GRI commands.
* **Render graph v2 (Complete)** — `RGBuilder` is a **persistent member** of the frame driver (e.g. `EditorLayer`). Always owns its `RenderGraph` internally as a direct member (no borrow ctor, no `UniquePtr` duality). `GRICommandList` is injected at `execute(cmd)` time, not at construction. Pool and arena survive across frames. Per-frame transient builders (`RGBuilder scratch`) are still supported for isolated secondary work.

## Branch layout
* `dev` — stable baseline (includes `asset-system` and the newly merged `scene-draw-list`)
* `render-graph` — active; implementing a builder-style render graph / frame graph architecture.

## Current focus
**Asset System V2** — dependency graph, time-sliced deferred loading, incremental file streaming, hot-reload with GPU-side invalidation. Single-threaded execution model with explicit frame-budget coordination.

## Up next (ordered)
1. **Project System**
2. **Compute passes, ray tracing**