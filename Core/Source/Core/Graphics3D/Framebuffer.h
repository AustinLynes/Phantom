#pragma once

#include <string>
#include "vulkan/vulkan.h"

namespace Core {
	namespace Graphics {



		struct Frame {
			VkCommandPool commandPool;
			VkCommandBuffer commandBuffer;
			VkFence fence;
			VkImage buffer;
			VkImageView bufferView;
			VkFramebuffer framebuffer;
		};

		class Framebuffer
		{
		public:
			Framebuffer(uint32_t width, uint32_t height);
			~Framebuffer();

			void Render();
			void Present();

			
			uint32_t width;
			uint32_t height;

		private:

			//Image* _depth;
			const int frameCount = 3;
			Frame* frames;

			int currentFrameIndex = 0;

			VkSwapchainKHR swapchain;
			


		};

	}
}