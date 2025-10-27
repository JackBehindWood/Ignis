#include "igpch.h"
#include "MetalDevice.h"
#include "MetalCommandQueue.h"

#include <Metal/Metal.hpp>

namespace Ignis
{
    MetalDevice::MetalDevice(MTL::Device* device) : m_device(device)
    {
        m_device->retain();

        m_command_queue = new MetalCommandQueue(*this);
    }

    MetalDevice::~MetalDevice()
    {    
        delete m_command_queue;

        m_device->release();
    }

    MetalDevice* MetalDevice::create_device()
    {
        MTL::Device* device = MTL::CreateSystemDefaultDevice();
        
        MetalDevice* mtl_device = new MetalDevice(device);
        
        return mtl_device;
    }

}