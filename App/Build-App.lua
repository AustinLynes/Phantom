project "App"
   kind "ConsoleApp"
   language "C++"
   cppdialect "C++20"
   targetdir "Build/%{cfg.buildcfg}"
   staticruntime "off"

   files { "Source/**.h", "Source/**.cpp",   }

   includedirs
   {
      "Source",
	  "../Core/Source",

      "../vendor/",

      "%{IncludeDir.VulkanSDK}",
   }  

   targetdir ("../Build/" .. OutputDir .. "/%{prj.name}")
   objdir ("../Build/" .. OutputDir .. "/%{prj.name}")

   links
   {
        "ImGui",
        
        "%{Library.Vulkan}",
        "Core"
   }

 

   filter "system:windows"
       systemversion "latest"
       defines { "WINDOWS" }

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