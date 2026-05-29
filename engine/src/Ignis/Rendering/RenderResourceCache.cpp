#include "igpch.h"
#include "RenderResourceCache.h"

namespace Ignis
{

uint16_t RenderResourceCache::register_mesh(uint64_t key, SharedPtr<RenderMesh> mesh)
{
    auto it = m_meshes.find(key);
    if (it != m_meshes.end())
    {
        it->second.resource = std::move(mesh);
        return it->second.id;
    }
    const uint16_t id = m_next_mesh_id++;
    m_meshes[key]     = {std::move(mesh), id};
    return id;
}

uint16_t RenderResourceCache::register_texture(uint64_t key, SharedPtr<RenderTexture2D> texture)
{
    auto it = m_textures.find(key);
    if (it != m_textures.end())
    {
        it->second.resource = std::move(texture);
        return it->second.id;
    }
    const uint16_t id = m_next_texture_id++;
    m_textures[key]   = {std::move(texture), id};
    return id;
}

uint16_t RenderResourceCache::register_material(uint64_t key, SharedPtr<Material> material)
{
    auto it = m_materials.find(key);
    if (it != m_materials.end())
    {
        it->second.resource = std::move(material);
        return it->second.id;
    }
    const uint16_t id = m_next_material_id++;
    m_materials[key]  = {std::move(material), id};
    return id;
}

SharedPtr<RenderMesh> RenderResourceCache::find_mesh(uint64_t key) const
{
    auto it = m_meshes.find(key);
    return it != m_meshes.end() ? it->second.resource : nullptr;
}

SharedPtr<RenderTexture2D> RenderResourceCache::find_texture(uint64_t key) const
{
    auto it = m_textures.find(key);
    return it != m_textures.end() ? it->second.resource : nullptr;
}

SharedPtr<Material> RenderResourceCache::find_material(uint64_t key) const
{
    auto it = m_materials.find(key);
    return it != m_materials.end() ? it->second.resource : nullptr;
}

uint16_t RenderResourceCache::find_mesh_id(uint64_t key) const
{
    auto it = m_meshes.find(key);
    return it != m_meshes.end() ? it->second.id : k_invalid_id;
}

uint16_t RenderResourceCache::find_material_id(uint64_t key) const
{
    auto it = m_materials.find(key);
    return it != m_materials.end() ? it->second.id : k_invalid_id;
}

void RenderResourceCache::evict(uint64_t key)
{
    m_meshes.erase(key);
    m_textures.erase(key);
    m_materials.erase(key);
}

void RenderResourceCache::clear()
{
    m_meshes.clear();
    m_textures.clear();
    m_materials.clear();
    m_next_mesh_id     = 0;
    m_next_texture_id  = 0;
    m_next_material_id = 0;
}

} // namespace Ignis
