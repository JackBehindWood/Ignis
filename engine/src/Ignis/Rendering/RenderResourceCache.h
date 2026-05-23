#pragma once

#include "Ignis/Rendering/RenderMesh.h"
#include "Ignis/Rendering/RenderTexture2D.h"

namespace Ignis
{
    class RenderResourceCache
    {
    public:
        void register_mesh(uint64_t key, SharedPtr<RenderMesh>);
        void register_texture(uint64_t key, SharedPtr<RenderTexture2D>);

        SharedPtr<RenderMesh>      find_mesh(uint64_t key)    const;
        SharedPtr<RenderTexture2D> find_texture(uint64_t key) const;

        void evict(uint64_t key);
        void clear();

    private:
        UnorderedMap<uint64_t, SharedPtr<RenderMesh>>      m_meshes;
        UnorderedMap<uint64_t, SharedPtr<RenderTexture2D>> m_textures;
    };
} // namespace Ignis
