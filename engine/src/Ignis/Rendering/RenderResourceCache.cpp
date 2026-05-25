#include "igpch.h"
#include "RenderResourceCache.h"

namespace Ignis
{
void RenderResourceCache::register_mesh(uint64_t key, SharedPtr<RenderMesh> mesh)
{
    m_meshes.insert_or_assign(key, std::move(mesh));
}

void RenderResourceCache::register_texture(uint64_t key, SharedPtr<RenderTexture2D> texture)
{
    m_textures.insert_or_assign(key, std::move(texture));
}

void RenderResourceCache::register_material(uint64_t key, SharedPtr<Material> material)
{
    m_materials.insert_or_assign(key, std::move(material));
}

SharedPtr<RenderMesh> RenderResourceCache::find_mesh(uint64_t key) const
{
    auto it = m_meshes.find(key);
    return it != m_meshes.end() ? it->second : nullptr;
}

SharedPtr<RenderTexture2D> RenderResourceCache::find_texture(uint64_t key) const
{
    auto it = m_textures.find(key);
    return it != m_textures.end() ? it->second : nullptr;
}

SharedPtr<Material> RenderResourceCache::find_material(uint64_t key) const
{
    auto it = m_materials.find(key);
    return it != m_materials.end() ? it->second : nullptr;
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
}
} // namespace Ignis
