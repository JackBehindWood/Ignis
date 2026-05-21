# Shader Pipeline

## Two-layer binary split

| File | Magic | Role |
|---|---|---|
| `<name>_<hash>.igasset` | `IGAS` v1 | Asset recipe — source path + entry points. No bytecode. |
| `<name>_<hash>.igsh`    | `IGSH` v8 | Compiled cache blob — bytecode + reflection + content hash. |

---

## Layer types

```
AssetShader  (Ignis/Asset/AssetShader.h)
  : public Asset
  Holds two SharedPtr<RenderShader> (m_vs, m_ps).
  get_vertex_render_shader() → RenderShader*
  get_pixel_render_shader()  → RenderShader*
  static_type()              → AssetType::Shader

RenderShader  (Ignis/Rendering/RenderShader.h)
  Per-stage. Holds GRIShaderPtr + GRIShaderStage + ShaderReflection.
  get_shader()     → GRIShader*
  get_stage()      → GRIShaderStage
  get_reflection() → const ShaderReflection&

ShaderTarget  (Ignis/Rendering/ShaderTarget.h)
  { Vulkan_SPIRV=0, Metal_MSL=1, DX12_DXIL=2, OpenGL_GLSL=3, HLSL=4 }
  HLSL=4 is a SpirvCompiler::create() factory selector, not a cook-time output.

ShaderReflection  (Ignis/Rendering/ShaderReflection.h)
  ShaderResourceBinding { name, set, binding }
  ShaderStageInput      { name, location }
  ShaderPushConstant    { name, size }
  Fields: uniform_buffers, storage_buffers, separate_images, separate_samplers,
          stage_inputs, push_constants
```

`SpirvReflection` is backend-private. `ShaderCompiler` translates it to `ShaderReflection`.

---

## .igasset format (IGAS v1)

```
[magic 'IGAS'][version u8=1]
source_path   len(u32) + chars
entry_vs      len(u32) + chars
entry_ps      len(u32) + chars
num_defines   u32               // 0 for now; reserved for [name(str) value(str)] × N
```

Written by `AssetShaderCompiler::compile()`. Rewrite whenever source path changes.

---

## Shader cache blob format (IGSH v8)

```
[magic 'IGSH'][version u8=8]
source_hash_lo  u32
source_hash_hi  u32             // FNV-1a 64-bit hash of source file content, split lo/hi
target          u8
num_stages      u8              // always 2

Per stage:
  stage_type(u8)  entry_point(str)  bytecode_size(u32)  bytecode
  reflection: [uniform_buffers, storage_buffers, separate_images, separate_samplers,
               stage_inputs, push_constants] — each as count(u32) + entries
```

Written and read exclusively by `ShaderCache`. Lives in `ShaderCache::m_cache_root`.

---

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

---

## Data flows

**Asset shader (cook + load):**
```
import()
  AssetShaderCompiler::compile() → writes IGAS v1 .igasset

AssetManager::load_as<AssetShader>(id)
  ShaderLoader::load()
    reads .igasset → source_path
    ShaderCache::get_or_compile(source_path) → SharedPtr<RenderShader>
    → AssetShader(id, *render_shader)
```

**Internal renderer shader:**
```
ShaderCache::get().get_or_compile("engine/shaders/pbr.hlsl") → SharedPtr<RenderShader>
```

---

## Caching layers

| Layer | Key | Eviction |
|---|---|---|
| `AssetManager::m_loaded_assets` | `AssetID` | `reload()` / `reload_all()` |
| `ShaderCache::m_memory` | path hash | source content hash mismatch |
| `.igsh` disk blob | filename (path hash) | embedded content hash mismatch → recompile |

---

## SPIRV compiler stack (IgnisBackend/spirv/)

```
SpirvCompiler                    — abstract base; factory via create(ShaderTarget)
HlslSpirvCompiler                — DXC: HLSL → SPIR-V
MslSpirvCompiler                 — SPIRV-Cross: SPIR-V → MSL → xcrun metallib

ShaderCompiler  (Ignis/Rendering/)
  explicit ShaderCompiler(ShaderTarget)
  compile(hlsl_source) → Vector<ShaderStageOutput>   empty on failure

ShaderStageOutput { stage, entry_point, bytecode, reflection }

AssetShaderCompiler : AssetCompiler  (Ignis/Asset/)
  compile(AssetMetadata) → bool      // writes IGAS v1 recipe only, no HLSL read
```
