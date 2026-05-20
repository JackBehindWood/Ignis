# Metal Backend

Only TUs under `IgnisBackend/Metal/` may include `<Metal/Metal.hpp>` or any metal-cpp imports.

## metal-cpp ownership rules

- `newX(…)` returns +1 — caller must `->release()`
- `MTL::Library::serializeToData(NS::Error**)` — takes `NS::Error**`, not zero-arg
- NS types (`NS::URL`, `NS::Data`, …) — include `<Foundation/Foundation.hpp>`
- Autorelease pool: wrap per-frame work in `NS::AutoreleasePool::alloc()->init()` / `->release()`

## MetalGRI : GRI

Singleton via `RenderSystem::get_GRI()`. Owns `MetalDevice*` and `MetalCommandContext`.

```
init() / shutdown()
create_texture2d(GRITexture2DDesc)                   → GRITexture2DPtr
create_viewport(GRIViewportDesc)                     → GRIViewportPtr (UniquePtr)
resize_viewport(GRIViewport*, w, h)
create_vertex_shader(GRIShaderDesc)                  → GRIVertexShaderPtr
create_pixel_shader(GRIShaderDesc)                   → GRIPixelShaderPtr
create_graphics_pipeline_state(GRIPipelineStateDesc) → GRIPipelineStatePtr
create_buffer(GRIBufferDesc, void*)                  → GRIBufferPtr
get_context()                                        → GRICommandContext* (→ MetalCommandContext)
get_device()                                         → MetalDevice*
get_api()                                            → GRIRenderAPI::Metal
```

## MetalCommandContext : GRICommandContext

Per-frame command encoding. Accessed via `GRI::get_context()`.

```
begin_frame()                          — acquires command buffer from queue
end_frame() / present()                — commits + presents drawable
begin_render_pass(GRIRenderPassInfo&) / end_render_pass()
```

Encodes draw calls between begin/end render pass.

## MetalShaderLibrary

Singleton (`MetalShaderLibrary::get()`). Call `init(MetalDevice*)` before use; `reset()` clears the function cache.

`load_hardware_function(data, size, entry_point)` — wraps `data` in `dispatch_data_create`, calls `MTL::Device::newLibrary(dispatch_data)`, extracts the function via `newFunction`, then releases the library (MTL::Function retains it). Function is cached in `m_function_cache` keyed by `"<ptr_as_uintptr>:<entry>"` — pointer is stable for the lifetime of the owning `AssetShader`. **Caller receives an owned +1 retain** — `MetalVertexShader`/`MetalPixelShader` destructors release it. `reset()` releases all cached functions.

No library cache; no filesystem access. Cook-time metallib compilation is in `MslSpirvCompiler::compile_to_metallib()`.

## Resource creation patterns

```
create_vertex_shader / create_pixel_shader
  — GRIShaderDesc.bytecode_data / bytecode_size = in-memory .metallib bytes
  — calls MetalShaderLibrary::load_hardware_function(bytecode_data, bytecode_size, entry_point), wraps MTL::Function*

create_graphics_pipeline_state
  — builds MTL::RenderPipelineState from VS/PS functions + format info

create_buffer
  — MTL::Device::newBuffer(size, MTL::ResourceStorageModeShared); mapped directly for CPU writes

create_texture2d
  — MTL::Device::newTexture(MTL::TextureDescriptor*); descriptor released after creation
```
