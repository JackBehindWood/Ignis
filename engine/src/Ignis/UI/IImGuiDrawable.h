#pragma once

namespace Ignis
{

class IImGuiDrawable
{
public:
    virtual ~IImGuiDrawable() = default;
    virtual void draw_imgui() = 0;
    virtual bool has_pending_resize()
    {
        return false;
    }
    virtual void flush_resize()
    {
    }
};

} // namespace Ignis
