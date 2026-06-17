#pragma once
#include "IPanel.h"
#include <Ignis/Asset/Asset.h>
#include <Ignis/Core/UUID.h>

namespace Ignis
{
class GRITexture2D;

class MaterialEditorPanel : public IPanel
{
public:
    PanelId     get_id() const override;
    const char* get_title() const override
    {
        return "Material Editor";
    }
    void update(float ts, IWorkspaceData* ctx) override;
    void draw(IWorkspaceData* ctx) override;
    bool is_closeable() const override
    {
        return true;
    }

private:
    static bool draw_pbr_texture_slot(const char* label, uint32_t& tex_idx_inout, AssetID* asset_id_inout = nullptr,
                                      GRITexture2D* default_preview = nullptr);
    UUID        m_entity_id = UUID(UUID::s_invalid);
};

} // namespace Ignis
