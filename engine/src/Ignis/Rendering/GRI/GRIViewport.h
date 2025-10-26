#pragma once

namespace Ignis
{
    class GRIViewport;
    typedef UniquePtr<GRIViewport> GRIViewportPtr;
    class GRIViewport
    {
    public:
        virtual ~GRIViewport() = default;
        virtual uint32_t get_width() const = 0;
        virtual uint32_t get_height() const = 0;

        virtual void* get_native_handle() const = 0;
    };
} // namespace Ignis