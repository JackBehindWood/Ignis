#pragma once

#include "GRI/GRIDefinitions.h"

namespace Ignis
{
    class RenderMesh;
    class Material;
    class GRIBuffer;
    class GRIViewport;

    class Renderer
    {
    public:
        static void begin(GRIViewport* viewport, GRIClearValue clear = {});
        static void submit(RenderMesh* mesh, Material* material, GRIBuffer* transform_ubo);
        static void end();
    };
} // namespace Ignis
