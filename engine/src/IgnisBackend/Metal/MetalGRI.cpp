#include "igpch.h"
#include "MetalGRI.h"
#include "MetalViewport.h"

namespace Ignis
{
    MetalGRI::MetalGRI()
    {
    }

    MetalGRI::~MetalGRI()
    {
    }

    void MetalGRI::init()
    {
        // Initialization code for Metal backend
    }

    void MetalGRI::shutdown()
    {
        // Shutdown code for Metal backend
    }

    GRIViewportPtr MetalGRI::create_viewport(uint32_t width, uint32_t height)
    {
        return create_unique<MetalViewport>(width, height);
    }
} // namespace Ignis