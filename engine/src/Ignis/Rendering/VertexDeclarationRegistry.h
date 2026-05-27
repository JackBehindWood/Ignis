#pragma once

#include "GRI/GRIResource.h"

namespace Ignis
{
// Owns named GRIVertexDeclaration instances. Lives for the engine lifetime.
// Returned raw pointers are borrowed — do not delete them.
class VertexDeclarationRegistry
{
public:
    static VertexDeclarationRegistry& get();

    void                        register_layout(const String& name, SharedPtr<GRIVertexDeclaration> vd);
    const GRIVertexDeclaration* find(const String& name) const;

private:
    VertexDeclarationRegistry();
    UnorderedMap<String, SharedPtr<GRIVertexDeclaration>> m_layouts;
};

} // namespace Ignis
