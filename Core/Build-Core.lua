project "Core"
   kind "StaticLib"
   language "C++"
   cppdialect "C++20"
   targetdir "Build/%{cfg.buildcfg}"
   staticruntime "off"

   files { "Source/**.h", "Source/**.cpp",  "Source/**.impl.h" }

   includedirs
   {
      "Source",

      "../vendor/",
      "../vendor/glfw/include/",
      "../vendor/DirectXTK12/inc/",
      "../vendor/DirectX-Headers/include/",

      "%{IncludeDir.VulkanSDK}",
   }
 
    links
    {
        
        "GLFW",
        "ImGui",
        "../Build/" .. OutputDir .. "/DirectXTK12/DirectXTK12",
        
        "%{Library.Vulkan}",
    }
   targetdir ("../Build/" .. OutputDir .. "/%{prj.name}")
   objdir ("../Build/" .. OutputDir .. "/%{prj.name}")

   filter "system:windows"
       systemversion "latest"
       defines { }

   filter "configurations:Debug"
       defines { "DEBUG" }
       runtime "Debug"
       symbols "On"

   filter "configurations:Release"
       defines { "RELEASE" }
       runtime "Release"
       optimize "On"
       symbols "On"

   filter "configurations:Dist"
       defines { "DIST" }
       runtime "Release"
       optimize "On"
       symbols "Off"