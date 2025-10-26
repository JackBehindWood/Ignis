#include "igpch.h"
#include "MetalDevice.h"

#include <Metal/Metal.hpp>

namespace Ignis
{
    MetalDevice::MetalDevice(MTL::Device* device) : m_device(device)
    {
        m_device->retain();
        }

    MetalDevice::~MetalDevice()
    {    
        m_device->release();
    }

    MetalDevice* MetalDevice::create_device()
    {
        MTL::Device* device = MTL::CreateSystemDefaultDevice();
        
        MetalDevice* mtl_device = new MetalDevice(device);
        
        return mtl_device;
    }

}