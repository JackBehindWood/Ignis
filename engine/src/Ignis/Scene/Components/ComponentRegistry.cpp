#include "igpch.h"
#include "Ignis/Scene/Components/ComponentRegistry.h"

namespace Ignis
{

Vector<ComponentDescriptor>      ComponentRegistry::s_descriptors;
UnorderedMap<uint32_t, uint32_t> ComponentRegistry::s_by_id;
UnorderedMap<String, uint32_t>   ComponentRegistry::s_by_name;

void ComponentRegistry::register_component(ComponentDescriptor desc)
{
    if (s_by_id.count(desc.type_id))
    {
        return;
    }
    uint32_t idx              = static_cast<uint32_t>(s_descriptors.size());
    s_by_id[desc.type_id]     = idx;
    s_by_name[desc.type_name] = idx;
    s_descriptors.push_back(std::move(desc));
}

bool ComponentRegistry::validate_unique_ids()
{
    UnorderedMap<uint32_t, const char*> seen;
    for (const auto& desc : s_descriptors)
    {
        auto [it, inserted] = seen.emplace(desc.type_id, desc.type_name);
        if (!inserted)
        {
            IG_CORE_ERROR("Duplicate component type_id 0x{:08X}: '{}' and '{}'", desc.type_id, it->second,
                          desc.type_name);
            return false;
        }
    }
    return true;
}

const ComponentDescriptor* ComponentRegistry::find(uint32_t type_id)
{
    auto it = s_by_id.find(type_id);
    if (it == s_by_id.end())
    {
        return nullptr;
    }
    return &s_descriptors[it->second];
}

const ComponentDescriptor* ComponentRegistry::find(StringView name)
{
    auto it = s_by_name.find(String(name));
    if (it == s_by_name.end())
    {
        return nullptr;
    }
    return &s_descriptors[it->second];
}

const Vector<ComponentDescriptor>& ComponentRegistry::all()
{
    return s_descriptors;
}

} // namespace Ignis
