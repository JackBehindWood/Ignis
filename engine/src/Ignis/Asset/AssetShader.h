#pragma once

#include "Ignis/Asset/Asset.h"
#include "Ignis/Rendering/RenderShader.h"

namespace Ignis
{
    class AssetShader : public Asset
    {
    public:
        AssetShader(AssetID id, SharedPtr<RenderShader> render_shader)
            : m_render_shader(std::move(render_shader))
        {
            m_id = id;
        }

        RenderShader* get_render_shader() const { return m_render_shader.get(); }

        static AssetType static_type() { return AssetType::Shader; }

    private:
        SharedPtr<RenderShader> m_render_shader;
    };

} // namespace Ignis
