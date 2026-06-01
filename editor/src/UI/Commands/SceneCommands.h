#pragma once

#include "CommandDispatcher.h"
#include "../Workspace/WorkspaceManager.h"
#include "../SceneEditor/SceneEditorContext.h"
#include <Ignis/Scene/Entity.h>
#include <Ignis/Scene/Components/Components.h>
#include <Ignis/Scene/SceneSerializer.h>
#include <Ignis/Math/Transform.h>
#include "Project/ProjectManager.h"

namespace Ignis
{

struct ChangeTransformCmd : IReversibleCommand
{
    Entity           entity;
    Math::Transformf before;
    Math::Transformf after;

    ChangeTransformCmd(Entity e, Math::Transformf b, Math::Transformf a)
        : entity(e),
          before(std::move(b)),
          after(std::move(a))
    {
    }

    void execute() override
    {
        apply(after);
    }
    void undo() override
    {
        apply(before);
    }

private:
    void apply(const Math::Transformf& xf)
    {
        auto& tc    = entity.get_component<TransformComponent>();
        tc.position = xf.position;
        tc.rotation = xf.rotation;
        tc.scale    = xf.scale;
    }
};

struct ChangeNameCmd : IReversibleCommand
{
    Entity entity;
    String before;
    String after;

    ChangeNameCmd(Entity e, String b, String a)
        : entity(e),
          before(std::move(b)),
          after(std::move(a))
    {
    }

    void execute() override
    {
        entity.get_component<NameComponent>().name = after;
    }
    void undo() override
    {
        entity.get_component<NameComponent>().name = before;
    }
};

template <typename T, typename Setter>
struct PropertyChangeCmd : IReversibleCommand
{
    Entity entity;
    T      before;
    T      after;
    Setter setter;

    PropertyChangeCmd(Entity e, T b, T a, Setter s)
        : entity(e),
          before(std::move(b)),
          after(std::move(a)),
          setter(std::move(s))
    {
    }

    void execute() override
    {
        setter(entity, after);
    }
    void undo() override
    {
        setter(entity, before);
    }
};

template <typename T, typename Setter>
auto make_property_cmd(Entity e, T before, T after, Setter&& s)
{
    return create_unique<PropertyChangeCmd<T, std::decay_t<Setter>>>(e, std::move(before), std::move(after),
                                                                     std::forward<Setter>(s));
}

struct CreateEntityCmd : IReversibleCommand
{
    Scene*  scene;
    String  name;
    Entity* selected_entity;
    Entity  entity;

    CreateEntityCmd(Scene* s, String n, Entity* sel = nullptr)
        : scene(s),
          name(std::move(n)),
          selected_entity(sel)
    {
    }

    void execute() override
    {
        entity = scene->create_entity(name);

        entity.add_component<TransformComponent>();

        if (selected_entity)
        {
            *selected_entity = entity;
        }
    }
    void undo() override
    {
        if (selected_entity && *selected_entity == entity)
        {
            *selected_entity = {};
        }
        scene->destroy_entity(entity);
        entity = {};
    }
};

struct RemoveEntityCmd : IReversibleCommand
{
    Scene*  scene;
    String  name;
    Entity  entity;
    Entity* selected_entity;

    RemoveEntityCmd(Scene* s, Entity e, Entity* sel = nullptr)
        : scene(s),
          entity(e),
          selected_entity(sel)
    {
        if (e.has_component<NameComponent>())
        {
            name = e.get_component<NameComponent>().name;
        }
    }

    void execute() override
    {
        if (selected_entity && *selected_entity == entity)
        {
            *selected_entity = {};
        }
        scene->destroy_entity(entity);
        entity = {};
    }

    void undo() override
    {
        entity = scene->create_entity(name);
        entity.add_component<TransformComponent>();
        if (selected_entity)
        {
            *selected_entity = entity;
        }
    }
};

struct AddComponentCmd : IReversibleCommand
{
    Entity entity;
    void (*add_fn)(Entity&);
    void (*remove_fn)(Entity&);

    AddComponentCmd(Entity e, void (*add)(Entity&), void (*remove)(Entity&))
        : entity(e),
          add_fn(add),
          remove_fn(remove)
    {
    }

    void execute() override
    {
        add_fn(entity);
    }
    void undo() override
    {
        remove_fn(entity);
    }
};

struct NewSceneCmd : IEditorCommand
{
    Scene*             scene;
    CommandDispatcher* dispatcher;

    NewSceneCmd(Scene* s, CommandDispatcher* d)
        : scene(s),
          dispatcher(d)
    {
    }

    void execute(WorkspaceManager& wm) override
    {
        scene->clear();
        dispatcher->clear_history();
        auto* ws_data = static_cast<SceneEditorData*>(wm.active_data());
        if (ws_data)
        {
            ws_data->current_scene_path = NullOpt;
        }
    }
};

struct LoadSceneCmd : IEditorCommand
{
    Scene*             scene;
    Path               path;
    CommandDispatcher* dispatcher;

    LoadSceneCmd(Scene* s, Path p, CommandDispatcher* d)
        : scene(s),
          path(std::move(p)),
          dispatcher(d)
    {
    }

    void execute(WorkspaceManager&) override
    {
        SceneSerializer serializer;
        if (!serializer.deserialize(*scene, path))
        {
            IG_ERROR("LoadSceneCmd: failed to deserialize '{}'", path.string());
        }
        dispatcher->clear_history();
    }
};

struct SaveSceneCmd : IEditorCommand
{
    const Scene* scene;
    Path         path;

    SaveSceneCmd(const Scene* s, Path p)
        : scene(s),
          path(std::move(p))
    {
    }

    void execute(WorkspaceManager&) override
    {
        SceneSerializer serializer;
        serializer.serialize(*scene, path);
    }
};

struct LoadProjectCmd : IEditorCommand
{
    Path               path;
    CommandDispatcher* dispatcher;

    LoadProjectCmd(Path p, CommandDispatcher* d)
        : path(std::move(p)),
          dispatcher(d)
    {
    }

    void execute(WorkspaceManager& wm) override
    {
        auto& pm = ProjectManager::get();
        pm.close();
        const auto result = pm.open(path);
        if (result != OpenResult::Ok)
        {
            IG_ERROR("LoadProjectCmd: failed to open '{}' — OpenResult={}", path.string(), static_cast<int>(result));
            return;
        }
        dispatcher->clear_history();

        auto* ws_data = static_cast<SceneEditorData*>(wm.active_data());
        if (!ws_data)
        {
            return;
        }

        const auto& startup = pm.descriptor().startup_scene;
        if (!startup.empty())
        {
            const Path      scene_path = pm.descriptor().root / startup;
            SceneSerializer serializer;
            if (serializer.deserialize(*ws_data->scene, scene_path))
            {
                ws_data->current_scene_path = scene_path;
            }
            else
            {
                IG_ERROR("LoadProjectCmd: failed to load startup scene '{}'", scene_path.string());
                ws_data->scene->clear();
                ws_data->current_scene_path = NullOpt;
            }
        }
        else
        {
            ws_data->scene->clear();
            ws_data->current_scene_path = NullOpt;
        }
    }
};

struct SaveProjectCmd : IEditorCommand
{
    void execute(WorkspaceManager&) override
    {
        ProjectManager::get().save();
    }
};

} // namespace Ignis
