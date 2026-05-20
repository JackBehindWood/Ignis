#include "igpch.h"
#include "MetalGRI.h"
#include "MetalDevice.h"
#include "MetalResource.h"

#include "MetalShaderLibrary.h"


#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_COCOA
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"


namespace Ignis
{
    // ---------- MetalGRI ----------

    MetalGRI::MetalGRI() : m_device(MetalDevice::create_device()), m_context(*m_device)
    {}

    void MetalGRI::init()
    {
        MTL_AUTORELEASE_POOL;
        IG_CORE_ASSERT(glfwInit(), "Failed to initialize GLFW");
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        MetalShaderLibrary& m_shader_library = MetalShaderLibrary::get();
        m_shader_library.init(m_device);
        m_shader_library.reset();
    }

    void MetalGRI::shutdown()
    {
        delete m_device;
        m_device = nullptr;
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