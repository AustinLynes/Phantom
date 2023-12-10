#include "Framebuffer.h"
#include "../Application/Application.h"
#include <Core/Debug/Debug.h>

// STEPS TO DRAWING TO TEXTURE

// 1. Allocate, Create, Bind Image
// description of the image we want to create.
	// we want to create a:
	// 2D texture with width and height (w, h), 
	// with the format R32G32B32A32_SFLOAT 
	//		each bit is specifief as a:
	//			four-component, 128-bit unsigned integer format that has:
	//				a 32-bit R component in bytes 0..3, 
	//				a 32-bit G component in bytes 4..7, 
	//				a 32-bit B component in bytes 8..11, and 
	//				a 32-bit A component in bytes 12..15.
	// 
// 2. Aquire Image View.
// 3. Create Framebuffer
// foreach frame: 
// 4. Begin Renderpass
// 5. Render Commands (Render-to-Texture)
// 6. End Renderpass
// 7. Use Texture



namespace Core {
	
	void check_vk_result(VkResult error);

	namespace Graphics {
		
		uint32_t GetVulkanMemoryType(VkMemoryPropertyFlags properties, uint32_t type_bits)
		{
			VkPhysicalDeviceMemoryProperties prop;
			vkGetPhysicalDeviceMemoryProperties(Application::GetPhysicalDevice(), &prop);
			for (uint32_t i = 0; i < prop.memoryTypeCount; i++)
			{
				if ((prop.memoryTypes[i].propertyFlags & properties) == properties && type_bits & (1 << i))
					return i;
			}

			return 0xffffffff;
		}

		Framebuffer::Framebuffer(uint32_t width, uint32_t height)
			:	width{width}, height{height}
		{

			Core::Debug::Log("Framebuffer Size: " + std::to_string(width) + ", " + std::to_string(height));
			
			auto device = Application::GetDevice();
			auto allocator = Application::GetAllacator();
			auto physicalDevice = Application::GetPhysicalDevice();

			VkResult err;
			
			frames = new Frame[frameCount]{};

			for (int i = 0; i < frameCount; i++)
			{
				Frame frame{};
				

				
			}

		}


		Framebuffer::~Framebuffer()
		{
			delete[] frames;
		}

		void Framebuffer::Render() {

			auto cmd = Application::GetCommandBuffer();

			VkClearValue clearValues[] = {0.0f, 0.0f, 0.0f, 0.0f};
			
			VkRenderPassBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			beginInfo.clearValueCount = 1;
			beginInfo.pClearValues = clearValues;
			beginInfo.framebuffer = framebuffer;
			beginInfo.pNext = nullptr;
			beginInfo.renderPass = colorpass;
			beginInfo.renderArea.extent = { width, height };
			

			vkCmdBeginRenderPass(cmd, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);;
			


			vkCmdEndRenderPass(cmd);
		
		}

		void Framebuffer::Present()
		{
		}


	}
}