project "IgnisEditor"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
   
    targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
    objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

    files 
    {
        "src/**.h", 
        "src/**.cpp"
    }

    includedirs 
    { 
        "%{wks.location}/engine/src",
        "%{wks.location}/engine/vendor",
        "%{wks.location}/engine/vendor/spdlog/include",
        "src",
        "vendor",
    }

    links { "IgnisEngine" } -- --"Metal", "MetalKit", "Cocoa" }

    filter "system:macos"
        systemversion "latest"

        includedirs
        {
           "%{wks.location}/engine/vendor/metal-cpp"
        }

        links { "Metal", "MetalKit", "Cocoa" }

    filter "configurations:Debug"
		defines "IG_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "IG_RELEASE"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines "IG_DIST"
		runtime "Release"
		optimize "on"
