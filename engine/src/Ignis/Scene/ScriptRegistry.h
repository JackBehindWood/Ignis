#pragma once

#include "Ignis/Foundation/Foundation.h"
#include "yaml-cpp/yaml.h"

#include <functional>

namespace Ignis
{

class ScriptableEntity;

struct PropertyDescriptor
{
    String                                        name;
    String                                        type_name;
    Vector<String>                                specifiers;
    std::function<void(void*, YAML::Emitter&)>    serialize;
    std::function<void(void*, const YAML::Node&)> deserialize;
};

struct ScriptDescriptor
{
    String                                       name;
    std::function<UniquePtr<ScriptableEntity>()> factory;
    Vector<PropertyDescriptor>                   properties;
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
