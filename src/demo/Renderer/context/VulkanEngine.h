#pragma once

#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

namespace HelloVulkan {
	class VulkanEngine {
	public:
		VulkanEngine(
			const std::vector<char const*>& validationLayers,
			const std::vector<const char*>& requiredInstanceExtension
		);

		inline vk::raii::Context& getContext() { return context; }
		inline vk::raii::Instance& getInstance() { return instance; }
		inline vk::raii::DebugUtilsMessengerEXT& getDebugMessenger() { return debugMessenger; }
	private:

		void createInstance(
			const std::vector<char const*>& validationLayers,
			const std::vector<const char*>& requiredInstanceExtension
		);

		void setupDebugMessenger();

		vk::raii::Context context;
		vk::raii::Instance instance = nullptr;
		vk::raii::DebugUtilsMessengerEXT	debugMessenger = nullptr;

#ifdef NDEBUG
		constexpr static bool enableValidationLayers = false;
#else
		constexpr static bool enableValidationLayers = true;
#endif
	};
}