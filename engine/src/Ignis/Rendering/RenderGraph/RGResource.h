#pragma once

#include <Ignis/Rendering/GRI/GRIDefinitions.h>
#include <Ignis/Rendering/GRI/GRIResource.h>

namespace Ignis
{

inline constexpr uint16_t k_rg_invalid_id = 0xFFFF;

// --- Texture handle ---

struct RGTextureHandle
{
    uint16_t id = k_rg_invalid_id;

    constexpr bool is_valid()                    const { return id != k_rg_invalid_id; }
    constexpr bool operator==(RGTextureHandle o) const { return id == o.id; }
    constexpr bool operator!=(RGTextureHandle o) const { return id != o.id; }
};

struct RGTextureDesc
{
    uint32_t       width          = 0;
    uint32_t       height         = 0;
    GRIPixelFormat format         = GRIPixelFormat::RGBA8Unorm;
    uint32_t       num_mip_levels = 1;
};

struct RGColorAttachmentDesc
{
    GRILoadAction  load_action  = GRILoadAction::Clear;
    GRIStoreAction store_action = GRIStoreAction::Store;
    GRIClearValue  clear_value  = { 0.f, 0.f, 0.f, 1.f };

    static RGColorAttachmentDesc clear(GRIClearValue cv = { 0.f, 0.f, 0.f, 1.f })
    {
        return { GRILoadAction::Clear, GRIStoreAction::Store, cv };
    }
    static RGColorAttachmentDesc load()
    {
        return { GRILoadAction::Load, GRIStoreAction::Store, {} };
    }
};

struct RGDepthAttachmentDesc
{
    GRILoadAction  load_action  = GRILoadAction::Clear;
    GRIStoreAction store_action = GRIStoreAction::DontCare;
    float          clear_depth  = 1.0f;

    static RGDepthAttachmentDesc clear(float d = 1.f)
    {
        return { GRILoadAction::Clear, GRIStoreAction::DontCare, d };
    }
};

// --- Buffer handle ---

struct RGBufferHandle
{
    uint16_t id = k_rg_invalid_id;

    constexpr bool is_valid()                   const { return id != k_rg_invalid_id; }
    constexpr bool operator==(RGBufferHandle o) const { return id == o.id; }
    constexpr bool operator!=(RGBufferHandle o) const { return id != o.id; }
};

struct RGBufferDesc
{
    uint32_t       size  = 0;
    GRIBufferUsage usage = GRIBufferUsage::UniformBuffer;
};

namespace RGInternal
{

struct AttachmentSlot
{
    uint16_t       texture_id   = k_rg_invalid_id;
    GRILoadAction  load_action  = GRILoadAction::Clear;
    GRIStoreAction store_action = GRIStoreAction::Store;
    GRIClearValue  clear_value  = {};
    float          clear_depth  = 1.f;
};

struct VirtualTexture
{
    const char*   name           = nullptr;
    RGTextureDesc desc           = {};
    GRITexture2D* physical       = nullptr;
    bool          is_imported    = false;
    uint16_t      ref_count      = 0;
    uint16_t      writer_pass_idx  = k_rg_invalid_id;
    uint16_t      first_used_pass  = k_rg_invalid_id;
    uint16_t      last_used_pass   = k_rg_invalid_id;
};

struct VirtualBuffer
{
    const char*   name           = nullptr;
    RGBufferDesc  desc           = {};
    GRIBuffer*    physical       = nullptr;
    bool          is_imported    = false;
    uint16_t      ref_count      = 0;
    uint16_t      writer_pass_idx  = k_rg_invalid_id;
    uint16_t      first_used_pass  = k_rg_invalid_id;
    uint16_t      last_used_pass   = k_rg_invalid_id;
};

} // namespace RGInternal
} // namespace Ignis
