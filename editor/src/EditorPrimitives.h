#pragma once

#include <Ignis.h>

namespace Ignis
{

enum class PrimShape : int
{
    Triangle = 0,
    Quad     = 1,
    Cube     = 2,
    Circle   = 3,
    Sphere   = 4,
    Pyramid  = 5,
    Count
};

class EditorPrimitives
{
public:
    static void init();

    static constexpr uint64_t prim_material_key()
    {
        return 0xED17'0000'0000'0000ULL;
    }
    static constexpr uint64_t prim_mesh_key(int index)
    {
        return 0xED17'0000'0000'0001ULL + static_cast<uint64_t>(index);
    }
    static constexpr int prim_count()
    {
        return static_cast<int>(PrimShape::Count);
    }

    static Entity spawn(Scene& scene, PrimShape shape, StringView name = {});
    static Entity spawn_cube(Scene& scene);
    static Entity spawn_sphere(Scene& scene);
    static Entity spawn_quad(Scene& scene);
    static Entity spawn_pyramid(Scene& scene);
};

} // namespace Ignis
