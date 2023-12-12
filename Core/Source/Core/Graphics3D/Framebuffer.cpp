#include "Framebuffer.h"
#include "../Application/Application.h"
#include <Core/Debug/Debug.h>
#include <fstream>
#include <imgui/backends/imgui_impl_vulkan.h>


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

		//char* readFile(const char* filename, size_t* fileSize) {
		//	FILE* file = fopen(filename, "rb");
		//	if (!file) {
		//		fprintf(stderr, "Failed to open file: %s\n", filename);
		//		return NULL;
		//	}
		//	fseek(file, 0, SEEK_END);
		//	*fileSize = ftell(file);
		//	fseek(file, 0, SEEK_SET);
		//	char* shaderCode = (char*)malloc(*fileSize + 1);
		//	if (!shaderCode) {
		//		fclose(file);
		//		fprintf(stderr, "Failed to allocate memory for shader code.\n");
		//		return NULL;
		//	}
		//	fread(shaderCode, *fileSize, 1, file);
		//	shaderCode[*fileSize] = '\0';
		//	fclose(file);
		//	return shaderCode;
		//}

		std::vector<uint32_t> readFile_SPIRV(const std::string& filename) {
			// Open the SPIR-V file in binary mode
			std::ifstream file(filename, std::ios::ate | std::ios::binary);

			// Check if file opening was successful
			if (!file.is_open()) {
				std::cerr << "Failed to open file: " << filename << std::endl;
				return {}; // Return an empty vector on failure
			}

			// Get the file size
			size_t fileSize = static_cast<size_t>(file.tellg());

			// Seek back to the beginning of the file
			file.seekg(0);

			// Read the file content into a vector of chars
			std::vector<char> buffer(fileSize);
			file.read(buffer.data(), fileSize);

			// Close the file
			file.close();

			// Interpret the char buffer as uint32_t
			std::vector<uint32_t> spirvBytes(fileSize / sizeof(uint32_t));
			std::memcpy(spirvBytes.data(), buffer.data(), fileSize);

			return spirvBytes; // Return the vector containing SPIR-V bytecode as uint32_t
		}

		VkResult CompileToSPIRV(const std::string& glsl, ShaderType type) {
			
			std::string command = "%VULKAN_SDK%/bin/glslangValidator.exe -V " + glsl + " -o " + glsl + ".spv";
			
			std::cout << "compiling \n\t\'" + command + "\'";
			
			int error = std::system(command.c_str());


			if (error != 0) {
				Debug::Log("Could Not Compile: " + glsl);
				return VK_ERROR_VALIDATION_FAILED_EXT;
			}
			else {
				auto temp = (glsl + ".spv");
				return VK_SUCCESS;
			}
		}
		
		VkShaderModule CreateShaderModuleExt(VkDevice device, const VkAllocationCallbacks* allocator, const std::string& glslFilepath, ShaderType shaderType) {
			
			VkShaderModule shader;

			// compile GLSL to SPIR-V
			VkResult error = CompileToSPIRV(glslFilepath, shaderType);
			check_vk_result(error);
			
			// read SPIR-V file
			std::vector<uint32_t> code = readFile_SPIRV(glslFilepath + ".spv");

			if (code.data() == nullptr) {
				throw std::runtime_error("file read error, spir-v file was not read correctly...");
			}

			
			VkShaderModuleCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			info.flags = 0;
			info.pNext = nullptr;
			
			info.codeSize = 4* code.size();
			info.pCode = code.data();

			error = vkCreateShaderModule(device, &info, allocator, &shader);
			check_vk_result(error);

			return shader;
		}


		Framebuffer::Framebuffer(uint32_t width, uint32_t height)
			: width{ width }, height{ height }
		{

			Core::Debug::Log("Framebuffer Size: " + std::to_string(width) + ", " + std::to_string(height));

			auto device = Application::GetDevice();
			auto allocator = Application::GetAllacator();
			auto physicalDevice = Application::GetPhysicalDevice();

			VkResult error;

			frames = new Frame[frameCount]{};

			for (int i = 0; i < frameCount; i++)
			{
				Frame frame{};

				// Create Color Image Attachment
				{
					// Create Image
					VkImageCreateInfo info{};
					info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
					info.pNext = nullptr;
					info.flags = 0;
					info.imageType = VK_IMAGE_TYPE_2D;
					info.format = VK_FORMAT_R32G32B32A32_SFLOAT; // 8bit uint ( 0 -> 255 )
					info.extent.width = width;
					info.extent.height = height;
					info.extent.depth = 1.0f;
					info.mipLevels = 1;
					info.arrayLayers = 1;
					info.samples = VK_SAMPLE_COUNT_1_BIT;
					info.tiling = VK_IMAGE_TILING_OPTIMAL;
					info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
					info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
					info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

					error = vkCreateImage(device, &info, allocator, &frame.ColorBuffer.image);
					check_vk_result(error);

					// Allocate Memory
					// determine what kind of memory is required.
					VkMemoryRequirements req;
					vkGetImageMemoryRequirements(device, frame.ColorBuffer.image, &req);

					VkMemoryAllocateInfo alloc_info = {};
					alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
					alloc_info.allocationSize = req.size;
					alloc_info.memoryTypeIndex = GetVulkanMemoryType(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, req.memoryTypeBits);
					error = vkAllocateMemory(device, &alloc_info, nullptr, &frame.ColorBuffer.memory);
					check_vk_result(error);
					// Bind Image and Memory
					error = vkBindImageMemory(device, frame.ColorBuffer.image, frame.ColorBuffer.memory, 0);
					check_vk_result(error);

					// Create Image View
					VkImageViewCreateInfo viewInfo{};
					viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
					viewInfo.flags = 0;
					viewInfo.image = frame.ColorBuffer.image;
					viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
					viewInfo.format = info.format;
					viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					viewInfo.subresourceRange.levelCount = 1;
					viewInfo.subresourceRange.layerCount = 1;

					error = vkCreateImageView(device, &viewInfo, allocator, &frame.ColorBuffer.view);
					check_vk_result(error);
					// create per-frame sampler
					{
						VkSamplerCreateInfo createInfo{};
						createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
						createInfo.pNext = nullptr;
						createInfo.flags = 0;
						createInfo.magFilter = VK_FILTER_NEAREST;
						createInfo.minFilter = VK_FILTER_NEAREST;
						createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
						createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
						createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
						createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
						createInfo.mipLodBias = 0.0;
						createInfo.anisotropyEnable = VK_FALSE;
						createInfo.maxAnisotropy = 1.0;
						createInfo.compareEnable = VK_TRUE;
						createInfo.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
						createInfo.minLod = 0.0f;
						createInfo.maxLod = FLT_MAX;
						createInfo.unnormalizedCoordinates = VK_FALSE;
						
						error = vkCreateSampler(device, &createInfo, allocator, &Sampler);
						check_vk_result(error);
					}

					frame.ColorBuffer.descriptor = (VkDescriptorSet)ImGui_ImplVulkan_AddTexture(Sampler, frame.ColorBuffer.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
				}

				// Create Depth Image Attachment
				{
					// Create Image
					VkImageCreateInfo info{};
					info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
					info.pNext = nullptr;
					info.flags = 0;
					info.imageType = VK_IMAGE_TYPE_2D;
					info.format = VK_FORMAT_D24_UNORM_S8_UINT; // depth - 24bit | unsigned normalized 
					info.extent.width = width;
					info.extent.height = height;
					info.extent.depth = 1.0f;
					info.mipLevels = 1;
					info.arrayLayers = 1;
					info.samples = VK_SAMPLE_COUNT_1_BIT;
					info.tiling = VK_IMAGE_TILING_OPTIMAL;
					info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
					info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
					info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

					error = vkCreateImage(device, &info, allocator, &frame.DepthBuffer.image);
					check_vk_result(error);

					// Allocate Memory
					// determine what kind of memory is required.
					VkMemoryRequirements req;
					vkGetImageMemoryRequirements(device, frame.DepthBuffer.image, &req);

					VkMemoryAllocateInfo alloc_info = {};
					alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
					alloc_info.allocationSize = req.size;
					alloc_info.memoryTypeIndex = GetVulkanMemoryType(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, req.memoryTypeBits);
					error = vkAllocateMemory(device, &alloc_info, nullptr, &frame.DepthBuffer.memory);
					check_vk_result(error);
					// Bind Image and Memory
					error = vkBindImageMemory(device, frame.DepthBuffer.image, frame.DepthBuffer.memory, 0);
					check_vk_result(error);

					// Create Image View
					VkImageViewCreateInfo viewInfo{};
					viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
					viewInfo.flags = 0;
					viewInfo.image = frame.DepthBuffer.image;
					viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
					viewInfo.format = info.format;
					viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
					viewInfo.subresourceRange.levelCount = 1;
					viewInfo.subresourceRange.layerCount = 1;

					error = vkCreateImageView(device, &viewInfo, allocator, &frame.DepthBuffer.view);
					check_vk_result(error);
				}




				// Create Renderpass and describe Graphics Pipeline settings
				{
					VkAttachmentDescription attachmentDescs[]{
						/*COLOR ATTACHMENT DESC*/
						{
							0,
							VK_FORMAT_R32G32B32A32_SFLOAT,
							VK_SAMPLE_COUNT_1_BIT,
							VK_ATTACHMENT_LOAD_OP_DONT_CARE,
							VK_ATTACHMENT_STORE_OP_DONT_CARE,
							VK_ATTACHMENT_LOAD_OP_DONT_CARE,
							VK_ATTACHMENT_STORE_OP_DONT_CARE,
							VK_IMAGE_LAYOUT_UNDEFINED,
							VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
						},
						/*DEPTH ATTACHMENT DESC*/
						//{
						//	0,
						//	VK_FORMAT_D24_UNORM_S8_UINT,
						//	VK_SAMPLE_COUNT_1_BIT,
						//	VK_ATTACHMENT_LOAD_OP_DONT_CARE,
						//	VK_ATTACHMENT_STORE_OP_DONT_CARE,
						//	VK_ATTACHMENT_LOAD_OP_DONT_CARE,
						//	VK_ATTACHMENT_STORE_OP_DONT_CARE,
						//	VK_IMAGE_LAYOUT_UNDEFINED,
						//	VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL
						//}
					};

					VkAttachmentReference attachRefs[]{
						/*COLOR*/
						//{0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL },
						{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
						/*DEPTH*/
					};

					VkSubpassDescription subpasses[]{
						// Draw Normal
						{
							0,								//    flags;
							VK_PIPELINE_BIND_POINT_GRAPHICS,//    pipelineBindPoint;
							0,								//    inputAttachmentCount;
							nullptr,						//const VkAttachmentReference pInputAttachments;
							1,								//    colorAttachmentCount;
							&attachRefs[0],					//const VkAttachmentReference pColorAttachments;
							nullptr,						//const VkAttachmentReference pResolveAttachments;
							/*&attachRefs[0]*/nullptr,					//const VkAttachmentReference pDepthStencilAttachment;
							0,								//    preserveAttachmentCount;
							nullptr							//const uint32_t pPreserveAttachments;
						}
					};

					VkRenderPassCreateInfo info{};
					info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
					info.pNext = nullptr;
					info.flags = 0;
					info.attachmentCount = 1;
					info.pAttachments = attachmentDescs;
					info.subpassCount = _countof(subpasses);
					info.pSubpasses = subpasses;					

					error = vkCreateRenderPass(device, &info, allocator, &frame.RenderPass);
					check_vk_result(error);

					// Describe Vertex Layout

					const int POSITION = 0;

					VkVertexInputBindingDescription binds[]{ 
						{POSITION, sizeof(float) * 4, VK_VERTEX_INPUT_RATE_VERTEX }
					};
					
					VkVertexInputAttributeDescription attrs[]{ 
						{ POSITION, 0, VK_FORMAT_R8G8B8A8_UNORM, 0} 
					};

					
					// Describe Shader Stages
					// Vertex Input
					VkPipelineVertexInputStateCreateInfo vi{};
					vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
					vi.pNext = nullptr;
					vi.flags = 0;
					vi.vertexBindingDescriptionCount = _countof(binds);
					vi.pVertexBindingDescriptions = binds;
					vi.pVertexAttributeDescriptions = attrs;
					vi.vertexAttributeDescriptionCount = _countof(attrs);
					
					// Vertex-Input Assybmly
					VkPipelineInputAssemblyStateCreateInfo ia{};
					ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
					ia.flags = 0;
					ia.pNext = nullptr;
					ia.primitiveRestartEnable = VK_FALSE;
					// !! TEMPORARY
					ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
					

					// load and compile shaders...

					VkPipelineShaderStageCreateInfo stages[] =
					{
						// VS
						{
							VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
							nullptr,
							0,
							VK_SHADER_STAGE_VERTEX_BIT,
							CreateShaderModuleExt(device, allocator, "shaders/test.vert", ShaderType::Vertex),
							"main",
						},
						// FS
						{
							VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
							nullptr,
							0,
							VK_SHADER_STAGE_FRAGMENT_BIT,
							CreateShaderModuleExt(device, allocator, "shaders/test.frag", ShaderType::Fragment),
							"main"
						}
					};

					// Describe the Viewport State

					VkRect2D scissor{};
					scissor.extent = { width, height };
					scissor.offset = { 0, 0 };

					VkViewport viewport{};
					viewport.width = width;
					viewport.height = height;
					viewport.minDepth = 0;
					viewport.maxDepth = 1;
					viewport.x = 0;
					viewport.y = 0;
						
					VkPipelineViewportStateCreateInfo vs{};
					vs.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
					vs.pNext = nullptr;
					vs.flags = 0;
					vs.viewportCount = 1;
					vs.pViewports = &viewport;
					vs.scissorCount = 1;
					vs.pScissors = &scissor;

					// Describe the Rasterization State
					VkPipelineRasterizationStateCreateInfo rs{};
					rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
					rs.pNext = nullptr;
					rs.flags = 0;
					rs.cullMode = VK_CULL_MODE_BACK_BIT;
					rs.frontFace = VK_FRONT_FACE_CLOCKWISE;
					rs.lineWidth = 1.0;
					rs.polygonMode = VK_POLYGON_MODE_FILL;
					
					VkPipelineLayout pipelinelayout;

					VkPipelineLayoutCreateInfo layout{};
					layout.flags = 0;
					layout.pNext = nullptr;
					layout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
					
					error = vkCreatePipelineLayout(device, &layout, allocator, &pipelinelayout);
					check_vk_result(error);


					VkPipelineMultisampleStateCreateInfo ms{};
					ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
					ms.flags = 0;
					ms.pNext = nullptr;
					ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
					ms.sampleShadingEnable = VK_FALSE;
					ms.minSampleShading = 1.0;
					ms.pSampleMask = nullptr;
					ms.alphaToCoverageEnable = VK_FALSE;
					ms.alphaToOneEnable = VK_FALSE;

					VkPipelineColorBlendAttachmentState blendAttachment = {};
					blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT; // Enable writing to all color channels
					blendAttachment.blendEnable = VK_FALSE; // Disable blending for this attachment


					VkPipelineColorBlendStateCreateInfo bs = {};
					bs.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
					bs.logicOpEnable = VK_FALSE; // Disable logical operation for blending
					bs.attachmentCount = 1; // Number of color attachments
					bs.pAttachments = &blendAttachment;

					// Global alpha and color blend factors
					bs.blendConstants[0] = 0.0f; // R
					bs.blendConstants[1] = 0.0f; // G
					bs.blendConstants[2] = 0.0f; // B
					bs.blendConstants[3] = 0.0f; // Alpha


					VkGraphicsPipelineCreateInfo pipeline{};
					pipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
					pipeline.pNext = nullptr;
					pipeline.flags = 0;
					pipeline.basePipelineHandle = nullptr;
					pipeline.stageCount = _countof(stages);
					pipeline.pStages = stages;
					pipeline.pVertexInputState = &vi;
					pipeline.pInputAssemblyState = &ia;
					pipeline.pRasterizationState = &rs;
					pipeline.layout = pipelinelayout;
					pipeline.pViewportState = &vs;
					pipeline.pMultisampleState = &ms;
					pipeline.renderPass = frame.RenderPass;
					pipeline.pColorBlendState = &bs;
					// tesselation
					// depth/stencil
					error = vkCreateGraphicsPipelines(device, nullptr, 1, &pipeline, allocator, &graphicsPipeline_STANDARD);
					check_vk_result(error);
				}

				// Create Framebuffer
				{
					// describe framebuffer attachments
					VkImageView attachments[] = {
						frame.ColorBuffer.view,
						//frame.DepthBuffer.view
					};

					VkFramebufferCreateInfo info{};
					info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
					info.pNext = nullptr;
					info.flags = 0;

					info.attachmentCount = _countof(attachments);
					info.pAttachments = attachments;
					info.renderPass = frame.RenderPass;
					info.width = width;
					info.height = height;
					info.layers = 1;

					error = vkCreateFramebuffer(device, &info, allocator, &frame.Framebuffer);
					check_vk_result(error);
				}


			}

		}


		Framebuffer::~Framebuffer()
		{
			auto device = Application::GetDevice();
			auto allocator = Application::GetAllacator();

			for (int i = 0; i < frameCount; i++)
			{
				vkDestroyImageView(device, frames[i].ColorBuffer.view, allocator);
			
				vkDestroyImage(device, frames[i].ColorBuffer.image, allocator);
			
				vkDestroyFramebuffer(device, frames[i].Framebuffer, allocator);

			}

			delete[] frames;
		}

		void Framebuffer::Render() {


		}

		void Framebuffer::Present()
		{
		}

		VkDescriptorSet Framebuffer::GetColorBuffer()
		{
			return frames[currentFrameIndex].ColorBuffer.descriptor;
		}


	}
}