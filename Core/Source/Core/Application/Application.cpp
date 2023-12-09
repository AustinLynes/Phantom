#include "Application.h"
#include "Application.impl.h"

#include "Layer.h"

extern bool g_ApplicationIsRunning;

namespace Core {

	void Application::Close() {
		shouldRun = false;
	}

	void Application::Startup() {
		glfwSetErrorCallback(glfw_error_callback);

		if (!glfwInit()) {
			std::cerr << "Could Not Initilize GLFW!\n";
			return;
		}

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		window = glfwCreateWindow(specification.Dimensions.width, specification.Dimensions.height, specification.Title.c_str(), NULL, NULL);

		if (!glfwVulkanSupported())
		{
			std::cerr << "[glfw] Vulkan Not Supported!\n";
			return;
		}
		uint32_t extensionsCount = 0;
		const char** requiredExtension = glfwGetRequiredInstanceExtensions(&extensionsCount);
		SetupVulkan(requiredExtension, extensionsCount);
		
		// update window title to reflect current GPU
		VkPhysicalDeviceProperties gpu;
		vkGetPhysicalDeviceProperties(g_physicalDevice, &gpu);
		glfwSetWindowTitle(window, std::string(specification.Title + " | " + gpu.deviceName).c_str());


		// Create Window Surface
		VkSurfaceKHR surface;
		VkResult error = glfwCreateWindowSurface(g_instance, window, g_Allacator, &surface);
		checK_vk_result(error);

		// create framebuffers
		int w, h;
		glfwGetFramebufferSize(window, &w, &h);
		ImGui_ImplVulkanH_Window* wd = &g_mainWindowData;
		SetupVulkanWindow(wd, surface, w, h);

		s_allocatedCommandBuffers.resize(wd->ImageCount);
		s_resourceFreeQueue.resize(wd->ImageCount);

		// Setup Imgui context.
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows

		ImGui::StyleColorsDark();

		ImGui_ImplGlfw_InitForVulkan(window, true);
		ImGui_ImplVulkan_InitInfo initInfo{};
		initInfo.Instance = g_instance;
		initInfo.PhysicalDevice = g_physicalDevice;
		initInfo.Device = g_device;
		initInfo.QueueFamily = g_queueFamily;
		initInfo.Queue = g_queue;
		initInfo.PipelineCache = g_pipelineCache;
		initInfo.DescriptorPool = g_descriptorPool;
		initInfo.Subpass = 0;
		initInfo.MinImageCount = g_minImagesCount;
		initInfo.ImageCount = wd->ImageCount;
		initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		initInfo.Allocator = g_Allacator;
		initInfo.CheckVkResultFn = checK_vk_result;

		ImGui_ImplVulkan_Init(&initInfo, wd->RenderPass);


	}

	void Application::Run() {

		shouldRun = true;
		 
		ImGui_ImplVulkanH_Window* wd = &g_mainWindowData;
		ImVec4 clearColor = ImVec4(0.16f, 0.16f, 0.16f, 1.0f);
		ImGuiIO& io = ImGui::GetIO();

		while (!glfwWindowShouldClose(window) && shouldRun) {
			glfwPollEvents();

			// implement layerstack.
			for (auto& layer : layerStack)
			{
				layer->OnUpdate(timeStep);
			}

			if (g_swapchainRebuild) {
				int w, h; 
				glfwGetFramebufferSize(window, &w, &h);
				if (w > 0 && h > 0) {
					ImGui_ImplVulkan_SetMinImageCount(g_minImagesCount);
					ImGui_ImplVulkanH_CreateOrResizeWindow(g_instance, g_physicalDevice, g_device, wd, g_queueFamily, g_Allacator, w, h, g_minImagesCount);
					g_mainWindowData.FrameIndex = 0;

					s_allocatedCommandBuffers.clear();
					s_allocatedCommandBuffers.resize(g_mainWindowData.ImageCount);

					g_swapchainRebuild = false;
				}
			}

			ImGui_ImplVulkan_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			// draw dockspawce
			{

				static bool opt_fullscreen_persistant = true;
				bool opt_fullscreen = opt_fullscreen_persistant;
				static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

				ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
				if (opt_fullscreen)
				{
					ImGuiViewport* viewport = ImGui::GetMainViewport();
					ImGui::SetNextWindowPos(viewport->Pos);
					ImGui::SetNextWindowSize(viewport->Size);
					ImGui::SetNextWindowViewport(viewport->ID);
					ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
					ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
					window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
					window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
				}

				if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
					window_flags |= ImGuiWindowFlags_NoBackground;
				bool open = true;
				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
				ImGui::Begin("###DockSpace", &open, window_flags);
				ImGui::PopStyleVar();

				if (opt_fullscreen)
					ImGui::PopStyleVar(2);

				// DockSpace
				ImGuiIO& io = ImGui::GetIO();
				if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
				{
					ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
					ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
				}

				ImGui::End();

				// render all layers in layer stack
				for (auto& layer : layerStack)
				{
					layer->OnUIRender();
				}

			}

			ImGui::Render();
			ImDrawData* mainDrawData = ImGui::GetDrawData();
			const bool minimized = (mainDrawData->DisplaySize.x <= 0 || mainDrawData->DisplaySize.y <= 0);
			wd->ClearValue.color.float32[0] = clearColor.x * clearColor.w;
			wd->ClearValue.color.float32[1] = clearColor.y * clearColor.w;
			wd->ClearValue.color.float32[2] = clearColor.z * clearColor.w;
			wd->ClearValue.color.float32[3] = clearColor.w ;
			
			if (!minimized)
				FrameRender(wd, mainDrawData);

			if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault();
			}

			if (!minimized)
				FramePresent(wd);

			float time = GetTime();
			frameTime = time - lastFrameTime;
			timeStep = glm::min<float>(frameTime, 1/90.0f);
			lastFrameTime = time;
		}
	}

	float Application::GetTime() {
		return (float) glfwGetTime();
	}

	void Application::Shutdown() {
		// detach all layers.
		// clear layerstack

		for (auto& layer : layerStack)
		{
			layer->OnDetach();
		}

		VkResult error = vkDeviceWaitIdle(g_device);
		checK_vk_result(error);

		for (auto& q : s_resourceFreeQueue) 
			for (auto& fn : q)
				fn();
		s_resourceFreeQueue.clear();

		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		CleanupVulkanWindow();
		CleanupVulkan();

		glfwDestroyWindow(window);
		glfwTerminate();

		g_ApplicationIsRunning = false;

	}

	Application::Application(const ApplicationSpecification& spec) : specification(spec) {
	
		instance = this;
		Startup();
	}

	Application::~Application()
	{
		Shutdown();
		instance = nullptr;

	}
	

	Application& Application::Get()
	{
		return *instance;
	}
	
}