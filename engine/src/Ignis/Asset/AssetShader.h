#pragma once

#include "Ignis/Asset/Asset.h"
#include "Ignis/Rendering/Shaders/RenderShader.h"

namespace Ignis
{
    class AssetShader : public Asset
    {
    public:
        AssetShader(AssetID id, SharedPtr<RenderShader> shader)
            : m_shader(std::move(shader))
        {
            m_id = id;
        }

        RenderShader*  get_render_shader() const { return m_shader.get(); }
        GRIShaderStage get_stage()         const { return m_shader->get_stage(); }

        static AssetType static_type() { return AssetType::Shader; }

    private:
        SharedPtr<RenderShader> m_shader;
    };

} // namespace Ignis
