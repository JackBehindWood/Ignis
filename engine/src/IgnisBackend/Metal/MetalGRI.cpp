#include "igpch.h"
#include "MetalGRI.h"
#include "MetalDevice.h"
#include "MetalResource.h"

#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_COCOA
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"

namespace Ignis
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

    // Compile MSL source and return the named function. Library is released after extraction.
    static MTL::Function* compile_metal_function(MTL::Device* device, const char* source, const char* entry_point)
    {
        NS::Error* error = nullptr;
        NS::String* ns_source = NS::String::string(source, NS::StringEncoding::UTF8StringEncoding);
        MTL::CompileOptions* options = MTL::CompileOptions::alloc()->init();

        MTL::Library* library = device->newLibrary(ns_source, options, &error);
        options->release();

        if (!library)
        {
            IG_CORE_ERROR("Metal shader compile error: {}", error->localizedDescription()->utf8String());
            return nullptr;
        }

        NS::String* ns_entry = NS::String::string(entry_point, NS::StringEncoding::UTF8StringEncoding);
        MTL::Function* function = library->newFunction(ns_entry);
        library->release();

        if (!function)
            IG_CORE_ERROR("Metal: entry point '{}' not found in shader", entry_point);

        return function;
    }

    // ---------- MetalGRI ----------

    MetalGRI::MetalGRI() : m_device(MetalDevice::create_device()), m_context(*m_device)
    {}

    void MetalGRI::init()
    {
        MTL_AUTORELEASE_POOL;
        IG_CORE_ASSERT(glfwInit(), "Failed to initialize GLFW");
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    }

    void MetalGRI::shutdown()
    {
        delete m_device;
        m_device = nullptr;
    }

    GRIVertexShaderPtr MetalGRI::create_vertex_shader(const GRIShaderDesc& desc)
    {
        MTL_AUTORELEASE_POOL;
        IG_CORE_ASSERT(desc.source && desc.entry_point, "GRIShaderDesc must have source and entry_point");

        MTL::Function* fn = compile_metal_function(m_device->get_device(), desc.source, desc.entry_point);
        if (!fn) return nullptr;

        return create_shared<MetalVertexShader>(fn);
    }

    GRIPixelShaderPtr MetalGRI::create_pixel_shader(const GRIShaderDesc& desc)
    {
        MTL_AUTORELEASE_POOL;
        IG_CORE_ASSERT(desc.source && desc.entry_point, "GRIShaderDesc must have source and entry_point");

        MTL::Function* fn = compile_metal_function(m_device->get_device(), desc.source, desc.entry_point);
        if (!fn) return nullptr;

        return create_shared<MetalPixelShader>(fn);
    }

    GRIPipelineStatePtr MetalGRI::create_graphics_pipeline_state(const GRIPipelineStateDesc& desc)
    {
        MTL_AUTORELEASE_POOL;
        IG_CORE_ASSERT(desc.vertex_shader && desc.pixel_shader, "Pipeline state requires vertex and pixel shaders");

        MTL::RenderPipelineDescriptor* pipeline_desc = MTL::RenderPipelineDescriptor::alloc()->init();

        pipeline_desc->setVertexFunction(resource_cast<GRIVertexShader>(desc.vertex_shader)->get_function());
        pipeline_desc->setFragmentFunction(resource_cast<GRIPixelShader>(desc.pixel_shader)->get_function());
        pipeline_desc->colorAttachments()->object(0)->setPixelFormat(metal_pixel_format(desc.render_target_format));

        if (desc.vertex_declaration)
        {
            const GRIVertexDeclaration* vd = desc.vertex_declaration;
            MTL::VertexDescriptor* vtx_desc = MTL::VertexDescriptor::alloc()->init();

            for (uint32_t i = 0; i < vd->num_elements; i++)
            {
                const GRIVertexElement& elem = vd->elements[i];
                vtx_desc->attributes()->object(i)->setFormat(metal_vertex_format(elem.format));
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
            pipeline_desc->setDepthAttachmentPixelFormat(metal_pixel_format(desc.depth_stencil_format));

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

        return create_shared<MetalPipelineState>(pso, depth_stencil_state, metal_primitive_type(desc.primitive_topology));
    }

    GRIBufferPtr MetalGRI::create_buffer(const GRIBufferDesc& desc, const void* initial_data)
    {
        IG_CORE_ASSERT(desc.size > 0, "Buffer size must be greater than zero");

        MTL::Buffer* buffer = initial_data
            ? m_device->get_device()->newBuffer(initial_data, desc.size, MTL::ResourceStorageModeShared)
            : m_device->get_device()->newBuffer(desc.size,              MTL::ResourceStorageModeShared);

        return create_shared<MetalBuffer>(buffer, desc.size);
    }
} // namespace Ignis