#pragma once

#include <Ignis/Scene/Scene.h>
#include <Ignis/Scene/Entity.h>

namespace Ignis
{

class PropertyPanel
{
public:
    PropertyPanel() = default;

    void draw(Scene& scene, Entity& selected_entity);
};

} // namespace Ignis
