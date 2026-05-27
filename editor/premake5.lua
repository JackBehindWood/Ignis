project "IgnisEditor"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
   
    targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
    objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

    pchheader "edpch.h"
    pchsource "src/edpch.cpp"

    files
    {
        "src/**.h",
        "src/**.cpp"
    }

    includedirs
    {
        "%{wks.location}/engine/src",
        "%{wks.location}/engine/vendor",
        "%{wks.location}/engine/generated",
        "%{include_dirs.spdlog}",
        "%{include_dirs.GLFW}",
        "%{include_dirs.yaml_cpp}",
        "src",
        "vendor",
    }

    links { "IgnisEngine", "GLFW", "SPIRV-Cross", "yaml-cpp" }

    defines
    {
        "_CRT_SECURE_NO_WARNINGS",
        "YAML_CPP_STATIC_DEFINE",
    }

    filter "system:macosx"
        --systemversion "latest"

        externalincludedirs { "%{include_dirs.metal}" }

        libdirs { "%{wks.location}/engine/vendor/dxc/lib" }

        linkoptions { "-rpath @executable_path/../../../engine/vendor/dxc/lib" }

        links
        {
            "Foundation.framework",
            "Metal.framework",
            "QuartzCore.framework",
            "AppKit.framework",
            "IoKit.framework",
            "dxcompiler",
        }

    filter "configurations:Debug"
		defines "IG_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "IG_RELEASE"
		runtime "Release"
		optimize "on"

	filter "configurations:Distribution"
		defines "IG_DIST"
		runtime "Release"
		optimize "on"
