#include "igpch.h"
#include "HlslSpirvCompiler.h"

#if !defined(IG_PLATFORM_WINDOWS)
#include <dxc/WinAdapter.h>
#endif
#include <dxc/dxcapi.h>

namespace Ignis
{

static const wchar_t* hlsl_target_profile(GRIShaderStage stage)
{
    switch (stage)
    {
        case GRIShaderStage::Vertex:  return L"vs_6_0";
        case GRIShaderStage::Pixel:   return L"ps_6_0";
        case GRIShaderStage::Compute: return L"cs_6_0";
        default:
            IG_CORE_ASSERT(false, "HlslSpirvCompiler: unsupported shader stage");
            return L"vs_6_0";
    }
}

Vector<uint32_t> HlslSpirvCompiler::compile(const String& source, const char* entry_point, GRIShaderStage stage)
{
    CComPtr<IDxcUtils>     dxc_utils;
    CComPtr<IDxcCompiler3> dxc_compiler;
    DxcCreateInstance(CLSID_DxcUtils,    IID_PPV_ARGS(&dxc_utils));
    DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxc_compiler));

    if (!dxc_utils || !dxc_compiler)
    {
        IG_CORE_ERROR("HlslSpirvCompiler: failed to initialise DXC — is libdxcompiler loaded?");
        return {};
    }

    CComPtr<IDxcBlobEncoding> source_blob;
    dxc_utils->CreateBlob(source.data(), static_cast<UINT32>(source.size()), DXC_CP_UTF8, &source_blob);

    // DXC requires wide strings; entry point names are always ASCII so the conversion is safe.
    const std::wstring wide_entry(entry_point, entry_point + std::strlen(entry_point));

    LPCWSTR args[] = {
        L"-spirv",
        L"-fspv-target-env=vulkan1.1",
        L"-T", hlsl_target_profile(stage),
        L"-E", wide_entry.c_str(),
        L"-Zpc",
    };

    DxcBuffer source_buf = { source_blob->GetBufferPointer(), source_blob->GetBufferSize(), DXC_CP_ACP };

    CComPtr<IDxcResult> result;
    HRESULT hr = dxc_compiler->Compile(
        &source_buf,
        args, static_cast<UINT32>(sizeof(args) / sizeof(*args)),
        nullptr,
        IID_PPV_ARGS(&result));

    if (FAILED(hr))
    {
        IG_CORE_ERROR("HlslSpirvCompiler: DXC Compile() failed (hr=0x{0:x})", static_cast<uint32_t>(hr));
        return {};
    }

    CComPtr<IDxcBlobUtf8> errors;
    result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
    if (errors && errors->GetStringLength() > 0)
        IG_CORE_ERROR("HlslSpirvCompiler: DXC:\n{0}", errors->GetStringPointer());

    HRESULT status;
    result->GetStatus(&status);
    if (FAILED(status))
        return {};

    CComPtr<IDxcBlob> spv;
    result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&spv), nullptr);
    if (!spv || spv->GetBufferSize() == 0)
    {
        IG_CORE_ERROR("HlslSpirvCompiler: DXC produced an empty SPIR-V blob");
        return {};
    }

    const auto* data = static_cast<const uint32_t*>(spv->GetBufferPointer());
    return Vector<uint32_t>(data, data + spv->GetBufferSize() / sizeof(uint32_t));
}

} // namespace Ignis
