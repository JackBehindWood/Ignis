#include "igpch.h"
#include "Ignis/Scene/Entity.h"

namespace Ignis
{
Entity Scene::create_entity()
{
    return Entity(m_registry.create(), this);
}

void Scene::destroy_entity(Entity entity)
{
    m_registry.destroy(entity);
}

void Scene::update(float ts)
{
}

} // namespace Ignis
