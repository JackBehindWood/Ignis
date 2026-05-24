#pragma once
#include <cstdint>

namespace Ignis
{
    enum class ShaderTarget : uint8_t
    {
        Vulkan_SPIRV     = 0,
        Metal_MSL        = 1, // Precompiled .metallib binary (MSL → AIR → metallib via xcrun metal + xcrun metallib)
        DX12_DXIL        = 2,
        OpenGL_GLSL      = 3,
        HLSL             = 4, // Source-language compiler target (HLSL → SPIR-V via DXC)
    };

} // namespace Ignis
