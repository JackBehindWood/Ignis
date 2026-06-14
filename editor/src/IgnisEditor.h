#pragma once

#include <Ignis.h>

namespace Ignis
{

class Editor : public Application
{
public:
    static Editor& editor_get()
    {
        return static_cast<Editor&>(Application::get());
    }

    explicit Editor(const ApplicationSpecification& spec);
    ~Editor() override;

private:
    void        bootstrap(const Path& engine_root, const Path& shaders_root);
    static void on_asset_reloaded(AssetID id);
};

} // namespace Ignis
