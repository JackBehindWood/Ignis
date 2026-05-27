#pragma once

#include "Ignis/Foundation/Foundation.h"
#include "yaml-cpp/yaml.h"

namespace Ignis
{

class ScriptableEntity;

struct ScriptDescriptor
{
    String name;
    UniquePtr<ScriptableEntity> (*factory)()                            = nullptr;
    void (*serialize_instance)(const ScriptableEntity&, YAML::Emitter&) = nullptr;
    void (*deserialize_instance)(ScriptableEntity&, const YAML::Node&)  = nullptr;
};

class ScriptRegistry
{
public:
    static void                            register_script(ScriptDescriptor desc);
    static UniquePtr<ScriptableEntity>     create(StringView name);
    static const ScriptDescriptor*         find(StringView name);
    static const Vector<ScriptDescriptor>& all();

private:
    static Vector<ScriptDescriptor>       s_scripts;
    static UnorderedMap<String, uint32_t> s_by_name;
};

} // namespace Ignis
