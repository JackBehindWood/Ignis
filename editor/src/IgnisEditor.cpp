#include "edpch.h"
#include <Ignis/Core/EntryPoint.h>

#include "IgnisEditor.h"
#include "EditorLayer.h"
#include "Asset/EditorAssetManager.h"
#include "EditorSettingsManager.h"
#include "Project/ProjectManager.h"
#include "Ignis/Rendering/Renderer.h"

#ifdef ENGINE_IMGUI
#include <Ignis/UI/ImGuiLayer.h>
#endif

namespace Ignis
{

Editor::Editor(const ApplicationSpecification& spec)
    : Application(spec)
{
    const Path engine_root = Filesystem::current_path() / "resources";
    bootstrap(engine_root);
    push_layer(new EditorLayer());

    ImGuiLayer* imgui_layer = new ImGuiLayer();
    push_overlay(imgui_layer);

    const Path ini_dir = Filesystem::current_path() / "resources" / ".ignis";

    Filesystem::create_directories(ini_dir);
    imgui_layer->set_ini_path((ini_dir / "imgui.ini").string());
}

Editor::~Editor()
{
    EditorResourceCache::get().shutdown();
    ProjectManager::get().close();
    EditorSettingsManager::get().save();
    EditorSettingsManager::get().save_engine();
}

void Editor::bootstrap(const Path& engine_root)
{
    // --- Engine-root setup (order matters) ---
    EditorAssetManager::get().set_engine_root(engine_root);
    EditorResourceCache::get().init(engine_root);

    // --- Register project observers (deterministic order) ---
    // EditorAssetManager is the sole gateway to AssetManager; EditorResourceCache follows.
    ProjectManager& pm = ProjectManager::get();
    pm.add_observer(&EditorAssetManager::get());
    pm.add_observer(&EditorResourceCache::get());

    // --- Load persisted settings ---
    EditorSettingsManager::get().load();
    EditorSettingsManager::get().load_engine();

    // --- Reload callback (routed through EditorAssetManager) ---
    EditorAssetManager::get().add_reload_callback(on_asset_reloaded);

    // --- Open test project from disk ---
    const Path       test_project = Filesystem::current_path() / "projects" / "TestProject" / "TestProject.igproject";
    const OpenResult r            = pm.open(test_project);
    IG_ASSERT(r == OpenResult::Ok, "Failed to open TestProject");
}

void Editor::on_asset_reloaded(AssetID id)
{
    Renderer::evict(static_cast<uint64_t>(id));
    if (EditorAssetManager::get().get_metadata(id).Type == AssetType::Shader)
    {
        Renderer::clear_pipeline_cache();
    }

    IG_INFO("Asset reloaded: {0}", static_cast<uint64_t>(id));
}

Application* create_application(const ApplicationCommandLineArgs& args)
{
    ApplicationSpecification spec;
    spec.name              = "Editor";
    spec.command_line_args = args;
    return new Editor(spec);
}

} // namespace Ignis
