#pragma once

#include <vulkan/vulkan.hpp>

// #define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS  -> for C++20 or later compilers
// #define VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS -> if not defined in compile definitions

#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <map>
#include <fstream>
#include <optional>
#include <chrono>
#include <algorithm>
#include <unordered_map>

// Parts to abstract away

#include "Layout.hpp"
#include "Core.h"

namespace HelloVulkan {

	constexpr uint32_t WIDTH = 800;
	constexpr uint32_t HEIGHT = 600;
	const std::string  MODEL_PATH = "models/viking_room.obj";
	const std::string  TEXTURE_PATH = "textures/viking_room.png";

	constexpr int MAX_FRAMES_IN_FLIGHT = 2;

	const std::vector<char const*> validationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};


	class HelloTriangleApplication {

	public:

		void run() {
			initWindow();
			initVulkan();
			mainLoop();
			cleanup();
		}

		bool framebufferResized = false;

	private:

		std::optional<Window> window;
		std::optional<VulkanEngine> vulkanEngine;
		std::optional<Swapchain> swapchain;
		std::optional<DescriptorSetLayout> descriptorSetLayout;
		std::optional<DescriptorPool> descriptorPool;

		vk::raii::PipelineLayout			pipelineLayout = nullptr;
		vk::raii::Pipeline					graphicsPipeline = nullptr;
		vk::raii::CommandPool				commandPool = nullptr;

		std::vector<FrameData> frames;
		std::vector<vk::raii::Semaphore>				renderFinishedSemaphores;
		uint32_t frameIndex = 0;

		std::vector<Vertex>    vertices;
		std::vector<uint32_t>  indices;

		std::optional<VertexBuffer> vertexBuffer;
		std::optional<IndexBuffer>	indexBuffer;
		std::vector<UniformBuffer> uniformBuffers;

		std::optional<ImageData> imageData;
		std::optional<DepthImage> depthImage;
		std::optional<TextureImage> textureImage;
		std::vector<DescriptorSet> descriptorSets;

		uint32_t mipLevels;

		void initWindow() {
			if (!SDL_Init(SDL_INIT_VIDEO))
			{
				throw std::runtime_error(SDL_GetError());
			}
			window.emplace( WindowProps{ "Hello Vulkan", 800, 600 } );
		}

		void initVulkan() {
			loadImage();
			createVulkanEngine();
			createSwapChain();
			createDepthResources();
			createDescriptorSetLayout();
			createGraphicsPipeline();
			createCommandPool();
			createFrameData();
			createTextureImage();
			loadModel();
			createVertexBuffer();
			createIndexBuffer();
			createUniformBuffers();
			createDescriptorPool();
			createDescriptorSets();
		}

		void mainLoop() {
			while (!window.value().ShouldClose()) {
				window.value().PollEvents();
				framebufferResized |= window.value().ConsumeFramebufferResize();
				drawFrame();
			}

			vulkanEngine.value().getDevice().waitIdle();
		}

		void drawFrame()
		{
			auto fenceResult = vulkanEngine.value().getDevice().waitForFences(*frames[frameIndex].inFlightFence, vk::True, UINT64_MAX);
			if (fenceResult != vk::Result::eSuccess)
			{
				throw std::runtime_error("failed to wait for fence!");
			}

			auto [result, imageIndex] = swapchain.value().getSwapchain().acquireNextImage(UINT64_MAX, *frames[frameIndex].presentCompleteSemaphore, nullptr);

			if (result == vk::Result::eErrorOutOfDateKHR)
			{
				recreateSwapChain();
				return;
			}

			if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
			{
				assert(result == vk::Result::eTimeout || result == vk::Result::eNotReady);
				throw std::runtime_error("failed to aquire swap chain image!");
			}

			vulkanEngine.value().getDevice().resetFences(*frames[frameIndex].inFlightFence);

			updateUniformBuffer(frameIndex);

			recordCommandBuffer(imageIndex);

			vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
			const vk::SubmitInfo submitInfo{
				.waitSemaphoreCount = 1,
				.pWaitSemaphores = &*frames[frameIndex].presentCompleteSemaphore,
				.pWaitDstStageMask = &waitDestinationStageMask,
				.commandBufferCount = 1,
				.pCommandBuffers = &*frames[frameIndex].commandBuffer,
				.signalSemaphoreCount = 1,
				.pSignalSemaphores = &*renderFinishedSemaphores[imageIndex]
			};

			vulkanEngine.value().getQueue().submit(submitInfo, *frames[frameIndex].inFlightFence);


			const vk::PresentInfoKHR presentInfoKHR{
				.waitSemaphoreCount = 1,
				.pWaitSemaphores = &*renderFinishedSemaphores[imageIndex],
				.swapchainCount = 1,
				.pSwapchains = &*swapchain.value().getSwapchain(),
				.pImageIndices = &imageIndex,
				.pResults = nullptr
			};

			result = vulkanEngine.value().getQueue().presentKHR(presentInfoKHR);

			if (result == vk::Result::eSuboptimalKHR || result == vk::Result::eErrorOutOfDateKHR || framebufferResized)
			{
				recreateSwapChain();
				framebufferResized = false;
			}
			else
			{
				assert(result == vk::Result::eSuccess);
			}

			frameIndex = (frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
		}

		void cleanup() {
			SDL_Quit();
		}

		void createVulkanEngine()
		{
			vulkanEngine.emplace(validationLayers, window.value().GetRequiredInstanceExtensions(), window.value());
		}


		void recreateSwapChain()
		{
			auto drawableReady = [&]()
				{
					auto [width, height] = window.value().GetPixelSize();

					return width > 0 &&
						height > 0 &&
						!(SDL_GetWindowFlags(window.value().GetNativeWindow())
							& SDL_WINDOW_MINIMIZED);
				};

			while (!drawableReady())
			{
				if (!window.value().WaitEvent())
					return;
			}

			vulkanEngine.value().getDevice().waitIdle();

			swapchain.value().recreate(vulkanEngine.value(), window.value().GetPixelSize());
			depthImage.reset();
			depthImage.emplace(vulkanEngine.value(), std::pair(window.value().GetPixelSize()));
		}

		void createSwapChain()
		{
			swapchain.emplace(vulkanEngine.value(), window.value().GetPixelSize());
		}


		void createDescriptorSetLayout()
		{
			descriptorSetLayout.emplace();
			descriptorSetLayout.value().addUniformBufferBinding(
				0, vk::ShaderStageFlagBits::eVertex
			);
			descriptorSetLayout.value().addCombinedImageSamplerBinding(
				1, vk::ShaderStageFlagBits::eFragment
			);
			descriptorSetLayout.value().Create(vulkanEngine.value());
		}

		void createGraphicsPipeline()
		{
			auto vertexShaderModule = createShaderModule(readFile("demo/shaders/triangle.vert.hlsl.spv"));
			auto fragShaderModule = createShaderModule(readFile("demo/shaders/triangle.frag.hlsl.spv"));

			vk::PipelineShaderStageCreateInfo vertShaderStageInfo{ .stage = vk::ShaderStageFlagBits::eVertex, .module = vertexShaderModule, .pName = "main" };
			vk::PipelineShaderStageCreateInfo fragShaderStageInfo{ .stage = vk::ShaderStageFlagBits::eFragment, .module = fragShaderModule, .pName = "main" };

			vk::PipelineShaderStageCreateInfo shaderStageInfo[]{ vertShaderStageInfo, fragShaderStageInfo };

			std::vector<vk::DynamicState> dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
			vk::PipelineDynamicStateCreateInfo dynamicState{ .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()), .pDynamicStates = dynamicStates.data() };

			// From vertex buffer
			auto bindingDescription = Vertex::getBindingDescription();
			auto attributeDescriptions = Vertex::getAttributeDescriptions();
			vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
				.vertexBindingDescriptionCount = 1,
				.pVertexBindingDescriptions = &bindingDescription,
				.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
				.pVertexAttributeDescriptions = attributeDescriptions.data()
			};

			vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo{ .topology = vk::PrimitiveTopology::eTriangleList };

			// For pipeline without dynamic state feature
			vk::Viewport viewport{ 0.0f, 0.0f, static_cast<float>(swapchain.value().getSwapchainExtent().width), static_cast<float>(swapchain.value().getSwapchainExtent().height), 0.0f, 1.0f};
			vk::Rect2D scissor{ vk::Offset2D{0, 0}, swapchain.value().getSwapchainExtent() };
			// vk::PipelineViewportStateCreateInfo viewportState{ .viewportCount = 1, .pViewports = &viewport, .scissorCount = 1, .pScissors = &scissor };

			vk::PipelineViewportStateCreateInfo viewportState{ .viewportCount = 1, .scissorCount = 1 };

			vk::PipelineDepthStencilStateCreateInfo depthStencil{
				.depthTestEnable = true,
				.depthWriteEnable = true,
				.depthCompareOp = vk::CompareOp::eLess,
				.depthBoundsTestEnable = false,
				.stencilTestEnable = false
			};

			vk::PipelineRasterizationStateCreateInfo rasterizer{
				.depthClampEnable = vk::False,
				.rasterizerDiscardEnable = vk::False,
				.polygonMode = vk::PolygonMode::eFill,
				.cullMode = vk::CullModeFlagBits::eBack,
				.frontFace = vk::FrontFace::eCounterClockwise,
				.depthBiasEnable = vk::False,
				.lineWidth = 1.0f
			};

			vk::PipelineMultisampleStateCreateInfo multisampling{ .rasterizationSamples = vk::SampleCountFlagBits::e1, .sampleShadingEnable = vk::False };

			vk::PipelineColorBlendAttachmentState colorBlendAttachment{
				.blendEnable = vk::True,
				.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
				.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
				.colorBlendOp = vk::BlendOp::eAdd,
				.srcAlphaBlendFactor = vk::BlendFactor::eOne,
				.dstAlphaBlendFactor = vk::BlendFactor::eZero,
				.alphaBlendOp = vk::BlendOp::eAdd,
				.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
			};

			vk::PipelineColorBlendStateCreateInfo colorBlending{
				.logicOpEnable = vk::False,
				.logicOp = vk::LogicOp::eCopy,
				.attachmentCount = 1,
				.pAttachments = &colorBlendAttachment
			};

			vk::PipelineLayoutCreateInfo pipelineLayoutInfo{ .setLayoutCount = 1, .pSetLayouts = &*descriptorSetLayout.value().get(), .pushConstantRangeCount = 0};
			pipelineLayout = vk::raii::PipelineLayout(vulkanEngine.value().getDevice(), pipelineLayoutInfo);

			vk::PipelineRenderingCreateInfo pipelineRenderingCreateinfo{ .colorAttachmentCount = 1, .pColorAttachmentFormats = &swapchain.value().getSwapchainSurfaceFormat().format};

			vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipelineCreateInfoChain = {

				{
					.stageCount = 2,
					.pStages = shaderStageInfo,
					.pVertexInputState = &vertexInputInfo,
					.pInputAssemblyState = &inputAssemblyInfo,
					.pViewportState = &viewportState,
					.pRasterizationState = &rasterizer,
					.pMultisampleState = &multisampling,
					.pDepthStencilState = &depthStencil,
					.pColorBlendState = &colorBlending,
					.pDynamicState = &dynamicState,
					.layout = pipelineLayout,
					.renderPass = nullptr
				},
				{
					.colorAttachmentCount = 1, 
					.pColorAttachmentFormats = &swapchain.value().getSwapchainSurfaceFormat().format,
					.depthAttachmentFormat = depthImage.value().getDepthFormat()
				}
			};

			graphicsPipeline = vk::raii::Pipeline(vulkanEngine.value().getDevice(), nullptr, pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>());
		}


		void createCommandPool()
		{
			vk::CommandPoolCreateInfo poolInfo{
				.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
				.queueFamilyIndex = vulkanEngine.value().getGraphicsQueueFamilyIndex()
			};

			commandPool = vk::raii::CommandPool(vulkanEngine.value().getDevice(), poolInfo);
		}

		void createTextureImage()
		{

			ImageCreateContext imageContext{
				.width = static_cast<uint32_t>(imageData.value().getWidth()),
				.height = static_cast<uint32_t>(imageData.value().getHeight()),
				.mipLevels = imageData.value().getMipLevels(),
				.format = vk::Format::eR8G8B8A8Srgb,
				.usage = vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
				.memoryProperties = vk::MemoryPropertyFlagBits::eDeviceLocal,
				.aspects = vk::ImageAspectFlagBits::eColor,
				.tiling = vk::ImageTiling::eOptimal
			};

			Buffer stagingBuffer(
				vulkanEngine.value(),
				imageData.value().size(),
				vk::BufferUsageFlagBits::eTransferSrc,
				vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

			stagingBuffer.mapData(imageData.value().data(), imageData.value().size(), 0);

			vk::raii::CommandBuffer commandBuffer = beginSingleTimeCommands();
			textureImage.emplace(vulkanEngine.value(), commandBuffer, imageContext, stagingBuffer);
			endSingleTimeCommands(std::move(commandBuffer));
		}

		void loadModel() {
			tinyobj::attrib_t					attrib;
			std::vector<tinyobj::shape_t>		shapes;
			std::vector<tinyobj::material_t>	materials;
			std::string							warn, err;

			if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
				throw std::runtime_error(warn + err);
			}

			std::unordered_map<Vertex, uint32_t> uniqueVertices{};

			for (auto& shape : shapes)
			{
				for (const auto& index : shape.mesh.indices)
				{
					Vertex vertex{};


					vertex.pos = {
						attrib.vertices[3 * index.vertex_index + 0],
						attrib.vertices[3 * index.vertex_index + 1],
						attrib.vertices[3 * index.vertex_index + 2],
					};
					
					vertex.texCoord = {
						attrib.texcoords[2 * index.texcoord_index + 0],
						1.0f - attrib.texcoords[2 * index.texcoord_index + 1],
					};

					vertex.col = { 1.0f, 1.0f, 1.0f };

					auto [it, inserted] = uniqueVertices.insert({ vertex, static_cast<uint32_t>(vertices.size()) });

					if (inserted)
						vertices.push_back(vertex);
					indices.push_back(it->second);
				}

			}
		}

		void createVertexBuffer() {
			vk::DeviceSize size = sizeof(vertices[0]) * vertices.size();

			Buffer stagingBuffer(vulkanEngine.value(), size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
			stagingBuffer.mapData(vertices.data(), size, 0);

			vertexBuffer.emplace(vulkanEngine.value(), size, vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal);

			copyBuffer(stagingBuffer, *vertexBuffer, size);
		}

		void createIndexBuffer() {
			vk::DeviceSize size = sizeof(indices[0]) * indices.size();

			Buffer stagingBuffer(vulkanEngine.value(), size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
			stagingBuffer.mapData(indices.data(), size, 0);

			indexBuffer.emplace(vulkanEngine.value(), size, vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal, vk::IndexTypeValue<decltype(indices)::value_type>::value);

			copyBuffer(stagingBuffer, *indexBuffer, size);
		}

		void createUniformBuffers() {
			for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
			{
				uniformBuffers.emplace_back(vulkanEngine.value(), sizeof(UniformBufferObject));
			};
		}

		void createDescriptorPool()
		{
			std::vector<vk::DescriptorPoolSize> poolSize{ 
				{.type = vk::DescriptorType::eUniformBuffer, .descriptorCount = MAX_FRAMES_IN_FLIGHT},
				{.type = vk::DescriptorType::eCombinedImageSampler, .descriptorCount = MAX_FRAMES_IN_FLIGHT}
			};
			descriptorPool.emplace(vulkanEngine.value(), MAX_FRAMES_IN_FLIGHT, poolSize);
		}

		void createDescriptorSets()
		{
			std::vector<DescriptorSet> sets;

			for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
			{
				sets.emplace_back(
					vulkanEngine.value(),
					descriptorSetLayout.value(),
					descriptorPool.value()
				);

				sets[i].writeUniformBuffer(0, uniformBuffers[i]);
				sets[i].writeCombinedImageSampler(1, textureImage.value());

			}

			descriptorSets = std::move(sets);
		}

		void createFrameData()
		{
			for (size_t i = 0; i < swapchain.value().getSwapchainImages().size(); ++i)
			{
				renderFinishedSemaphores.emplace_back(vulkanEngine.value().getDevice(), vk::SemaphoreCreateInfo());
			}

			frames.resize(MAX_FRAMES_IN_FLIGHT);

			vk::CommandBufferAllocateInfo allocInfo{
				.commandPool = commandPool,
				.level = vk::CommandBufferLevel::ePrimary,
				.commandBufferCount = MAX_FRAMES_IN_FLIGHT
			};

			std::vector<vk::raii::CommandBuffer> commandBuffers = vk::raii::CommandBuffers(vulkanEngine.value().getDevice(), allocInfo);

			for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
			{
				frames[i].commandBuffer = std::move(commandBuffers[i]);
				frames[i].presentCompleteSemaphore = std::move(vk::raii::Semaphore(vulkanEngine.value().getDevice(), vk::SemaphoreCreateInfo()));
				frames[i].inFlightFence = std::move(vk::raii::Fence(vulkanEngine.value().getDevice(), vk::FenceCreateInfo{ .flags = vk::FenceCreateFlagBits::eSignaled }));
			}
		}

		void createDepthResources()
		{
			depthImage.emplace(vulkanEngine.value(), std::pair(window.value().GetPixelSize()));
		}

		void updateUniformBuffer(uint32_t currentImage)
		{
			static auto startTime = std::chrono::high_resolution_clock::now();

			auto currentTime = std::chrono::high_resolution_clock::now();
			float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

			UniformBufferObject ubo{};
			ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			ubo.view = glm::lookAt(glm::vec3(2.0f), glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			ubo.proj = glm::perspective(glm::radians(45.0f), static_cast<float>(swapchain.value().getSwapchainExtent().width) / static_cast<float>(swapchain.value().getSwapchainExtent().height), 0.1f, 10.0f);

			uniformBuffers[currentImage].update(&ubo, sizeof(UniformBufferObject), 0);
		}

		void loadImage()
		{
			imageData.emplace(TEXTURE_PATH.c_str());

			mipLevels = imageData.value().getMipLevels();
		}


		vk::raii::CommandBuffer beginSingleTimeCommands()
		{
			vk::CommandBufferAllocateInfo allocInfo{ 
				.commandPool = commandPool,
				.level = vk::CommandBufferLevel::ePrimary,
				.commandBufferCount = 1 
			};
			vk::raii::CommandBuffer commandBuffer = std::move(vk::raii::CommandBuffers(vulkanEngine.value().getDevice(), allocInfo).front());

			vk::CommandBufferBeginInfo beginInfo{ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
			commandBuffer.begin(beginInfo);

			return std::move(commandBuffer);

		}

		void endSingleTimeCommands(vk::raii::CommandBuffer&& commandBuffer)
		{
			commandBuffer.end();

			vk::SubmitInfo submitInfo{ .commandBufferCount = 1, .pCommandBuffers = &*commandBuffer };
			vulkanEngine.value().getQueue().submit(submitInfo);
			vulkanEngine.value().getQueue().waitIdle();
		}


		static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
			vk::DebugUtilsMessageSeverityFlagBitsEXT		severity,
			vk::DebugUtilsMessageTypeFlagsEXT				type,
			const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData)
		{
			std::cerr << "validation layer: type " << to_string(type) << " msg: " << pCallbackData->pMessage << std::endl;

			return vk::False;
		}

		void copyBuffer(Buffer& srcBuffer, Buffer& dstBuffer, vk::DeviceSize size)
		{
			vk::raii::CommandBuffer commandCopyBuffer = beginSingleTimeCommands();
			commandCopyBuffer.copyBuffer(srcBuffer.getBufferHandle(), dstBuffer.getBufferHandle(), vk::BufferCopy(0, 0, size));
			endSingleTimeCommands(std::move(commandCopyBuffer));
		}

		static std::vector<char> readFile(const std::string& filename)
		{
			std::ifstream file(filename, std::ios::ate | std::ios::binary);

			if (!file.is_open())
			{
				throw std::runtime_error("Failed to open file!");
			}

			std::vector<char> buffer(file.tellg());

			file.seekg(0, std::ios::beg);
			file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
			file.close();

			return buffer;
		}

		vk::raii::ShaderModule createShaderModule(const std::vector<char>& code) const
		{
			vk::ShaderModuleCreateInfo createInfo{ .codeSize = code.size() * sizeof(char), .pCode = reinterpret_cast<const uint32_t*>(code.data()) };
			vk::raii::ShaderModule shaderModule{ vulkanEngine.value().getDevice(), createInfo };
			return shaderModule;
		}

		void recordCommandBuffer(uint32_t imageIndex)
		{
			frames[frameIndex].commandBuffer.begin({});

			transition_image_layout(
				swapchain.value().getSwapchainImages()[imageIndex],
				vk::ImageLayout::eUndefined,
				vk::ImageLayout::eColorAttachmentOptimal,
				{},
				vk::AccessFlagBits2::eColorAttachmentWrite,
				vk::PipelineStageFlagBits2::eColorAttachmentOutput,
				vk::PipelineStageFlagBits2::eColorAttachmentOutput,
				vk::ImageAspectFlagBits::eColor,
				1
			);

			transition_image_layout(
				*depthImage.value().getImage(),
				vk::ImageLayout::eUndefined,
				vk::ImageLayout::eDepthAttachmentOptimal,
				vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
				vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
				vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
				vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
				vk::ImageAspectFlagBits::eDepth,
				1
			);

			vk::ClearValue	clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
			vk::ClearValue clearDepth = vk::ClearDepthStencilValue(1.0f, 0);

			vk::RenderingAttachmentInfo attachmentInfo = {
				.imageView = swapchain.value().getSwapchainImageViews()[imageIndex],
				.imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
				.loadOp = vk::AttachmentLoadOp::eClear,
				.storeOp = vk::AttachmentStoreOp::eStore,
				.clearValue = clearColor
			};

			vk::RenderingAttachmentInfo depthAttachmentInfo = {
				.imageView = depthImage.value().getDepthImageView(),
				.imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
				.loadOp = vk::AttachmentLoadOp::eClear,
				.storeOp = vk::AttachmentStoreOp::eDontCare,
				.clearValue = clearDepth
			};

			vk::RenderingInfo renderingInfo = {
				.renderArea = {.offset = {0, 0}, .extent = swapchain.value().getSwapchainExtent()},
				.layerCount = 1,
				.colorAttachmentCount = 1,
				.pColorAttachments = &attachmentInfo,
				.pDepthAttachment = &depthAttachmentInfo
			};

			frames[frameIndex].commandBuffer.beginRendering(renderingInfo);

			frames[frameIndex].commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline);
			vertexBuffer->bind(frames[frameIndex].commandBuffer);
			indexBuffer->bind(frames[frameIndex].commandBuffer);

			frames[frameIndex].commandBuffer.setViewport(0, vk::Viewport(0.0f, static_cast<float>(swapchain.value().getSwapchainExtent().height), static_cast<float>(swapchain.value().getSwapchainExtent().width), -static_cast<float>(swapchain.value().getSwapchainExtent().height), 0.0f, 1.0f));
			frames[frameIndex].commandBuffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapchain.value().getSwapchainExtent()));
			frames[frameIndex].commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, *descriptorSets[frameIndex].get(), nullptr);
			frames[frameIndex].commandBuffer.drawIndexed(static_cast<uint16_t>(indices.size()), 1, 0, 0, 0);

			frames[frameIndex].commandBuffer.endRendering();

			transition_image_layout(
				swapchain.value().getSwapchainImages()[imageIndex],
				vk::ImageLayout::eColorAttachmentOptimal,
				vk::ImageLayout::ePresentSrcKHR,
				vk::AccessFlagBits2::eColorAttachmentWrite,
				{},
				vk::PipelineStageFlagBits2::eColorAttachmentOutput,
				vk::PipelineStageFlagBits2::eBottomOfPipe,
				vk::ImageAspectFlagBits::eColor,
				1
			);

			frames[frameIndex].commandBuffer.end();
		}

		void transition_image_layout(
			const vk::Image&		image,
			vk::ImageLayout			oldLayout,
			vk::ImageLayout			newLayout,
			vk::AccessFlags2		src_access_mask,
			vk::AccessFlags2		dst_access_mask,
			vk::PipelineStageFlags2 src_stage_mask,
			vk::PipelineStageFlags2 dst_stage_mask,
			vk::ImageAspectFlags    image_aspect_flags,
			uint32_t				mipLevels
		)
		{
			vk::ImageMemoryBarrier2 barrier =
			{
				.srcStageMask = src_stage_mask,
				.srcAccessMask = src_access_mask,
				.dstStageMask = dst_stage_mask,
				.dstAccessMask = dst_access_mask,
				.oldLayout = oldLayout,
				.newLayout = newLayout,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.image = image,
				.subresourceRange = {
					.aspectMask = image_aspect_flags,
					.baseMipLevel = 0,
					.levelCount = mipLevels,
					.baseArrayLayer = 0,
					.layerCount = 1
				}
			};

			vk::DependencyInfo dependency_info = {
				.dependencyFlags = {},
				.imageMemoryBarrierCount = 1,
				.pImageMemoryBarriers = &barrier
			};

			frames[frameIndex].commandBuffer.pipelineBarrier2(dependency_info);
		}


	};
}
