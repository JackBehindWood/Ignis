#pragma once

#include <Ignis/Rendering/GRI/GRIDefinitions.h>
#include <Ignis/Rendering/GRI/GRIResource.h>
#include <Ignis/Rendering/Material.h>
#include <Ignis/Rendering/RenderGraph/RGResource.h>
#include <Ignis/Scene/Entity.h>

namespace Ignis
{
class RGBuilder;

class EditorSceneOverlay
{
public:
    void            resize(uint32_t w, uint32_t h);
    void            draw_grid(RGTextureHandle color, RGTextureHandle depth, RGBuilder& builder);
    RGTextureHandle draw_selection_mask(Entity selected, RGTextureHandle depth, RGBuilder& builder);
    void            draw_outline_composite(RGTextureHandle color, RGTextureHandle mask_rt, RGBuilder& builder);

    GRITexture2D* get_sel_mask_rt() const
    {
        return m_sel_mask_rt.get();
    }

private:
    void ensure_sel_material();

    GRITexture2DPtr     m_sel_mask_rt;
    GRIBufferPtr        m_sel_instance_buf;
    SharedPtr<Material> m_sel_material;
    uint32_t            m_width  = 0;
    uint32_t            m_height = 0;
};

} // namespace Ignis
