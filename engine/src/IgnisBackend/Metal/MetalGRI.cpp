#include "igpch.h"
#include "MetalGRI.h"
#include "MetalDevice.h"
#include "MetalResource.h"

#include "MetalShaderLibrary.h"

#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_COCOA
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"

namespace Ignis
{
// ---------- MetalGRI ----------

MetalGRI::MetalGRI()
    : m_device(MetalDevice::create_device()),
      m_context(*m_device)
{
}

void MetalGRI::init()
{
    MTL_AUTORELEASE_POOL;
    IG_CORE_ASSERT(glfwInit(), "Failed to initialize GLFW");
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    MetalShaderLibrary& m_shader_library = MetalShaderLibrary::get();
    m_shader_library.init(m_device);
    m_shader_library.reset();
}

void MetalGRI::shutdown()
{
    delete m_device;
    m_device = nullptr;
}

GRIBufferPtr MetalGRI::create_buffer(const GRIBufferDesc& desc, const void* initial_data)
{
    IG_CORE_ASSERT(desc.size > 0, "Buffer size must be greater than zero");

    MTL::Buffer* buffer =
        initial_data ? m_device->get_device()->newBuffer(initial_data, desc.size, MTL::ResourceStorageModeShared)
                     : m_device->get_device()->newBuffer(desc.size, MTL::ResourceStorageModeShared);

    return create_shared<MetalBuffer>(buffer, desc.size);
}

// TODO: use our resource transfer command buffer!
void MetalGRI::update_buffer(GRIBuffer* buffer, const void* data, uint32_t size, uint32_t offset)
{
    MetalBuffer* mb = resource_cast<GRIBuffer>(buffer);
    IG_CORE_ASSERT(mb->get_buffer()->contents(), "update_buffer: buffer not CPU-accessible (not StorageModeShared)");
    IG_CORE_ASSERT(offset + size <= mb->get_size(), "update_buffer: write range exceeds buffer size");
    memcpy(static_cast<uint8_t*>(mb->get_buffer()->contents()) + offset, data, size);
    // On Apple Silicon (StorageModeShared) no flush is needed.
    // Intel / StorageModeManaged would require:
    // mb->get_buffer()->didModifyRange(NS::Range::Make(offset, size));
}
void MetalGRI::invalidate_compiled_shader(uint64_t bytecode_hash)
{
    MetalShaderLibrary::get().invalidate(bytecode_hash);
}

void MetalGRI::init_bindless_array(GRIShader* ps)
{
    auto* mps = static_cast<MetalPixelShader*>(ps);
    m_context.init_bindless_array(mps->get_function());
}

uint32_t MetalGRI::register_bindless_texture(GRITexture2DPtr texture, GRISamplerStatePtr sampler)
{
    return m_context.register_bindless_texture(std::move(texture), std::move(sampler));
}

} // namespace Ignis
