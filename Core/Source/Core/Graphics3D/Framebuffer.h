#pragma once

#include <string>
#include "vulkan/vulkan.h"
#include "../Utils/image.h"

namespace Core {
	namespace Graphics {

		class Framebuffer
		{
		public:
			Framebuffer(uint32_t width, uint32_t height);
			~Framebuffer();

			void Render();
			uint32_t width;
			uint32_t height;

			Image* GetColorBuffer(){ return _color; }
			Image* GetDepthBuffer(){ return _depth; }

		private:

			Image* _color;
			Image* _depth;

			/*VkImage _color;
			VkImageView _colorImageView;
			
			VkImage _depth;
			VkImageView _depthImageView;
			
			VkDescriptorSet _colorBuffer;
			VkDescriptorSet _depthBuffer;*/

		};

	}
}