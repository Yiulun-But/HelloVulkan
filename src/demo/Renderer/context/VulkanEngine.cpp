#include "VulkanEngine.h"

#include <stdexcept>
#include <map>
#include <algorithm>
#include <iostream>

namespace HelloVulkan {

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
		const std::vector<const char*>& requiredInstanceExtension
	)
	{
		createInstance(validationLayers, requiredInstanceExtension);
		setupDebugMessenger();
	}

	void VulkanEngine::createInstance(
		const std::vector<char const*>& validationLayers,
		const std::vector<const char*>& requiredInstanceExtension
	)
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

		auto extensionProperties = context.enumerateInstanceExtensionProperties();
		auto unsupportedPropertyIt =
			std::ranges::find_if(requiredInstanceExtension,
				[&extensionProperties](auto const& requiredExtension) {
					return std::ranges::none_of(extensionProperties,
						[requiredExtension](auto const& extensionProperty) {
							return strcmp(extensionProperty.extensionName, requiredExtension) == 0;
						});
				});
		if (unsupportedPropertyIt != requiredInstanceExtension.end())
		{
			throw std::runtime_error("Required extension not supported: " + std::string(*unsupportedPropertyIt));
		}

		vk::InstanceCreateInfo createInfo{
			.pApplicationInfo = &appInfo,
			.enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
			.ppEnabledLayerNames = requiredLayers.data(),
			.enabledExtensionCount = static_cast<uint32_t>(requiredInstanceExtension.size()),
			.ppEnabledExtensionNames = requiredInstanceExtension.data()
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

}