-- Externals


VULKAN_SDK = os.getenv("VULKAN_SDK")

IncludeDir = {}
IncludeDir["VulkanSDK"] = "%{VULKAN_SDK}/Include"

LibraryDir = {}
LibraryDir["VulkanSDK"] = "%{VULKAN_SDK}/Lib"

Library = {}
Library["Vulkan"] = "%{LibraryDir.VulkanSDK}/vulkan-1.lib"


group "dependencies"
    include "Vendor/imgui"
    include "Vendor/glfw"
    include "Vendor/stb"
    include "Vendor/glm"
group ""


group "Core"
    include "Core/Build-Core.lua"
group ""