#pragma once

#include <Ignis.h>
#include <Ignis/Rendering/GRI/GRIDefinitions.h>
#include <Ignis/Rendering/GRI/GRIResource.h>
#include <Ignis/Rendering/Material.h>
#include "EditorShaderCache.h"
#include "Project/IProjectObserver.h"

namespace Ignis
{

enum class EditorMaterial : uint16_t
{
    Grid             = 0,
    SelectionOutline = 1,
    Count
};

class EditorResourceCache : public IProjectObserver
{
public:
    static EditorResourceCache& get()
    {
        static EditorResourceCache instance;
        return instance;
    }

    void init(const Path& engine_root);
    void shutdown();
    void reload();

    SharedPtr<Material> get_material(EditorMaterial mat) const;
    GRIBuffer*          get_outline_params() const;

    void on_project_opened(const ProjectContext& ctx) override;
    void on_project_closed() override;

private:
    EditorResourceCache() = default;
    void compile_all();

    EditorShaderCache                       m_shader_cache;
    mutable SharedMutex                     m_mutex;
    static constexpr size_t                 k_mat_count = static_cast<size_t>(EditorMaterial::Count);
    Array<SharedPtr<Material>, k_mat_count> m_materials{};
    GRIBufferPtr                            m_outline_params;
};

} // namespace Ignis
