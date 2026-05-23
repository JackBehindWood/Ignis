# Asset System

## Core types

```
AssetID           = UUID (uint64_t; UUID::s_invalid for null)

AssetType : uint16_t  { None=0, Texture2D, Shader, Mesh }

AssetMetadata {
    AssetID  ID
    AssetType Type
    Path     source_path    // raw source (e.g. resources/assets/textures/rock.png)
    Path     compiled_path  // cooked binary (e.g. cache/<uuid>.igasset)
    is_valid() → bool
}

Asset : RefCounted          // base; subclass via static_type() → AssetType

AssetHandle<T>              // lightweight lazy handle
    AssetHandle(AssetID)
    get()    → SharedPtr<T>  // resolves via AssetManager on first call
    get_id() → AssetID
    is_valid() → bool

AssetCompiler               // abstract cook interface
    compile(AssetMetadata const&) → bool

AssetLoader                 // abstract load interface
    load(AssetMetadata const&) → SharedPtr<Asset>
```

## AssetManager API

```cpp
AssetManager::get()
  .import(path, AssetType, cache=false)  // register + cook → AssetID
  .load(id)                              // → SharedPtr<Asset> (cached)
  .load_as<T>(id)                        // → SharedPtr<T>
  .reload(id)                            // evict + reload from .igasset
  .reload_all()                          // recompile all (editor hotkey)
  .set_compiled_root(path)               // .igasset output dir; default "cache/"
  .prune_cache()                         // remove orphaned entries on startup

// static — used by loaders to trigger on-demand cooking:
AssetManager::get_compiler(AssetType) → AssetCompiler*
AssetManager::get_loader(AssetType)   → AssetLoader*
```

## AssetBinaryStream

Cook: `AssetBinaryWriter::open(metadata, header)` — writes `.igasset` header then raw fields.
Load: `AssetBinaryReader::open(metadata, header)` — validates magic + version before first read.
Both return a not-open instance on failure; check `is_open()` before use.

```
AssetBlobHeader { magic[4], version: uint8_t }

AssetBinaryWriter / AssetBinaryReader
  static open(metadata, header) → Self
  is_open() → bool
  good()    → bool
  write_u8 / write_u32 / write_bytes
  read_u8  / read_u32  / read_bytes
```

## AssetMesh  (`Ignis/Asset/AssetMesh.h`)

CPU-side mesh asset — format-agnostic byte buffer + uint32 indices, no GRI pointers, no vertex layout knowledge.

```
AssetMesh : Asset
  AssetMesh(id, Vector<uint8_t> vertices, Vector<uint32_t> indices)
  get_vertices() → const Vector<uint8_t>&   // raw packed bytes; stride is caller's concern
  get_indices()  → const Vector<uint32_t>&
  static_type()  → AssetType::Mesh
```

**AssetMeshCompiler** (`Ignis/Asset/AssetMeshCompiler.h/.cpp`): parses `.obj` (v/vn/vt/f, fan-triangulated, dedup by v/vt/vn triple) and writes an `IGAM v1` binary. No vertex struct — bytes are packed directly as `pos(12) nrm(12) uv(8)` = 32-byte stride. Binary layout: `vertex_stride u32, vertex_count u32, [raw vertex bytes], index_count u32, [uint32 indices]`. Registered in `AssetManager::get_compiler(AssetType::Mesh)`.

**MeshLoader** (`Ignis/Asset/MeshLoader.h/.cpp`): reads `IGAM v1` binary from `compiled_path`; on-demand cooks via `AssetMeshCompiler` if the binary is missing. Layout-agnostic — reads `vertex_stride` from the binary and passes raw bytes straight into `AssetMesh`. No OBJ parsing, no vertex struct knowledge.

Source files live in `resources/assets/meshes/`. Import via:
```cpp
AssetID id = EditorAssetManager::get().import_mesh("triangle.obj");
SharedPtr<AssetMesh> mesh = EditorAssetManager::get().load_mesh(id);
```

## AssetShader  (`Ignis/Asset/AssetShader.h`)

Asset-layer shader — lives in the Asset module, does NOT hold GRI pointers directly.
One `AssetShader` = one compiled stage. VS and PS from the same `.hlsl` are separate `AssetID`s.

```
AssetShader : Asset
  AssetShader(id, SharedPtr<RenderShader>)
  get_render_shader()  → RenderShader*
  get_stage()          → GRIShaderStage   (delegates to RenderShader)
  static_type()        → AssetType::Shader
```

Owns a `SharedPtr<RenderShader>`. Created by `ShaderLoader::load` and cached by `AssetManager`.

`EditorAssetManager::import_shader(filename)` → `std::pair<AssetID,AssetID>` (vs_id, ps_id).  
`AssetMetadata::user_data` carries `GRIShaderStage` from importer → compiler → loader.  
`AssetManager::import(path, type, cache, user_data)` factors `user_data` into the ID and cache name so VS/PS from the same source get distinct entries.
