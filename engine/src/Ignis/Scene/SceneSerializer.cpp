#include "igpch.h"
#include "SceneSerializer.h"
#include "Scene.h"
#include "Entity.h"
#include "ScriptRegistry.h"
#include "Components/ComponentRegistry.h"
#include "Components/Components.h"
#include "yaml-cpp/yaml.h"

#include <fstream>

namespace Ignis
{

// TODO: we should use Foundation/YamlStream.h

void SceneSerializer::serialize(const Scene& scene, const Path& path)
{
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "scene" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "entities" << YAML::Value << YAML::BeginSeq;

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
                        sd->serialize_instance(*sc.instance, out);
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

    out << YAML::EndSeq;
    out << YAML::EndMap;
    out << YAML::EndMap;

    std::ofstream file(path);
    file << out.c_str();
}

bool SceneSerializer::deserialize(Scene& scene, const Path& path)
{
    YAML::Node root;
    try
    {
        root = YAML::LoadFile(path.string());
    }
    catch (const YAML::Exception& ex)
    {
        IG_CORE_ERROR("SceneSerializer: failed to parse '{}': {}", path.string(), ex.what());
        return false;
    }

    auto scene_node = root["scene"];
    if (!scene_node)
    {
        return false;
    }

    auto entities_node = scene_node["entities"];
    if (!entities_node)
    {
        return true;
    }

    for (const auto& entity_node : entities_node)
    {
        Entity e = scene.create_entity_raw();

        auto components_node = entity_node["components"];
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
                            sd->deserialize_instance(*sc.instance, it->second["properties"]);
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
