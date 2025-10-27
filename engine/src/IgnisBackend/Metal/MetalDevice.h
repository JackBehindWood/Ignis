#pragma once

namespace MTL
{
    class Device;
}

namespace Ignis
{
    class MetalCommandQueue;
    class MetalDevice
    {
        static constexpr int s_device_index = 0;
    private:
        MTL::Device* m_device;
        MetalCommandQueue* m_command_queue;
        
        MetalDevice(MTL::Device* device);
    public:
        ~MetalDevice();
        
        inline MTL::Device* get_device() { return m_device; }
        inline MetalCommandQueue& get_queue() { return *m_command_queue; }
        
        static MetalDevice* create_device();
    };

}