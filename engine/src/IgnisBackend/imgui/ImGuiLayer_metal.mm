#ifdef ENGINE_IMGUI

#include "igpch.h"
#include "Ignis/UI/ImGuiLayer.h"
#include "Ignis/Core/Application.h"
#include "Ignis/Rendering/RenderSystem.h"

#include "IgnisBackend/Metal/MetalAutoReleasePool.h"
#include "IgnisBackend/Metal/MetalGRI.h"
#include "IgnisBackend/Metal/MetalResource.h"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_metal.h>

#include <QuartzCore/CAMetalLayer.hpp>
#include <QuartzCore/CAMetalDrawable.hpp>

namespace Ignis
{

void ImGuiLayer::attach()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.IniFilename = nullptr;

    apply_dark_theme();

    io.Fonts->AddFontFromFileTTF("/System/Library/Fonts/Helvetica.ttc", 13.0f);
    s_mono_font = io.Fonts->AddFontFromFileTTF("/System/Library/Fonts/Menlo.ttc", 12.0f);
    IG_CORE_ASSERT(s_mono_font, "Menlo.ttc not found — monospace font unavailable");

    MetalGRI*      gri = static_cast<MetalGRI*>(RenderSystem::get_gri());
    MetalViewport* vp  = static_cast<MetalViewport*>(Application::get().get_window().get_viewport());

    ImGui_ImplGlfw_InitForOther(vp->get_window(), true);
    ImGui_ImplMetal_Init((__bridge id<MTLDevice>)(gri->get_device()->get_device()));
}

void ImGuiLayer::detach()
{
    ImGui_ImplMetal_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiLayer::render()
{
    MetalGRI*      gri = static_cast<MetalGRI*>(RenderSystem::get_gri());
    MetalViewport* vp  = static_cast<MetalViewport*>(Application::get().get_window().get_viewport());

    CA::MetalDrawable* drawable = vp->get_metal_layer()->nextDrawable();
    if (!drawable)
    {
        return;
    }

    MTL::Texture* drawable_tex = drawable->texture();

    MTL_AUTORELEASE_POOL
    {
        MTL::RenderPassDescriptor* rpd = MTL::RenderPassDescriptor::renderPassDescriptor();

        auto colorAttachment = rpd->colorAttachments()->object(0);
        colorAttachment->setTexture(drawable_tex);
        colorAttachment->setLoadAction(MTL::LoadActionClear);
        colorAttachment->setStoreAction(MTL::StoreActionStore);
        colorAttachment->setClearColor(MTL::ClearColor::Make(0.08, 0.08, 0.08, 1.0));

        ImGui_ImplMetal_NewFrame((__bridge MTLRenderPassDescriptor*)(rpd));
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        for (IImGuiDrawable* d : s_drawables)
        {
            d->draw_imgui();
        }

        ImGui::Render();

        MTL::CommandQueue*  queue  = static_cast<MTL::CommandQueue*>(gri->get_device()->graphics_queue().get_queue());
        MTL::CommandBuffer* cmdBuf = queue->commandBuffer();

        NS::String* cmdBufLabel = NS::String::string("ImGuiCommandBuffer", NS::UTF8StringEncoding);
        cmdBuf->setLabel(cmdBufLabel);

        MTL::RenderCommandEncoder* encoder      = cmdBuf->renderCommandEncoder(rpd);
        NS::String*                encoderLabel = NS::String::string("ImGuiEncoder", NS::UTF8StringEncoding);
        encoder->setLabel(encoderLabel);

        ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), (__bridge id<MTLCommandBuffer>)(cmdBuf),
                                       (__bridge id<MTLRenderCommandEncoder>)(encoder));

        encoder->endEncoding();
        cmdBuf->presentDrawable(drawable);
        cmdBuf->commit();
    }

    drawable->release();

    // Deferred resize: flush AFTER the command buffer commits so that EditorLayer
    // has already rendered to the old RT this frame. On the next frame, EditorLayer
    // renders to the new RT and ImGui displays it — no frame with undefined content.
    for (IImGuiDrawable* d : s_drawables)
    {
        if (d->has_pending_resize())
        {
            d->flush_resize();
        }
    }
}

} // namespace Ignis

#endif
