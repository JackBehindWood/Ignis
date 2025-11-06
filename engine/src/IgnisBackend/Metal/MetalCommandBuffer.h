
#include <Metal/Metal.hpp>

namespace Ignis
{
    class MetalCommandBuffer
    {
    private:
        MTL::CommandBuffer* m_command_buffer;
    public:
        MetalCommandBuffer(MTL::CommandBuffer* command_buffer) : m_command_buffer(command_buffer) 
        {
            m_command_buffer->retain();
        }

        ~MetalCommandBuffer()
        {
            m_command_buffer->release();
        }

        inline MTL::CommandBuffer* get_buffer() { return m_command_buffer; }

    };
}