#pragma once

namespace Ignis
{

IG_CLASS(Component)
struct NameComponent
{
    IG_PROPERTY(EditAnywhere, SaveGame)
    String name = "Entity";
};

} // namespace Ignis
