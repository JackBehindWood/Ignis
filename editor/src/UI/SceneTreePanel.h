#pragma once

#include <Ignis/Scene/Scene.h>
#include <Ignis/Scene/Entity.h>

namespace Ignis
{

class SceneTreePanel
{
public:
    SceneTreePanel() = default;

    void draw(Scene& scene, Entity& selected_entity);

private:
    void draw_entity_node(Scene& scene, entt::entity e, Entity& selected_entity);
};

} // namespace Ignis
