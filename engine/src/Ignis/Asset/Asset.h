#pragma once

#include "Ignis/Core/UUID.h"
#include "Ignis/Foundation/RefCounted.h"
#include "Ignis/Foundation/Filesystem.h"

namespace Ignis
{

using AssetID = UUID;

enum class AssetType : uint16_t
{
    None = 0,
    Texture2D,
    Shader,
    Mesh,
    Material,
};

struct AssetMetadata
{
    AssetID   ID;
    AssetType Type = AssetType::None;
    Path      source_path;
    Path      compiled_path;
    bool      cache_compiled = false;
    uint64_t  user_data      = 0;

    bool is_valid() const
    {
        return static_cast<uint64_t>(ID) != UUID::s_invalid;
    }
};

class Asset : public RefCounted
{
public:
    AssetID get_id() const
    {
        return m_id;
    }

protected:
    AssetID m_id;
};

} // namespace Ignis
