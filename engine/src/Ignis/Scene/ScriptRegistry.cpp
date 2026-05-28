#include "igpch.h"
#include "ScriptRegistry.h"
#include "ScriptableEntity.h"

namespace Ignis
{

Vector<ScriptDescriptor>       ScriptRegistry::s_scripts;
UnorderedMap<String, uint32_t> ScriptRegistry::s_by_name;

void ScriptRegistry::register_script(ScriptDescriptor desc)
{
    String key = desc.name;
    if (s_by_name.count(key))
    {
        return;
    }
    const uint32_t idx = static_cast<uint32_t>(s_scripts.size());
    s_by_name[key]     = idx;
    s_scripts.push_back(std::move(desc));
}

UniquePtr<ScriptableEntity> ScriptRegistry::create(StringView name)
{
    const ScriptDescriptor* d = find(name);
    if (d && d->factory)
    {
        return d->factory();
    }
    return nullptr;
}

const ScriptDescriptor* ScriptRegistry::find(StringView name)
{
    auto it = s_by_name.find(String(name));
    if (it == s_by_name.end())
    {
        return nullptr;
    }
    return &s_scripts[it->second];
}

const Vector<ScriptDescriptor>& ScriptRegistry::all()
{
    return s_scripts;
}

} // namespace Ignis
