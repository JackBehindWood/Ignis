# Ignis Engine
C++20 game engine, Metal/macOS backend. `IgnisEditor` is the host app.

## Behavior
Developer: Jack Agterdenbos. Deep systems-level C++, terse style.

1. **Design First**: Update `.claude/memory/design.md` with objective, approach, and impact before any plan or code. Flow: `design.md` → scratchpad → code.
2. **Plan & Scratchpad**: State a max 2-sentence plan before touching any file. For 3+ files or sequential steps, write a step checklist to `.claude/memory/scratchpad.md`; check off items as you go and clear on completion.
3. **No Filler**: No reasoning narration, no C++/Metal explanations, no post-mortems unless asked. Max 2 sentences of reasoning before the first code block. Prioritize direct action.
4. **Lazy Reads**: Read only files in the immediate scope of the requested change. Prefer targeted line ranges. Flag heavy context load immediately. Do not speculatively read adjacent files.
5. **Docs Autonomy**: Freely update `.claude/docs/` and `.claude/memory/` as the system evolves.

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
- Always add braces in if statements and loops.
- `PascalCase` types · `snake_case` methods · `m_` private · `s_` static · `k_` const static.
- Namespace: `Ignis`. **Zero comments by default.** Only add a comment when the WHY is non-obvious (hidden constraint, subtle invariant, or a specific bug workaround). Never explain WHAT the code does. No docstrings.
- **No `---` in `.md` files.** Use `##`/`###` headers for section breaks.

## Session-Start Reads
**Trigger:** Execute a single, lazy read of these files ONLY on the first turn of a session, and ONLY if the prompt involves code changes, debugging, or architectural review. Never re-read them mid-session.

- `.claude/memory/user_profile.md` (Read-only context)
- `.claude/memory/project_state.md` (Validate focus; cross-check with `git status` in the same tool call)
- `.claude/memory/scratchpad.md` (If non-empty, resume the checklist immediately)
- `.claude/memory/design.md` (If non-empty, design is active. Abort and demand design approval before writing code)

Do not read these files for quick queries, syntax checks, or non-code discussions.

## Memory File Hygiene
Never rewrite a memory file in full. Surgical edits only:
- **`scratchpad.md`**: check items off as `[x]` in-place; never rewrite unchecked lines; wipe the body only when all steps are done.
- **`project_state.md`**: edit targeted lines only; append new systems under "Standing Systems"; update "Current Focus" in-place; update at session end or milestone, not mid-task.
- **`design.md`**: append refinements as new `###` subsections; never reformat prior content; wipe only when Jack confirms the system is complete.

## On-Demand Docs — load before touching any related file

| Doc | Load when touching… |
|-----|---------------------|
| `.claude/docs/renderer/renderer.md` | `Renderer`, `RenderResourceCache`, `MaterialFactory`, `FrameUniformAllocator`, bind helpers, uniform slots |
| `.claude/docs/renderer/design.md` | renderer philosophy, system architecture diagram |
| `.claude/docs/renderer/mesh.md` | `RenderMesh`, mesh upload pipeline, vertex layout |
| `.claude/docs/renderer/gri.md` | GRI interface, resource/descriptor types, pipeline state, render passes, `RenderSystem` |
| `.claude/docs/renderer/shader.md` | `RenderShader`, `ShaderTarget`, `ShaderReflection`, `ShaderCache`, SPIR-V/MSL compilation |
| `.claude/docs/renderer/metal.md` | anything in `IgnisBackend/Metal/`, metal-cpp ownership, `MetalGRI`, `MetalCommandContext`, `MetalShaderLibrary` |
| `.claude/docs/asset-system.md` | `AssetManager`, `AssetHandle`, `AssetShader`, `AssetMesh`, asset cooking, `AssetType`, binary streams |
| `.claude/docs/build.md` | build system, Premake, vendor layout, plugins, entry points |
| `.claude/docs/editor.md` | `EditorLayer`, `EditorAssetManager`, `resources/` layout |
| `.claude/docs/scene.md` | `Scene`, `Entity`, `Component`, `SceneRenderer`, ECS model, culling/sorting/batching |
| `.claude/docs/docs.md` | doc layout, domain boundary rules, where to put new docs |
