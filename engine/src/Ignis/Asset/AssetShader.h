#pragma once

#include "Ignis/Asset/Asset.h"
#include "Ignis/Rendering/RenderShader.h"

namespace Ignis
{
    class AssetShader : public Asset
    {
    public:
        AssetShader(AssetID id, SharedPtr<RenderShader> vs, SharedPtr<RenderShader> ps)
            : m_vs(std::move(vs)), m_ps(std::move(ps))
        {
            m_id = id;
        }

        RenderShader* get_vertex_render_shader() const { return m_vs.get(); }
        RenderShader* get_pixel_render_shader()  const { return m_ps.get(); }

        static AssetType static_type() { return AssetType::Shader; }

    private:
        SharedPtr<RenderShader> m_vs;
        SharedPtr<RenderShader> m_ps;
    };

} // namespace Ignis
