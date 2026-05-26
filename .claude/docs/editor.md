# Editor (IgnisEditor)

Host application. All source under `editor/src/`. Precompiled header: `edpch.h`.

## Architecture Invariant

In the editor module, **only `EditorAssetManager.cpp` may include or call `AssetManager` directly**. All other editor TUs route through `EditorAssetManager`. The project system (`ProjectManager`, `IProjectObserver`, etc.) lives exclusively in the editor module — engine types do not implement project interfaces.

---

## Bootstrap order (`Editor::Editor`)

```
Editor(spec)
  → Application(spec)          // window, RenderSystem::init
  → bootstrap(engine_root)
      EditorAssetManager::set_engine_root()
      ShaderCache::set_engine_cache_root()
      ProjectManager::add_observer(&EditorAssetManager::get())
      EditorAssetManager::add_reload_callback(on_asset_reloaded)
      ProjectManager::open_in_memory(desc)   // dispatches on_project_opened
  → push_layer(new EditorLayer)
      EditorLayer::attach()
        IG_ASSERT(ProjectManager::get().is_open())
```

---

## Project System (`editor/src/Project/`)

### `IProjectObserver` + `ProjectContext`

Pure interface implemented only by editor types. `ProjectManager` holds a `Vector<IProjectObserver*>` and dispatches `on_project_opened` / `on_project_closed` in explicit registration order.

```cpp
struct ProjectContext {
    const ProjectDescriptor& descriptor;
    Path asset_source_abs;
    Path compiled_cache_abs;
    bool in_memory;
};

class IProjectObserver {
public:
    virtual void on_project_opened(const ProjectContext& ctx) = 0;
    virtual void on_project_closed() = 0;
};
```

### `ProjectManager`

Meyer's singleton. In-memory phase only: calls `open_in_memory`. The disk path (`open`, `create`) exists but is unused until the disk phase.

Key invariants:
- `validate(ProjectOpenMode::InMemory)` → zero filesystem I/O (name non-empty check only)
- `validate(ProjectOpenMode::FromDisk)` → full directory + write-probe checks
- `close()` skips `save()` when `is_in_memory()` is true
- Observers are dispatched in explicit registration order (deterministic, no constructor-timing races)

### `ProjectDescriptor`

Pure POD. `asset_source_dir` and `compiled_cache_dir` are `String` (always relative to `root`).

---

## `EditorAssetManager`

Singleton. Sole project observer for all asset/engine subsystem setup. Implements `IProjectObserver`.

On `on_project_opened`: calls `AssetManager::shutdown()` then `AssetManager::init(cfg)` with correct compiled roots.  
On `on_project_closed`: calls `AssetManager::shutdown()`, clears project paths.

**Delegated API (all editor code must use these instead of calling `AssetManager` directly):**

```
update(float dt)                   // AssetManager pump — call each frame
reload_all()                       // force-reload all assets (F5)
add_reload_callback(fn)            // → token
remove_reload_callback(token)
get_metadata(id)                   // → AssetMetadata

import_engine_asset(relative, type)
import_project_asset(relative, type)
import_texture / import_shader / import_mesh / import_material
load_texture / load_shader / load_mesh / load_material
load_deferred(id)
```

Engine/project root layout:
```
<engine_root>/
  assets/          engine bundled assets
  cache/           engine compiled cache (also used as compiled_root in in-memory mode)

<project_root>/
  <asset_source_dir>/   project assets (relative, default "assets")
  <compiled_cache_dir>/ project compiled cache (relative, default "cache")
```

---

## `EditorSettingsManager`

Owns `EditorSettings` (recent projects, projects_root). Provides:
- `editor_prefs_path()` — `<cwd>/resources/.ignis/editor.cfg`
- `push_recent(path)` — dedup insert, capped at `k_max_recent_projects`
- `load()` / `save()` — YAML via `SettingsSerializer`

`EditorSettings` is now a pure POD; all mutation logic lives in the manager.

---

## `EditorLayer`

`Layer` subclass. Rendering layer only — no project bootstrap.

```
attach()   → IG_ASSERT(ProjectManager::get().is_open()); log only
detach()   → EditorSettingsManager::get().save()
update(ts) → EditorAssetManager::get().update(2.0f); render scene if m_scene_ready
event(e)   → F5 → EditorAssetManager::get().reload_all()
```

---

## Settings Split

| Layer  | Type                   | Owner                   | Serializer location          |
|--------|------------------------|-------------------------|------------------------------|
| Engine | `EngineSettings`       | `EngineSettingsManager` | `engine/src/Ignis/Settings/` |
| Editor | `EditorSettings`       | `EditorSettingsManager` | `editor/src/`                |
| Project| `ProjectDescriptor`    | `ProjectManager`        | `editor/src/Project/`        |

`EngineSettingsManager` (engine/src/Ignis/Settings/EngineSettingsManager.h):
- Holds `EngineSettings`, `Vector<ISettingsObserver*>`, `apply(const EngineSettings&)`
- `Application` no longer owns settings or callbacks

`SettingsSerializer` still exists in `editor/src/Project/` but `read/write_engine_settings` are only called from the disk-open path.
