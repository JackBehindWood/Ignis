#pragma once

#include <Ignis.h>
#include <Ignis/Rendering/RenderMesh.h>
#include <Ignis/Rendering/FrameUniformAllocator.h>
#include <Ignis/Rendering/Material.h>
#include <Ignis/Math/Math.h>

namespace Ignis
{
class EditorShaderCache;

struct ThumbnailRenderRequest
{
    const RenderMesh* mesh          = nullptr;
    const Material*   material      = nullptr;
    GRITexture2D*     output_rt     = nullptr;
    float             bounds_radius = 1.0f;
    Math::Vec3f       bounds_center = {};
};

class ThumbnailRenderer
{
public:
    void init(EditorShaderCache& shaders);
    void shutdown();

    bool has_pending() const;
    void enqueue(ThumbnailRenderRequest req);
    void flush();

    const RenderMesh* get_sphere_mesh() const
    {
        return m_sphere_mesh.get();
    }

    static SharedPtr<RenderMesh> make_sphere_mesh();

private:
    FrameUniformAllocator          m_uniform_alloc;
    SharedPtr<RenderMesh>          m_sphere_mesh;
    Vector<ThumbnailRenderRequest> m_queue;
    GRIBufferPtr                   m_identity_instance_buf;
    GRIBufferPtr                   m_zero_visible_buf;
};

} // namespace Ignis
