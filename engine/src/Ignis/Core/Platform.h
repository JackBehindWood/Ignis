#pragma once
#include <stdint.h>

namespace Ignis
{
enum class PlatformType
{
    Windows,
    MacOS,
    Linux,
    Unknown
};

class Platform
{
public:
    static PlatformType get_platform_type();

    static void init();
    static void shutdown();

    static void   poll_events();
    static double get_time();

    // Process utilities
    static int64_t current_pid();
    static bool    is_process_alive(int64_t pid);
    static int64_t boot_time_epoch_s();
    static int64_t current_time_s();
};
} // namespace Ignis
