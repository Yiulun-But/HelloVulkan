#pragma once

#include <vulkan/vulkan.hpp>

// #define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS  -> for C++20 or later compilers
// #define VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS -> if not defined in compile definitions

#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan.hpp;
#endif

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <map>
#include <fstream>
#include <optional>

// Parts to abstract away
#include "VertexBuffer.hpp"

namespace HelloVulkan {

	constexpr uint32_t WIDTH = 800;
	constexpr uint32_t HEIGHT = 600;

	constexpr int MAX_FRAMES_IN_FLIGHT = 2;

	const std::vector<char const*> validationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};

#ifdef NDEBUG
	constexpr bool enableValidationLayers = false;
#else
	constexpr bool enableValidationLayers = true;
#endif

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

		GLFWwindow* window = nullptr;

		vk::raii::Context					context;
		vk::raii::Instance					instance = nullptr;
		vk::raii::DebugUtilsMessengerEXT	debugMessenger = nullptr;
		vk::raii::PhysicalDevice			physicalDevice = nullptr;
		vk::raii::Device					device = nullptr;
		vk::raii::Queue						graphicsQueue = nullptr;
		vk::raii::SurfaceKHR				surface = nullptr;
		vk::raii::SwapchainKHR				swapChain = nullptr;
		vk::raii::PipelineLayout			pipelineLayout = nullptr;
		vk::raii::Pipeline					graphicsPipeline = nullptr;
		vk::raii::CommandPool				commandPool = nullptr;

		std::vector<vk::raii::CommandBuffer>			commandBuffers;
		std::vector<vk::raii::Semaphore>				presentCompleteSemaphores;
		std::vector<vk::raii::Semaphore>				renderFinishedSemaphores;
		std::vector<vk::raii::Fence>					inFlightFences;
		uint32_t frameIndex = 0;

		std::vector<vk::Image> swapChainImages;
		std::vector<vk::raii::ImageView> swapChainImageViews;
		vk::SurfaceFormatKHR swapChainSurfaceFormat;
		vk::Extent2D swapChainExtent;
		uint32_t queueIndex;

		std::optional<VertexBuffer> vertexBuffer;


		std::vector<const char*> requiredDeviceExtension = { vk::KHRSwapchainExtensionName };


		void initWindow() {
			glfwInit();
			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
			glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
			window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
			glfwSetWindowUserPointer(window, this);
			glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
		}

		void initVulkan() {
			createInstance();
			setupDebugMessenger();
			createSurface();
			pickPhysicalDevice();
			createLogicalDevice();
			createSwapChain();
			createImageViews();
			createGraphicsPipeline();
			createCommandPool();
			createVertexBuffer();
			createCommandBuffers();
			createSyncObjects();
		}

		void mainLoop() {
			while (!glfwWindowShouldClose(window)) {
				glfwPollEvents();
				drawFrame();
			}

			device.waitIdle();
		}

		void drawFrame()
		{
			auto fenceResult = device.waitForFences(*inFlightFences[frameIndex], vk::True, UINT64_MAX);
			if (fenceResult != vk::Result::eSuccess)
			{
				throw std::runtime_error("failed to wait for fence!");
			}

			auto [result, imageIndex] = swapChain.acquireNextImage(UINT64_MAX, *presentCompleteSemaphores[frameIndex], nullptr);

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

			device.resetFences(*inFlightFences[frameIndex]);

			recordCommandBuffer(imageIndex);

			vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
			const vk::SubmitInfo submitInfo{
				.waitSemaphoreCount = 1,
				.pWaitSemaphores = &*presentCompleteSemaphores[frameIndex],
				.pWaitDstStageMask = &waitDestinationStageMask,
				.commandBufferCount = 1,
				.pCommandBuffers = &*commandBuffers[frameIndex],
				.signalSemaphoreCount = 1,
				.pSignalSemaphores = &*renderFinishedSemaphores[imageIndex]
			};

			graphicsQueue.submit(submitInfo, *inFlightFences[frameIndex]);


			const vk::PresentInfoKHR presentInfoKHR{
				.waitSemaphoreCount = 1,
				.pWaitSemaphores = &*renderFinishedSemaphores[imageIndex],
				.swapchainCount = 1,
				.pSwapchains = &*swapChain,
				.pImageIndices = &imageIndex,
				.pResults = nullptr
			};

			result = graphicsQueue.presentKHR(presentInfoKHR);

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
			cleanupSwapChain();

			glfwDestroyWindow(window);

			glfwTerminate();
		}

		void createInstance()
		{

			constexpr vk::ApplicationInfo appInfo{
				.pApplicationName = "Hello Triangle",
				.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
				.pEngineName = "No Engine",
				.engineVersion = VK_MAKE_VERSION(1, 0, 0),
				.apiVersion = vk::ApiVersion14
			};

			// Get the required layers
			std::vector<char const*> requiredLayers;
			if (enableValidationLayers)
			{
				requiredLayers.assign(validationLayers.begin(), validationLayers.end());
			}

			// Check layers supported
			auto layerProperties = context.enumerateInstanceLayerProperties();
			auto unsupportedLayerIt =
				std::ranges::find_if(requiredLayers,
					[&layerProperties](auto const& requiredLayer) {
						return std::ranges::none_of(layerProperties,
							[requiredLayer](auto const& layerProperty) {
								return strcmp(layerProperty.layerName, requiredLayer) == 0;
							});
					});
			if (unsupportedLayerIt != requiredLayers.end())
			{
				throw std::runtime_error("Required layer not supported: " + std::string(*unsupportedLayerIt));
			}

			auto requiredExtensions = getRequiredInstanceExtensions();

			auto extensionProperties = context.enumerateInstanceExtensionProperties();
			auto unsupportedPropertyIt =
				std::ranges::find_if(requiredExtensions,
					[&extensionProperties](auto const& requiredExtension) {
						return std::ranges::none_of(extensionProperties,
							[requiredExtension](auto const& extensionProperty) {
								return strcmp(extensionProperty.extensionName, requiredExtension) == 0;
							});
					});
			if (unsupportedPropertyIt != requiredExtensions.end())
			{
				throw std::runtime_error("Required extension not supported: " + std::string(*unsupportedPropertyIt));
			}

			vk::InstanceCreateInfo createInfo{
				.pApplicationInfo = &appInfo,
				.enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
				.ppEnabledLayerNames = requiredLayers.data(),
				.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
				.ppEnabledExtensionNames = requiredExtensions.data()
			};

			instance = vk::raii::Instance(context, createInfo);
		}

		void setupDebugMessenger()
		{
			if (!enableValidationLayers) return;

			vk::DebugUtilsMessageSeverityFlagsEXT	severityFlags(
				vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
				vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
			vk::DebugUtilsMessageTypeFlagsEXT		messageTypeFlags(
				vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
			vk::DebugUtilsMessengerCreateInfoEXT	debugUtilsMessengerCreateInfoEXT{
				.messageSeverity = severityFlags,
				.messageType = messageTypeFlags,
				.pfnUserCallback = &debugCallback };

			debugMessenger = instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);

		}

		void createSurface()
		{
			VkSurfaceKHR	_surface;
			if (glfwCreateWindowSurface(*instance, window, nullptr, &_surface) != 0) {
				throw std::runtime_error("failed to create window surface!");
			}
			surface = vk::raii::SurfaceKHR(instance, _surface);
		}

		void pickPhysicalDevice()
		{
			auto physicalDevices = vk::raii::PhysicalDevices(instance);
			if (physicalDevices.empty())
			{
				throw std::runtime_error("failed to find GPUs with Vulkan support!");
			}

			// Use an ordered map to automatically sort condidates by increasing score
			std::multimap<int, vk::raii::PhysicalDevice> candidates;

			for (const auto& pd : physicalDevices)
			{
				auto deviceProperties = pd.getProperties();
				auto deviceFeatures = pd.getFeatures();
				uint32_t score = 0;

				// Discrete GPUs have a significant performance advantage
				if (deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
				{
					score += 1000;
				}

				// Maximum possible size of texture affects 
				score += deviceProperties.limits.maxImageDimension2D;

				// Application don't function without geometry shaders
				if (!deviceFeatures.geometryShader)
				{
					continue;
				}
				candidates.insert(std::make_pair(score, pd));
			}

			// Check if the best candidate is suitable at all
			if (!candidates.empty() && candidates.rbegin()->first > 0 && isDeviceSuitable(candidates.rbegin()->second))
			{
				physicalDevice = candidates.rbegin()->second;
			}
			else
			{
				throw std::runtime_error("failed to find a suitable GPU!");
			}

		}

		void createLogicalDevice()
		{
			std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
			queueIndex = ~0;
			for (uint32_t qfpIndex = 0; qfpIndex < queueFamilyProperties.size(); qfpIndex++)
			{
				if ((queueFamilyProperties[qfpIndex].queueFlags & vk::QueueFlagBits::eGraphics) &&
					physicalDevice.getSurfaceSupportKHR(qfpIndex, *surface))
				{
					queueIndex = qfpIndex;
					break;
				}
			}
			if (queueIndex == ~0)
			{
				throw std::runtime_error("Could not find queue for graphics and present -> terminating");
			}

			float queuePriority = 0.5f;
			vk::DeviceQueueCreateInfo deviceQueueCreateInfo{ .queueFamilyIndex = queueIndex, .queueCount = 1, .pQueuePriorities = &queuePriority };

			vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> featureChain = {
				{},
				{.synchronization2 = true, .dynamicRendering = true},
				{.extendedDynamicState = true }
			};

			vk::DeviceCreateInfo deviceCreateInfo{
				.pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
				.queueCreateInfoCount = 1,
				.pQueueCreateInfos = &deviceQueueCreateInfo,
				.enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtension.size()),
				.ppEnabledExtensionNames = requiredDeviceExtension.data()
			};

			device = vk::raii::Device(physicalDevice, deviceCreateInfo);
			graphicsQueue = vk::raii::Queue(device, queueIndex, 0);
		}

		void cleanupSwapChain()
		{
			swapChainImageViews.clear();
			swapChain = nullptr;
		}

		void recreateSwapChain()
		{
			int height = 0, width = 0;
			glfwGetFramebufferSize(window, &width, &height);
			while (width == 0 || height == 0) {
				glfwGetFramebufferSize(window, &width, &height);
				glfwWaitEvents();
			}

			device.waitIdle();

			cleanupSwapChain();

			createSwapChain();

			createImageViews();
		}

		void createSwapChain()
		{
			vk::SurfaceCapabilitiesKHR surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(*surface);
			swapChainExtent = chooseSwapExtent(surfaceCapabilities);
			uint32_t minImageCount = chooseSwapMinImageCount(surfaceCapabilities);

			std::vector<vk::SurfaceFormatKHR> availableFormats = physicalDevice.getSurfaceFormatsKHR(*surface);
			swapChainSurfaceFormat = chooseSwapSurfaceformat(availableFormats);

			std::vector<vk::PresentModeKHR> availablePresentModes = physicalDevice.getSurfacePresentModesKHR(*surface);

			vk::SwapchainCreateInfoKHR swapChainCreateInfo{
				.surface = *surface,
				.minImageCount = minImageCount,
				.imageFormat = swapChainSurfaceFormat.format,
				.imageColorSpace = swapChainSurfaceFormat.colorSpace,
				.imageExtent = swapChainExtent,
				.imageArrayLayers = 1,
				.imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
				.imageSharingMode = vk::SharingMode::eExclusive,
				.preTransform = surfaceCapabilities.currentTransform,
				.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
				.presentMode = chooseSwapPresentMode(availablePresentModes),
				.clipped = true,
				.oldSwapchain = nullptr
			};

			swapChain = vk::raii::SwapchainKHR(device, swapChainCreateInfo);
			swapChainImages = swapChain.getImages();
		}

		void createImageViews()
		{
			assert(swapChainImageViews.empty());

			vk::ImageViewCreateInfo imageViewCreateInfo{
				.viewType = vk::ImageViewType::e2D,
				.format = swapChainSurfaceFormat.format,
				.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 }
			};

			imageViewCreateInfo.components = {
				vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity
			};

			imageViewCreateInfo.subresourceRange = {
				.aspectMask = vk::ImageAspectFlagBits::eColor, .levelCount = 1, .layerCount = 1
			};

			for (auto& image : swapChainImages)
			{
				imageViewCreateInfo.image = image;
				swapChainImageViews.emplace_back(device, imageViewCreateInfo);
			}
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
			auto bindingDescription = VertexBuffer::Vertex::getBindingDescription();
			auto attributeDescriptions = VertexBuffer::Vertex::getAttributeDescriptions();
			vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
				.vertexBindingDescriptionCount = 1,
				.pVertexBindingDescriptions = &bindingDescription,
				.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
				.pVertexAttributeDescriptions = attributeDescriptions.data()
			};

			vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo{ .topology = vk::PrimitiveTopology::eTriangleList };

			// For pipeline without dynamic state feature
			vk::Viewport viewport{ 0.0f, 0.0f, static_cast<float>(swapChainExtent.width), static_cast<float>(swapChainExtent.height), 0.0f, 1.0f };
			vk::Rect2D scissor{ vk::Offset2D{0, 0}, swapChainExtent };
			// vk::PipelineViewportStateCreateInfo viewportState{ .viewportCount = 1, .pViewports = &viewport, .scissorCount = 1, .pScissors = &scissor };

			vk::PipelineViewportStateCreateInfo viewportState{ .viewportCount = 1, .scissorCount = 1 };

			vk::PipelineRasterizationStateCreateInfo rasterizer{
				.depthClampEnable = vk::False,
				.rasterizerDiscardEnable = vk::False,
				.polygonMode = vk::PolygonMode::eFill,
				.cullMode = vk::CullModeFlagBits::eBack,
				.frontFace = vk::FrontFace::eClockwise,
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

			vk::PipelineLayoutCreateInfo pipelineLayoutInfo{ .setLayoutCount = 0, .pushConstantRangeCount = 0 };
			pipelineLayout = vk::raii::PipelineLayout(device, pipelineLayoutInfo);

			vk::PipelineRenderingCreateInfo pipelineRenderingCreateinfo{ .colorAttachmentCount = 1, .pColorAttachmentFormats = &swapChainSurfaceFormat.format };

			vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipelineCreateInfoChain = {
			{
				.stageCount = 2,
				.pStages = shaderStageInfo,
				.pVertexInputState = &vertexInputInfo,
				.pInputAssemblyState = &inputAssemblyInfo,
				.pViewportState = &viewportState,
				.pRasterizationState = &rasterizer,
				.pMultisampleState = &multisampling,
				.pColorBlendState = &colorBlending,
				.pDynamicState = &dynamicState,
				.layout = pipelineLayout,
				.renderPass = nullptr},
			{.colorAttachmentCount = 1, .pColorAttachmentFormats = &swapChainSurfaceFormat.format} };

			graphicsPipeline = vk::raii::Pipeline(device, nullptr, pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>());
		}


		void createCommandPool()
		{
			vk::CommandPoolCreateInfo poolInfo{
				.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
				.queueFamilyIndex = queueIndex
			};

			commandPool = vk::raii::CommandPool(device, poolInfo);
		}

		void createVertexBuffer() {
			vertexBuffer.emplace(device, physicalDevice);
		}

		void createCommandBuffers()
		{
			vk::CommandBufferAllocateInfo allocInfo{
				.commandPool = commandPool,
				.level = vk::CommandBufferLevel::ePrimary,
				.commandBufferCount = MAX_FRAMES_IN_FLIGHT
			};

			commandBuffers = std::move(vk::raii::CommandBuffers(device, allocInfo));
		}

		void createSyncObjects()
		{
			assert(presentCompleteSemaphores.empty() && renderFinishedSemaphores.empty() && inFlightFences.empty());

			for (size_t i = 0; i < swapChainImages.size(); ++i)
			{
				renderFinishedSemaphores.emplace_back(device, vk::SemaphoreCreateInfo());
			}

			for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
			{
				presentCompleteSemaphores.emplace_back(device, vk::SemaphoreCreateInfo());
				inFlightFences.emplace_back(device, vk::FenceCreateInfo{ .flags = vk::FenceCreateFlagBits::eSignaled });
			}
		}


		std::vector<const char*> getRequiredInstanceExtensions()
		{
			uint32_t glfwExtensionCount = 0;
			auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

			std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
			if (enableValidationLayers)
			{
				extensions.push_back(vk::EXTDebugUtilsExtensionName);
			}

			return extensions;
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

		bool isDeviceSuitable(vk::raii::PhysicalDevice const& physicalDevice)
		{
			bool supportsVulkan1_3 = physicalDevice.getProperties().apiVersion >= vk::ApiVersion13;

			auto queueFamilies = physicalDevice.getQueueFamilyProperties();
			bool supportsGraphics = std::ranges::any_of(queueFamilies, [](auto const& qfp) { return !!(qfp.queueFlags & vk::QueueFlagBits::eGraphics); });

			auto availableDeviceExtensions = physicalDevice.enumerateDeviceExtensionProperties();
			bool supportsAllRequiredExtensions =
				std::ranges::all_of(requiredDeviceExtension,
					[&availableDeviceExtensions](auto const& requiredDeviceExtension)
					{
						return std::ranges::any_of(availableDeviceExtensions,
							[requiredDeviceExtension](auto const& availableDeviceExtension)
							{ return strcmp(availableDeviceExtension.extensionName, requiredDeviceExtension) == 0; });
					});

			auto features =
				physicalDevice
				.template getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
			bool supportsRequiredFeatures = features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
				features.template get<vk::PhysicalDeviceVulkan13Features>().synchronization2 &&
				features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;

			return supportsVulkan1_3 && supportsGraphics && supportsAllRequiredExtensions && supportsRequiredFeatures;
		}

		vk::SurfaceFormatKHR chooseSwapSurfaceformat(std::vector<vk::SurfaceFormatKHR> const& availableFormats)
		{
			assert(!availableFormats.empty());
			const auto formatIt = std::ranges::find_if(
				availableFormats,
				[](const auto& format) { return format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;  }
			);
			return formatIt != availableFormats.end() ? *formatIt : availableFormats[0];
		}

		vk::PresentModeKHR chooseSwapPresentMode(std::vector<vk::PresentModeKHR> const& availablePresentModes)
		{
			assert(std::ranges::any_of(availablePresentModes, [](auto presentMode) {return presentMode == vk::PresentModeKHR::eFifo; }));
			return std::ranges::any_of(availablePresentModes,
				[](const vk::PresentModeKHR value) {return vk::PresentModeKHR::eMailbox == value; }) ?
				vk::PresentModeKHR::eMailbox :
				vk::PresentModeKHR::eFifo;
		}

		vk::Extent2D chooseSwapExtent(vk::SurfaceCapabilitiesKHR const& capabilities)
		{
			if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
			{
				return capabilities.currentExtent;
			}
			int width, height;
			glfwGetFramebufferSize(window, &width, &height);

			return {
				std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
				std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
			};
		}

		uint32_t chooseSwapMinImageCount(vk::SurfaceCapabilitiesKHR const& surfaceCapabilities)
		{
			auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);
			if ((0 < surfaceCapabilities.maxImageCount) && (minImageCount > surfaceCapabilities.maxImageCount))
			{
				minImageCount = surfaceCapabilities.maxImageCount;
			}
			return minImageCount;
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

		[[nodiscard]] vk::raii::ShaderModule createShaderModule(const std::vector<char>& code) const
		{
			vk::ShaderModuleCreateInfo createInfo{ .codeSize = code.size() * sizeof(char), .pCode = reinterpret_cast<const uint32_t*>(code.data()) };
			vk::raii::ShaderModule shaderModule{ device, createInfo };
			return shaderModule;
		}

		void recordCommandBuffer(uint32_t imageIndex)
		{
			commandBuffers[frameIndex].begin({});

			transition_image_layout(
				imageIndex,
				vk::ImageLayout::eUndefined,
				vk::ImageLayout::eColorAttachmentOptimal,
				{},
				vk::AccessFlagBits2::eColorAttachmentWrite,
				vk::PipelineStageFlagBits2::eColorAttachmentOutput,
				vk::PipelineStageFlagBits2::eColorAttachmentOutput
			);

			vk::ClearValue	clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
			vk::RenderingAttachmentInfo attachmentInfo = {
				.imageView = swapChainImageViews[imageIndex],
				.imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
				.loadOp = vk::AttachmentLoadOp::eClear,
				.storeOp = vk::AttachmentStoreOp::eStore,
				.clearValue = clearColor
			};

			vk::RenderingInfo renderingInfo = {
				.renderArea = {.offset = {0, 0}, .extent = swapChainExtent},
				.layerCount = 1,
				.colorAttachmentCount = 1,
				.pColorAttachments = &attachmentInfo
			};

			commandBuffers[frameIndex].beginRendering(renderingInfo);

			commandBuffers[frameIndex].bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline);
			vertexBuffer->bind(commandBuffers[frameIndex]);

			commandBuffers[frameIndex].setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChainExtent.width), static_cast<float>(swapChainExtent.height)));
			commandBuffers[frameIndex].setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChainExtent));
			commandBuffers[frameIndex].draw(static_cast<uint32_t>(vertexBuffer->vertices.size()), 1, 0, 0);

			commandBuffers[frameIndex].endRendering();

			transition_image_layout(
				imageIndex,
				vk::ImageLayout::eColorAttachmentOptimal,
				vk::ImageLayout::ePresentSrcKHR,
				vk::AccessFlagBits2::eColorAttachmentWrite,
				{},
				vk::PipelineStageFlagBits2::eColorAttachmentOutput,
				vk::PipelineStageFlagBits2::eBottomOfPipe
			);

			commandBuffers[frameIndex].end();
		}

		void transition_image_layout(
			uint32_t				imageIndex,
			vk::ImageLayout			oldLayout,
			vk::ImageLayout			newLayout,
			vk::AccessFlags2		src_access_mask,
			vk::AccessFlags2		dst_access_mask,
			vk::PipelineStageFlags2 src_stage_mask,
			vk::PipelineStageFlags2 dst_stage_mask
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
				.image = swapChainImages[imageIndex],
				.subresourceRange = {
					.aspectMask = vk::ImageAspectFlagBits::eColor,
					.baseMipLevel = 0,
					.levelCount = 1,
					.baseArrayLayer = 0,
					.layerCount = 1
				}
			};

			vk::DependencyInfo dependency_info = {
				.dependencyFlags = {},
				.imageMemoryBarrierCount = 1,
				.pImageMemoryBarriers = &barrier
			};

			commandBuffers[frameIndex].pipelineBarrier2(dependency_info);
		}

		static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
		{
			auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
			app->framebufferResized = true;
		}
	};
}
