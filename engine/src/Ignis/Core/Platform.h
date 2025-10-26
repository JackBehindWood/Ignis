#pragma once

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

        static void poll_events();
        static double get_time();
    };
}