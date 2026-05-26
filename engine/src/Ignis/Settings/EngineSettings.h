#pragma once

namespace Ignis
{
struct RenderSettings
{
    bool     vsync        = true;
    uint32_t msaa_samples = 1;
    uint32_t target_fps   = 60;
};

struct EngineSettings
{
    RenderSettings rendering;
};

} // namespace Ignis
