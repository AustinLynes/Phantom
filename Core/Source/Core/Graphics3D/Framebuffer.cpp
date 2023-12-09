#include "Framebuffer.h"
#include "../Application/Application.h"

namespace Core {
	namespace Graphics {
		
		Framebuffer::Framebuffer(uint32_t width, uint32_t height)
			:	width{width}, height{height}
		{
			_color = new Image(width, height, ImageFormat::RGBA32F);
			_depth = new Image(width, height, ImageFormat::R8);
		}

		void Framebuffer::Render() {

			// describe attachments
			VkRenderingAttachmentInfoKHR color{};
			color.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
			color.imageView = _color->GetView();
			color.clearValue = { 0.0f , 0.0f, 0.0f, 0.0f };
			color.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			
			VkRenderingAttachmentInfoKHR color_attachments[]{
				color
			};

			VkRenderingAttachmentInfoKHR depth{};
			color.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
			color.imageView = _depth->GetView();
			color.clearValue = { 0.0f , 0.0f, 0.0f, 0.0f };
			color.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL_KHR;

			VkRenderingInfoKHR renderInfo{};
			renderInfo.pColorAttachments = color_attachments;
			renderInfo.colorAttachmentCount = _countof(color_attachments);
			renderInfo.pDepthAttachment = &depth;
			
			auto cmd = Application::GetCommandBuffer(true);

			VkPipeline graphicsPipeline{};
			VkGraphicsPipelineCreateInfo gfxPipeCreateInfo{};
			gfxPipeCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			
			//vkCreatePipelineLayout(Application::GetDevice(), &gfxPipeCreateInfo, nullptr,  );

			
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
			//vkCmdBeginRendering(cmd, &renderInfo);
			// render here?
			{

			}
			vkCmdEndRendering(cmd);
		}

		Framebuffer::~Framebuffer()
		{

		}

	}
}