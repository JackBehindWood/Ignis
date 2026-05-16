#include "igpch.h"
#include "MetalResource.h"

#include <Metal/Metal.hpp>

namespace Ignis
{
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

    MetalPipelineState::MetalPipelineState(MTL::RenderPipelineState* pipeline_state, MTL::DepthStencilState* depth_stencil_state, MTL::PrimitiveType primitive_type)
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
