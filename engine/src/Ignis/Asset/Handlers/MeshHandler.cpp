#include "igpch.h"
#include "MeshHandler.h"
#include "Ignis/Asset/AssetMesh.h"
#include "Ignis/Asset/AssetBinaryStream.h"

namespace Ignis
{

static constexpr AssetBlobHeader k_mesh_header   = {{'I', 'G', 'A', 'M'}, 3};
static constexpr uint32_t        k_vertex_stride = 12 + 12 + 8; // pos(3f) nrm(3f) uv(2f)

// ---------------------------------------------------------------------------
// OBJ parsing helpers
// ---------------------------------------------------------------------------

struct ObjIndex
{
    int v, vt, vn;
};

static ObjIndex parse_face_vertex(const String& token)
{
    ObjIndex     idx{};
    Stringstream ss(token);
    String       part;
    int          component = 0;
    while (std::getline(ss, part, '/'))
    {
        if (!part.empty())
        {
            const int val = std::stoi(part) - 1;
            if (component == 0)
            {
                idx.v = val;
            }
            if (component == 1)
            {
                idx.vt = val;
            }
            if (component == 2)
            {
                idx.vn = val;
            }
        }
        ++component;
    }
    return idx;
}

// Reads packed vertex/index data from a reader already positioned at the mesh payload.
template <typename R>
static SharedPtr<Asset> parse_mesh_payload(AssetID id, R& r)
{
    float bd[10];
    r.read_bytes(bd, sizeof(bd));

    const uint32_t vertex_stride = r.read_u32();
    const uint32_t vertex_count  = r.read_u32();
    const size_t   vertex_bytes  = static_cast<size_t>(vertex_count) * vertex_stride;

    Vector<uint8_t> vertices(vertex_bytes);
    r.read_bytes(vertices.data(), vertex_bytes);

    const uint32_t   index_count = r.read_u32();
    Vector<uint32_t> indices(index_count);
    r.read_bytes(indices.data(), index_count * sizeof(uint32_t));

    if (!r.good())
    {
        IG_CORE_ERROR("MeshHandler: corrupted payload for asset {0}", static_cast<uint64_t>(id));
        return nullptr;
    }
    return create_shared<AssetMesh>(id, vertices, indices, vertex_stride);
}

// ---------------------------------------------------------------------------

bool MeshHandler::compile(const AssetMetadata& metadata)
{
    String content;
    if (!read_source_text(metadata, content))
    {
        return false;
    }

    if (content.empty())
    {
        IG_CORE_ERROR("MeshHandler: empty source for '{}'", metadata.source_path.string());
        return false;
    }

    Stringstream file_stream(content);

    Vector<Array<float, 3>>          positions;
    Vector<Array<float, 3>>          normals;
    Vector<Array<float, 2>>          uvs;
    Vector<uint8_t>                  vertices;
    Vector<uint32_t>                 indices;
    UnorderedMap<uint64_t, uint32_t> index_cache;

    static constexpr float k_zero3[3] = {};
    static constexpr float k_zero2[2] = {};

    auto pack_key = [](ObjIndex i) -> uint64_t
    { return (uint64_t)(uint16_t)i.v | ((uint64_t)(uint16_t)i.vt << 16) | ((uint64_t)(uint16_t)i.vn << 32); };

    String line;
    while (std::getline(file_stream, line))
    {
        if (line.empty() || line[0] == '#')
        {
            continue;
        }

        Stringstream ss(line);
        String       token;
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
            String           vert_token;
            while (ss >> vert_token)
            {
                face_verts.push_back(parse_face_vertex(vert_token));
            }

            for (size_t i = 1; i + 1 < face_verts.size(); ++i)
            {
                ObjIndex tri[3] = {face_verts[0], face_verts[i], face_verts[i + 1]};
                for (const ObjIndex& oi : tri)
                {
                    const uint64_t key = pack_key(oi);
                    auto           it  = index_cache.find(key);
                    if (it != index_cache.end())
                    {
                        indices.push_back(it->second);
                        continue;
                    }

                    const float* pos = (oi.v >= 0 && oi.v < (int)positions.size()) ? positions[oi.v].data() : k_zero3;
                    const float* nrm = (oi.vn >= 0 && oi.vn < (int)normals.size()) ? normals[oi.vn].data() : k_zero3;
                    const float* uv  = (oi.vt >= 0 && oi.vt < (int)uvs.size()) ? uvs[oi.vt].data() : k_zero2;

                    const uint32_t new_idx = static_cast<uint32_t>(vertices.size() / k_vertex_stride);
                    const size_t   cur     = vertices.size();
                    vertices.resize(cur + k_vertex_stride);
                    std::memcpy(&vertices[cur + 0], pos, 12);
                    std::memcpy(&vertices[cur + 12], nrm, 12);
                    std::memcpy(&vertices[cur + 24], uv, 8);

                    index_cache[key] = new_idx;
                    indices.push_back(new_idx);
                }
            }
        }
    }

    if (vertices.empty() || positions.empty())
    {
        IG_CORE_ERROR("MeshHandler: no geometry in '{}'", metadata.source_path.string());
        return false;
    }

    // AABB — exact min/max over all declared positions
    Math::Vec3f bounds_min{positions[0][0], positions[0][1], positions[0][2]};
    Math::Vec3f bounds_max = bounds_min;
    for (const auto& p : positions)
    {
        bounds_min.x = Math::min(bounds_min.x, p[0]);
        bounds_min.y = Math::min(bounds_min.y, p[1]);
        bounds_min.z = Math::min(bounds_min.z, p[2]);
        bounds_max.x = Math::max(bounds_max.x, p[0]);
        bounds_max.y = Math::max(bounds_max.y, p[1]);
        bounds_max.z = Math::max(bounds_max.z, p[2]);
    }

    // Ritter sphere — pass 1: pick P farthest from seed, Q farthest from P
    auto sq3 = [](const Array<float, 3>& a, const Array<float, 3>& b) -> float
    {
        float dx = a[0] - b[0], dy = a[1] - b[1], dz = a[2] - b[2];
        return dx * dx + dy * dy + dz * dz;
    };

    size_t p_idx = 0;
    float  p_sq  = 0.0f;
    for (size_t i = 1; i < positions.size(); ++i)
    {
        float d = sq3(positions[0], positions[i]);
        if (d > p_sq)
        {
            p_sq  = d;
            p_idx = i;
        }
    }

    size_t q_idx = 0;
    float  q_sq  = 0.0f;
    for (size_t i = 0; i < positions.size(); ++i)
    {
        float d = sq3(positions[p_idx], positions[i]);
        if (d > q_sq)
        {
            q_sq  = d;
            q_idx = i;
        }
    }

    Math::Vec3f s_center{
        (positions[p_idx][0] + positions[q_idx][0]) * 0.5f,
        (positions[p_idx][1] + positions[q_idx][1]) * 0.5f,
        (positions[p_idx][2] + positions[q_idx][2]) * 0.5f,
    };
    float s_radius = Math::sqrt(q_sq) * 0.5f;

    // Ritter pass 2: grow sphere to contain all points
    for (const auto& p : positions)
    {
        float dx   = p[0] - s_center.x;
        float dy   = p[1] - s_center.y;
        float dz   = p[2] - s_center.z;
        float dist = Math::sqrt(dx * dx + dy * dy + dz * dz);
        if (dist > s_radius)
        {
            float t = (dist - s_radius) / (2.0f * dist);
            s_center.x += dx * t;
            s_center.y += dy * t;
            s_center.z += dz * t;
            s_radius = (s_radius + dist) * 0.5f;
        }
    }

    AssetBinaryWriter w = open_writer(metadata, k_mesh_header);
    if (!w.is_open())
    {
        IG_CORE_ERROR("MeshHandler: could not open output '{}'", metadata.compiled_path.string());
        return false;
    }

    const uint32_t vertex_count = static_cast<uint32_t>(vertices.size() / k_vertex_stride);
    const uint32_t index_count  = static_cast<uint32_t>(indices.size());

    const float bd[10] = {
        bounds_min.x, bounds_min.y, bounds_min.z, bounds_max.x, bounds_max.y,
        bounds_max.z, s_center.x,   s_center.y,   s_center.z,   s_radius,
    };
    w.write_bytes(bd, sizeof(bd));
    w.write_u32(k_vertex_stride);
    w.write_u32(vertex_count);
    w.write_bytes(vertices.data(), vertices.size());
    w.write_u32(index_count);
    w.write_bytes(indices.data(), index_count * sizeof(uint32_t));

    return w.finalize();
}

SharedPtr<Asset> MeshHandler::load(const AssetMetadata& metadata)
{
    AssetBinaryReader r = open_reader(metadata, k_mesh_header);
    if (!r.is_open() || r.version() < 3)
    {
        if (!compile(metadata))
        {
            IG_CORE_ERROR("MeshHandler: cook failed for '{}'", metadata.source_path.string());
            return nullptr;
        }
        r = open_reader(metadata, k_mesh_header);
        if (!r.is_open())
        {
            IG_CORE_ERROR("MeshHandler: failed to open compiled asset for '{}'", metadata.compiled_path.string());
            return nullptr;
        }
    }

    float bd[10];
    r.read_bytes(bd, sizeof(bd));
    const Math::Vec3f bounds_min{bd[0], bd[1], bd[2]};
    const Math::Vec3f bounds_max{bd[3], bd[4], bd[5]};
    const Math::Vec3f sphere_center{bd[6], bd[7], bd[8]};
    const float       sphere_radius = bd[9];

    const uint32_t vertex_stride = r.read_u32();
    const uint32_t vertex_count  = r.read_u32();

    Vector<uint8_t> vertices(vertex_count * vertex_stride);
    r.read_bytes(vertices.data(), vertices.size());

    const uint32_t   index_count = r.read_u32();
    Vector<uint32_t> indices(index_count);
    r.read_bytes(indices.data(), index_count * sizeof(uint32_t));

    if (!r.good())
    {
        IG_CORE_ERROR("MeshHandler: corrupted binary for '{}'", metadata.compiled_path.string());
        return nullptr;
    }

    auto mesh = create_shared<AssetMesh>(metadata.ID, vertices, indices, vertex_stride);
    mesh->set_bounds(sphere_center, sphere_radius);
    mesh->set_aabb(bounds_min, bounds_max);
    return mesh;
}

SharedPtr<Asset> MeshHandler::load_from_bytes(const AssetMetadata& metadata, const Vector<uint8_t>& bytes)
{
    MemBinaryReader  r(bytes.data(), bytes.size());
    SharedPtr<Asset> asset = parse_mesh_payload(metadata.ID, r);
    if (!asset)
    {
        IG_CORE_ERROR("MeshHandler: corrupted payload for '{}'", metadata.compiled_path.string());
    }
    return asset;
}

AssetType MeshHandler::get_type() const
{
    return AssetType::Mesh;
}

SharedPtr<Asset> MeshHandler::create_default_fallback() const
{
    struct CubeVert
    {
        float px, py, pz, nx, ny, nz, u, v;
    };
    static const CubeVert k_verts[24] = {
        {-0.5f, -0.5f, 0.5f, 0, 0, 1, 0, 1},   {0.5f, -0.5f, 0.5f, 0, 0, 1, 1, 1},
        {0.5f, 0.5f, 0.5f, 0, 0, 1, 1, 0},     {-0.5f, 0.5f, 0.5f, 0, 0, 1, 0, 0},
        {0.5f, -0.5f, -0.5f, 0, 0, -1, 0, 1},  {-0.5f, -0.5f, -0.5f, 0, 0, -1, 1, 1},
        {-0.5f, 0.5f, -0.5f, 0, 0, -1, 1, 0},  {0.5f, 0.5f, -0.5f, 0, 0, -1, 0, 0},
        {0.5f, -0.5f, 0.5f, 1, 0, 0, 0, 1},    {0.5f, -0.5f, -0.5f, 1, 0, 0, 1, 1},
        {0.5f, 0.5f, -0.5f, 1, 0, 0, 1, 0},    {0.5f, 0.5f, 0.5f, 1, 0, 0, 0, 0},
        {-0.5f, -0.5f, -0.5f, -1, 0, 0, 0, 1}, {-0.5f, -0.5f, 0.5f, -1, 0, 0, 1, 1},
        {-0.5f, 0.5f, 0.5f, -1, 0, 0, 1, 0},   {-0.5f, 0.5f, -0.5f, -1, 0, 0, 0, 0},
        {-0.5f, 0.5f, 0.5f, 0, 1, 0, 0, 1},    {0.5f, 0.5f, 0.5f, 0, 1, 0, 1, 1},
        {0.5f, 0.5f, -0.5f, 0, 1, 0, 1, 0},    {-0.5f, 0.5f, -0.5f, 0, 1, 0, 0, 0},
        {-0.5f, -0.5f, -0.5f, 0, -1, 0, 0, 1}, {0.5f, -0.5f, -0.5f, 0, -1, 0, 1, 1},
        {0.5f, -0.5f, 0.5f, 0, -1, 0, 1, 0},   {-0.5f, -0.5f, 0.5f, 0, -1, 0, 0, 0},
    };
    static const uint32_t k_idx[36] = {
        0,  1,  2,  0,  2,  3,  4,  5,  6,  4,  6,  7,  8,  9,  10, 8,  10, 11,
        12, 13, 14, 12, 14, 15, 16, 17, 18, 16, 18, 19, 20, 21, 22, 20, 22, 23,
    };
    Vector<uint8_t> verts(sizeof(k_verts));
    std::memcpy(verts.data(), k_verts, sizeof(k_verts));
    Vector<uint32_t> idx(k_idx, k_idx + 36);
    auto             mesh = create_shared<AssetMesh>(UUID{UUID::s_invalid}, verts, idx, 32u);
    mesh->set_bounds({0.0f, 0.0f, 0.0f}, 0.866f);
    mesh->set_aabb({-0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, 0.5f});
    return mesh;
}

} // namespace Ignis
