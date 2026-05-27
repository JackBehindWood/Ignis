#include "igpch.h"

#include "MetalStateCache.h"
#include <Metal/Metal.hpp>
#include <QuartzCore/CAMetalLayer.hpp>

namespace Ignis
{
MetalStateCache::MetalStateCache(MetalDevice& device)
    : m_device(device),
      m_render_pass_descriptor(nullptr),
      m_index_buffer(nullptr),
      m_index_type(MTL::IndexTypeUInt16),
      m_index_buffer_offset(0),
      m_primitive_type(MTL::PrimitiveTypeTriangle)
{
}

MetalStateCache::~MetalStateCache()
{
}

void MetalStateCache::reset()
{
    if (m_render_pass_descriptor)
    {
        MetalRenderPassDescriptorPool::get().release_descriptor(m_render_pass_descriptor);
        m_render_pass_descriptor = nullptr;
    }

    m_index_buffer        = nullptr;
    m_index_type          = MTL::IndexTypeUInt16;
    m_index_buffer_offset = 0;
    m_primitive_type      = MTL::PrimitiveTypeTriangle;
    m_pending_pass_info   = GRIRenderPassInfo{};
}

void MetalStateCache::set_pending_pass_info(const GRIRenderPassInfo& info)
{
    m_pending_pass_info = info;
}

void MetalStateCache::set_index_buffer(MTL::Buffer* buffer, MTL::IndexType type, NS::UInteger offset)
{
    m_index_buffer        = buffer;
    m_index_type          = type;
    m_index_buffer_offset = offset;
}

void MetalStateCache::set_primitive_type(MTL::PrimitiveType type)
{
    m_primitive_type = type;
}

static MTL::LoadAction to_metal_load_action(GRILoadAction action)
{
    switch (action)
    {
        case GRILoadAction::Load:
            return MTL::LoadActionLoad;
        case GRILoadAction::Clear:
            return MTL::LoadActionClear;
        case GRILoadAction::DontCare:
            return MTL::LoadActionDontCare;
        default:
            return MTL::LoadActionDontCare;
    }
}

static MTL::StoreAction to_metal_store_action(GRIStoreAction action)
{
    switch (action)
    {
        case GRIStoreAction::Store:
            return MTL::StoreActionStore;
        case GRIStoreAction::DontCare:
            return MTL::StoreActionDontCare;
        default:
            return MTL::StoreActionDontCare;
    }
}

void MetalStateCache::set_render_pass_info(const GRIRenderPassInfo& info)
{
    if (m_render_pass_descriptor)
    {
        MetalRenderPassDescriptorPool::get().release_descriptor(m_render_pass_descriptor);
        m_render_pass_descriptor = nullptr;
    }

    m_render_pass_descriptor = MetalRenderPassDescriptorPool::get().create_descriptor();
    m_render_pass_descriptor->setVisibilityResultBuffer(nullptr);

    for (uint32_t i = 0; i < info.get_num_colour_targets(); i++)
    {
        const GRIRenderPassInfo::ColourEntry&     entry   = info.colour_targets[i];
        MTL::Texture*                             texture = (MTL::Texture*)entry.render_target->get_native_handle();
        MTL::RenderPassColorAttachmentDescriptor* colour_attachment =
            m_render_pass_descriptor->colorAttachments()->object(i);
        colour_attachment->setTexture(texture);
        colour_attachment->setLoadAction(to_metal_load_action(entry.load_action));
        colour_attachment->setStoreAction(to_metal_store_action(entry.store_action));
        colour_attachment->setClearColor(
            MTL::ClearColor::Make(entry.clear_value.r, entry.clear_value.g, entry.clear_value.b, entry.clear_value.a));
    }

    if (info.depth_stencil_target.depth_stencil_target)
    {
        const GRIRenderPassInfo::DepthEntry& depth = info.depth_stencil_target;
        MTL::Texture* depth_texture                = (MTL::Texture*)depth.depth_stencil_target->get_native_handle();
        MTL::RenderPassDepthAttachmentDescriptor* depth_attachment = m_render_pass_descriptor->depthAttachment();
        depth_attachment->setTexture(depth_texture);
        depth_attachment->setLoadAction(to_metal_load_action(depth.load_action));
        depth_attachment->setStoreAction(to_metal_store_action(depth.store_action));
        depth_attachment->setClearDepth(depth.clear_depth);
    }
}

MetalRenderPassDescriptorPool::~MetalRenderPassDescriptorPool()
{
    for (MTL::RenderPassDescriptor* desc : m_descriptors)
    {
        desc->release();
    }
}

MTL::RenderPassDescriptor* MetalRenderPassDescriptorPool::create_descriptor()
{
    MTL::RenderPassDescriptor* desc = nullptr;
    if (!m_descriptors.empty())
    {
        desc = m_descriptors.back();
        m_descriptors.pop_back();
    }

    if (!desc)
    {
        desc = MTL::RenderPassDescriptor::alloc()->init();
        IG_CORE_ASSERT(desc, "Failed to create render pass descriptor");
    }
    return desc;
}

void MetalRenderPassDescriptorPool::release_descriptor(MTL::RenderPassDescriptor* descriptor)
{
    MTL::RenderPassColorAttachmentDescriptorArray* attachments = descriptor->colorAttachments();
    for (uint32_t i = 0; i < max_simultaneous_render_targets; i++)
    {
        MTL::RenderPassColorAttachmentDescriptor* colour = attachments->object(i);
        colour->setTexture(nullptr);
        colour->setResolveTexture(nullptr);
        colour->setStoreAction(MTL::StoreActionStore);
    }

    MTL::RenderPassDepthAttachmentDescriptor* depth = descriptor->depthAttachment();
    depth->setTexture(nullptr);
    depth->setResolveTexture(nullptr);
    depth->setStoreAction(MTL::StoreActionStore);

    MTL::RenderPassStencilAttachmentDescriptor* stencil = descriptor->stencilAttachment();
    stencil->setTexture(nullptr);
    stencil->setResolveTexture(nullptr);
    stencil->setStoreAction(MTL::StoreActionStore);

    descriptor->setVisibilityResultBuffer(nullptr);

    descriptor->setRenderTargetArrayLength(1);

    m_descriptors.push_back(descriptor);
}

} // namespace Ignis