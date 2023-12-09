#pragma once

#include "Layer.h"
#include <vector>
#include <string>
#include <imgui.h>

namespace Core {
	namespace Layers {
		

		class Console : public Layer {
			friend class Debug;
		private:
			std::vector<std::string> buffer;
			const size_t min_buffer_size = 50;
		
		public: /* LAYER */
			virtual void OnAttach() override;
			virtual void OnUIRender() override;
			virtual void OnDetach() override;
			
		public: /* STATIC */
			static Console* Get();
			
		};

		
	}
}