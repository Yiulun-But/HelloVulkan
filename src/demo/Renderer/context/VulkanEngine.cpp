#include "VulkanEngine.h"

#include <stdexcept>
#include <map>
#include <algorithm>
#include <iostream>
#include <utility>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>


namespace HelloVulkan {

	std::vector<const char*> requiredDeviceExtension = { vk::KHRSwapchainExtensionName };

	static bool isDeviceSuitable(vk::raii::PhysicalDevice const& physicalDevice)
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
		bool supportsRequiredFeatures =
			features.template get<vk::PhysicalDeviceFeatures2>().features.samplerAnisotropy &&
			features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
			features.template get<vk::PhysicalDeviceVulkan13Features>().synchronization2 &&
			features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;

		return supportsVulkan1_3 && supportsGraphics && supportsAllRequiredExtensions && supportsRequiredFeatures;
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

	VulkanEngine::VulkanEngine(
		const std::vector<char const*>& validationLayers,
		const std::vector<const char*>& requiredInstanceExtension,
		Window& window
	)
	{

		createInstance(validationLayers, requiredInstanceExtension);
		setupDebugMessenger();
		createSurface(window);
		pickPhysicalDevice();
		createLogicalDevice();
	}

	void VulkanEngine::createInstance(
		const std::vector<char const*>& validationLayers,
		const std::vector<const char*>& requiredInstanceExtension
	)
	{
		auto requiredExtensions = requiredInstanceExtension;
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

		if (enableValidationLayers)
		{
			requiredExtensions.push_back(
				vk::EXTDebugUtilsExtensionName
			);
		}

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

		instance = std::move(vk::raii::Instance(context, createInfo));
	}

	void VulkanEngine::setupDebugMessenger()
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

	void VulkanEngine::pickPhysicalDevice()
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

	void VulkanEngine::createLogicalDevice()
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
			{.features = {.samplerAnisotropy = true } },
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

	void VulkanEngine::createSurface(Window& window)
	{
		VkSurfaceKHR	_surface;
		if (!SDL_Vulkan_CreateSurface(window.GetNativeWindow(), *instance, nullptr, &_surface)) {
			throw std::runtime_error("failed to create window surface!");
		}
		surface = std::move(vk::raii::SurfaceKHR(instance, _surface));
	}




}