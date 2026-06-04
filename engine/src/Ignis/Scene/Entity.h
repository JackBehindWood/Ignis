#pragma once

#include "Ignis/Scene/Scene.h"

namespace Ignis
{
class Entity
{
    friend class Scene;

public:
    Entity() = default;
    Entity(entt::entity handle, Scene* scene)
        : m_handle(handle),
          m_scene(scene)
    {
    }

    void destroy()
    {
        if (is_valid())
        {
            m_scene->destroy_entity(*this);
        }
    }

    template <typename T, typename... Args>
    T& add_component(Args&&... args)
    {
        return m_scene->registry().emplace<T>(m_handle, std::forward<Args>(args)...);
    }

    template <typename T>
    void remove_component()
    {
        m_scene->registry().erase<T>(m_handle);
    }

    template <typename T>
    T& get_component()
    {
        return m_scene->registry().get<T>(m_handle);
    }

    template <typename T>
    const T& get_component() const
    {
        return m_scene->registry().get<T>(m_handle);
    }

    template <typename T>
    bool has_component() const
    {
        return m_scene->registry().all_of<T>(m_handle);
    }

    operator entt::entity() const
    {
        return m_handle;
    }
    bool is_valid() const
    {
        return m_scene && m_scene->registry().valid(m_handle);
    }

    operator bool() const
    {
        return is_valid();
    }
    operator uint32_t() const
    {
        return static_cast<uint32_t>(m_handle);
    }

    bool operator==(const Entity& other) const
    {
        return m_handle == other.m_handle && m_scene == other.m_scene;
    }
    bool operator!=(const Entity& other) const
    {
        return !(*this == other);
    }

private:
    entt::entity m_handle = entt::null;
    Scene*       m_scene  = nullptr;
};

template <typename Func>
inline void Scene::each_entity(Func&& func)
{
    for (auto handle : m_registry.storage<entt::entity>())
    {
        func(Entity{handle, this});
    }
}

inline Entity Scene::EntityIterator::operator*() const
{
    return Entity{*it, scene};
}

} // namespace Ignis
