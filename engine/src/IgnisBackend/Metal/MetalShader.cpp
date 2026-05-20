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
                case GRIPixelFormat::BGRA8Unorm:   return MTL::PixelFormatBGRA8Unorm;
                case GRIPixelFormat::RGBA8Unorm:   return MTL::PixelFormatRGBA8Unorm;
                case GRIPixelFormat::Depth32Float: return MTL::PixelFormatDepth32Float;
                default:                           return MTL::PixelFormatInvalid;
            }
        }

        static MTL::VertexFormat metal_vertex_format(GRIVertexElementFormat format)
        {
            switch (format)
            {
                case GRIVertexElementFormat::Float1: return MTL::VertexFormatFloat;
                case GRIVertexElementFormat::Float2: return MTL::VertexFormatFloat2;
                case GRIVertexElementFormat::Float3: return MTL::VertexFormatFloat3;
                case GRIVertexElementFormat::Float4: return MTL::VertexFormatFloat4;
                default:                             return MTL::VertexFormatInvalid;
            }
        }

        static MTL::PrimitiveType metal_primitive_type(GRIPrimitiveTopology topology)
        {
            switch (topology)
            {
                case GRIPrimitiveTopology::TriangleList:  return MTL::PrimitiveTypeTriangle;
                case GRIPrimitiveTopology::TriangleStrip: return MTL::PrimitiveTypeTriangleStrip;
                case GRIPrimitiveTopology::LineList:      return MTL::PrimitiveTypeLine;
                case GRIPrimitiveTopology::PointList:     return MTL::PrimitiveTypePoint;
                default:                                  return MTL::PrimitiveTypeTriangle;
            }
        }

    }

    GRIVertexShaderPtr MetalGRI::create_vertex_shader(const GRIShaderDesc& desc)
    {
        IG_CORE_ASSERT(desc.bytecode_data && desc.bytecode_size && desc.entry_point,
                       "GRIShaderDesc requires bytecode and entry point");

        MTL::Function* fn = MetalShaderLibrary::get().load_hardware_function(
            desc.bytecode_data, desc.bytecode_size, desc.entry_point);

        if (!fn)
            return nullptr;

        return create_shared<MetalVertexShader>(fn);
    }

    GRIPixelShaderPtr MetalGRI::create_pixel_shader(const GRIShaderDesc& desc)
    {
        IG_CORE_ASSERT(desc.bytecode_data && desc.bytecode_size && desc.entry_point,
                       "GRIShaderDesc requires bytecode and entry point");

        MTL::Function* fn = MetalShaderLibrary::get().load_hardware_function(
            desc.bytecode_data, desc.bytecode_size, desc.entry_point);

        if (!fn)
            return nullptr;

        return create_shared<MetalPixelShader>(fn);
    }

    GRIPipelineStatePtr MetalGRI::create_graphics_pipeline_state(const GRIPipelineStateDesc& desc)
    {
        MTL_AUTORELEASE_POOL;
        IG_CORE_ASSERT(desc.vertex_shader && desc.pixel_shader, "Pipeline state requires vertex and pixel shaders");

        MTL::RenderPipelineDescriptor* pipeline_desc = MTL::RenderPipelineDescriptor::alloc()->init();

        pipeline_desc->setVertexFunction(resource_cast<GRIVertexShader>(desc.vertex_shader)->get_function());
        pipeline_desc->setFragmentFunction(resource_cast<GRIPixelShader>(desc.pixel_shader)->get_function());
        pipeline_desc->colorAttachments()->object(0)->setPixelFormat(Utils::metal_pixel_format(desc.render_target_format));

        if (desc.vertex_declaration)
        {
            const GRIVertexDeclaration* vd = desc.vertex_declaration;
            MTL::VertexDescriptor* vtx_desc = MTL::VertexDescriptor::alloc()->init();

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
            depth_desc->setDepthCompareFunction(MTL::CompareFunctionLessEqual);
            depth_desc->setDepthWriteEnabled(true);
            depth_stencil_state = m_device->get_device()->newDepthStencilState(depth_desc);
            depth_desc->release();
        }

        NS::Error* error = nullptr;
        MTL::RenderPipelineState* pso = m_device->get_device()->newRenderPipelineState(pipeline_desc, &error);
        pipeline_desc->release();

        if (!pso)
        {
            IG_CORE_ERROR("Metal pipeline state error: {}", error->localizedDescription()->utf8String());
            return nullptr;
        }

        return create_shared<MetalPipelineState>(pso, depth_stencil_state, Utils::metal_primitive_type(desc.primitive_topology));
    }

    MetalVertexShader::MetalVertexShader(MTL::Function* function)
        : m_function(function)
    {}

    MetalVertexShader::~MetalVertexShader()
    {
        if (m_function)
            m_function->release();
    }

    MetalPixelShader::MetalPixelShader(MTL::Function* function)
        : m_function(function)
    {}

    MetalPixelShader::~MetalPixelShader()
    {
        if (m_function)
            m_function->release();
    }

    MetalPipelineState::MetalPipelineState(MTL::RenderPipelineState* pipeline_state,
                                           MTL::DepthStencilState*   depth_stencil_state,
                                           MTL::PrimitiveType         primitive_type)
        : m_pipeline_state(pipeline_state)
        , m_depth_stencil_state(depth_stencil_state)
        , m_primitive_type(primitive_type)
    {}

    MetalPipelineState::~MetalPipelineState()
    {
        if (m_pipeline_state)
            m_pipeline_state->release();
        if (m_depth_stencil_state)
            m_depth_stencil_state->release();
    }
} // namespace Ignis
