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
    void   clear();
    void   update(float ts);

    Optional<CameraData> get_primary_camera_data() const;

    struct EntityIterator
    {
        using StorageIt = decltype(std::declval<entt::registry>().storage<entt::entity>().begin());

        Scene*    scene;
        StorageIt it;

        Entity          operator*() const;
        EntityIterator& operator++()
        {
            ++it;
            return *this;
        }
        bool operator!=(const EntityIterator& o) const
        {
            return it != o.it;
        }
    };

    struct EntityView
    {
        Scene*         scene;
        EntityIterator begin()
        {
            auto& storage = scene->m_registry.storage<entt::entity>();
            return {scene,
                    storage.end() - static_cast<EntityIterator::StorageIt::difference_type>(storage.free_list())};
        }
        EntityIterator end()
        {
            return {scene, scene->m_registry.storage<entt::entity>().end()};
        }
    };

    EntityView get_entities()
    {
        return {this};
    }

    template <typename Func>
    void each_entity(Func&& func);

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
