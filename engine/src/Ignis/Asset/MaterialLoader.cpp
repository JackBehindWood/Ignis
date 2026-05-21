#include "igpch.h"
#include "MaterialLoader.h"
#include "AssetMaterial.h"
#include "AssetBinaryStream.h"
#include "AssetManager.h"
#include "Ignis/Rendering/Material.h"
#include "Ignis/Rendering/RenderSystem.h"
#include "Ignis/Rendering/ShaderCache.h"
#include "Ignis/Rendering/ShaderCompiler.h"

namespace Ignis
{

static constexpr AssetBlobHeader k_material_header = {{'I', 'G', 'M', 'T'}, 1};

static String read_str(AssetBinaryReader& r)
{
    const uint32_t len = r.read_u32();
    String s(len, '\0');
    if (len > 0)
        r.read_bytes(s.data(), len);
    return s;
}

SharedPtr<Asset> MaterialLoader::load(const AssetMetadata& metadata)
{
    AssetBinaryReader r = AssetBinaryReader::open(metadata.compiled_path, k_material_header);
    if (!r.is_open())
    {
        AssetCompiler* compiler = AssetManager::get_compiler(AssetType::Material);
        if (!compiler || !compiler->compile(metadata))
        {
            IG_CORE_ERROR("MaterialLoader: recipe cook failed for '{}'", metadata.source_path.string());
            return nullptr;
        }

        r = AssetBinaryReader::open(metadata.compiled_path, k_material_header);
        if (!r.is_open())
            return nullptr;
    }

    const Path shader_source(read_str(r));
    if (!r.good())
    {
        IG_CORE_ERROR("MaterialLoader: corrupted recipe for '{}'", metadata.compiled_path.string());
        return nullptr;
    }

    ShaderCompilerOptions opts;
    opts.stages[0] = { GRIShaderStage::Vertex };
    opts.stages[1] = { GRIShaderStage::Pixel };
    opts.count = 2;

    SharedPtr<RenderShader> vs = ShaderCache::get().get_or_compile(shader_source, GRIShaderStage::Vertex, opts);
    SharedPtr<RenderShader> ps = ShaderCache::get().get_or_compile(shader_source, GRIShaderStage::Pixel,  opts);
    if (!vs || !ps)
    {
        IG_CORE_ERROR("MaterialLoader: shader compile failed for '{}'", shader_source.string());
        return nullptr;
    }

    // TODO: vertex declaration registry — materials should declare their required input layout
    // and the Renderer should match it against mesh vertex buffers at draw time.
    // Hardcoded standard layout (pos:12 nrm:12 uv:8, stride 32) matches IGAM cook output.
    GRIVertexDeclaration vd;
    vd.elements[0] = { GRIVertexElementSemantic::Position, GRIVertexElementFormat::Float3,  0, 0 };
    vd.elements[1] = { GRIVertexElementSemantic::Normal,   GRIVertexElementFormat::Float3, 12, 0 };
    vd.elements[2] = { GRIVertexElementSemantic::TexCoord, GRIVertexElementFormat::Float2, 24, 0 };
    vd.num_elements = 3;
    vd.bindings[0]  = { 0, 32 };
    vd.num_bindings = 1;

    GRIPipelineStateDesc pso_desc;
    pso_desc.vertex_shader        = vs->get_shader();
    pso_desc.pixel_shader         = ps->get_shader();
    pso_desc.vertex_declaration   = &vd;
    pso_desc.render_target_format = GRIPixelFormat::RGBA8Unorm;
    pso_desc.depth_stencil_format = GRIPixelFormat::Depth32Float;
    pso_desc.primitive_topology   = GRIPrimitiveTopology::TriangleList;

    GRIPipelineStatePtr pso = RenderSystem::get_gri()->create_graphics_pipeline_state(pso_desc);
    if (!pso)
    {
        IG_CORE_ERROR("MaterialLoader: PSO creation failed for '{}'", shader_source.string());
        return nullptr;
    }

    auto material = create_shared<Material>(vs, ps, std::move(pso));
    return create_shared<AssetMaterial>(metadata.ID, std::move(material));
}

} // namespace Ignis
