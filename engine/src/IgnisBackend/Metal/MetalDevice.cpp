#include "igpch.h"
#include "MetalDevice.h"
#include "MetalCommandQueue.h"

#include <Metal/Metal.hpp>

namespace Ignis
{
    MetalDevice::MetalDevice(MTL::Device* device) : m_device(MetalPtr<MTL::Device>::from_retained(device))
    {
        for (size_t i = 0; i < static_cast<size_t>(MetalQueueType::_Count); i++)
        {
            m_command_queues[i] = new MetalCommandQueue(*this);
        }
    }

    MetalDevice::~MetalDevice()
    {
        for (size_t i = 0; i < static_cast<size_t>(MetalQueueType::_Count); i++)
        {
            delete m_command_queues[i];
            m_command_queues[i] = nullptr;
        }
    }

    MetalDevice* MetalDevice::create_device()
    {
        MTL::Device* device = MTL::CreateSystemDefaultDevice();
        
        MetalDevice* mtl_device = new MetalDevice(device);
        
        return mtl_device;
    }
}