# Shader Pipeline

## Two-layer binary split

| File | Magic | Role |
|---|---|---|
| `<name>_<hash>.igasset` | `IGAS` v2 | Asset recipe — source path + entry points. No bytecode. |
| `<name>_<hash>.igsh`    | `IGSH` v2 | Compiled cache blob — bytecode + reflection + content hash. One file per stage. |


## Layer types

Asset-layer shader (`AssetShader`) is documented in `asset-system.md`.

```
RenderShader  (Ignis/Rendering/RenderShader.h)
  Per-stage. Holds GRIShaderPtr + GRIShaderStage + ShaderReflection.
  get_shader()     → GRIShader*
  get_stage()      → GRIShaderStage
  get_reflection() → const ShaderReflection&

ShaderTarget  (Ignis/Rendering/ShaderTarget.h)
  { Vulkan_SPIRV=0, Metal_MSL=1, DX12_DXIL=2, OpenGL_GLSL=3, HLSL=4 }
  HLSL=4 is a SpirvCompiler::create() factory selector, not a cook-time output.

ShaderReflection  (Ignis/Rendering/ShaderReflection.h)
  ShaderResourceBinding { name, set, binding, is_unbounded }
  ShaderStageInput      { name, location }
  ShaderPushConstant    { name, size }
  Fields: uniform_buffers, storage_buffers, separate_images, separate_samplers,
          stage_inputs, stage_outputs, push_constants
```

`SpirvReflection` is backend-private. `ShaderCompiler` translates it to `ShaderReflection`.

`AssetShaderCompiler` (in `Ignis/Asset/`) is asset-layer — see `asset-system.md`.


## .igasset format (IGAS v2)

One file per stage.

```
[magic 'IGAS'][version u8=2]
source_path  : str
stage        : u8              // GRIShaderStage cast to uint8
entry_point  : str             // "VSMain" / "PSMain" / "CSMain"
num_defines  : u32             // 0 for now; reserved for [key(str) value(str)] × N
num_deps     : u32
per dep:
  dep_path   : str
  mtime_ns   : u64
```

Written by `AssetShaderCompiler::compile()`. `dep_path` includes the source file and all `#include "..."` files. `ShaderLoader` checks each dep's mtime against the recorded value; any newer → recompile.


## Shader cache blob format (IGSH v2)

One file per stage; filename: `<stem>_<stage_hash_hex8>.igsh`.

```
[magic 'IGSH'][version u8=2]
source_hash_lo  u32
source_hash_hi  u32             // FNV-1a 64-bit hash of source content, split lo/hi
target          u8

stage_type(u8)  entry_point(str)  bytecode_size(u32)  bytecode
reflection:
  uniform_buffers, storage_buffers, separate_images, separate_samplers
    — each as count(u32) + entries: name(str) set(u32) binding(u32) is_unbounded(u8)
  stage_inputs  — count(u32) + entries: name(str) location(u32)
  stage_outputs — count(u32) + entries: name(str) location(u32)
  push_constants — count(u32) + entries: name(str) size(u32)
```

Written and read exclusively by `ShaderCache`. Lives in `ShaderCache::m_cache_root`.


## ShaderCache  (Ignis/Rendering/ShaderCache.h)

```
ShaderCache::get()
  set_cache_root(Path)                              — editor calls on project load
  get_or_compile(source_path, GRIShaderStage)       → SharedPtr<RenderShader>
  get_or_compile(source_text, virtual_name, stage)  → SharedPtr<RenderShader>
```

Full source compiles both stages in one pass; CacheEntry stores { vs, ps } internally.
Callers request one stage at a time — second call for same source is a free memory hit.

Lookup order: memory cache → disk (`.igsh`) → compile. Content hash drives invalidation:
if the HLSL source changed, hash mismatch on disk → recompile.

Cache filename: `<stem>_<path_hash_hex8>.igsh` — stable name, invalidated by embedded hash.

Internal renderer shaders call `get_or_compile()` directly — no `AssetID`, no registry.


## Data flows

**Asset shader (cook + load):**
```
EditorAssetManager::import_shader("triangle.hlsl") → {vs_id, ps_id}
  AssetManager::import(source, Shader, true, GRIShaderStage::Vertex) → vs_id
  AssetManager::import(source, Shader, true, GRIShaderStage::Pixel)  → ps_id

AssetManager::load_as<AssetShader>(vs_id)
  ShaderLoader::load(vs_metadata)          // user_data = GRIShaderStage::Vertex
    open IGAS v2 (cook if missing/stale)
    dep staleness check
    ShaderCache::get_or_compile(source, Vertex, opts) → SharedPtr<RenderShader>
    → AssetShader(vs_id, render_shader)    // single stage

AssetManager::load_as<AssetShader>(ps_id)  // same flow, stage = Pixel
```

**Internal renderer shader:**
```
ShaderCache::get().get_or_compile("engine/shaders/pbr.hlsl") → SharedPtr<RenderShader>
```


## Caching layers

| Layer | Key | Eviction |
|---|---|---|
| `AssetManager::m_loaded_assets` | `AssetID` | `reload()` / `reload_all()` |
| `ShaderCache::m_memory` | path hash | source content hash mismatch |
| `.igsh` disk blob | filename (path hash) | embedded content hash mismatch → recompile |


## MSL Metal Buffer Index Convention

`MslSpirvCompiler::apply_bindings_and_compile` maps both UBOs and SBOs using `msl_buffer = desc_set`.
The HLSL register space IS the Metal buffer index:

| HLSL declaration | set | msl_buffer | C++ binding call slot |
|---|---|---|---|
| `register(b0)` (no space) | 0 | 0 | slot 0 — legacy shaders |
| `register(b0, space1)` | 1 | 1 | slot 1 — `g_frame` (Ignis.hlsl) |
| `register(b0, space3)` | 3 | 3 | slot 3 — `g_material` (MaterialArgs) |
| `register(t0, space28)` | 28 | 28 | Metal vertex buffer 28 — instance SBO |

`Renderer::bind_frame_data` binds at both slot 0 and slot 1 for backward + forward compatibility.
`UniformSlot::MaterialArgs = 3`.


## SPIRV compiler stack (IgnisBackend/spirv/)

```
SpirvCompiler                    — abstract base; factory via create(ShaderTarget)
HlslSpirvCompiler                — DXC: HLSL → SPIR-V
MslSpirvCompiler                 — SPIRV-Cross: SPIR-V → MSL → xcrun metallib

ShaderCompiler  (Ignis/Rendering/)
  explicit ShaderCompiler(ShaderTarget)
  compile(hlsl_source) → Vector<ShaderStageOutput>   empty on failure

ShaderStageOutput { stage, entry_point, bytecode, reflection }
```
