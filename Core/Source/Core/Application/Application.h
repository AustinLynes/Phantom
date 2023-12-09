#pragma once

#include <iostream>
#include <cstdint>
#include <string>
#include <functional>

#include "imgui.h"
#include "vulkan/vulkan.h"


struct GLFWwindow;

namespace Core {

	struct ApplicationSpecification {
		struct {
			uint32_t width=1600;
			uint32_t height=900;
		} Dimensions;
		std::string Title = "Test";
	};
	
	class Application {
	public:
		Application(const ApplicationSpecification& spec=ApplicationSpecification());
		~Application();

		void Run();

		void Close();

	public:

		template<typename T>
		void PushLayer() {
			static_assert(std::is_base_of<Layer, T>::value, "Pushed Type is not a Layer!");
			layerStack.emplace_back(std::make_shared<T>())->OnAttach();

		}

	public: //* STATIC *//
		static Application& Get();


		static VkInstance GetInstance();
		static VkPhysicalDevice GetPhysicalDevice();
		static VkDevice GetDevice();


		static VkCommandBuffer GetCommandBuffer(bool begin);
		static void FlushCommandBuffer(VkCommandBuffer buffer);
		static void SubmitResourceFree(std::function<void()>&& func);


	private:
		float GetTime();

		void Startup();
		void Shutdown();


		bool shouldRun = true;

		GLFWwindow* window;
		ApplicationSpecification specification;

		float frameTime;
		float timeStep;
		float lastFrameTime;

		std::vector<std::shared_ptr<class Layer>> layerStack;


	};

	Application* CreateApplication(int argc, char** argv); // CONTRACT COMPLETED IN CLIENT

}