#pragma once

#include "Ignis/Asset/Asset.h"
#include "Ignis/Rendering/GRI/GRIResource.h"

namespace Ignis
{
    class Shader : public Asset
    {
    public:
        Shader(AssetID id, GRIVertexShaderPtr vs, GRIPixelShaderPtr ps)
            : m_vertex_shader(std::move(vs)), m_pixel_shader(std::move(ps))
        {
            m_id = id;
        }

        GRIVertexShader* get_vertex_shader() const { return m_vertex_shader.get(); }
        GRIPixelShader*  get_pixel_shader()  const { return m_pixel_shader.get(); }

        static AssetType static_type() { return AssetType::Shader; }

    private:
        GRIVertexShaderPtr m_vertex_shader;
        GRIPixelShaderPtr  m_pixel_shader;
    };

} // namespace Ignis
