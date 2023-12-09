#include "Core/Application/Application.h"
#include "Core/Application/Layer.h"

#include "Core/EntryPoint.h"

bool g_ApplicationIsRunning = true;


class TestLayer : public Core::Layer {
public:
	virtual void OnUIRender() override {
		ImGui::Begin("HelloWorld!");
		if (ImGui::Button("Shutdown"))
		{
			g_ApplicationIsRunning = false;
		}
		ImGui::End();

		ImGui::ShowDemoWindow();
	}
};

Core::Application* Core::CreateApplication(int argc, char** argv) {

	Core::Application* app = new Core::Application();
	app->PushLayer<TestLayer>();
	return app;
}