#pragma once

#include "Ignis/Core/UUID.h"

namespace Ignis
{

IG_CLASS(Component)
struct IDComponent
{
    IG_PROPERTY(id)
    UUID id = UUID();
};

} // namespace Ignis
