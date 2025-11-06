#pragma once

#include "GRI/GRI.h"
#include "GRI/GRICommandList.h"
#include "Ignis/Core/Log.h"

namespace Ignis
{
    class RenderSystem
    {
    private:
        inline static GRI* s_GRI = nullptr;
        inline static GRICommandListExecutor s_cmd_list_executor;
    public:
        //TODO: Remove
        static inline void submit() { s_cmd_list_executor.submit(); }

        static void init(GRIRenderAPI api)
        {
            if (s_GRI)
            {
                IG_CORE_WARN("RenderSystem already initialized!");
                return;
            }
            
            s_GRI = GRI::create(api);
            s_GRI->init();
            get_command_list().initialise_context(s_GRI->get_context());
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
        static inline GRICommandList& get_command_list() { return s_cmd_list_executor.get_command_list(); }
    };
} // namespace Ignis
