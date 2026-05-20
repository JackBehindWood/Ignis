#include "igpch.h"
#include "MeshLoader.h"
#include "AssetMesh.h"
#include "AssetManager.h"

namespace Ignis
{


//TODO: remove any vertex class declarations!
struct MeshVertex
{
    float position[3];
    float normal[3];
    float uv[2];
};

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
            int val = std::stoi(part) - 1; // .obj is 1-based
            if (component == 0) idx.v  = val;
            if (component == 1) idx.vt = val;
            if (component == 2) idx.vn = val;
        }
        ++component;
    }
    return idx;
}

SharedPtr<Asset> MeshLoader::load(const AssetMetadata& metadata)
{
    // Use your custom BinaryReader to handle file operations
    BinaryReader reader(metadata.source_path);
    if (!reader.is_open())
    {
        IG_CORE_ERROR("MeshLoader: cannot open '{}'", metadata.source_path.string());
        return nullptr;
    }

    // Read the entire .obj text payload directly into memory at once
    String content = reader.read_all_text();
    Stringstream file_stream(content);

    Vector<Array<float, 3>> positions;
    Vector<Array<float, 3>> normals;
    Vector<Array<float, 2>> uvs;

    // Fully updated to track vertices as raw layout-agnostic byte blocks
    Vector<uint8_t>  vertices;
    Vector<uint32_t> indices;

    // Dedup map: packed ObjIndex -> final vertex index
    UnorderedMap<uint64_t, uint32_t> index_cache;

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
                    }
                    else
                    {
                        MeshVertex mv{};
                        if (oi.v  >= 0 && oi.v  < (int)positions.size())
                            std::memcpy(mv.position, positions[oi.v].data(), 12);
                        if (oi.vn >= 0 && oi.vn < (int)normals.size())
                            std::memcpy(mv.normal, normals[oi.vn].data(), 12);
                        if (oi.vt >= 0 && oi.vt < (int)uvs.size())
                            std::memcpy(mv.uv, uvs[oi.vt].data(), 8);

                        // Calculate current vertex index based on total packed bytes
                        uint32_t new_idx = static_cast<uint32_t>(vertices.size() / sizeof(MeshVertex));
                        
                        // Resize byte buffer and copy raw structured layout payload into it
                        size_t current_size = vertices.size();
                        vertices.resize(current_size + sizeof(MeshVertex));
                        std::memcpy(&vertices[current_size], &mv, sizeof(MeshVertex));

                        index_cache[key] = new_idx;
                        indices.push_back(new_idx);
                    }
                }
            }
        }
    }

    if (vertices.empty())
    {
        IG_CORE_ERROR("MeshLoader: no geometry in '{}'", metadata.source_path.string());
        return nullptr;
    }

    return create_shared<AssetMesh>(metadata.ID, std::move(vertices), std::move(indices));
}

} // namespace Ignis