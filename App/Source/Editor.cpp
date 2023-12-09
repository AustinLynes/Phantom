#include "Core/Application/Application.h"
#include "Core/Layers/Layer.h"

#include "Core/EntryPoint.h"
#include <Core/Utils/Image.h>

class DemoLayer : public Core::Layer {
public:
	virtual void OnUIRender() override {
		ImGui::ShowDemoWindow();
	}
};

class SceneEditorLayer : public Core::Layer {
public:
	virtual void OnUIRender() override
	{
		if (ImGui::Begin("Scene")) {
			
			const auto format = Core::ImageFormat::RGBA32F;
			const uint32_t width = 32u, height = 16u;
			const uint32_t bytesPerPixel = Core::Image::BytesPerPixel(format);
			struct C {
				float r;
				float g;
				float b;
				float a;
			};
			int bufferSize = width * height;
			C* buffer = new C[bufferSize] {0};

			for (size_t i = 0; i < bufferSize; i++)
			{
				buffer[i] = { 1.0f, 1.0f, 1.0f, 1.0f };
			}

			auto image = Core::Image(width, height, format, (void*)buffer);
			ImGui::Image(ImTextureID(image.GetDescriptorSet()), ImGui::GetContentRegionAvail());
			

			delete[] buffer;

			ImGui::End();
		}
	} 
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