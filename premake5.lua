workspace "Ignis"
   configurations { "Debug", "Release", "Distribution" }
   architecture "ARM64"
   flags { "MultiProcessorCompile"}
   startproject "IgnisEditor"

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"


-- Include directories
include_dirs = {}
include_dirs["GLFW"] = "%{wks.location}/engine/vendor/GLFW/include"
include_dirs["spdlog"] = "%{wks.location}/engine/vendor/spdlog/include"
include_dirs["metal"] = "%{wks.location}/engine/vendor/metal-cpp"

group "Dependencies"
   include "engine/vendor/GLFW"
group ""

-- Include engine and editor premake files
include "engine"
include "editor"
