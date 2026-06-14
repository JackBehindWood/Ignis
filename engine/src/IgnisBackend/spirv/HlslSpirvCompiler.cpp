#include "igpch.h"
#include "HlslSpirvCompiler.h"

#if !defined(IG_PLATFORM_WINDOWS)
#include <dxc/WinAdapter.h>
#endif
#include <dxc/dxcapi.h>

namespace Ignis
{
namespace
{

// Converts a DXC-provided wide-character path to UTF-8.
// On macOS, wchar_t is 4 bytes (UTF-32). This encodes every valid Unicode codepoint
// without locale dependence or deprecated std::codecvt.
static String wchar_to_utf8(const wchar_t* wstr)
{
    String result;
    while (wstr && *wstr)
    {
        const uint32_t cp = static_cast<uint32_t>(*wstr++);
        if (cp < 0x80)
        {
            result += static_cast<char>(cp);
        }
        else if (cp < 0x800)
        {
            result += static_cast<char>(0xC0 | (cp >> 6));
            result += static_cast<char>(0x80 | (cp & 0x3F));
        }
        else if (cp < 0x10000)
        {
            result += static_cast<char>(0xE0 | (cp >> 12));
            result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            result += static_cast<char>(0x80 | (cp & 0x3F));
        }
        else if (cp < 0x110000)
        {
            result += static_cast<char>(0xF0 | (cp >> 18));
            result += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
            result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            result += static_cast<char>(0x80 | (cp & 0x3F));
        }
        // Codepoints >= 0x110000 are invalid Unicode; skip silently.
    }
    return result;
}

// Stack-allocated COM include handler for DXC.
//
// DXC calls Compile() synchronously and does not retain the handler pointer after the
// call returns. AddRef / Release are therefore stubbed to return a constant — the object
// is never heap-allocated and delete-this is never needed. QueryInterface satisfies DXC's
// internal cast but hands back the same stack pointer, which is safe for the call duration.
class IgnisIncludeHandler : public IDxcIncludeHandler
{
public:
    IgnisIncludeHandler(CComPtr<IDxcUtils> utils, const Vector<Path>& include_dirs,
                        const UnorderedMap<String, String>& virtual_fs)
        : m_utils(std::move(utils)),
          m_include_dirs(include_dirs),
          m_virtual_fs(virtual_fs)
    {
    }

    HRESULT STDMETHODCALLTYPE LoadSource(LPCWSTR filename, IDxcBlob** include_source) override
    {
        *include_source = nullptr;

        String path = wchar_to_utf8(filename);
        if (path.empty())
        {
            return E_INVALIDARG;
        }

        // DXC prepends "./" to relative includes when the source has no filename context.
        if (path.size() >= 2 && path[0] == '.' && (path[1] == '/' || path[1] == '\\'))
        {
            path = path.substr(2);
        }

        // Virtual includes take priority over disk resolution.
        auto vit = m_virtual_fs.find(path);
        if (vit != m_virtual_fs.end())
        {
            CComPtr<IDxcBlobEncoding> blob;
            const String&             src = vit->second;
            HRESULT hr = m_utils->CreateBlob(src.data(), static_cast<UINT32>(src.size()), DXC_CP_UTF8, &blob);
            if (SUCCEEDED(hr))
            {
                *include_source = blob.Detach();
            }
            return hr;
        }

        // Relative disk resolution — first matching directory wins.
        for (const Path& dir : m_include_dirs)
        {
            const Path full_path = dir / path;
            if (Filesystem::exists(full_path))
            {
                BinaryReader reader(full_path);
                if (reader.is_open())
                {
                    const size_t size = reader.get_size();
                    Vector<char> buf(size);
                    reader.read_bytes(buf.data(), size);

                    CComPtr<IDxcBlobEncoding> blob;
                    HRESULT hr = m_utils->CreateBlob(buf.data(), static_cast<UINT32>(size), DXC_CP_UTF8, &blob);
                    if (SUCCEEDED(hr))
                    {
                        *include_source = blob.Detach();
                    }
                    return hr;
                }
            }
        }

        IG_CORE_ERROR("HlslSpirvCompiler: unresolved #include '{}'", path);
        return E_FAIL;
    }

    ULONG STDMETHODCALLTYPE AddRef() override
    {
        return 1;
    }
    ULONG STDMETHODCALLTYPE Release() override
    {
        return 1;
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** object) override
    {
        if (IsEqualIID(riid, __uuidof(IDxcIncludeHandler)) || IsEqualIID(riid, __uuidof(IUnknown)))
        {
            *object = this;
            return S_OK;
        }
        *object = nullptr;
        return E_NOINTERFACE;
    }

private:
    CComPtr<IDxcUtils>                  m_utils;
    const Vector<Path>&                 m_include_dirs;
    const UnorderedMap<String, String>& m_virtual_fs;
};

} // anonymous namespace

// -------------------------------------------------------------------------

void HlslSpirvCompiler::register_virtual_include(const String& virtual_path, const String& source)
{
    m_virtual_includes[virtual_path] = source;
}

void HlslSpirvCompiler::set_include_dirs(const Vector<Path>& dirs)
{
    m_include_dirs = dirs;
}

// -------------------------------------------------------------------------

static const wchar_t* hlsl_target_profile(spv::ExecutionModel exec_model)
{
    switch (exec_model)
    {
        case spv::ExecutionModelVertex:
            return L"vs_6_0";
        case spv::ExecutionModelFragment:
            return L"ps_6_0";
        case spv::ExecutionModelGLCompute:
            return L"cs_6_0";
        default:
            IG_CORE_ASSERT(false, "HlslSpirvCompiler: unsupported execution model");
            return L"vs_6_0";
    }
}

Vector<uint32_t> HlslSpirvCompiler::compile_to_target(const String& source, const char* entry_point,
                                                      spv::ExecutionModel                 exec_model,
                                                      const Vector<Pair<String, String>>& defines)
{
    CComPtr<IDxcUtils>     dxc_utils;
    CComPtr<IDxcCompiler3> dxc_compiler;
    DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxc_utils));
    DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxc_compiler));

    if (!dxc_utils || !dxc_compiler)
    {
        IG_CORE_ERROR("HlslSpirvCompiler: failed to initialise DXC — is libdxcompiler loaded?");
        return {};
    }

    CComPtr<IDxcBlobEncoding> source_blob;
    dxc_utils->CreateBlob(source.data(), static_cast<UINT32>(source.size()), DXC_CP_UTF8, &source_blob);

    const std::wstring wide_entry(entry_point, entry_point + std::strlen(entry_point));

    Vector<std::wstring> define_strs;
    define_strs.reserve(defines.size());
    for (const auto& d : defines)
    {
        std::wstring w(d.first.begin(), d.first.end());
        if (!d.second.empty())
        {
            w += L'=' + std::wstring(d.second.begin(), d.second.end());
        }
        define_strs.push_back(std::move(w));
    }

    Vector<std::wstring> include_dir_strs;
    include_dir_strs.reserve(m_include_dirs.size());
    for (const Path& dir : m_include_dirs)
    {
        include_dir_strs.push_back(std::wstring(dir.wstring()));
    }

    Vector<LPCWSTR> args = {
        L"-spirv", L"-fspv-target-env=vulkan1.1", L"-T", hlsl_target_profile(exec_model), L"-E", wide_entry.c_str(),
        L"-Zpc",   L"-fspv-preserve-interface",
    };
#ifdef IG_DEBUG
    args.push_back(L"-Zi");
    args.push_back(L"-Qembed_debug");
#endif
    for (const auto& d : define_strs)
    {
        args.push_back(L"-D");
        args.push_back(d.c_str());
    }
    for (const auto& dir : include_dir_strs)
    {
        args.push_back(L"-I");
        args.push_back(dir.c_str());
    }

    DxcBuffer source_buf = {source_blob->GetBufferPointer(), source_blob->GetBufferSize(), DXC_CP_ACP};

    IgnisIncludeHandler include_handler(dxc_utils, m_include_dirs, m_virtual_includes);

    CComPtr<IDxcResult> result;
    HRESULT hr = dxc_compiler->Compile(&source_buf, args.data(), static_cast<UINT32>(args.size()), &include_handler,
                                       IID_PPV_ARGS(&result));

    if (FAILED(hr))
    {
        IG_CORE_ERROR("HlslSpirvCompiler: DXC Compile() failed (hr=0x{0:x})", static_cast<uint32_t>(hr));
        return {};
    }

    CComPtr<IDxcBlobUtf8> errors;
    result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
    if (errors && errors->GetStringLength() > 0)
    {
        IG_CORE_ERROR("HlslSpirvCompiler: DXC:\n{0}", errors->GetStringPointer());
    }

    HRESULT status;
    result->GetStatus(&status);

    CComPtr<IDxcBlob> spv;
    result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&spv), nullptr);

#ifdef IG_DEBUG
    if (FAILED(status) && spv && spv->GetBufferSize() > 0)
    {
        DxcBuffer           spv_buf{spv->GetBufferPointer(), spv->GetBufferSize(), DXC_CP_ACP};
        CComPtr<IDxcResult> disasm_result;
        dxc_compiler->Disassemble(&spv_buf, IID_PPV_ARGS(&disasm_result));
        CComPtr<IDxcBlobUtf8> disasm;
        disasm_result->GetOutput(DXC_OUT_DISASSEMBLY, IID_PPV_ARGS(&disasm), nullptr);
        if (disasm && disasm->GetStringLength() > 0)
        {
            IG_CORE_TRACE("HlslSpirvCompiler: SPIR-V disassembly:\n{0}", disasm->GetStringPointer());
        }
    }
#endif

    if (FAILED(status))
    {
        return {};
    }

    if (!spv || spv->GetBufferSize() == 0)
    {
        IG_CORE_ERROR("HlslSpirvCompiler: DXC produced an empty SPIR-V blob");
        return {};
    }

    const auto* data = static_cast<const uint32_t*>(spv->GetBufferPointer());
    return Vector<uint32_t>(data, data + spv->GetBufferSize() / sizeof(uint32_t));
}

} // namespace Ignis
