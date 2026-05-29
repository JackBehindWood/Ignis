#pragma once

#include "Ignis/Asset/Asset.h"

namespace Ignis
{
class AssetMaterial : public Asset
{
public:
    AssetMaterial(AssetID id, Path shader_source, String vertex_layout, Vector<uint8_t> param_data,
                  bool two_sided = false, bool transparent = false)
        : m_shader_source(std::move(shader_source)),
          m_vertex_layout(std::move(vertex_layout)),
          m_param_data(std::move(param_data)),
          m_two_sided(two_sided),
          m_transparent(transparent)
    {
        m_id = id;
    }

    const Path& get_shader_source() const
    {
        return m_shader_source;
    }
    const String& get_vertex_layout() const
    {
        return m_vertex_layout;
    }
    const Vector<uint8_t>& get_param_data() const
    {
        return m_param_data;
    }
    bool is_two_sided() const
    {
        return m_two_sided;
    }
    bool is_transparent() const
    {
        return m_transparent;
    }

    static AssetType static_type()
    {
        return AssetType::Material;
    }

private:
    Path            m_shader_source;
    String          m_vertex_layout;
    Vector<uint8_t> m_param_data;
    bool            m_two_sided   = false;
    bool            m_transparent = false;
};

} // namespace Ignis
