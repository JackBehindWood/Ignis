#include "igpch.h"
#include "MetalGRI.h"
#include "MetalViewport.h"

#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_COCOA
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"

namespace Ignis
{
    MetalGRI::MetalGRI() : m_device(nullptr)
    {
    }

    void MetalGRI::init()
    {
        // Initialize Metal device
        m_device = MTL::CreateSystemDefaultDevice();

        // Initialize GLFW for Metal
        IG_CORE_ASSERT(glfwInit(), "Failed to initialize GLFW");
        //glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    }

    void MetalGRI::shutdown()
    {
        // Shutdown code for Metal backend
        m_device->release();
        m_device = nullptr;

        //glfwTerminate();
    }

    GRIViewportPtr MetalGRI::create_viewport(uint32_t width, uint32_t height)
    {
        return create_unique<MetalViewport>(m_device, width, height);
    }
} // namespace Ignis