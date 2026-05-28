#include "igpch.h"
#include "VertexDeclarationRegistry.h"

namespace Ignis
{

VertexDeclarationRegistry& VertexDeclarationRegistry::get()
{
    static VertexDeclarationRegistry s_instance;
    return s_instance;
}

VertexDeclarationRegistry::VertexDeclarationRegistry()
{
    // pos(12) nrm(12) uv(8), stride 32 — matches IGAM cook output
    auto vd                    = create_shared<GRIVertexDeclaration>();
    vd->elements[0]            = {GRIVertexElementSemantic::Position, GRIVertexElementFormat::Float3, 0, 29};
    vd->elements[1]            = {GRIVertexElementSemantic::Normal, GRIVertexElementFormat::Float3, 12, 29};
    vd->elements[2]            = {GRIVertexElementSemantic::TexCoord, GRIVertexElementFormat::Float2, 24, 29};
    vd->num_elements           = 3;
    vd->bindings[0]            = {29, 32};
    vd->num_bindings           = 1;
    m_layouts["standard_mesh"] = std::move(vd);
}

void VertexDeclarationRegistry::register_layout(const String& name, SharedPtr<GRIVertexDeclaration> vd)
{
    m_layouts[name] = std::move(vd);
}

const GRIVertexDeclaration* VertexDeclarationRegistry::find(const String& name) const
{
    auto it = m_layouts.find(name);
    return it != m_layouts.end() ? it->second.get() : nullptr;
}

} // namespace Ignis
