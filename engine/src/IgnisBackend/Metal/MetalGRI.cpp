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
    MetalGRI::MetalGRI() : m_device(MetalDevice::create_device()), m_context(*m_device)
    {
    }

    void MetalGRI::init()
    {
        MTL_AUTORELEASE_POOL;

        // Initialize GLFW for Metal
        IG_CORE_ASSERT(glfwInit(), "Failed to initialize GLFW");
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    }

    void MetalGRI::shutdown()
    {
        // Shutdown code for Metal backend
        delete m_device;
        m_device = nullptr;
    }
} // namespace Ignis