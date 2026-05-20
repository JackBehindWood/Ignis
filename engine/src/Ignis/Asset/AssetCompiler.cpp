#include "igpch.h"
#include "AssetCompiler.h"
#include "AssetShaderCompiler.h"

namespace Ignis
{

    bool AssetCompiler::read_source_text(const AssetMetadata& metadata, String& out_text) const
    {
        if (!Filesystem::exists(metadata.source_path))
        {
            IG_CORE_ERROR("AssetCompiler: source not found: {0}", metadata.source_path.string());
            return false;
        }

        BinaryReader src(metadata.source_path);
        if (!src.is_open())
        {
            IG_CORE_ERROR("AssetCompiler: failed to open source file: {0}", metadata.source_path.string());
            return false;
        }

        out_text = src.read_all_text();
        return true;
    }

    bool AssetCompiler::read_source_bytes(const AssetMetadata& metadata, Vector<uint8_t>& out_bytes) const
    {
        if (!Filesystem::exists(metadata.source_path))
        {
            IG_CORE_ERROR("AssetCompiler: source not found: {0}", metadata.source_path.string());
            return false;
        }

        BinaryReader src(metadata.source_path);
        if (!src.is_open())
        {
            IG_CORE_ERROR("AssetCompiler: failed to open source file: {0}", metadata.source_path.string());
            return false;
        }

        size_t size = src.get_size(); 
        out_bytes.resize(size);
        src.read_bytes(out_bytes.data(), size);
        
        return true;
    }

    AssetBinaryWriter AssetCompiler::open_writer(const AssetMetadata& metadata, const AssetBlobHeader& header) const
    {
        return AssetBinaryWriter::open(metadata, header);
    }


    static constexpr AssetBlobHeader k_shader_header = {{'I', 'G', 'S', 'H'}, 7};

    static void write_str(AssetBinaryWriter& w, const String& s)
    {
        w.write_u32(static_cast<uint32_t>(s.size()));
        w.write_bytes(s.data(), s.size());
    }

    static void write_reflection(AssetBinaryWriter& w, const ShaderReflection& r)
    {
        auto write_bindings = [&](const Vector<ShaderResourceBinding>& v) {
            w.write_u32(static_cast<uint32_t>(v.size()));
            for (const auto& b : v) { write_str(w, b.name); w.write_u32(b.set); w.write_u32(b.binding); }
        };
        write_bindings(r.uniform_buffers);
        write_bindings(r.storage_buffers);
        write_bindings(r.separate_images);
        write_bindings(r.separate_samplers);

        w.write_u32(static_cast<uint32_t>(r.stage_inputs.size()));
        for (const auto& i : r.stage_inputs) { write_str(w, i.name); w.write_u32(i.location); }

        w.write_u32(static_cast<uint32_t>(r.push_constants.size()));
        for (const auto& p : r.push_constants) { write_str(w, p.name); w.write_u32(p.size); }
    }

    static void write_stage(AssetBinaryWriter& w, const ShaderStageOutput& s)
    {
        w.write_u8(static_cast<uint8_t>(s.stage));
        write_str(w, s.entry_point);
        w.write_u32(static_cast<uint32_t>(s.bytecode.size()));
        w.write_bytes(s.bytecode.data(), s.bytecode.size());
        write_reflection(w, s.reflection);
    }

    AssetShaderCompiler::AssetShaderCompiler(ShaderTarget target)
        : m_target(target), m_compiler(target)
    {}

    bool AssetShaderCompiler::compile(const AssetMetadata& metadata)
    {
        String source;
        if (!read_source_text(metadata, source))
            return false;

        const Vector<ShaderStageOutput> stages = m_compiler.compile(source);
        if (stages.empty())
            return false;

        if (!metadata.cache_compiled)
            return true;

        AssetBinaryWriter w = open_writer(metadata, k_shader_header);
        if (!w.is_open())
            return false;

        w.write_u8(static_cast<uint8_t>(m_target));
        w.write_u8(static_cast<uint8_t>(stages.size()));
        for (const auto& s : stages)
            write_stage(w, s);

        if (w.good())
            IG_CORE_INFO("AssetShaderCompiler: compiled '{}' -> {}",
                        metadata.source_path.filename().string(),
                        metadata.compiled_path.filename().string());

        return w.good();
    }

} // namespace Ignis