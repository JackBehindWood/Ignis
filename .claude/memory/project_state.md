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
* **Renderer architecture (Complete, v2)** — `Renderer` is a **pass-agnostic** coordinator: `init`/`shutdown`/`begin_frame(viewport)`/`end_frame()`; static bind helpers (`bind_mesh`, `bind_material`, `bind_transform`) record into a caller-supplied `GRICommandListBase&`. Render pass boundaries (`begin_render_pass` / `end_render_pass`) are **caller-owned** — callers hold and configure their own `GRIRenderPassInfo`. `DrawPacket` removed. `RenderResourceCache`, `MaterialFactory` (PSO cache), `FrameUniformAllocator` (4 MB ring buffer, 256-byte aligned). Ref: `.claude/docs/renderer/renderer.md`, `.claude/docs/renderer/design.md`.

## Branch layout
* `dev` — stable baseline (now includes the complete `asset-system`)
* `renderer-architecture` — active; renderer architecture complete, pending merge

## Current focus
* **Renderer Architecture v2 DONE.** `DrawPacket` removed. `Renderer` is fully pass-agnostic: no `s_forward_pass`, no internal `begin/end_render_pass`. All pass boundaries (`GRIRenderPassInfo` config, `cmd.begin_render_pass`, `cmd.end_render_pass`) are caller-owned. `EditorLayer` holds `m_forward_pass` and manages it explicitly. `bind_mesh/bind_material/bind_transform` are pure bind utilities.

## Up next (ordered)
1.  **Scene / draw list** *(active)* — scene representation that produces renderables per frame; caller configures one or more `GRIRenderPassInfo`, loops over drawables, and records explicit `begin_render_pass` / `Renderer::bind_*` / `draw_indexed_primitives` / `end_render_pass` sequences into a `GRICommandList`
2.  **Render graph / Frame graph** — graph nodes own `GRIRenderPassInfo` + `GRICommandList`; replaces per-caller manual pass management; `Renderer::bind_*` unchanged
3.  **Asset System V2** — dependency graph, async loading, hot-reloading, streaming
4.  **Project system** — project configurations, workspaces, engine-to-project separation