#pragma once

#include "Asset.h"
#include "AssetManager.h"
#include "Ignis/Foundation/SharedPtr.h"

namespace Ignis
{

    // Lightweight handle that resolves to a loaded asset on demand.
    // Prefer extracting the SharedPtr once per frame rather than calling get() in tight loops.
    template<typename T>
    class AssetHandle
    {
    public:
        AssetHandle() = default;
        explicit AssetHandle(AssetID id) : m_id(id) {}

        SharedPtr<T>    get()           const { return AssetManager::get().load_as<T>(m_id); }
        SharedPtr<T>    operator->()    const { return get(); }
        T&              operator*()     const { return *get(); }

        AssetID         get_id()        const { return m_id; }
        bool            is_valid()      const { return static_cast<uint64_t>(m_id) != UUID::s_invalid; }
        explicit        operator bool() const { return is_valid(); }

    private:
        AssetID m_id;
    };

} // namespace Ignis
