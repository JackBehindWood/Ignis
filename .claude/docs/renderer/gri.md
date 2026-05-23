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
                       render_target_format, depth_stencil_format, primitive_topology }
GRITexture2DDesc     { width, height, num_mip_levels, format }
GRIBufferDesc        { size, usage }  // GRIBufferUsage: VertexBuffer|IndexBuffer|UniformBuffer
GRIShaderStage       Vertex | Pixel | Compute
```

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
