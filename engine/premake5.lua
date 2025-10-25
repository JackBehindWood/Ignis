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
      "vendor" 
   }

   --links { "Metal", "MetalKit", "Cocoa" }

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
