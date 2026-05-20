# Ignis Engine
C++20 game engine, Metal/macOS backend. `IgnisEditor` is the host app.

## Behavior
Developer: Jack Agterdenbos. Deep systems-level C++, terse style.

1. **Plan & Scratchpad**: Before touching any engine/editor file, state a max 2-sentence plan. For 3+ files or sequential steps, you **must** write a step checklist to `.claude/memory/scratchpad.md`; check off items as you go and clear the file on completion.
2. **No Filler**: No reasoning narration, no C++/Metal explanations, no post-mortems unless asked.
3. **Lazy Reads**: Prefer targeted line ranges over whole files. Flag heavy context load immediately.
4. **Docs Autonomy**: Freely update `.claude/docs/` and `.claude/memory/` as the system evolves.

## Architecture
- **Asset** `Ignis/Asset/`: No GRI pointers, no Metal headers.
- **Engine** `Ignis/Rendering/`: GRI interface only; Metal behind `#if`.
- **Backend** `IgnisBackend/Metal/`: Only TUs here may include `<Metal/Metal.hpp>`.

## Types (`Foundation/` + `igpch.h`)
Always use Foundation aliases, never raw `std::` in TUs. New type: alias in `Foundation/X.h`, include that header in `igpch.h`.
- `String`, `Stringstream`, `Vector<T>`, `UnorderedMap<K,V>`, `Path`, `Filesystem`
- `SharedPtr<T>` → `create_shared<T>(...)` · `UniquePtr<T>` → `create_unique<T>(...)`

## Macros
- **Platform**: `IG_PLATFORM_MACOS` only. Windows/iOS/Android/Linux trigger `#error`.
- **Configs**: `IG_DEBUG`, `IG_RELEASE`, `IG_DIST`.
- **Assert**: `IG_ASSERT(cond, [msg])`, `IG_CORE_ASSERT(cond, [msg])`.
- **Log**: `IG_CORE_{TRACE..CRITICAL}` (engine) · `IG_{TRACE..CRITICAL}` (app).
- **Util**: `BIT(x)`, `IG_DEBUGBREAK()`, `IG_BIND_EVENT_FN(fn)`.

## Style
- `PascalCase` types · `snake_case` methods · `m_` private · `s_` static · `k_` const static.
- Namespace: `Ignis`. No comments unless absolutely necessary — WHY only, never WHAT. No docstrings.

---

## Session-Start Reads
Skip if clearly non-code. Load when touching files, resuming, reviewing, or debugging:
- `.claude/memory/user_profile.md`
- `.claude/memory/project_state.md`
- `.claude/memory/scratchpad.md` — non-empty means task in progress; resume it.

Cross-check `project_state.md` against `git status` when touching active work.

## On-Demand Docs — load before touching any related file

| Doc | Load when touching… |
|-----|---------------------|
| `.claude/docs/renderer/gri.md` | GRI interface, resource/descriptor types, pipeline state, render passes, `RenderSystem` |
| `.claude/docs/renderer/shader.md` | shader cooking/loading, `RenderShader`, `ShaderTarget`, `SpirvReflection`, SPIR-V/MSL compilation |
| `.claude/docs/renderer/metal.md` | anything in `IgnisBackend/Metal/`, metal-cpp ownership, `MetalGRI`, `MetalCommandContext`, `MetalShaderLibrary` |
| `.claude/docs/asset-system.md` | `AssetManager`, `AssetHandle`, `AssetShader`, asset cooking, `AssetType`, binary streams |
| `.claude/docs/build.md` | build system, Premake, vendor layout, plugins, entry points |
| `.claude/docs/editor.md` | `EditorLayer`, `EditorAssetManager`, `resources/` layout |
