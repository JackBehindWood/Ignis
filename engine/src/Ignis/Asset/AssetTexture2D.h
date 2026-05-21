#pragma once

#include "Ignis/Asset/Asset.h"
#include "Ignis/Rendering/RenderTexture2D.h"

namespace Ignis
{
    class AssetTexture2D : public Asset
    {
    public:
        AssetTexture2D(AssetID id, SharedPtr<RenderTexture2D> rt)
            : m_render_texture(std::move(rt))
        {
            m_id = id;
        }

        SharedPtr<RenderTexture2D> get_render_texture() const { return m_render_texture; }

        static AssetType static_type() { return AssetType::Texture2D; }

    private:
        SharedPtr<RenderTexture2D> m_render_texture;
    };
}
