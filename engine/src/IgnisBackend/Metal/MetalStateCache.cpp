#include "igpch.h"

#include "MetalStateCache.h"
#include <Metal/Metal.hpp>
#include <QuartzCore/CAMetalLayer.hpp>


namespace Ignis
{
    MetalStateCache::MetalStateCache(MetalDevice& device) : m_device(device), m_active_viewport(nullptr), m_render_pass_descriptor(nullptr) {}
    MetalStateCache::~MetalStateCache() {}

    void MetalStateCache::reset()
    {
        m_active_viewport = nullptr;
        if (m_render_pass_descriptor)
        {
            MetalRenderPassDescriptorPool::get().release_descriptor(m_render_pass_descriptor);
            m_render_pass_descriptor = nullptr;
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

        MTL::ClearColor colour = MTL::ClearColor::Make(1, 0, 0, 1);

        CA::MetalDrawable* drawable = m_active_viewport->get_drawable();

        for (uint32_t i = 0; i < max_simultaneous_render_targets; i++)
        {
            MTL::RenderPassColorAttachmentDescriptor* colour_attachment = m_render_pass_descriptor->colorAttachments()->object(i);
            colour_attachment->setClearColor(colour);
            colour_attachment->setLoadAction(MTL::LoadActionClear);
            colour_attachment->setStoreAction(MTL::StoreActionStore);
            colour_attachment->setTexture(drawable->texture());
        }
    }

    MetalRenderPassDescriptorPool::~MetalRenderPassDescriptorPool()
    {
        for (MTL::RenderPassDescriptor* desc : m_descriptors)
        {
            desc->release();
        }
    }

    MTL::RenderPassDescriptor *MetalRenderPassDescriptorPool::create_descriptor()
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


}