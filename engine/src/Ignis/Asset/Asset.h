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
    };

    struct AssetMetadata
    {
        AssetID     ID;
        AssetType   Type            = AssetType::None;
        Path        source_path;    // raw source asset  (e.g. resources/assets/textures/rock.png)
        Path        compiled_path;  // cooked binary     (e.g. resources/cache/<uuid>.igasset)
        bool        cache_compiled  = false; // write .igasset to disk only if true

        bool        is_valid() const { return static_cast<uint64_t>(ID) != UUID::s_invalid; }
    };

    class Asset : public RefCounted
    {
    public:
        AssetID get_id() const { return m_id; }

    protected:
        AssetID m_id;
    };

} // namespace Ignis
