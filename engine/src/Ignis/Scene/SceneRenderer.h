#pragma once

#include "Scene.h"
#include "Ignis/Math/Math.h"
#include "Ignis/Rendering/GRI/GRICommandList.h"
#include "Ignis/Rendering/RenderMesh.h"
#include "Ignis/Rendering/Material.h"
#include "Ignis/Rendering/RenderGraph/RGResource.h"

namespace Ignis
{
    class RGBuilder;

    struct FrameDrawItem
    {
        const RenderMesh* mesh     = nullptr;
        const Material*   material = nullptr;
        Math::Mat4f       world    = Math::Mat4f::identity();
        float             depth    = 0.0f;
    };

    class SceneRenderer
    {
    public:
        void render_scene(Scene& scene, RGBuilder& builder, RGTextureHandle backbuffer, GRITexture2D* scene_texture);

    private:
        // Persistent across frames; .clear()'d at the top of each render_scene call.
        // Avoids per-frame heap allocation once capacity stabilises.
        Vector<FrameDrawItem> m_opaque;
        Vector<FrameDrawItem> m_transparent;
    };

} // namespace Ignis
