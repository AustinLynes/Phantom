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

      "../vendor/imgui",
      "../vendor/stb",
      "../vendor/glm",
      
      "%{IncludeDir.VulkanSDK}",
   }  

   links
   {
        "ImGui",
        "%{Library.Vulkan}",
        "Core"
   }

   targetdir ("../Build/" .. OutputDir .. "/%{prj.name}")
   objdir ("../Build/Intermediates/" .. OutputDir .. "/%{prj.name}")

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