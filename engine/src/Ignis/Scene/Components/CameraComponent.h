#pragma once

#include "Ignis/Scene/SceneCamera.h"

namespace Ignis
{

IG_CLASS(Component)
struct CameraComponent
{
    IG_PROPERTY(camera)
    SceneCamera camera;

    IG_PROPERTY(is_primary)
    bool is_primary = true;
};

} // namespace Ignis
