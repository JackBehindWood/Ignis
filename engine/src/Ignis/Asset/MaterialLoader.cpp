#include "igpch.h"
#include "MaterialLoader.h"
#include "AssetMaterial.h"
#include "AssetBinaryStream.h"
#include "AssetManager.h"
#include "Ignis/Rendering/Material.h"
#include "Ignis/Rendering/RenderSystem.h"
#include "Ignis/Rendering/Shaders/ShaderCache.h"
#include "Ignis/Rendering/Shaders/ShaderCompiler.h"
#include "Ignis/Rendering/VertexDeclarationRegistry.h"

namespace Ignis
{

static constexpr AssetBlobHeader k_material_header = {{'I', 'G', 'M', 'T'}, 2};

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

    const Path   shader_source(read_str(r));
    const String vertex_layout_name = read_str(r);
    const uint32_t param_data_size  = r.read_u32();
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

    const GRIVertexDeclaration* vd = VertexDeclarationRegistry::get().find(vertex_layout_name);
    if (!vd)
    {
        IG_CORE_ERROR("MaterialLoader: unknown vertex layout '{}' for '{}'", vertex_layout_name, shader_source.string());
        return nullptr;
    }

    GRIPipelineStateDesc pso_desc;
    pso_desc.vertex_shader        = vs->get_shader();
    pso_desc.pixel_shader         = ps->get_shader();
    pso_desc.vertex_declaration   = const_cast<GRIVertexDeclaration*>(vd);
    pso_desc.render_target_format = GRIPixelFormat::RGBA8Unorm;
    pso_desc.depth_stencil_format = GRIPixelFormat::Depth32Float;
    pso_desc.primitive_topology   = GRIPrimitiveTopology::TriangleList;

    GRIPipelineStatePtr pso = RenderSystem::get_gri()->create_graphics_pipeline_state(pso_desc);
    if (!pso)
    {
        IG_CORE_ERROR("MaterialLoader: PSO creation failed for '{}'", shader_source.string());
        return nullptr;
    }

    GRIBufferPtr params_buffer;
    if (param_data_size > 0)
    {
        Vector<uint8_t> param_data(param_data_size);
        r.read_bytes(param_data.data(), param_data_size);

        GRIBufferDesc buf_desc;
        buf_desc.size  = param_data_size;
        buf_desc.usage = GRIBufferUsage::UniformBuffer;
        params_buffer  = RenderSystem::get_gri()->create_buffer(buf_desc, param_data.data());
    }

    auto material = create_shared<Material>(vs, ps, std::move(pso), vd, std::move(params_buffer));
    return create_shared<AssetMaterial>(metadata.ID, std::move(material));
}

} // namespace Ignis
