#pragma once

#include "Ignis/Asset/Asset.h"
#include "Ignis/Rendering/RenderShader.h"

namespace Ignis
{
    class AssetShader : public Asset
    {
    public:
        AssetShader(AssetID id, RenderShader render_shader)
            : m_render_shader(std::move(render_shader))
        {
            m_id = id;
        }

        const RenderShader& get_render_shader() const { return m_render_shader; }

        static AssetType static_type() { return AssetType::Shader; }

    private:
        RenderShader m_render_shader;
    };

} // namespace Ignis
