#pragma once

#include <Ignis.h>

namespace Ignis
{

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
        return 6;
    }
};

} // namespace Ignis
