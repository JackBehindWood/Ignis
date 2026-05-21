#pragma once

#include "Ignis/Asset/Asset.h"
#include "Ignis/Rendering/Material.h"

namespace Ignis
{
    class AssetMaterial : public Asset
    {
    public:
        AssetMaterial(AssetID id, SharedPtr<Material> material)
            : m_material(std::move(material))
        {
            m_id = id;
        }

        Material*        get_material() const { return m_material.get(); }
        static AssetType static_type()        { return AssetType::Material; }

    private:
        SharedPtr<Material> m_material;
    };

} // namespace Ignis
