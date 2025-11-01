#pragma once

#include "MetalCommandQueue.h"

namespace MTL
{
    class Device;
}

namespace Ignis
{
    class MetalDevice
    {
    private:
        MTL::Device* m_device;
        MetalCommandQueue* m_command_queues[static_cast<size_t>(MetalQueueType::_Count)];

    public:
        MetalDevice(MTL::Device* device);
        ~MetalDevice();
        
        inline MTL::Device* get_device() const { return m_device; }

        inline MetalCommandQueue& get_queue(MetalQueueType type) 
        { 
            return *m_command_queues[static_cast<size_t>(type)]; 
        }

        inline MetalCommandQueue& graphics_queue() { return get_queue(MetalQueueType::Graphics); }
        inline MetalCommandQueue& compute_queue()  { return get_queue(MetalQueueType::Compute); }
        inline MetalCommandQueue& transfer_queue() { return get_queue(MetalQueueType::Transfer); }

        static MetalDevice* create_device();
    };

}