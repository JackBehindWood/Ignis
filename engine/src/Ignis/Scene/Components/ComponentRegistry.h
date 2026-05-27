#pragma once

#include "Ignis/Scene/Entity.h"
#include "yaml-cpp/yaml.h"

namespace Ignis
{

struct ComponentDescriptor
{
    const char* type_name = nullptr;
    uint32_t    type_id   = 0;

    void (*add_to)(Entity&)                          = nullptr;
    void (*remove_from)(Entity&)                     = nullptr;
    bool (*has_on)(const Entity&)                    = nullptr;
    void (*serialize)(const Entity&, YAML::Emitter&) = nullptr;
    void (*deserialize)(Entity&, const YAML::Node&)  = nullptr;
};

class ComponentRegistry
{
public:
    static void                               register_component(ComponentDescriptor desc);
    static bool                               validate_unique_ids();
    static const ComponentDescriptor*         find(uint32_t type_id);
    static const ComponentDescriptor*         find(StringView name);
    static const Vector<ComponentDescriptor>& all();

private:
    static Vector<ComponentDescriptor>      s_descriptors;
    static UnorderedMap<uint32_t, uint32_t> s_by_id;
    static UnorderedMap<String, uint32_t>   s_by_name;
};

} // namespace Ignis
