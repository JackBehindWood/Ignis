project "IgnisEngine"
   kind "StaticLib"
   language "C++"
   cppdialect "C++20"
   
   targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

   pchheader "igpch.h"
	pchsource "src/igpch.cpp"

   files 
   { 
      "src/**.h", 
      "src/**.cpp" 
   }

   includedirs 
   { 
      "src", 
      "vendor",

      "%{include_dirs.spdlog}",
      "%{include_dirs.GLFW}",

   }

   links
   {
      "GLFW",
   }

   defines
   {
      "_CRT_SECURE_NO_WARNINGS",
   }

   filter "system:macosx"
      --systemversion "latest"

      externalincludedirs { "%{include_dirs.metal}" }


      links
      {
         "Foundation.framework",
         "Metal.framework",
         "QuartzCore.framework",
         "AppKit.framework",
         "IoKit.framework",
      }
      files { "src/IgnisBackend/**.mm" }  -- ensure these are included
      buildoptions { "-fobjc-arc" }  -- optional if using ARC

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
