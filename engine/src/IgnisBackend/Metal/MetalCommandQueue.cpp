#include "MetalCommandQueue.h"
#include "MetalDevice.h"

#include <Metal/Metal.hpp>

namespace Ignis
{
    MetalCommandQueue::MetalCommandQueue(MetalDevice& device) : m_device(device)
    {
        m_command_queue = device.get_device()->newCommandQueue();
    }
}

