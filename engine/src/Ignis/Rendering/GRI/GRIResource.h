#pragma once

namespace Ignis
{
    class GRI;
    class GRIResource;
    class GRIViewport;

    typedef UniquePtr<GRIViewport> GRIViewportPtr;

    enum class GRIResourceType
    {
        Texture,
        Buffer,
        Shader,
        Pipeline,
        RenderTarget,
        Sampler,
        Unknown
    };

    struct GRIViewportDesc
    {
        uint32_t width = 800;
        uint32_t height = 600;

        const char* title = "GRIViewport";
    };

    class GRIViewport
    {
    public:
        virtual ~GRIViewport() = default;
        virtual uint32_t get_width() const = 0;
        virtual uint32_t get_height() const = 0;

        virtual void* get_native_handle() const = 0;
    };
}