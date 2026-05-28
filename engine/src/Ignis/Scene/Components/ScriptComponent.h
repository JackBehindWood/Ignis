#pragma once

#include "Ignis/Scene/ScriptableEntity.h"

namespace Ignis
{

IG_CLASS(Component)
struct ScriptComponent
{
    IG_PROPERTY(EditAnywhere, SaveGame)
    String script_class;

    UniquePtr<ScriptableEntity> instance; // runtime-only, not serialized
};

} // namespace Ignis
