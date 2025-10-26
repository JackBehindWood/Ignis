#pragma once

namespace MTL
{
    class Device;
}

namespace Ignis
{
    class MetalDevice
    {
        static constexpr int s_device_index = 0;
    private:
        MTL::Device* m_device;
        
        MetalDevice(MTL::Device* device);
    public:
        ~MetalDevice();
        
        inline MTL::Device* get_device() { return m_device; }
        
        static MetalDevice* create_device();
    };

}