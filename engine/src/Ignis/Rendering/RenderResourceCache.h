#pragma once

#include "Ignis/Rendering/Material.h"
#include "Ignis/Rendering/RenderMesh.h"
#include "Ignis/Rendering/RenderTexture2D.h"

namespace Ignis
{
class RenderResourceCache
{
public:
    uint16_t register_mesh(uint64_t key, SharedPtr<RenderMesh>);
    uint16_t register_texture(uint64_t key, SharedPtr<RenderTexture2D>);
    uint16_t register_material(uint64_t key, SharedPtr<Material>);

    SharedPtr<RenderMesh>      find_mesh(uint64_t key) const;
    SharedPtr<RenderTexture2D> find_texture(uint64_t key) const;
    SharedPtr<Material>        find_material(uint64_t key) const;

    uint16_t find_mesh_id(uint64_t key) const;
    uint16_t find_material_id(uint64_t key) const;

    void evict(uint64_t key);
    void clear();

private:
    static constexpr uint16_t k_invalid_id = 0xFFFFu;

    struct MeshEntry
    {
        SharedPtr<RenderMesh> resource;
        uint16_t              id = k_invalid_id;
    };
    struct TextureEntry
    {
        SharedPtr<RenderTexture2D> resource;
        uint16_t                   id = k_invalid_id;
    };
    struct MaterialEntry
    {
        SharedPtr<Material> resource;
        uint16_t            id = k_invalid_id;
    };

    UnorderedMap<uint64_t, MeshEntry>     m_meshes;
    UnorderedMap<uint64_t, TextureEntry>  m_textures;
    UnorderedMap<uint64_t, MaterialEntry> m_materials;

    uint16_t m_next_mesh_id     = 0;
    uint16_t m_next_texture_id  = 0;
    uint16_t m_next_material_id = 0;
};
} // namespace Ignis
