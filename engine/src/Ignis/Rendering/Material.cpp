#include "igpch.h"
#include "Material.h"
#include "Ignis/Rendering/RenderSystem.h"

namespace Ignis
{

void Material::update_pbr_params(const PBRMaterialParams& p)
{
    m_pbr_params = p;
    if (m_params_buffer)
    {

        RenderSystem::get_gri()->update_buffer(m_params_buffer.get(), &m_pbr_params, sizeof(PBRMaterialParams));
    }
}

} // namespace Ignis
