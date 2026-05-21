#include "igpch.h"
#include "MeshLoader.h"
#include "AssetMesh.h"
#include "AssetManager.h"
#include "AssetBinaryStream.h"

namespace Ignis
{

static constexpr AssetBlobHeader k_mesh_header = {{'I', 'G', 'A', 'M'}, 1};

SharedPtr<Asset> MeshLoader::load(const AssetMetadata& metadata)
{
    AssetBinaryReader r = AssetBinaryReader::open(metadata.compiled_path, k_mesh_header);
    if (!r.is_open())
    {
        AssetCompiler* compiler = AssetManager::get_compiler(AssetType::Mesh);
        if (!compiler || !compiler->compile(metadata))
        {
            IG_CORE_ERROR("MeshLoader: cook failed for '{}'", metadata.source_path.string());
            return nullptr;
        }
        r = AssetBinaryReader::open(metadata.compiled_path, k_mesh_header);
        if (!r.is_open())
            return nullptr;
    }

    const uint32_t vertex_stride = r.read_u32();
    const uint32_t vertex_count  = r.read_u32();

    Vector<uint8_t> vertices(vertex_count * vertex_stride);
    r.read_bytes(vertices.data(), vertices.size());

    const uint32_t index_count = r.read_u32();
    Vector<uint32_t> indices(index_count);
    r.read_bytes(indices.data(), index_count * sizeof(uint32_t));

    if (!r.good())
    {
        IG_CORE_ERROR("MeshLoader: corrupted binary for '{}'", metadata.compiled_path.string());
        return nullptr;
    }

    return create_shared<AssetMesh>(metadata.ID, vertices, indices, vertex_stride);
}

} // namespace Ignis
