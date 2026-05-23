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
* **Renderer (Initial)** — static `Renderer`: `begin(viewport, clear)` / `submit(...)` / `end()`; integrated `VertexDeclarationRegistry` for layout management.

## Branch layout
* `dev` — stable baseline (now includes the complete `asset-system`)
* `renderer-architecture` — (upcoming/active) integration branch for the new renderer design

## Current focus
* **Initial Asset System DONE.** All major asset subsystems are functional and integrated.
* **Active Focus:** Renderer architecture design.

## Up next (ordered)
1.  **Renderer architecture design** — define general pipeline shape: Renderer, Scene/draw-list, Render/Frame graph interconnect; everything downstream depends on this
2.  **Scene / draw list** — feeds the Renderer each frame; shape determined by renderer architecture
3.  **Render graph / Frame graph** — integrates renderer + scene once both are stable