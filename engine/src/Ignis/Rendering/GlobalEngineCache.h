#pragma once

#include "Ignis/Rendering/RenderTexture2D.h"

namespace Ignis
{

class GlobalEngineCache
{
public:
    SharedPtr<RenderTexture2D> get_brdf_lut() const
    {
        SharedLock lock(m_mutex);
        return m_brdf_lut;
    }

    void set_brdf_lut(SharedPtr<RenderTexture2D> lut)
    {
        UniqueLock lock(m_mutex);
        m_brdf_lut = std::move(lut);
    }

    void clear()
    {
        UniqueLock lock(m_mutex);
        m_brdf_lut.reset();
    }

private:
    mutable SharedMutex        m_mutex;
    SharedPtr<RenderTexture2D> m_brdf_lut;
};

} // namespace Ignis
