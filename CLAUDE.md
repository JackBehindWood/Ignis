# Ignis Engine
C++20 game engine, Metal/macOS backend. `IgnisEditor` is the host app.

## Behavior
Developer: Jack Agterdenbos. Deep systems-level C++, terse style.

1. **Design First**: Write objective, approach, and impact to `.claude/memory/design.md` before any plan or code. Load it on-demand only — never at session start. Flow: `design.md` → scratchpad → code.
2. **Plan & Scratchpad**: State a max 2-sentence plan before touching any file. For 3+ files or sequential steps, write a step checklist to `.claude/memory/scratchpad.md`; check off items as you go and clear on completion.
3. **No Filler or Overthinking**: No reasoning narration, no C++/Metal explanations, and no post-mortems unless explicitly asked. Max 2 sentences of architectural context before a code block. Do not weigh pros/cons, analyse multiple speculative approaches, or over-reason edge cases—select the most direct, idiomatic C++20 path fitting the design doc and execute immediately.
4. **Docs Autonomy**: Freely update `.claude/docs/` and `.claude/memory/` as the system evolves.

## Scoped Rules
- `.claude/rules/cpp-style.md` — Architecture, Types, Macros, Style (`**/*.{cpp,h,hpp,hlsl,metal}`)
- `.claude/rules/memory-hygiene.md` — Session-Start Reads, Memory File Hygiene, Lazy Reads (global)
- `.claude/rules/docs-rendering.md` — renderer docs (`engine/src/Ignis/Rendering/**`)
- `.claude/rules/docs-metal.md` — Metal backend docs (`engine/src/IgnisBackend/Metal/**`)
- `.claude/rules/docs-editor.md` — editor docs (`editor/src/**`)
- `.claude/rules/docs-asset.md` — asset docs (`engine/src/Ignis/Asset/**`)
- `.claude/rules/docs-scene.md` — scene docs (`engine/src/Ignis/Scene/**`)
- `.claude/rules/docs-build.md` — build docs (`**/premake5.lua`, `build/**/*.py`)
