#include "igpch.h"
#include "Scene.h"
#include "Entity.h"
#include "Components/Components.h"

namespace Ignis
{

Scene::Scene()
{
    m_registry.on_construct<ScriptComponent>().connect<&Scene::on_script_component_added>(this);
    m_registry.on_destroy<ScriptComponent>().connect<&Scene::on_script_component_removed>(this);
}

Entity Scene::create_entity(StringView name)
{
    Entity e(m_registry.create(), this);
    e.add_component<IDComponent>();
    e.add_component<NameComponent>().name = String(name);
    return e;
}

Entity Scene::create_entity_raw()
{
    return Entity(m_registry.create(), this);
}

void Scene::destroy_entity(Entity entity)
{
    m_registry.destroy(entity);
}

void Scene::update(float ts)
{
    auto view = m_registry.view<ScriptComponent>();
    for (auto [handle, sc] : view.each())
    {
        if (sc.instance)
        {
            sc.instance->update(ts);
        }
    }
}

Optional<CameraData> Scene::get_primary_camera_data() const
{
    auto cam_view = m_registry.view<const TransformComponent, const CameraComponent>();
    for (auto [handle, transform, cam_comp] : cam_view.each())
    {
        if (!cam_comp.is_primary)
        {
            continue;
        }

        const Math::Vec3f forward  = transform.rotation.rotate({0.0f, 0.0f, 1.0f});
        const Math::Vec3f world_up = {0.0f, 1.0f, 0.0f};
        const Math::Mat4f view     = Math::look_at(transform.position, transform.position + forward, world_up);
        const Math::Mat4f proj     = cam_comp.camera.get_projection();
        return CameraData::from_matrices(view, proj, transform.position);
    }
    return NullOpt;
}

void Scene::on_script_component_added(entt::registry& reg, entt::entity handle)
{
    auto& sc = reg.get<ScriptComponent>(handle);
    if (!sc.instance)
    {
        return;
    }
    sc.instance->m_entity = Entity(handle, this);
    sc.instance->create();
}

void Scene::on_script_component_removed(entt::registry& reg, entt::entity handle)
{
    auto& sc = reg.get<ScriptComponent>(handle);
    if (!sc.instance)
    {
        return;
    }
    sc.instance->destroy();
}

} // namespace Ignis
