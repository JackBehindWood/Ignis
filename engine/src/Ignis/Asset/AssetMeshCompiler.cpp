#include "igpch.h"
#include "AssetMeshCompiler.h"

namespace Ignis
{

static constexpr AssetBlobHeader k_mesh_header    = {{'I', 'G', 'A', 'M'}, 1};
static constexpr uint32_t        k_vertex_stride  = 12 + 12 + 8; // pos(3f) nrm(3f) uv(2f)

struct ObjIndex { int v, vt, vn; };

static ObjIndex parse_face_vertex(const String& token)
{
    ObjIndex idx{};
    Stringstream ss(token);
    String part;
    int component = 0;
    while (std::getline(ss, part, '/'))
    {
        if (!part.empty())
        {
            int val = std::stoi(part) - 1;
            if (component == 0) idx.v  = val;
            if (component == 1) idx.vt = val;
            if (component == 2) idx.vn = val;
        }
        ++component;
    }
    return idx;
}

bool AssetMeshCompiler::compile(const AssetMetadata& metadata)
{
    String content;
    if (!read_source_text(metadata, content))
        return false;

    Stringstream file_stream(content);

    Vector<Array<float, 3>> positions;
    Vector<Array<float, 3>> normals;
    Vector<Array<float, 2>> uvs;
    Vector<uint8_t>         vertices;
    Vector<uint32_t>        indices;
    UnorderedMap<uint64_t, uint32_t> index_cache;

    static constexpr float k_zero3[3] = {};
    static constexpr float k_zero2[2] = {};

    auto pack_key = [](ObjIndex i) -> uint64_t {
        return (uint64_t)(uint16_t)i.v
             | ((uint64_t)(uint16_t)i.vt << 16)
             | ((uint64_t)(uint16_t)i.vn << 32);
    };

    String line;
    while (std::getline(file_stream, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        Stringstream ss(line);
        String token;
        ss >> token;

        if (token == "v")
        {
            auto& p = positions.emplace_back();
            ss >> p[0] >> p[1] >> p[2];
        }
        else if (token == "vn")
        {
            auto& n = normals.emplace_back();
            ss >> n[0] >> n[1] >> n[2];
        }
        else if (token == "vt")
        {
            auto& uv = uvs.emplace_back();
            ss >> uv[0] >> uv[1];
        }
        else if (token == "f")
        {
            Vector<ObjIndex> face_verts;
            String vert_token;
            while (ss >> vert_token)
                face_verts.push_back(parse_face_vertex(vert_token));

            for (size_t i = 1; i + 1 < face_verts.size(); ++i)
            {
                ObjIndex tri[3] = { face_verts[0], face_verts[i], face_verts[i + 1] };
                for (const ObjIndex& oi : tri)
                {
                    uint64_t key = pack_key(oi);
                    auto it = index_cache.find(key);
                    if (it != index_cache.end())
                    {
                        indices.push_back(it->second);
                        continue;
                    }

                    const float* pos = (oi.v  >= 0 && oi.v  < (int)positions.size()) ? positions[oi.v].data()  : k_zero3;
                    const float* nrm = (oi.vn >= 0 && oi.vn < (int)normals.size())   ? normals[oi.vn].data()   : k_zero3;
                    const float* uv  = (oi.vt >= 0 && oi.vt < (int)uvs.size())       ? uvs[oi.vt].data()       : k_zero2;

                    uint32_t new_idx = static_cast<uint32_t>(vertices.size() / k_vertex_stride);
                    size_t cur = vertices.size();
                    vertices.resize(cur + k_vertex_stride);
                    std::memcpy(&vertices[cur +  0], pos, 12);
                    std::memcpy(&vertices[cur + 12], nrm, 12);
                    std::memcpy(&vertices[cur + 24], uv,   8);

                    index_cache[key] = new_idx;
                    indices.push_back(new_idx);
                }
            }
        }
    }

    if (vertices.empty())
    {
        IG_CORE_ERROR("AssetMeshCompiler: no geometry in '{}'", metadata.source_path.string());
        return false;
    }

    AssetBinaryWriter w = open_writer(metadata, k_mesh_header);
    if (!w.is_open())
        return false;

    const uint32_t vertex_count = static_cast<uint32_t>(vertices.size() / k_vertex_stride);
    const uint32_t index_count  = static_cast<uint32_t>(indices.size());

    w.write_u32(k_vertex_stride);
    w.write_u32(vertex_count);
    w.write_bytes(vertices.data(), vertices.size());
    w.write_u32(index_count);
    w.write_bytes(indices.data(), index_count * sizeof(uint32_t));

    return w.good();
}

} // namespace Ignis
