# Claude Code Active Scratchpad
## Current Objective
Initially store GRIVertexDeclaration inside RenderMesh. Pass it into create(). EditorLayer uses m_mesh->get_vertex_declaration() for PSO.

## Execution Plan
1. [ ] RenderMesh.h — add vd param to ctor + create(); add m_vertex_declaration member + getter; remove GRI* from create() (user already moved that inside)
2. [ ] RenderMesh.cpp — fix typo `gri* gri` → `GRI* gri`; add GRI.h include; forward vd to ctor
3. [ ] EditorLayer.cpp — pass vd into RenderMesh::create(); use m_mesh->get_vertex_declaration() in pso_desc; drop local vd
4. [ ] AssetMesh.h - Make it be independant of MeshVertex
5. [ ] RenderMesh.h - find a way to remove m_vertex_declaration!

## Checklist
- [ ] RenderMesh.h
- [ ] RenderMesh.cpp
- [ ] EditorLayer.cpp
