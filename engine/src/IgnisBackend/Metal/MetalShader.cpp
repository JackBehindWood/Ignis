#include "igpch.h"
#include "MetalResource.h"
#include "MetalGRI.h"

#include "MetalShaderLibrary.h"

#include <Metal/Metal.hpp>

namespace Ignis
{
namespace Utils
{
static MTL::PixelFormat metal_pixel_format(GRIPixelFormat format)
{
    switch (format)
    {
        case GRIPixelFormat::BGRA8Unorm:
            return MTL::PixelFormatBGRA8Unorm;
        case GRIPixelFormat::RGBA8Unorm:
            return MTL::PixelFormatRGBA8Unorm;
        case GRIPixelFormat::RGBA16Float:
            return MTL::PixelFormatRGBA16Float;
        case GRIPixelFormat::Depth32Float:
            return MTL::PixelFormatDepth32Float;
        default:
            return MTL::PixelFormatInvalid;
    }
}

static MTL::VertexFormat metal_vertex_format(GRIVertexElementFormat format)
{
    switch (format)
    {
        case GRIVertexElementFormat::Float1:
            return MTL::VertexFormatFloat;
        case GRIVertexElementFormat::Float2:
            return MTL::VertexFormatFloat2;
        case GRIVertexElementFormat::Float3:
            return MTL::VertexFormatFloat3;
        case GRIVertexElementFormat::Float4:
            return MTL::VertexFormatFloat4;
        default:
            return MTL::VertexFormatInvalid;
    }
}

static MTL::PrimitiveType metal_primitive_type(GRIPrimitiveTopology topology)
{
    switch (topology)
    {
        case GRIPrimitiveTopology::TriangleList:
            return MTL::PrimitiveTypeTriangle;
        case GRIPrimitiveTopology::TriangleStrip:
            return MTL::PrimitiveTypeTriangleStrip;
        case GRIPrimitiveTopology::LineList:
            return MTL::PrimitiveTypeLine;
        case GRIPrimitiveTopology::PointList:
            return MTL::PrimitiveTypePoint;
        default:
            return MTL::PrimitiveTypeTriangle;
    }
}

static MTL::CompareFunction metal_compare_func(GRICompareFunc func)
{
    switch (func)
    {
        case GRICompareFunc::Never:
            return MTL::CompareFunctionNever;
        case GRICompareFunc::Less:
            return MTL::CompareFunctionLess;
        case GRICompareFunc::Equal:
            return MTL::CompareFunctionEqual;
        case GRICompareFunc::LessEqual:
            return MTL::CompareFunctionLessEqual;
        case GRICompareFunc::Greater:
            return MTL::CompareFunctionGreater;
        case GRICompareFunc::NotEqual:
            return MTL::CompareFunctionNotEqual;
        case GRICompareFunc::GreaterEqual:
            return MTL::CompareFunctionGreaterEqual;
        case GRICompareFunc::Always:
            return MTL::CompareFunctionAlways;
        default:
            return MTL::CompareFunctionLessEqual;
    }
}

static MTL::BlendFactor metal_blend_factor(GRIBlendFactor factor)
{
    switch (factor)
    {
        case GRIBlendFactor::Zero:
            return MTL::BlendFactorZero;
        case GRIBlendFactor::One:
            return MTL::BlendFactorOne;
        case GRIBlendFactor::SrcColor:
            return MTL::BlendFactorSourceColor;
        case GRIBlendFactor::InvSrcColor:
            return MTL::BlendFactorOneMinusSourceColor;
        case GRIBlendFactor::SrcAlpha:
            return MTL::BlendFactorSourceAlpha;
        case GRIBlendFactor::InvSrcAlpha:
            return MTL::BlendFactorOneMinusSourceAlpha;
        case GRIBlendFactor::DstAlpha:
            return MTL::BlendFactorDestinationAlpha;
        case GRIBlendFactor::InvDstAlpha:
            return MTL::BlendFactorOneMinusDestinationAlpha;
        default:
            return MTL::BlendFactorOne;
    }
}

static MTL::BlendOperation metal_blend_op(GRIBlendOp op)
{
    switch (op)
    {
        case GRIBlendOp::Add:
            return MTL::BlendOperationAdd;
        case GRIBlendOp::Subtract:
            return MTL::BlendOperationSubtract;
        case GRIBlendOp::RevSubtract:
            return MTL::BlendOperationReverseSubtract;
        case GRIBlendOp::Min:
            return MTL::BlendOperationMin;
        case GRIBlendOp::Max:
            return MTL::BlendOperationMax;
        default:
            return MTL::BlendOperationAdd;
    }
}

static MTL::CullMode metal_cull_mode(GRICullMode mode)
{
    switch (mode)
    {
        case GRICullMode::None:
            return MTL::CullModeNone;
        case GRICullMode::Front:
            return MTL::CullModeFront;
        case GRICullMode::Back:
            return MTL::CullModeBack;
        default:
            return MTL::CullModeBack;
    }
}

static MTL::TriangleFillMode metal_fill_mode(GRIFillMode mode)
{
    switch (mode)
    {
        case GRIFillMode::Solid:
            return MTL::TriangleFillModeFill;
        case GRIFillMode::Wireframe:
            return MTL::TriangleFillModeLines;
        default:
            return MTL::TriangleFillModeFill;
    }
}

} // namespace Utils

GRIVertexShaderPtr MetalGRI::create_vertex_shader(const GRIShaderDesc& desc)
{
    IG_CORE_ASSERT(desc.bytecode_data && desc.bytecode_size && desc.entry_point,
                   "GRIShaderDesc requires bytecode and entry point");

    MTL::Function* fn =
        MetalShaderLibrary::get().load_hardware_function(desc.bytecode_data, desc.bytecode_size, desc.entry_point);

    if (!fn)
    {
        return nullptr;
    }

    return create_shared<MetalVertexShader>(fn);
}

GRIPixelShaderPtr MetalGRI::create_pixel_shader(const GRIShaderDesc& desc)
{
    IG_CORE_ASSERT(desc.bytecode_data && desc.bytecode_size && desc.entry_point,
                   "GRIShaderDesc requires bytecode and entry point");

    MTL::Function* fn =
        MetalShaderLibrary::get().load_hardware_function(desc.bytecode_data, desc.bytecode_size, desc.entry_point);

    if (!fn)
    {
        return nullptr;
    }

    return create_shared<MetalPixelShader>(fn);
}

GRIComputeShaderPtr MetalGRI::create_compute_shader(const GRIShaderDesc& desc)
{
    IG_CORE_ASSERT(desc.bytecode_data && desc.bytecode_size && desc.entry_point,
                   "GRIShaderDesc requires bytecode and entry point");

    MTL::Function* fn =
        MetalShaderLibrary::get().load_hardware_function(desc.bytecode_data, desc.bytecode_size, desc.entry_point);

    if (!fn)
    {
        return nullptr;
    }

    return create_shared<MetalComputeShader>(fn);
}

GRIComputePipelineStatePtr MetalGRI::create_compute_pipeline_state(const GRIComputePipelineStateDesc& desc)
{
    IG_CORE_ASSERT(desc.compute_shader, "GRIComputePipelineStateDesc requires a compute shader");
    IG_CORE_ASSERT(desc.compute_shader->get_stage() == GRIShaderStage::Compute,
                   "create_compute_pipeline_state: shader stage must be Compute");

    auto* mcs = static_cast<MetalComputeShader*>(desc.compute_shader);

    NS::Error*                 error = nullptr;
    MTL::ComputePipelineState* pso   = m_device->get_device()->newComputePipelineState(mcs->get_function(), &error);

    if (!pso)
    {
        IG_CORE_ERROR("MetalGRI: compute pipeline state error: {}",
                      error ? error->localizedDescription()->utf8String() : "unknown");
        return nullptr;
    }

    const MTL::Size tg{desc.threadgroup_size_x, desc.threadgroup_size_y, desc.threadgroup_size_z};
    return create_shared<MetalComputePipelineState>(pso, tg);
}

GRIPipelineStatePtr MetalGRI::create_graphics_pipeline_state(const GRIPipelineStateDesc& desc)
{
    MTL_AUTORELEASE_POOL;
    IG_CORE_ASSERT(desc.vertex_shader, "Pipeline state requires a vertex shader");

    MTL::RenderPipelineDescriptor* pipeline_desc = MTL::RenderPipelineDescriptor::alloc()->init();

    pipeline_desc->setVertexFunction(static_cast<MetalVertexShader*>(desc.vertex_shader)->get_function());
    if (desc.pixel_shader)
    {
        pipeline_desc->setFragmentFunction(static_cast<MetalPixelShader*>(desc.pixel_shader)->get_function());
    }
    if (desc.render_target_format != GRIPixelFormat::Unknown)
    {
        MTL::RenderPipelineColorAttachmentDescriptor* color_att = pipeline_desc->colorAttachments()->object(0);
        color_att->setPixelFormat(Utils::metal_pixel_format(desc.render_target_format));

        if (desc.blend.enable)
        {
            color_att->setBlendingEnabled(true);
            color_att->setSourceRGBBlendFactor(Utils::metal_blend_factor(desc.blend.src_factor));
            color_att->setDestinationRGBBlendFactor(Utils::metal_blend_factor(desc.blend.dst_factor));
            color_att->setRgbBlendOperation(Utils::metal_blend_op(desc.blend.blend_op));
            color_att->setSourceAlphaBlendFactor(Utils::metal_blend_factor(desc.blend.src_alpha));
            color_att->setDestinationAlphaBlendFactor(Utils::metal_blend_factor(desc.blend.dst_alpha));
            color_att->setAlphaBlendOperation(Utils::metal_blend_op(desc.blend.alpha_op));
        }
    }

    if (desc.vertex_declaration)
    {
        const GRIVertexDeclaration* vd       = desc.vertex_declaration;
        MTL::VertexDescriptor*      vtx_desc = MTL::VertexDescriptor::alloc()->init();

        for (uint32_t i = 0; i < vd->num_elements; i++)
        {
            const GRIVertexElement& elem = vd->elements[i];
            vtx_desc->attributes()->object(i)->setFormat(Utils::metal_vertex_format(elem.format));
            vtx_desc->attributes()->object(i)->setOffset(elem.offset);
            vtx_desc->attributes()->object(i)->setBufferIndex(elem.buffer_index);
        }

        for (uint32_t i = 0; i < vd->num_bindings; i++)
        {
            const GRIVertexBufferBinding& binding = vd->bindings[i];
            vtx_desc->layouts()->object(binding.buffer_index)->setStride(binding.stride);
            vtx_desc->layouts()->object(binding.buffer_index)->setStepFunction(MTL::VertexStepFunctionPerVertex);
        }

        pipeline_desc->setVertexDescriptor(vtx_desc);
        vtx_desc->release();
    }

    MTL::DepthStencilState* depth_stencil_state = nullptr;
    if (desc.depth_stencil_format != GRIPixelFormat::Unknown)
    {
        pipeline_desc->setDepthAttachmentPixelFormat(Utils::metal_pixel_format(desc.depth_stencil_format));

        MTL::DepthStencilDescriptor* depth_desc = MTL::DepthStencilDescriptor::alloc()->init();
        depth_desc->setDepthCompareFunction(Utils::metal_compare_func(desc.depth_stencil.depth_func));
        depth_desc->setDepthWriteEnabled(desc.depth_stencil.depth_write);
        depth_stencil_state = m_device->get_device()->newDepthStencilState(depth_desc);
        depth_desc->release();
    }

    NS::Error*                error = nullptr;
    MTL::RenderPipelineState* pso   = m_device->get_device()->newRenderPipelineState(pipeline_desc, &error);
    pipeline_desc->release();

    if (!pso)
    {
        IG_CORE_ERROR("Metal pipeline state error: {}", error->localizedDescription()->utf8String());
        return nullptr;
    }

    return create_shared<MetalPipelineState>(
        pso, depth_stencil_state, Utils::metal_primitive_type(desc.primitive_topology),
        Utils::metal_cull_mode(desc.raster.cull_mode), Utils::metal_fill_mode(desc.raster.fill_mode));
}

MetalVertexShader::MetalVertexShader(MTL::Function* function)
    : m_function(function)
{
}

MetalVertexShader::~MetalVertexShader()
{
    if (m_function)
    {
        m_function->release();
    }
}

MetalPixelShader::MetalPixelShader(MTL::Function* function)
    : m_function(function)
{
}

MetalPixelShader::~MetalPixelShader()
{
    if (m_function)
    {
        m_function->release();
    }
}

MetalPipelineState::MetalPipelineState(MTL::RenderPipelineState* pipeline_state,
                                       MTL::DepthStencilState* depth_stencil_state, MTL::PrimitiveType primitive_type,
                                       MTL::CullMode cull_mode, MTL::TriangleFillMode fill_mode)
    : m_pipeline_state(pipeline_state),
      m_depth_stencil_state(depth_stencil_state),
      m_primitive_type(primitive_type),
      m_cull_mode(cull_mode),
      m_fill_mode(fill_mode)
{
}

MetalPipelineState::~MetalPipelineState()
{
    if (m_pipeline_state)
    {
        m_pipeline_state->release();
    }
    if (m_depth_stencil_state)
    {
        m_depth_stencil_state->release();
    }
}

MetalComputeShader::MetalComputeShader(MTL::Function* function)
    : m_function(function)
{
}

MetalComputeShader::~MetalComputeShader()
{
    if (m_function)
    {
        m_function->release();
    }
}

MetalComputePipelineState::MetalComputePipelineState(MTL::ComputePipelineState* pso, MTL::Size threadgroup_size)
    : m_pso(pso),
      m_threadgroup_size(threadgroup_size)
{
}

MetalComputePipelineState::~MetalComputePipelineState()
{
    if (m_pso)
    {
        m_pso->release();
    }
}

} // namespace Ignis
