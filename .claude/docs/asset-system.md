# Asset System V2

## Architectural Invariants

**Strict Mutual Ignorance** — `Ignis/Asset/` **must never** include headers from `Ignis/Rendering/` or `IgnisBackend/`. Rendering must never include Asset headers. The application / scene layer (`EditorLayer`, `SceneRenderer`) is the sole mediation bridge.

Asset types hold **raw CPU data only**. The `RenderResourceCache` in the Rendering layer holds the matching GPU resources (keyed by `AssetID → uint64_t`). Upload happens lazily in the app layer on first use.

**Zero Main-Loop Disk Polling via Native OS Event Binding** — `IFileWatcher` (declared in `Ignis/Core/`) is implemented by `MacOSFileWatcher` (in `IgnisBackend/macOS/`) using FSEvents bound to `dispatch_get_main_queue()`. Events are delivered during the frame's existing run-loop drain (`glfwPollEvents`). `AssetManager::update` calls `consume_events` to dispatch any accumulated reloads. No timestamp polling, no per-frame directory scanning.

**64-bit Primitives for Runtime Path Identification** — `RuntimePathId` (`Ignis/Foundation/RuntimePathId.h`) wraps a FNV-1a 64-bit hash. `AssetRegistry::m_source_index` is `UnorderedMap<RuntimePathId, uint64_t>`; hot lookup paths compare integers, not heap strings. In `IG_DEBUG` builds the original string is stored for editor tooltips; in release it is elided.

---

## Core types

```
AssetID           = UUID (uint64_t; UUID::s_invalid = 0 for null)

AssetType : uint16_t  { None=0, Texture2D, Shader, Mesh, Material }

AssetMetadata {
    AssetID  ID
    AssetType Type
    Path     source_path    // raw source (e.g. resources/assets/textures/rock.png)
    Path     compiled_path  // cooked binary (e.g. cache/<uuid>.igasset)
    uint64_t user_data      // stage enum for shaders, etc.
    is_valid() → bool
}

Asset : RefCounted          // base; subclass via static_type() → AssetType

RuntimePathId               // FNV-1a 64-bit hash of a path string (Foundation layer)
    hash : uint64_t
    debug_path : String     // IG_DEBUG only — elided in release
    RuntimePathId(const Path&)
    RuntimePathId(std::string_view)
    static fnv1a(std::string_view) → uint64_t
```

### Local GRI-mirror enums (`Ignis/Asset/AssetTypes.h`)

Asset types use their own enum copies to avoid Rendering headers:

```cpp
enum class AssetShaderStage : uint8_t { Vertex=0, Pixel=1, Compute=2 };
enum class AssetPixelFormat  : uint8_t { Unknown=0, RGBA8Unorm=1, BGRA8Unorm=2, Depth32Float=3 };
```

`GRIDefinitions.h` carries `static_assert`s that these values match the GRI enums. The Rendering layer casts with `static_cast<GRIShaderStage>(asset->get_stage())`.

---

## AssetManager API

```cpp
AssetManager::get()
  .import(path, AssetType, cache=false, user_data=0) → AssetID
  .load_deferred(id)           // enqueue to streaming pipeline
  .update(max_budget_ms)       // call every frame; drains work queue
  .is_ready(id)  → bool
  .get_asset(id) → SharedPtr<Asset>   // returns prev_asset_ptr during hot-reload
  .get_asset_as<T>(id) → SharedPtr<T>
  .load_sync<T>(id, timeout_ms) → SharedPtr<T>
  .flag_for_reload(id)         // manual hot-reload trigger
  .reload_all()
  .prune_cache()
  .add_reload_callback(fn)  → uint32_t token   // subscriber registry
  .remove_reload_callback(token)
  .get_fallback(AssetType)  → SharedPtr<Asset>
```

### State machine

```
Unloaded → Discovered → ReadingMetadata → ReadingData (streamed chunks) → Finalizing → Ready / Failed
```

`AssetWorkQueue.ready_queue` is a `Deque<AssetID>` (O(1) push_front for hot-reload priority).

---

## Asset subtypes (CPU-only)

### AssetMesh  (`Ignis/Asset/AssetMesh.h`)

```cpp
AssetMesh : Asset
  AssetMesh(id, Vector<uint8_t> vertices, Vector<uint32_t> indices, uint32_t vertex_stride)
  get_vertices()      → const Vector<uint8_t>&
  get_indices()       → const Vector<uint32_t>&
  get_vertex_stride() → uint32_t
  static_type()       → AssetType::Mesh
```

No GRI pointers. `SceneRenderer` uploads on first use via `RenderMesh::create()` + `register_mesh`.

### AssetShader  (`Ignis/Asset/AssetShader.h`)

```cpp
AssetShader : Asset
  AssetShader(id, Path source_path, AssetShaderStage stage, String entry_point)
  get_source_path() → const Path&
  get_stage()       → AssetShaderStage
  get_entry_point() → const String&
  static_type()     → AssetType::Shader
```

Stores the **HLSL source path + stage metadata**. No RenderShader, no SPIR-V blob. `RenderResourceCache` (or `ShaderCache`) compiles GPU objects on demand. `AssetMetadata::user_data` carries `AssetShaderStage` value from importer.

### AssetTexture2D  (`Ignis/Asset/AssetTexture2D.h`)

```cpp
AssetTexture2D : Asset
  AssetTexture2D(id, AssetPixelFormat, uint32_t width, uint32_t height, Vector<uint8_t> pixels)
  get_format() → AssetPixelFormat
  get_width()  → uint32_t
  get_height() → uint32_t
  get_pixels() → const Vector<uint8_t>&
  static_type() → AssetType::Texture2D
```

Raw pixel blob. `EditorLayer` uploads on first use via `RenderSystem::get_gri()->create_texture2d()` + `register_texture`.

### AssetMaterial  (`Ignis/Asset/AssetMaterial.h`)

```cpp
AssetMaterial : Asset
  AssetMaterial(id, Path shader_source, String vertex_layout, Vector<uint8_t> param_data)
  get_shader_source() → const Path&
  get_vertex_layout() → const String&
  get_param_data()    → const Vector<uint8_t>&
  static_type()       → AssetType::Material
```

Pure recipe — shader path + vertex layout name + optional parameter block. No PSO, no Material pointer. `SceneRenderer` compiles the `Material` (PSO + shaders) on cache-miss and stores in `RenderResourceCache`.

---

## Handlers (`Ignis/Asset/Handlers/`)

Each handler: `compile(metadata)` → writes `.igasset`; `load(metadata)` / `load_from_bytes(metadata, bytes)` → returns CPU-only Asset.

| Handler | compile writes | load returns |
|---------|---------------|-------------|
| ShaderHandler  | source_path, stage, entry, deps | `AssetShader` (source_path, stage, entry) |
| TextureHandler | format, w, h, pixels | `AssetTexture2D` (raw pixels) |
| MaterialHandler | shader_source, vertex_layout, param_data | `AssetMaterial` (recipe) |
| MeshHandler | vertex_stride, verts, indices | `AssetMesh` (raw buffers) |

---

## AssetBinaryStream

Cook: `AssetBinaryWriter::open(metadata, header)` — writes `.igasset` header then fields.  
Load: `AssetBinaryReader::open(metadata, header)` — validates magic + version.  
`MemBinaryReader(data, size)` — reads from an in-memory staging buffer (streaming path).

```
AssetBlobHeader { magic[4], version: uint8_t }
```

---

## Mediation Pattern (App / Scene layer)

```cpp
// EditorLayer — texture upload on cache-miss:
if (auto tex = AssetManager::get().get_asset_as<AssetTexture2D>(id))
{
    auto cached = Renderer::get_resource_cache().find_texture(uint64_t(id));
    if (!cached) {
        GRITexture2DDesc desc{ ... tex->get_width(), tex->get_height(),
                               static_cast<GRIPixelFormat>(tex->get_format()), ... };
        auto rt = create_shared<RenderTexture2D>(gri->create_texture2d(desc), ...);
        Renderer::get_resource_cache().register_texture(uint64_t(id), rt);
    }
}

// SceneRenderer — material PSO compile on cache-miss:
if (auto mat = AssetManager::get().get_asset_as<AssetMaterial>(id))
{
    auto cached = Renderer::get_resource_cache().find_material(uint64_t(id));
    if (!cached) {
        // ShaderCache + GRI → create_shared<Material>(...) → register_material(...)
    }
}
```

---

## Hot-reload sequence

```
FSEvents (OS kernel) → MacOSFileWatcher::s_callback (main dispatch queue, during glfwPollEvents)
    └─ hash event path → lookup m_path_map → push to m_pending

AssetManager::update → consume_events → flag_for_reload(id)
    ├─ prev_asset_ptr = asset_ptr   // fallback for in-flight frames
    ├─ state → Discovered
    └─ cascade to dependents
            │
process_finalize() → asset_ptr updated → prev_asset_ptr cleared
            │
reload_subscribers notified → Renderer::evict(id) → RenderResourceCache evicts GPU resource
            │
Next frame: cache-miss triggers re-upload
```

## AssetRegistry source index

```cpp
// m_source_index keyed by RuntimePathId — integer comparison, no string heap allocs on lookup
UnorderedMap<RuntimePathId, uint64_t> m_source_index;

// register: hashes source_path once on import
// find_by_source: hashes query path, O(1) bucket lookup
// remove: hashes metadata.source_path, O(1) erase (was O(n) linear scan)
```