#pragma once

#include "Ignis/Foundation/Foundation.h"

namespace Ignis
{

class Scene;

class SceneSerializer
{
public:
    void serialize(const Scene& scene, const Path& path);
    bool deserialize(Scene& scene, const Path& path);
};

} // namespace Ignis
