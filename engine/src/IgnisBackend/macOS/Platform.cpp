#include "igpch.h"

#include "Ignis/Core/Platform.h"

#ifdef IG_PLATFORM_MACOS

#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_COCOA
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"

namespace Ignis
{
    static void glfw_error_callback(int32_t error, const char* description)
    {
        IG_CORE_ERROR("GLFW Error ({0}): {1}", error, description);
    }

    PlatformType Platform::get_platform_type()
    {
        return PlatformType::MacOS;
    }

    void Platform::init()
    {
        // Initialize platform-specific resources for macOS
        IG_CORE_INFO("Initializing macOS platform");

        IG_CORE_ASSERT(glfwInit(), "Failed to initialize GLFW");

        glfwSetErrorCallback(glfw_error_callback);

        // Prevent GLFW from creating an OpenGL context
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    }

    void Platform::shutdown()
    {
        // Cleanup platform-specific resources for macOS
        IG_CORE_INFO("Shutting down macOS platform");
        glfwTerminate();
    }

    void Platform::poll_events()
    {
        // Poll events from the underlying windowing system
        glfwPollEvents();
    }

    double Platform::get_time()
    {
        return glfwGetTime();
    }
}

#endif