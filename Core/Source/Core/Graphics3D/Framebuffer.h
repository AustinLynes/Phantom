#pragma once

#include <string>
#include "vulkan/vulkan.h"

namespace Core {
	namespace Graphics {



		struct Frame {
			struct ImageResource {
				VkImage image;
				VkImageView view;
				VkDeviceMemory memory;
				VkDescriptorSet descriptor;
			};
			VkCommandPool CommandPool;
			VkCommandBuffer CommandBuffer;
			VkFence Fence;
			ImageResource ColorBuffer;
			ImageResource DepthBuffer;
			VkRenderPass RenderPass;

			VkFramebuffer Framebuffer;

		};

		enum class ShaderType {
			Vertex,
			Fragment,
			Geometry
		};

		class Framebuffer
		{
		public:
			Framebuffer(uint32_t width, uint32_t height);
			~Framebuffer();

			void Render();
			void Present();

			VkDescriptorSet GetColorBuffer();

			uint32_t width;
			uint32_t height;

		private:

			//Image* _depth;
			Frame* frames;
			int currentFrameIndex = 0;
			const int frameCount = 3;

			VkPipeline graphicsPipeline_STANDARD;
			VkBuffer StagingBuffer = nullptr;
			VkDeviceMemory StagingBufferMemory = nullptr;
			size_t AlignedSize = 0;

			VkSwapchainKHR swapchain;

			// differnent types of samplers
			VkSampler Sampler;



		};

	}
}