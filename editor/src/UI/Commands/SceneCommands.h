#pragma once

#include "CommandDispatcher.h"
#include <Ignis/Scene/Entity.h>
#include <Ignis/Scene/Components/Components.h>
#include <Ignis/Math/Transform.h>

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

} // namespace Ignis
