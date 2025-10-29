#include "IgnisBackend/Metal/MetalAutoReleasePool.h"

#include <Foundation/Foundation.hpp>

namespace Ignis
{
    MetalScopedAutoreleasePool::MetalScopedAutoreleasePool()
    {
        m_autorelease_pool = NS::AutoreleasePool::alloc()->init();
    }

    MetalScopedAutoreleasePool::~MetalScopedAutoreleasePool()
    {
        m_autorelease_pool->release();
    }
}