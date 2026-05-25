#pragma once

#include "Ignis/Asset/Asset.h"
#include "Ignis/Asset/AssetTypes.h"

namespace Ignis
{
class AssetShader : public Asset
{
public:
    AssetShader(AssetID id, Path source_path, AssetShaderStage stage, String entry_point)
        : m_source_path(std::move(source_path)),
          m_stage(stage),
          m_entry_point(std::move(entry_point))
    {
        m_id = id;
    }

    const Path& get_source_path() const
    {
        return m_source_path;
    }
    AssetShaderStage get_stage() const
    {
        return m_stage;
    }
    const String& get_entry_point() const
    {
        return m_entry_point;
    }

    static AssetType static_type()
    {
        return AssetType::Shader;
    }

private:
    Path             m_source_path;
    AssetShaderStage m_stage;
    String           m_entry_point;
};

} // namespace Ignis
