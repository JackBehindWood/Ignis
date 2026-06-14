#pragma once

#include "Scene.h"
#include "RenderScene.h"
#include "Ignis/Scene/CameraData.h"

namespace Ignis
{

// SceneExtractor owns a per-view RenderScene and reuses its vector capacity across frames.
// The returned reference is valid until the next call to extract().
class SceneExtractor
{
public:
    // Asset-loading phase: traverse ECS, trigger deferred loads, populate
    // RenderResourceCache with meshes/materials/textures/PSOs, and create
    // engine fallback resources (white texture, fallback material).
    // Safe to call every frame; no-ops on already-cached resources.
    void prepare(Scene& scene);

    // Snapshot phase: cache-hit-only zone — no GPU resource creation.
    // Fills and returns a reference to the internally-owned RenderScene.
    // Valid until the next call to extract() on this instance.
    const RenderScene& extract(const Scene& scene, const CameraData& camera);

private:
    RenderScene       m_scene;       // reused each frame — capacity retained in its vectors
    Vector<DrawProxy> m_vis_scratch; // pre-frustum-cull all-candidate list
};

} // namespace Ignis
