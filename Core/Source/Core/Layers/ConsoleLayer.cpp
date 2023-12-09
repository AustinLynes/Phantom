#include "ConsoleLayer.h"
#include "../Application/Application.h"
#include "GLFW/glfw3.h"

namespace Core {

	namespace Layers {

		static Console* s_consoleInst;
		
		void Console::OnAttach()
		{
			s_consoleInst = this;
			buffer.reserve(min_buffer_size);
		}
		void Console::OnUIRender()
		{
			const auto& drawbuffer = std::vector<std::string>(buffer);
			// temp
			// 
			if (ImGui::Begin("Mouse Info")) {
				
				double x, y;
				auto window = Application::GetWindow();
				glfwGetCursorPos(window, &x, &y);
				float data[2]{ x, y };
				ImGui::InputFloat2("Mouse Position: ", data);
				
				ImGui::End();
			}
			// logs should be able to be filtered by their priority 
			if (ImGui::Begin("Console")) {

				if (ImGui::Button("Clear")) {
					buffer.clear();
					ImGui::End();
					return;
				}

				for (const auto& what : drawbuffer)
				{
					// create some area to hold all of the logs.. .
					auto fontColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
					float timestamp = 0;
					auto priority = "Info";
					ImGui::TextColored(fontColor, "[%d] %s: %s", timestamp, priority, what.c_str());
				}
			}

			ImGui::End();

		}

		void Console::OnDetach()
		{
			buffer.clear();
			s_consoleInst = nullptr;
		}
		Console* Console::Get()
		{
			return s_consoleInst;
		}
	}
}