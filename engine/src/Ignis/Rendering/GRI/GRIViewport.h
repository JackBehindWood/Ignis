#pragma once

namespace Ignis
{
    class GRIViewport;
    typedef UniquePtr<GRIViewport> GRIViewportPtr;

    struct GRIViewportDesc
    {
        uint32_t width = 800;
        uint32_t height = 600;

        const char* title = "Ignis GRI Viewport";
    };

    class GRIViewport
    {
    public:
        virtual ~GRIViewport() = default;
        virtual uint32_t get_width() const = 0;
        virtual uint32_t get_height() const = 0;

        virtual void* get_native_handle() const = 0;
    };
} // namespace Ignis