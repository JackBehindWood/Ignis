# Project State

## Current Focus
**Rendering restored on PBR-Material branch. Compute Phase 4 (GPU frustum culling + indirect draws) complete.**

Critical rendering regression fixed: SPIRV-Cross `argument_buffers = true` without a discrete mask wrapped every descriptor set in argument buffer indirection. CPU code passed raw buffer data at slot 1; shader tried to dereference that as a FrameUniforms pointer → invalid GPU address → nothing rendered (including grid and thumbnails). Fix: `opts.argument_buffer_discrete_mask = ~(1u << 0u)` (MslSpirvCompiler.cpp) + `k_version` bumped 3→4 (ShaderCache.cpp) to purge stale .igsh blobs. Space0 (unbounded bindless arrays) still uses argument buffers as required by Apple Silicon; space0 CPU-side argument buffer encoding is not yet implemented — PBR texture sampling returns undefined until that work is done.

Phase 1: `engine/shaders/include/PBR.hlsl` (Cook-Torrance BRDF), `engine/shaders/pbr.hlsl` (full VS/PS with bindless material indexing + direct lighting + IBL stub), `GRITexture2DDesc` extended with `is_cubemap`/`array_layers`, `MetalGRI::create_texture2d_cubemap`, `RGBA16Float` pixel format added. MslSpirvCompiler UBO binding fixed (`msl_buffer = desc_set` — eliminates space1/space3 collision). `bind_frame_data` now binds at both slot 0 (legacy) and slot 1 (Ignis.hlsl shaders).

Phase 2: `DirectionalLightComponent`, `PointLightComponent`, `SpotLightComponent` added to Components.h. `Bindings.hlsl` FrameUniforms extended with `DirectionalLight[4]` + `PointLight[64]` + counts. `GPUFrameData` extended to match. `SceneRenderer` now owns frame data upload (lights harvested from ECS every frame). `m_color_rt` is RGBA16Float (HDR); `m_ldr_rt` BGRA8 LDR. Tonemap pass (ACES + γ2.2) added after ForwardScene; `get_color_rt()` returns LDR result. ViewportPanel no longer calls `upload_frame_data`. `engine/shaders/tonemap.hlsl` written.

GRI compute infrastructure (Phase 1 of compute-design.md): `GRIComputeShader`, `GRIComputePipelineState`, `GRIAccessFlags`, `StorageBuffer`/`IndirectBuffer` usage flags, 6 new deferred commands (`set_compute_pipeline_state`, `set_storage_buffer`, `set_storage_texture`, `dispatch`, `draw_indexed_primitives_indirect`, `memory_barrier`), `MetalComputeShader`/`MetalComputePipelineState` backends, `MetalCommandContext` encoder exclusivity state machine, `ShaderReflection::storage_textures`/`threadgroup_size_x/y/z`, IGSH cache v3, `MslSpirvCompiler` storage_images bindings.

RenderGraph Phase 2 (compute-design.md §2) complete: `RGPassType` enum, `pass_type` on `RGPassNode`, 4 storage dependency vectors (`storage_buffer_reads/writes`, `storage_texture_reads/writes`), `add_compute_pass`/`add_graphics_pass` explicit registration, reset-on-seal accumulator contract, `VirtualBuffer::allow_aliasing` (false for `IndirectBuffer`), `RGPool::acquire_buffer` aliasing gate, `GRICommandContext::begin_compute_pass`/`end_compute_pass` virtual + Metal impl + deferred command types, compile Phases 1/2/4/5/6 updated for storage resources.

Phase 4 (GPU frustum culling & indirect draws) complete: `m_visible`/`m_instance_data` removed. 5 persistent GPU cull buffers (`m_cull_input_buffer`, `m_visible_indices_buffer`, `m_atomic_counter_buffer`, `m_draw_args_buffer`, `m_cull_cb`) + `m_entity_data_buffer` (all-entity world matrices) + `m_gpu_cull_visible_buffer` (scratch for GPU output). `GPUCullInstance`/`GPUCullConstants`/`FrustumPlane` structs. `engine/shaders/cull.hlsl` compute kernel. Cull PSO compiled in `prepare()`. `build_cull_proxies` uploads entity data + cull instances. `build_commands(frustum)` CPU-culls, builds batches, uploads visible_indices and DrawIndexedArguments CPU-side. DepthPrePass/ForwardScene use `draw_indexed_primitives_indirect`. Shaders updated: slot27=GPUInstanceData all-entities, slot28=uint visible_indices. `begin_render_pass` extended with resource list parameter; `RGBuilder::execute` assembles storage+IndirectBuffer resources per graphics pass; `MetalCommandContext::begin_render_pass` calls `useResource` on all supplied buffers/textures (fixes black-screen metal hazard).

Phase 3 (hazard resolution / memory barriers) complete: `RGBarrier` struct + `m_sorted_barriers` (indexed by sorted-pass position) in `RenderGraph`. `compile()` Phase 3.5 walks adjacent non-culled sorted pairs — detects RAW/WAR/WAW for shared buffers and textures via `get_buffer_access`/`get_texture_access`/`is_hazard` helpers; aliased transient transitions handled separately (Phase 3.5b). `RGBuilder::execute()` rewritten with compute-chain fusion: a single `begin_compute_pass` covers all consecutive WAW-chained compute passes (union resource sweep); WAW barriers fire intra-encoder; non-WAW barriers emit after `end_compute_pass` (no-op for encoder management, coherence from `endEncoding`). `MetalCommandContext::memory_barrier` unchanged — already correct from Phase 1.

## Standing Systems

### Scene System (Phases 1–4 complete)
- IHT pipeline: `scanner.py` FSM, `generator.py` with codec table, `worker.py` manifest diff, all run as build pre-step
- `ComponentRegistry` + `ScriptRegistry` — descriptor stores with idempotent `register_*` + `validate_unique_ids()`
- Generated: `Components.gen.h`, `CameraComponent.gen.h`, `IDComponent.gen.h`, `NameComponent.gen.h`, `ScriptComponent.gen.h`, `SceneRegistry.gen.h`
- `SceneSerializer` — data-driven, ScriptComponent special-cased for ScriptRegistry delegation
- `SceneCamera` + `CameraComponent` — projection state machine, IHT custom codec path
- `FlyCamera` — engine-side free-look controller (no editor dep)
- `SceneRenderer` — Phase 1 production arch: DrawBatch/DrawIndexedArguments, dense uint16_t IDs (ResourceCache), async PSO path in prepare(), MeshRendererComponent+MaterialComponent ECS views, MeshSlot-driven draw args, VB/IB bind-on-change in emit loops; GRI API Extension complete (GRIDepthStencilDesc/GRIRasterDesc/GRIBlendDesc, multi-pass PSO state, disjoint depth PSO IDs)
- `Scene::update` — script dispatch; `get_primary_camera_data()` — optional CameraData query

### Project & Settings (editor-only, in-memory phase complete)
Observer-pattern project dispatch (`IProjectObserver`/`ProjectContext`), mode-aware validate, `EditorAssetManager` as sole engine gateway + project observer, `EditorSettingsManager`, `EngineSettingsManager`, settings-agnostic `Application`, deterministic bootstrap in `Editor::Editor()`.

### Renderer
- Pass-agnostic GRI API with `RenderResourceCache`, `FrameUniformAllocator`, bind helpers, uniform slots
- `RenderGraph` + `RGBuilder` integration — declarative pass graph, barrier inference
- `RenderMesh` pipeline: vertex layout, upload, `MetalGRI` backend
- `RenderShader` / `ShaderCache` / SPIR-V→MSL compilation via SPIRV-Cross
- `MetalCommandContext`, metal-cpp ownership discipline
- **Depth buffer Phase 1 complete**: `GRIViewport::get_depth_texture()` virtual, `MetalViewport` override, `Renderer::get_depth_texture()`/`get_config()`, `RGBuilder::import_viewport_depth()`, `SceneRenderer` now declares depth to RG + PSO formats from `RendererConfig`

### Math & Scene
- Foundation math library (Vec2/3/4, Mat4, Quat)
- ECS scene renderer integration — entity → draw call path

### Asset System V2 *(completed, stable baseline)*
- `AssetManager` v2: binary `.igasset` format, cook pipeline, `AssetHandle<T>`, `AssetType` registry
- `RuntimePathId`: interned 64-bit hash of virtual path string; zero heap per frame after warm-up
- FSEvents watcher (macOS): physical file change → virtual path invalidation via `RuntimePathId`
- `EditorAssetManager` isolation: cook + import tooling stays editor-only, engine sees handle/stream API only

## Completed but Pre-Baseline
- Initial renderer scaffolding (pass 1 – pre-RenderGraph)
- Initial scene system and scene renderer

## Upcoming
- **PBR material system + lighting pipeline** — typed `PBRMaterialParams`, GGX/Smith BRDF, IBL (BRDF LUT, irradiance, prefilter), directional/point/spot lights, tonemapping
- SPIR-V audit: verify HLSL→SPIR-V→MSL round-trip for PBR shader permutations
- Round-trip validation test: save scene → reload → verify field equality

## Key Invariants
- Engine TUs: no Metal headers, no `EditorSettings`, no heavy YAML dependency in hot paths
- `IgnisBackend/Metal/`: only TUs here include `<Metal/Metal.hpp>`
- Foundation aliases always; never raw `std::` in TUs
- All new public types: alias in `Foundation/X.h`, added to `igpch.h`
- **Editor invariant**: only `EditorAssetManager.cpp` may include/call `AssetManager` directly
- **Project invariant**: `IProjectObserver` and all project system types are editor-module-only
