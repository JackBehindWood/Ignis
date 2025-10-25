workspace "Ignis"
   configurations { "Debug", "Release", "Distribution" }
   architecture "arm64"
   startproject "IgnisEditor"

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Include engine and editor premake files
include "engine"
include "editor"
