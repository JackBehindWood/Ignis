#include "igpch.h"
#include "MaterialHandler.h"

#include "Ignis/Asset/AssetMaterial.h"
#include "Ignis/Asset/AssetBinaryStream.h"
#include "Ignis/Rendering/Material.h"
#include "Ignis/Rendering/RenderSystem.h"
#include "Ignis/Rendering/Shaders/ShaderCache.h"
#include "Ignis/Rendering/Shaders/ShaderCompiler.h"
#include "Ignis/Rendering/VertexDeclarationRegistry.h"

namespace Ignis
{

// IGMT v2 — material recipe: shader source path, vertex layout name, param data blob.
static constexpr AssetBlobHeader k_material_header = {{'I', 'G', 'M', 'T'}, 2};

// ---------------------------------------------------------------------------

bool MaterialHandler::compile(const AssetMetadata& metadata)
{
    String source_text;
    if (!read_source_text(metadata, source_text))
    {
        IG_CORE_ERROR("MaterialHandler: failed to read source for '{}'",
                      metadata.source_path.string());
        return false;
    }

    const auto trim = [](const String& s) -> String {
        size_t a = s.find_first_not_of(" \t\r\n");
        if (a == String::npos) return {};
        size_t b = s.find_last_not_of(" \t\r\n");
        return s.substr(a, b - a + 1);
    };

    String shader_filename;
    String vertex_layout = "standard_mesh";
    {
        const size_t nl = source_text.find('\n');
        shader_filename = trim(nl != String::npos ? source_text.substr(0, nl) : source_text);
        if (nl != String::npos)
        {
            String line2 = trim(source_text.substr(nl + 1));
            if (!line2.empty())
                vertex_layout = line2;
        }
    }

    if (shader_filename.empty())
    {
        IG_CORE_ERROR("MaterialHandler: empty shader filename in '{}'",
                      metadata.source_path.string());
        return false;
    }

    // materials/ → parent(assets/) → assets/shaders/<filename>
    const Path shader_source =
        metadata.source_path.parent_path().parent_path() / "shaders" / shader_filename;
    if (!Filesystem::exists(shader_source))
    {
        IG_CORE_ERROR("MaterialHandler: shader not found: '{}'", shader_source.string());
        return false;
    }

    AssetBinaryWriter w = open_writer(metadata, k_material_header);
    if (!w.is_open())
    {
        IG_CORE_ERROR("MaterialHandler: could not open output '{}'",
                      metadata.compiled_path.string());
        return false;
    }

    write_string(w, shader_source.string());
    write_string(w, vertex_layout);
    w.write_u32(0); // param_data_size
    return w.finalize();
}

SharedPtr<Asset> MaterialHandler::load(const AssetMetadata& metadata)
{
    AssetBinaryReader r = open_reader(metadata, k_material_header);
    if (!r.is_open())
    {
        if (!compile(metadata))
        {
            IG_CORE_ERROR("MaterialHandler: recipe cook failed for '{}'",
                          metadata.source_path.string());
            return nullptr;
        }
        r = open_reader(metadata, k_material_header);
        if (!r.is_open())
        {
            IG_CORE_ERROR("MaterialHandler: failed to open compiled asset for '{}'",
                          metadata.compiled_path.string());
            return nullptr;
        }
    }

    const Path   shader_source(read_string(r));
    const String vertex_layout_name  = read_string(r);
    const uint32_t param_data_size   = r.read_u32();
    if (!r.good())
    {
        IG_CORE_ERROR("MaterialHandler: corrupted recipe for '{}'",
                      metadata.compiled_path.string());
        return nullptr;
    }

    ShaderCompilerOptions opts;
    opts.stages[0] = { GRIShaderStage::Vertex };
    opts.stages[1] = { GRIShaderStage::Pixel };
    opts.count = 2;

    SharedPtr<RenderShader> vs = ShaderCache::get().get_or_compile(
        shader_source, GRIShaderStage::Vertex, opts);
    SharedPtr<RenderShader> ps = ShaderCache::get().get_or_compile(
        shader_source, GRIShaderStage::Pixel, opts);
    if (!vs || !ps)
    {
        IG_CORE_ERROR("MaterialHandler: shader compile failed for '{}'",
                      shader_source.string());
        return nullptr;
    }

    const GRIVertexDeclaration* vd = VertexDeclarationRegistry::get().find(vertex_layout_name);
    if (!vd)
    {
        IG_CORE_ERROR("MaterialHandler: unknown vertex layout '{}' for '{}'",
                      vertex_layout_name, shader_source.string());
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
        IG_CORE_ERROR("MaterialHandler: PSO creation failed for '{}'", shader_source.string());
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

    SharedPtr<Material> material = create_shared<Material>(
        vs, ps, std::move(pso), vd, std::move(params_buffer));
    return create_shared<AssetMaterial>(metadata.ID, std::move(material));
}

} // namespace Ignis
