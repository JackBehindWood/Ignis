#pragma once

#include "entt/entt.hpp"
#include "Ignis/Scene/CameraData.h"

namespace Ignis
{
class Entity;
struct ScriptComponent;

class Scene
{
public:
    Scene();

    Entity create_entity(StringView name = "Entity");
    Entity create_entity_raw();
    void   destroy_entity(Entity entity);
    void   update(float ts);

    Optional<CameraData> get_primary_camera_data() const;

    entt::registry& registry()
    {
        return m_registry;
    }
    const entt::registry& registry() const
    {
        return m_registry;
    }

private:
    void on_script_component_added(entt::registry& registry, entt::entity handle);
    void on_script_component_removed(entt::registry& registry, entt::entity handle);

    entt::registry m_registry;
};

} // namespace Ignis
