#include "igpch.h"
#include "GRI.h"

#include "IgnisBackend/Metal/MetalGRI.h"

namespace Ignis
{
    GRI* GRI::init(GRIRenderAPI api)
    {
        switch (api)
        {
        case GRIRenderAPI::None:
            IG_CORE_ASSERT(false, "No rendering API selected!");
            return nullptr;
        case GRIRenderAPI::OpenGL:
            IG_CORE_ASSERT(false, "OpenGL not yet implemented!");
            return nullptr;
        case GRIRenderAPI::Vulkan:
            IG_CORE_ASSERT(false, "Vulkan not yet implemented!");
            return nullptr;
        case GRIRenderAPI::DirectX12:
            IG_CORE_ASSERT(false, "DirectX12 not yet implemented!");
            return nullptr;
        case GRIRenderAPI::Metal:
            return new MetalGRI();
        default:
            IG_CORE_ASSERT(false, "Unknown rendering API!");
            return nullptr;
        }
    }
} // namespace Ignis