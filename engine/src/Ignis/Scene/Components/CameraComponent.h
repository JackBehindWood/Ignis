#pragma once

#include "Ignis/Scene/SceneCamera.h"

namespace Ignis
{

IG_CLASS(Component)
struct CameraComponent
{
    IG_PROPERTY(EditAnywhere, SaveGame)
    SceneCamera camera;

    IG_PROPERTY(EditAnywhere, SaveGame)
    bool is_primary = true;
};

} // namespace Ignis
