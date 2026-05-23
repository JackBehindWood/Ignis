#pragma once

#include "entt/entt.hpp"

namespace Ignis
{
    class Entity;

    class Scene
    {
    public:
        Entity create_entity();
        void   destroy_entity(Entity entity);
        void  update(float ts);

        entt::registry& registry() { return m_registry; }

    private:
        entt::registry m_registry;
    };

} // namespace Ignis
