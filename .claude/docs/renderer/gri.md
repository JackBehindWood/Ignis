# GRI — Graphics Resource Interface

Abstract RHI. All resource creation goes through the `GRI*` singleton via `RenderSystem::get_GRI()`.

## GRI API

```
create_vertex_shader(GRIShaderDesc)                    → GRIVertexShaderPtr
create_pixel_shader(GRIShaderDesc)                     → GRIPixelShaderPtr
create_graphics_pipeline_state(GRIPipelineStateDesc)   → GRIPipelineStatePtr
create_buffer(GRIBufferDesc, void* initial_data)       → GRIBufferPtr
create_texture2d(GRITexture2DDesc)                     → GRITexture2DPtr
create_viewport(GRIViewportDesc)                       → GRIViewportPtr (UniquePtr)
resize_viewport(GRIViewport*, w, h)
get_context()                                          → GRICommandContext*
```

## Descriptor types

```
GRIShaderDesc        { stage, entry_point, bytecode_data (const uint8_t*), bytecode_size (size_t) }
GRIPipelineStateDesc { vertex_shader*, pixel_shader*, vertex_declaration*,
                       render_target_format, depth_stencil_format, primitive_topology,
                       depth_stencil: GRIDepthStencilDesc, raster: GRIRasterDesc, blend: GRIBlendDesc }
GRITexture2DDesc     { width, height, num_mip_levels, format }
GRIBufferDesc        { size, usage }  // GRIBufferUsage: VertexBuffer|IndexBuffer|UniformBuffer
GRIShaderStage       Vertex | Pixel | Compute
```

## Pipeline State Sub-Descriptors

Replaces the former flat `depth_write_enabled: bool` + `blend_mode: GRIBlendMode` fields.

```cpp
struct GRIDepthStencilDesc {
    bool depth_test  = true;
    bool depth_write = true;
    GRICompareFunc depth_func = LessEqual;
};

struct GRIRasterDesc {
    GRICullMode cull_mode     = Back;
    GRIFillMode fill_mode     = Solid;
    bool        front_face_ccw = true;
};

struct GRIBlendDesc {
    bool          enable     = false;
    GRIBlendFactor src_factor = One;     GRIBlendFactor dst_factor = Zero;    GRIBlendOp blend_op  = Add;
    GRIBlendFactor src_alpha  = One;     GRIBlendFactor dst_alpha  = Zero;    GRIBlendOp alpha_op  = Add;
};
```

**Enums** (all `uint8_t`):

```
GRICompareFunc  Never | Less | Equal | LessEqual | Greater | NotEqual | GreaterEqual | Always
GRICullMode     None | Front | Back
GRIFillMode     Solid | Wireframe
GRIBlendFactor  Zero | One | SrcColor | InvSrcColor | SrcAlpha | InvSrcAlpha | DstAlpha | InvDstAlpha
GRIBlendOp      Add | Subtract | RevSubtract | Min | Max
```

**Metal mapping** (`MetalShader.cpp`, `Utils` namespace):
- `metal_compare_func` → `MTL::CompareFunction`
- `metal_blend_factor` → `MTL::BlendFactor`
- `metal_blend_op`     → `MTL::BlendOperation`
- `GRICullMode`/`GRIFillMode` are PSO-key fields but applied as **dynamic encoder state** (`setCullMode`, `setTriangleFillMode` on `MTLRenderCommandEncoder`). This wiring is pending in `MetalCommandContext::set_graphics_pipeline_state`; until it lands cull defaults to Metal's `None`.

**`MaterialFactory` cache key:** All three sub-descriptor fields are included in `CacheKey`. Hash packs DS fields into `uint32_t`, raster into `uint32_t`, blend into `uint64_t`. `get_depth_pso()` always forces `depth_write=true, LessEqual` and inherits `forward_key.raster` so cull mode is consistent between the depth pre-pass and forward pass for the same surface.

## Vertex types

```
GRIVertexElement      { attribute_index, stream_index, type, normalized, stride, offset }
GRIVertexBufferBinding { vertex_buffer*, offset }
GRIVertexDeclaration  : GRIResource — owns Vector<GRIVertexElement>
```

## Render pass types

```
GRIRenderTargetView      { texture2d*, mip_index, array_layer }
GRIDepthRenderTargetView { texture2d*, mip_index, array_layer }
GRIRenderTargetsInfo     { color[]: GRIRenderTargetView, depth: GRIDepthRenderTargetView }
GRIRenderPassInfo        { render_targets: GRIRenderTargetsInfo, load/store actions }
```

## GRIShader hierarchy

```
GRIShader : GRIResource          — base; get_stage() → GRIShaderStage
  GRIVertexShader : GRIShader
  GRIPixelShader  : GRIShader
```

`GRIPipelineStateDesc::vertex_shader` and `pixel_shader` are `GRIShader*`.
Metal backend static_casts to `MetalVertexShader*`/`MetalPixelShader*` by position.

## Pointer aliases

All `SharedPtr<T>` except Viewport:

```
GRIShaderPtr        GRIVertexShaderPtr  GRIPixelShaderPtr
GRIPipelineStatePtr GRITexture2DPtr     GRIBufferPtr
GRIViewportPtr (UniquePtr)
```

## RenderSystem

Static singleton. Owns `GRI*` and `GRICommandListExecutor`.

```
RenderSystem::init(GRIRenderAPI api) / shutdown()
RenderSystem::get_GRI() → GRI*
RenderSystem::submit()  // temporary — TODO remove
```
