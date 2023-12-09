#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "vulkan/vulkan.h"

#include "glm/glm.hpp"

#include "stb_image.h"
#include <vector>
#include <functional>
#include <memory>

namespace Core {

	// Initilize Rendering System.. 
	// Vulkan and IMGUI require quite a bit of data to function properly.

#ifdef _DEBUG
#define DEBUG_VULKAN_IMGUI_INTERFACE
#endif

	// vulkan resources.
	static VkAllocationCallbacks* g_Allacator = NULL;
	static VkInstance g_instance = VK_NULL_HANDLE;
	static VkPhysicalDevice g_physicalDevice = VK_NULL_HANDLE;
	static VkDevice g_device = VK_NULL_HANDLE;
	static uint32_t g_queueFamily = (uint32_t)-1;
	static VkQueue g_queue = VK_NULL_HANDLE;
	static VkDebugReportCallbackEXT g_debugReport = VK_NULL_HANDLE;
	static VkPipelineCache g_pipelineCache = VK_NULL_HANDLE;
	static VkDescriptorPool g_descriptorPool = VK_NULL_HANDLE;

	static ImGui_ImplVulkanH_Window g_mainWindowData;
	static int g_minImagesCount = 2;
	static bool g_swapchainRebuild = false;

	static std::vector<std::vector<VkCommandBuffer>> s_allocatedCommandBuffers;
	static std::vector<std::vector<std::function<void()>>> s_resourceFreeQueue;

	static uint32_t s_currentFrameIndex = 0;
	Application* instance = nullptr;

	void check_vk_result(VkResult error) {
		if (error == VK_SUCCESS)
			return;

		fprintf_s(stderr, "[vulkan] Error: vkResult: %d\n", error);

		if (error < 0)
			abort();
	}

	void glfw_error_callback(int error_code, const char* description) {
		fprintf(stderr, "[glfw] error code: %d description: %s", error_code, description);
	}

#ifdef DEBUG_VULKAN_IMGUI_INTERFACE
	static VKAPI_ATTR VkBool32 VKAPI_CALL debug_report(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData)
	{
		(void)flags; (void)object; (void)location; (void)messageCode; (void)pUserData; (void)pLayerPrefix; // Unused arguments
		fprintf(stderr, "[vulkan] Debug report from ObjectType: %i\nMessage: %s\n\n", objectType, pMessage);
		return VK_FALSE;
	}
#endif // DEBUG_VULKAN_IMGUI_INTERFACE

	static void SetupVulkan(const char** extensions, uint32_t extensionsCount) {
		VkResult error;

		// Create Instance
		{
			VkInstanceCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			info.enabledExtensionCount = extensionsCount;
			info.ppEnabledExtensionNames = extensions;
#ifdef DEBUG_VULKAN_IMGUI_INTERFACE
			// enable validation layers
			const char* layers[]{ "VK_LAYER_KHRONOS_validation" };
			info.ppEnabledLayerNames = layers;
			info.enabledLayerCount = _countof(layers);
			// enable debug reporting
			const char** extensions_ext = (const char**)malloc(sizeof(const char*) * (extensionsCount + 1));

			if (extensions_ext != 0) {
				try {
					memcpy(extensions_ext, extensions, extensionsCount * sizeof(const char*));
					extensions_ext[extensionsCount] = "VK_EXT_debug_report";
					info.enabledExtensionCount = extensionsCount + 1;
					info.ppEnabledExtensionNames = extensions_ext;
				}
				catch (std::exception err) {
					fprintf(stderr, "[fatal-memory] memory could not be copied, hint: %s", err.what());
				}
			}

			// create instance 
			error = vkCreateInstance(&info, g_Allacator, &g_instance);
			check_vk_result(error);
			free(extensions_ext);

			// Find function pointer required and load it.
			auto vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(g_instance, "vkCreateDebugReportCallbackEXT");
			IM_ASSERT(vkCreateDebugReportCallbackEXT != NULL);

			VkDebugReportCallbackCreateInfoEXT debugInfo{};
			debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
			debugInfo.pfnCallback = debug_report;
			debugInfo.pUserData = NULL;
			debugInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;

			error = vkCreateDebugReportCallbackEXT(g_instance, &debugInfo, g_Allacator, &g_debugReport);
			check_vk_result(error);
#else

			error = vkCreateInstance(&info, g_Allacator, &g_instance);
			checK_vk_result(error);
			IM_UNUSED(g_debugReport);
#endif // DEBUG_VULKAN_IMGUI_INTERFACE


		}
		// Select GPU
		{
			uint32_t gpuCount;
			error = vkEnumeratePhysicalDevices(g_instance, &gpuCount, nullptr);
			check_vk_result(error);
			IM_ASSERT(gpuCount > 0);


			try {

				VkPhysicalDevice* gpus = (VkPhysicalDevice*)malloc(sizeof(VkPhysicalDevice) * gpuCount);
				error = vkEnumeratePhysicalDevices(g_instance, &gpuCount, gpus);
				check_vk_result(error);

				if (gpus != nullptr) {

					int target = 0;

					for (int i = 0; i < gpuCount; i++)
					{
						VkPhysicalDeviceProperties props;
						vkGetPhysicalDeviceProperties(gpus[i], &props);
						if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
							target = i;
							break;
						}
					}

					g_physicalDevice = gpus[target];

					free(gpus);
				}

			}
			catch (std::runtime_error err) {
				fprintf(stderr, "[memerr] could not allocate memory for gpus, : %s", err.what());
			}
		}
		// Select Graphics Queue Family
		{
			uint32_t count;
			vkGetPhysicalDeviceQueueFamilyProperties(g_physicalDevice, &count, NULL);
			try {
				VkQueueFamilyProperties* properties = (VkQueueFamilyProperties*)malloc(sizeof(VkQueueFamilyProperties) * count);
				if (properties != nullptr) {

					for (size_t i = 0; i < count; i++)
					{
						if (properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
							g_queueFamily = i;
							break;
						}
					}

				}
				free(properties);
			}
			catch (std::runtime_error err) {
				fprintf(stderr, "[memerr] could not allocate memory!");
				return;
			}

			IM_ASSERT(g_queueFamily != (uint32_t)-1);

		}
		// Create Logical Device (with 1 queue)
		{
			const char* deviceExtensions[] = { "VK_KHR_swapchain" };
			int extensionCount = _countof(deviceExtensions);

			const float queue_priorities[] = { 1.0f };

			VkDeviceQueueCreateInfo queueInfo[1]{};
			queueInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueInfo[0].queueFamilyIndex = g_queueFamily;
			queueInfo[0].queueCount = 1;
			queueInfo[0].pQueuePriorities = queue_priorities;

			VkDeviceCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			createInfo.queueCreateInfoCount = sizeof(queueInfo) / sizeof(queueInfo[0]);
			createInfo.pQueueCreateInfos = queueInfo;
			createInfo.enabledExtensionCount = extensionCount;
			createInfo.ppEnabledExtensionNames = deviceExtensions;

			error = vkCreateDevice(g_physicalDevice, &createInfo, g_Allacator, &g_device);
			check_vk_result(error);
			vkGetDeviceQueue(g_device, g_queueFamily, 0, &g_queue);

		}
		// Create Descriptor Pool
		{
			VkDescriptorPoolSize pool_sizes[] =
			{
				{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
				{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
				{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
			};
			VkDescriptorPoolCreateInfo pool_info = {};
			pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
			pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
			pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
			pool_info.pPoolSizes = pool_sizes;
			error = vkCreateDescriptorPool(g_device, &pool_info, g_Allacator, &g_descriptorPool);
			check_vk_result(error);
		}


	}

	static void SetupVulkanWindow(ImGui_ImplVulkanH_Window* wd, VkSurfaceKHR surface, int width, int height) {
		wd->Surface = surface;

		VkBool32 res;
		vkGetPhysicalDeviceSurfaceSupportKHR(g_physicalDevice, g_queueFamily, wd->Surface, &res);
		if (res != VK_TRUE) {
			fprintf(stderr, "[fatal] Error no WSI support of Physical Device");
			exit(-1);
		}

		const VkFormat requestSurfaceImageFormat[] = { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM };
		const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
		wd->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(g_physicalDevice, wd->Surface, requestSurfaceImageFormat, _countof(requestSurfaceImageFormat), requestSurfaceColorSpace);


#ifdef IMGUI_UNLIMITED_FRAMERATE 
		VkPresentModeKHR presentModes[] = { VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR };
#else
		VkPresentModeKHR presentModes[] = { VK_PRESENT_MODE_FIFO_KHR };
#endif
		wd->PresentMode = ImGui_ImplVulkanH_SelectPresentMode(g_physicalDevice, wd->Surface, &presentModes[0], _countof(presentModes));

		IM_ASSERT(g_minImagesCount >= 2);

		ImGui_ImplVulkanH_CreateOrResizeWindow(g_instance, g_physicalDevice, g_device, wd, g_queueFamily, g_Allacator, width, height, g_minImagesCount);

	}

	static void CleanupVulkan() {
		vkDestroyDescriptorPool(g_device, g_descriptorPool, g_Allacator);
#ifdef DEBUG_VULKAN_IMGUI_INTERFACE
		auto vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(g_instance, "vkDestroyDebugReportCallbackEXT");
		vkDestroyDebugReportCallbackEXT(g_instance, g_debugReport, g_Allacator);
#endif

		vkDestroyDevice(g_device, g_Allacator);
		vkDestroyInstance(g_instance, g_Allacator);
	}

	static void CleanupVulkanWindow() {
		ImGui_ImplVulkanH_DestroyWindow(g_instance, g_device, &g_mainWindowData, g_Allacator);
	}

	static void FrameRender(ImGui_ImplVulkanH_Window* wd, ImDrawData* drawData) {
		VkResult error;

		VkSemaphore	imageAquiredSemaphore = wd->FrameSemaphores[wd->SemaphoreIndex].ImageAcquiredSemaphore;
		VkSemaphore	renderCompleteSemaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
		error = vkAcquireNextImageKHR(g_device, wd->Swapchain, UINT64_MAX, imageAquiredSemaphore, VK_NULL_HANDLE, &wd->FrameIndex);
		if (error == VK_ERROR_OUT_OF_DATE_KHR || error == VK_SUBOPTIMAL_KHR) {
			g_swapchainRebuild = true;
			return;
		}
		check_vk_result(error);

		s_currentFrameIndex = (s_currentFrameIndex + 1) % g_mainWindowData.ImageCount;

		ImGui_ImplVulkanH_Frame* fd = &wd->Frames[wd->FrameIndex];
		{
			error = vkWaitForFences(g_device, 1, &fd->Fence, VK_TRUE, UINT64_MAX);
			check_vk_result(error);

			error = vkResetFences(g_device, 1, &fd->Fence);
			check_vk_result(error);
		}
		// free resources in queue
		{
			for (auto& func : s_resourceFreeQueue[s_currentFrameIndex])
				func();

			s_resourceFreeQueue[s_currentFrameIndex].clear();

		}
		{
			auto& allocatedCommandBuffers = s_allocatedCommandBuffers[wd->FrameIndex];
			if (allocatedCommandBuffers.size() > 0) {
				vkFreeCommandBuffers(g_device, fd->CommandPool, (uint32_t)allocatedCommandBuffers.size(), allocatedCommandBuffers.data());
				allocatedCommandBuffers.clear();
			}

			error = vkResetCommandPool(g_device, fd->CommandPool, 0);
			check_vk_result(error);

			VkCommandBufferBeginInfo info{};
			info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			error = vkBeginCommandBuffer(fd->CommandBuffer, &info);
			check_vk_result(error);

		}

		// describe the imgui RenderPass
		{
			VkRenderPassBeginInfo info{};
			info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			info.renderPass = wd->RenderPass;
			info.framebuffer = fd->Framebuffer;
			info.renderArea.extent.width = wd->Width;
			info.renderArea.extent.height = wd->Height;
			info.clearValueCount = 1;
			info.pClearValues = &wd->ClearValue;
			vkCmdBeginRenderPass(fd->CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);

			ImGui_ImplVulkan_RenderDrawData(drawData, fd->CommandBuffer);

			vkCmdEndRenderPass(fd->CommandBuffer);
		}

		// Submit command buffer
		{
			VkPipelineStageFlags waitSage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			VkSubmitInfo info{};
			info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			info.waitSemaphoreCount = 1;
			info.pWaitSemaphores = &imageAquiredSemaphore;
			info.pWaitDstStageMask = &waitSage;
			info.commandBufferCount = 1;
			info.pCommandBuffers = &fd->CommandBuffer;
			info.signalSemaphoreCount = 1;
			info.pSignalSemaphores = &renderCompleteSemaphore;

			error = vkEndCommandBuffer(fd->CommandBuffer);
			check_vk_result(error);

			error = vkQueueSubmit(g_queue, 1, &info, fd->Fence);
			check_vk_result(error);

		}

	}

	static void FramePresent(ImGui_ImplVulkanH_Window* wd) {
		if (g_swapchainRebuild)
			return;

		VkSemaphore renderComplate = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
		VkPresentInfoKHR info{};
		info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		info.waitSemaphoreCount = 1;
		info.pWaitSemaphores = &renderComplate;
		info.swapchainCount = 1;
		info.pSwapchains = &wd->Swapchain;
		info.pImageIndices = &wd->FrameIndex;
		VkResult error = vkQueuePresentKHR(g_queue, &info);
		if (error == VK_ERROR_OUT_OF_DATE_KHR || error == VK_SUBOPTIMAL_KHR) {
			g_swapchainRebuild = true;
			return;
		}

		check_vk_result(error);
		wd->SemaphoreIndex = (wd->SemaphoreIndex + 1) % wd->ImageCount;
	}

	VkInstance Application::GetInstance() {
		return g_instance;
	}

	VkPhysicalDevice Application::GetPhysicalDevice() {
		return g_physicalDevice;
	}

	VkDevice Application::GetDevice() {
		return g_device;
	}

	VkCommandBuffer Application::GetCommandBuffer(bool begin) {
		ImGui_ImplVulkanH_Window* wd = &g_mainWindowData;

		VkCommandPool pool = wd->Frames[wd->FrameIndex].CommandPool;

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandBufferCount = 1;
		allocInfo.commandPool = pool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		VkCommandBuffer& buffer = s_allocatedCommandBuffers[wd->FrameIndex].emplace_back();
		auto error = vkAllocateCommandBuffers(g_device, &allocInfo, &buffer);
		check_vk_result(error);

		VkCommandBufferBeginInfo info{};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		error = vkBeginCommandBuffer(buffer, &info);
		check_vk_result(error);

		return buffer;
	}

	void Application::FlushCommandBuffer(VkCommandBuffer buffer) {
		const uint64_t DEFAULT_FENCE_TIMEOUT = 100000000000;
		VkSubmitInfo endinfo{};
		endinfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		endinfo.commandBufferCount = 1;
		endinfo.pCommandBuffers = &buffer;

		auto error = vkEndCommandBuffer(buffer);

		VkFenceCreateInfo fenceCreateInfo{};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.flags = 0;

		VkFence fence;
		error = vkCreateFence(g_device, &fenceCreateInfo, g_Allacator, &fence);
		check_vk_result(error);
		error = vkQueueSubmit(g_queue, 1, &endinfo, fence);
		check_vk_result(error);
		error = vkWaitForFences(g_device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT);
		check_vk_result(error);

		vkDestroyFence(g_device, fence, g_Allacator);
	}

	void Application::SubmitResourceFree(std::function<void()>&& func) {
		s_resourceFreeQueue[s_currentFrameIndex].emplace_back(func);
	}


	GLFWwindow* Application::GetWindow() {
		return instance->window;
	}


	// Layers

	//template<typename T>
	//void Application::PushLayer() {
	//	static_assert(std::is_base_of<Layer, T>::value, "Pushed Type is not a Layer!");
	//	layerStack.emplace_back(std::make_shared<T>())->OnAttach();

	//}

}