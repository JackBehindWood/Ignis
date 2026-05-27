#pragma once

#include "Ignis/Scene/Entity.h"

namespace Ignis
{

class ScriptableEntity
{
    friend class SceneSerializer;

public:
    virtual ~ScriptableEntity() = default;

    virtual void create()
    {
    }
    virtual void destroy()
    {
    }
    virtual void update(float ts)
    {
    }

    template <typename T>
    T& get_component()
    {
        return m_entity.get_component<T>();
    }

    template <typename T>
    bool has_component() const
    {
        return m_entity.has_component<T>();
    }

protected:
    Entity m_entity;
    friend class Scene;
};

} // namespace Ignis
