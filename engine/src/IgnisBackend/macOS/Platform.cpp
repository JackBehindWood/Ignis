#include "igpch.h"

#include "Ignis/Core/Platform.h"

#ifdef IG_PLATFORM_MACOS

#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_COCOA
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"

#include <csignal>
#include <unistd.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <ctime>
#include <cstdlib>

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

int64_t Platform::current_pid()
{
    return static_cast<int64_t>(::getpid());
}

bool Platform::is_process_alive(int64_t pid)
{
    return pid > 0 && ::kill(static_cast<pid_t>(pid), 0) == 0;
}

int64_t Platform::boot_time_epoch_s()
{
    struct timeval tv{};
    size_t         sz = sizeof(tv);
    if (::sysctlbyname("kern.boottime", &tv, &sz, nullptr, 0) == 0)
    {
        return static_cast<int64_t>(tv.tv_sec);
    }
    return 0;
}

int64_t Platform::current_time_s()
{
    return static_cast<int64_t>(::time(nullptr));
}
} // namespace Ignis

#endif
