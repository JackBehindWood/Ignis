# Editor (IgnisEditor)

Host application. All source under `editor/src/`. Precompiled header: `edpch.h`.

## EditorAssetManager

Singleton wrapper around `AssetManager` that enforces the `resources/` directory layout.

```
EditorAssetManager::get()
  .set_root(path)            // override default (<cwd>/resources); also sets compiled_root
  .import_shader(filename)   // resources/assets/shaders/<filename> → AssetID
  .import_texture(filename)  // resources/assets/textures/<filename> → AssetID
  .load_shader(id)           // → SharedPtr<Shader>
  .reload_all()              // AssetManager::get().reload_all() — bound to F5
```

Resource root layout (default `<cwd>/resources`):
```
resources/
  assets/
    shaders/     HLSL source files
    textures/
  cache/         cooked .igasset output (set as compiled_root automatically)
```

Assets imported via `EditorAssetManager` are always cached to disk (`cache_compiled = true`).

## EditorLayer

`Layer` subclass. Currently a hardcoded triangle demo that exercises the full render path.

```
attach()   — import triangle.hlsl, create VB/IB/UB, build GRIVertexDeclaration + PSO
detach()   — null out all GRI resource SharedPtrs (release order: PSO → UB → IB → VB → shader)
update(ts) — full draw tick:
               begin_drawing_viewport → begin_render_pass
               → set_pipeline_state → set_vertex/index/uniform_buffer → draw_indexed_primitives(3)
               → end_render_pass → RenderSystem::submit()
event(e)   — dispatches KeyPressedEvent; F5 → EditorAssetManager::reload_all()
```

Members: `m_shader` (`SharedPtr<Shader>`), `m_vertex_buffer`, `m_index_buffer`, `m_uniform_buffer` (`GRIBufferPtr`), `m_pipeline_state` (`GRIPipelineStatePtr`).

PSO is built with `RGBA8Unorm` render target + `Depth32Float` depth, `TriangleList` topology.
