#include "Core/Application/Application.h"
#include "Core/Layers/Layer.h"

#include "Core/EntryPoint.h"
#include <Core/Utils/Image.h>

#include "Core/Graphics3D/Framebuffer.h"

class DemoLayer : public Core::Layer {
public:
	virtual void OnUIRender() override {
		ImGui::ShowDemoWindow();
	}
};

class SceneEditorLayer : public Core::Layer {
public:
	virtual void OnAttach() override {
		framebuffer = new Core::Graphics::Framebuffer(width, height);
	}
	virtual void OnUpdate(float ts) override {
		framebuffer->Render();
		framebuffer->Present();
	}
	
	virtual void OnUIRender() override
	{
		if (ImGui::Begin("Scene")) {
			
			
			//ImGui::Image(ImTextureID(framebuffer->GetColorBuffer()), {(float)framebuffer->width, (float)framebuffer->height});
			
			
			ImGui::End();
		}
	} 

	virtual void OnDetach() override {
		delete framebuffer;
	}

	uint32_t width = 1024;
	uint32_t height = 1024;

	Core::Graphics::Framebuffer* framebuffer;

};

Core::Application* Core::CreateApplication(int argc, char** argv) {

	Core::ApplicationSpecification specs{};
	specs.Title = "Phantom";
	specs.Dimensions = { 1920, 1080 };
	const char* requiredLayers[] = { EDITOR_LAYER_CONSOLE };
	specs.pRequiredLayerNames = requiredLayers;
	specs.requiredLayersCount = _countof(requiredLayers);

	Core::Application* app = new Core::Application(specs);
	app->SetMenuBarCallback([app]() {
		if (ImGui::BeginMenu("File")) 
		{
			if (ImGui::MenuItem("Exit")) {
				app->Close();
			}
			ImGui::EndMenu();
		}
	});

	app->PushLayer<SceneEditorLayer>();
	app->PushLayer<DemoLayer>();

	return app;
}