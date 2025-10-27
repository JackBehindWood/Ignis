#pragma once

#include "GRI/GRI.h"
#include "Ignis/Core/Log.h"

namespace Ignis
{
    class RenderSystem
    {
    private:
        inline static GRI* s_GRI = nullptr;
    public:
        static void init(GRIRenderAPI api)
        {
            if (s_GRI)
            {
                IG_CORE_WARN("RenderSystem already initialized!");
                return;
            }
            
            s_GRI = GRI::create(api);
            s_GRI->init();
        }

        static void shutdown()
        {
            if (!s_GRI)
            {
                IG_CORE_WARN("RenderSystem not initialized!");
                return;
            }

            s_GRI->shutdown();
            delete s_GRI;
            s_GRI = nullptr;
        }

        static inline GRI* get_gri() { return s_GRI; }
    };
} // namespace Ignis
