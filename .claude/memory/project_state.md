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
* **Math Library** - in Math namespace.
* **Scene System** - ECS scene with renderer and components.

## Branch layout
* `dev` — stable baseline (now includes the complete `asset-system`)
* `renderer-architecture` — active; core pass-agnostic architecture complete, pending final merge
* `scene-draw-list` — active; implementing the asset-decoupled scene layer that tracks and merges into `renderer-architecture`
* `Render-graph` - active; implementing a render graph / frame graph (builder style) and merges into `renderer-architecture`

## Current focus
`scene-draw-list` branch audit complete — ready for merge review.

## Up next (ordered)
1.  **Merge `scene-draw-list` → `renderer-architecture`** — verify clean compile, then merge
2.  **Render graph / Frame graph** — graph nodes own `GRIRenderPassInfo` + `GRICommandList`; replaces per-caller manual pass management; `Renderer::bind_*` unchanged
3.  **Asset System V2** — dependency graph, async loading, hot-reload, streaming
4.  **Compute passes, ray tracing**