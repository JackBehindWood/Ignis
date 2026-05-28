#include "igpch.h"
#include "SceneSerializer.h"
#include "Scene.h"
#include "Entity.h"
#include "ScriptRegistry.h"
#include "Components/ComponentRegistry.h"
#include "Components/Components.h"
#include "Ignis/Foundation/YamlStream.h"

namespace Ignis
{

void SceneSerializer::serialize(const Scene& scene, const Path& path)
{
    YamlWriter writer;
    writer.begin_map("scene");
    writer.begin_sequence("entities");

    YAML::Emitter& out = writer.emitter();

    for (auto handle : const_cast<Scene&>(scene).registry().storage<entt::entity>())
    {
        Entity e(handle, const_cast<Scene*>(&scene));
        if (!e.is_valid())
        {
            continue;
        }

        out << YAML::BeginMap;
        out << YAML::Key << "components" << YAML::Value << YAML::BeginMap;

        for (const auto& desc : ComponentRegistry::all())
        {
            if (!desc.has_on(e))
            {
                continue;
            }

            if (StringView(desc.type_name) == "ScriptComponent")
            {
                const auto& sc = e.get_component<ScriptComponent>();
                out << YAML::Key << "ScriptComponent" << YAML::Value << YAML::BeginMap;
                out << YAML::Key << "script_class" << YAML::Value << sc.script_class;
                if (sc.instance)
                {
                    if (const ScriptDescriptor* sd = ScriptRegistry::find(sc.script_class))
                    {
                        out << YAML::Key << "properties" << YAML::Value << YAML::BeginMap;
                        for (const PropertyDescriptor& pd : sd->properties)
                        {
                            if (pd.serialize)
                            {
                                pd.serialize(sc.instance.get(), out);
                            }
                        }
                        out << YAML::EndMap;
                    }
                }
                out << YAML::EndMap;
            }
            else
            {
                desc.serialize(e, out);
            }
        }

        out << YAML::EndMap;
        out << YAML::EndMap;
    }

    writer.end_sequence();
    writer.end_map();
    writer.write(path);
}

bool SceneSerializer::deserialize(Scene& scene, const Path& path)
{
    YamlReader reader(path);
    if (!reader.is_open())
    {
        IG_CORE_ERROR("SceneSerializer: failed to parse '{}'", path.string());
        return false;
    }

    const YAML::Node& root       = reader.root_node();
    const YAML::Node  scene_node = root["scene"];
    if (!scene_node)
    {
        return false;
    }

    const YAML::Node entities_node = scene_node["entities"];
    if (!entities_node)
    {
        return true;
    }

    for (const auto& entity_node : entities_node)
    {
        Entity e = scene.create_entity_raw();

        const YAML::Node components_node = entity_node["components"];
        if (!components_node)
        {
            continue;
        }

        for (auto it = components_node.begin(); it != components_node.end(); ++it)
        {
            String key = it->first.as<std::string>();

            if (key == "ScriptComponent")
            {
                auto& sc        = e.add_component<ScriptComponent>();
                sc.script_class = it->second["script_class"] ? it->second["script_class"].as<std::string>() : "";
                sc.instance     = ScriptRegistry::create(sc.script_class);
                if (sc.instance)
                {
                    sc.instance->m_entity = e;
                    if (const ScriptDescriptor* sd = ScriptRegistry::find(sc.script_class))
                    {
                        if (it->second["properties"])
                        {
                            const YAML::Node& props_node = it->second["properties"];
                            for (const PropertyDescriptor& pd : sd->properties)
                            {
                                if (pd.deserialize)
                                {
                                    pd.deserialize(sc.instance.get(), props_node);
                                }
                            }
                        }
                    }
                    sc.instance->create();
                }
            }
            else
            {
                const ComponentDescriptor* desc = ComponentRegistry::find(StringView(key));
                if (desc)
                {
                    desc->deserialize(e, it->second);
                }
                else
                {
                    IG_CORE_WARN("SceneSerializer: unknown component '{}' — skipping", key);
                }
            }
        }
    }

    return true;
}

} // namespace Ignis
