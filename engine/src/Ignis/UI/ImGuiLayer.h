#pragma once

#ifdef ENGINE_IMGUI

#include "IImGuiDrawable.h"
#include "Ignis/Core/Layer.h"
#include "Ignis/Foundation/Foundation.h"

struct ImFont;

namespace Ignis
{

class ImGuiLayer : public Layer
{
public:
    ImGuiLayer();
    ~ImGuiLayer() override = default;

    void attach() override;
    void detach() override;
    void update(Timestep ts) override
    {
    }
    void render() override;
    void event(Event& e) override;

    static void register_drawable(IImGuiDrawable* d);
    static void unregister_drawable(IImGuiDrawable* d);

    void set_ini_path(const String& path);

    static ImFont* mono_font()
    {
        return s_mono_font;
    }

private:
    static void                           apply_dark_theme();
    String                                m_ini_path;
    inline static Vector<IImGuiDrawable*> s_drawables;
    inline static ImFont*                 s_mono_font = nullptr;
};

} // namespace Ignis

#endif // ENGINE_IMGUI
