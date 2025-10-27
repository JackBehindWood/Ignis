#pragma once

namespace MTL
{
    class CommandQueue;
}

namespace Ignis
{
    class MetalDevice;

    class MetalCommandQueue
    {
    private:
        MetalDevice& m_device;
        MTL::CommandQueue* m_command_queue;
    public:
        MetalCommandQueue(MetalDevice& device);
        ~MetalCommandQueue() = default;
        
        inline MetalDevice& get_device() {return m_device;}
        inline MTL::CommandQueue* get_queue() {return m_command_queue;}
    };
}

