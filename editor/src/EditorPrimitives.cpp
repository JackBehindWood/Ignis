#include "edpch.h"
#include "EditorPrimitives.h"
#include "EditorShaderCache.h"
#include "Ignis/Rendering/Renderer.h"
#include "Ignis/Rendering/MaterialFactory.h"
#include "Ignis/Rendering/RenderMesh.h"

namespace Ignis
{

namespace
{

struct PrimVertex
{
    float px, py, pz;
    float nx, ny, nz;
    float u, v;
};

static const PrimVertex k_tri_verts[] = {
    {0.000f, 0.500f, 0.f, 0, 0, 1, 0.5f, 0.0f},
    {-0.433f, -0.250f, 0.f, 0, 0, 1, 0.0f, 1.0f},
    {0.433f, -0.250f, 0.f, 0, 0, 1, 1.0f, 1.0f},
};
static const uint32_t k_tri_indices[] = {0, 1, 2};

static const PrimVertex k_quad_verts[] = {
    {-0.5f, -0.5f, 0.f, 0, 0, 1, 0, 1},
    {0.5f, -0.5f, 0.f, 0, 0, 1, 1, 1},
    {0.5f, 0.5f, 0.f, 0, 0, 1, 1, 0},
    {-0.5f, 0.5f, 0.f, 0, 0, 1, 0, 0},
};
static const uint32_t k_quad_indices[] = {0, 1, 2, 0, 2, 3};

static const PrimVertex k_cube_verts[] = {
    {0.5f, -0.5f, -0.5f, 1, 0, 0, 0, 1},   {0.5f, 0.5f, -0.5f, 1, 0, 0, 0, 0},    {0.5f, 0.5f, 0.5f, 1, 0, 0, 1, 0},
    {0.5f, -0.5f, 0.5f, 1, 0, 0, 1, 1},    {-0.5f, -0.5f, 0.5f, -1, 0, 0, 0, 1},  {-0.5f, 0.5f, 0.5f, -1, 0, 0, 0, 0},
    {-0.5f, 0.5f, -0.5f, -1, 0, 0, 1, 0},  {-0.5f, -0.5f, -0.5f, -1, 0, 0, 1, 1}, {0.5f, 0.5f, 0.5f, 0, 1, 0, 0, 1},
    {0.5f, 0.5f, -0.5f, 0, 1, 0, 0, 0},    {-0.5f, 0.5f, -0.5f, 0, 1, 0, 1, 0},   {-0.5f, 0.5f, 0.5f, 0, 1, 0, 1, 1},
    {0.5f, -0.5f, -0.5f, 0, -1, 0, 0, 1},  {0.5f, -0.5f, 0.5f, 0, -1, 0, 0, 0},   {-0.5f, -0.5f, 0.5f, 0, -1, 0, 1, 0},
    {-0.5f, -0.5f, -0.5f, 0, -1, 0, 1, 1}, {-0.5f, -0.5f, 0.5f, 0, 0, 1, 0, 1},   {0.5f, -0.5f, 0.5f, 0, 0, 1, 1, 1},
    {0.5f, 0.5f, 0.5f, 0, 0, 1, 1, 0},     {-0.5f, 0.5f, 0.5f, 0, 0, 1, 0, 0},    {0.5f, -0.5f, -0.5f, 0, 0, -1, 0, 1},
    {-0.5f, -0.5f, -0.5f, 0, 0, -1, 1, 1}, {-0.5f, 0.5f, -0.5f, 0, 0, -1, 1, 0},  {0.5f, 0.5f, -0.5f, 0, 0, -1, 0, 0},
};
static const uint32_t k_cube_indices[] = {
    0,  1,  2,  0,  2,  3,  4,  5,  6,  4,  6,  7,  8,  9,  10, 8,  10, 11,
    12, 13, 14, 12, 14, 15, 16, 17, 18, 16, 18, 19, 20, 21, 22, 20, 22, 23,
};

static const PrimVertex k_pyramid_verts[] = {
    {0.5f, -0.5f, 0.5f, 0, -1, 0, 1, 0},
    {-0.5f, -0.5f, 0.5f, 0, -1, 0, 0, 0},
    {-0.5f, -0.5f, -0.5f, 0, -1, 0, 0, 1},
    {0.5f, -0.5f, -0.5f, 0, -1, 0, 1, 1},
    {-0.5f, -0.5f, 0.5f, 0, 0.4472f, 0.8944f, 0, 0},
    {0.5f, -0.5f, 0.5f, 0, 0.4472f, 0.8944f, 1, 0},
    {0.0f, 0.5f, 0.0f, 0, 0.4472f, 0.8944f, 0.5f, 1},
    {0.5f, -0.5f, 0.5f, 0.8944f, 0.4472f, 0, 0, 0},
    {0.5f, -0.5f, -0.5f, 0.8944f, 0.4472f, 0, 1, 0},
    {0.0f, 0.5f, 0.0f, 0.8944f, 0.4472f, 0, 0.5f, 1},
    {0.5f, -0.5f, -0.5f, 0, 0.4472f, -0.8944f, 0, 0},
    {-0.5f, -0.5f, -0.5f, 0, 0.4472f, -0.8944f, 1, 0},
    {0.0f, 0.5f, 0.0f, 0, 0.4472f, -0.8944f, 0.5f, 1},
    {-0.5f, -0.5f, -0.5f, -0.8944f, 0.4472f, 0, 0, 0},
    {-0.5f, -0.5f, 0.5f, -0.8944f, 0.4472f, 0, 1, 0},
    {0.0f, 0.5f, 0.0f, -0.8944f, 0.4472f, 0, 0.5f, 1},
};
static const uint32_t k_pyramid_indices[] = {
    0, 1, 2, 0, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
};

static SharedPtr<RenderMesh> build_circle(int segs = 32)
{
    Vector<PrimVertex> verts;
    Vector<uint32_t>   indices;
    verts.reserve(static_cast<size_t>(segs + 1));
    indices.reserve(static_cast<size_t>(segs * 3));

    verts.push_back({0, 0, 0, 0, 0, 1, 0.5f, 0.5f});
    for (int i = 0; i < segs; ++i)
    {
        const float a  = static_cast<float>(i) * 2.0f * static_cast<float>(Math::pi) / static_cast<float>(segs);
        const float cx = Math::cos(a) * 0.5f;
        const float cy = Math::sin(a) * 0.5f;
        verts.push_back({cx, cy, 0, 0, 0, 1, 0.5f + cx, 0.5f + cy});
    }
    for (int i = 0; i < segs; ++i)
    {
        indices.push_back(0);
        indices.push_back(static_cast<uint32_t>(1 + i));
        indices.push_back(static_cast<uint32_t>(1 + (i + 1) % segs));
    }
    return RenderMesh::create(verts.data(), static_cast<uint32_t>(verts.size() * sizeof(PrimVertex)), indices.data(),
                              static_cast<uint32_t>(indices.size()), GRIIndexFormat::Uint32,
                              Math::Vec3f{0.0f, 0.0f, 0.0f}, 0.5f);
}

static SharedPtr<RenderMesh> build_sphere(int rings = 16, int segs = 32)
{
    Vector<PrimVertex> verts;
    Vector<uint32_t>   indices;
    verts.reserve(static_cast<size_t>((rings + 1) * (segs + 1)));
    indices.reserve(static_cast<size_t>(rings * segs * 6));

    const float k_pi = static_cast<float>(Math::pi);
    for (int r = 0; r <= rings; ++r)
    {
        const float phi = k_pi * static_cast<float>(r) / static_cast<float>(rings);
        const float sp  = Math::sin(phi);
        const float cp  = Math::cos(phi);
        for (int s = 0; s <= segs; ++s)
        {
            const float theta = 2.0f * k_pi * static_cast<float>(s) / static_cast<float>(segs);
            const float nx    = sp * Math::cos(theta);
            const float ny    = cp;
            const float nz    = sp * Math::sin(theta);
            verts.push_back({0.5f * nx, 0.5f * ny, 0.5f * nz, nx, ny, nz,
                             static_cast<float>(s) / static_cast<float>(segs),
                             static_cast<float>(r) / static_cast<float>(rings)});
        }
    }
    for (int r = 0; r < rings; ++r)
    {
        for (int s = 0; s < segs; ++s)
        {
            const uint32_t i0 = static_cast<uint32_t>(r * (segs + 1) + s);
            const uint32_t i1 = i0 + 1;
            const uint32_t i2 = i0 + static_cast<uint32_t>(segs + 1);
            const uint32_t i3 = i2 + 1;
            indices.push_back(i0);
            indices.push_back(i2);
            indices.push_back(i1);
            indices.push_back(i1);
            indices.push_back(i2);
            indices.push_back(i3);
        }
    }
    return RenderMesh::create(verts.data(), static_cast<uint32_t>(verts.size() * sizeof(PrimVertex)), indices.data(),
                              static_cast<uint32_t>(indices.size()), GRIIndexFormat::Uint32,
                              Math::Vec3f{0.0f, 0.0f, 0.0f}, 0.5f);
}

} // namespace

static const char* k_prim_names[] = {"Triangle", "Plane", "Cube", "Circle", "Sphere", "Pyramid"};

static Entity spawn_prim(Scene& scene, int idx, StringView name)
{
    const StringView resolved = name.empty() ? StringView(k_prim_names[idx]) : name;
    Entity           e        = scene.create_entity(resolved);

    TransformComponent trans;
    e.add_component<TransformComponent>(trans);

    MeshRendererComponent mrc;
    mrc.mesh_id = AssetID{EditorPrimitives::prim_mesh_key(idx)};
    e.add_component<MeshRendererComponent>(mrc);

    MaterialComponent matc;
    matc.material_id = AssetID{EditorPrimitives::prim_material_key()};
    e.add_component<MaterialComponent>(matc);

    return e;
}

Entity EditorPrimitives::spawn(Scene& scene, PrimShape shape, StringView name)
{
    return spawn_prim(scene, static_cast<int>(shape), name);
}

Entity EditorPrimitives::spawn_cube(Scene& scene)
{
    return spawn_prim(scene, 2, {});
}
Entity EditorPrimitives::spawn_sphere(Scene& scene)
{
    return spawn_prim(scene, 4, {});
}
Entity EditorPrimitives::spawn_quad(Scene& scene)
{
    return spawn_prim(scene, 1, {});
}
Entity EditorPrimitives::spawn_pyramid(Scene& scene)
{
    return spawn_prim(scene, 5, {});
}

void EditorPrimitives::init(EditorShaderCache& shader_cache)
{
    const RendererConfig& cfg = Renderer::get_config();

    SharedPtr<RenderShader> vs = shader_cache.get_or_compile("primitive.hlsl", GRIShaderStage::Vertex);
    SharedPtr<RenderShader> ps = shader_cache.get_or_compile("primitive.hlsl", GRIShaderStage::Pixel);

    GRIRasterDesc raster;
    raster.cull_mode = GRICullMode::None;

    SharedPtr<Material> mat = Renderer::get_material_factory().get_or_create(
        vs, ps, "standard_mesh", cfg.render_target_format, cfg.depth_format, {}, raster, {});
    Renderer::get_resource_cache().register_material(prim_material_key(), mat);

    Renderer::get_resource_cache().register_mesh(
        prim_mesh_key(0), RenderMesh::create(k_tri_verts, sizeof(k_tri_verts), k_tri_indices, 3, GRIIndexFormat::Uint32,
                                             Math::Vec3f{0.0f, 0.0f, 0.0f}, 0.5f));
    Renderer::get_resource_cache().register_mesh(
        prim_mesh_key(1), RenderMesh::create(k_quad_verts, sizeof(k_quad_verts), k_quad_indices, 6,
                                             GRIIndexFormat::Uint32, Math::Vec3f{0.0f, 0.0f, 0.0f}, 0.7071068f));
    Renderer::get_resource_cache().register_mesh(
        prim_mesh_key(2), RenderMesh::create(k_cube_verts, sizeof(k_cube_verts), k_cube_indices, 36,
                                             GRIIndexFormat::Uint32, Math::Vec3f{0.0f, 0.0f, 0.0f}, 0.8660254f));
    Renderer::get_resource_cache().register_mesh(prim_mesh_key(3), build_circle());
    Renderer::get_resource_cache().register_mesh(prim_mesh_key(4), build_sphere());
    Renderer::get_resource_cache().register_mesh(
        prim_mesh_key(5), RenderMesh::create(k_pyramid_verts, sizeof(k_pyramid_verts), k_pyramid_indices, 18,
                                             GRIIndexFormat::Uint32, Math::Vec3f{0.0f, 0.0f, 0.0f}, 0.8660254f));
}

} // namespace Ignis
