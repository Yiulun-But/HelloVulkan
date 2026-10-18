#pragma once

#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

#include "renderer/window/Window.h"


namespace HelloVulkan {
	class VulkanEngine {
	public:
		VulkanEngine(
			const std::vector<char const*>& validationLayers,
			const std::vector<const char*>& requiredInstanceExtension,
			Window& window
		);

		inline vk::raii::Context& getContext() { return context; }
		inline vk::raii::Instance& getInstance() { return instance; }
		inline vk::raii::PhysicalDevice& getPhysicalDevice() { return physicalDevice; }
		inline vk::raii::Device& getDevice() { return device; }
		inline vk::raii::Queue& getQueue() { return graphicsQueue; }
		inline vk::raii::SurfaceKHR& getSurface() { return surface; }
		inline vk::raii::DebugUtilsMessengerEXT& getDebugMessenger() { return debugMessenger; }

		inline const vk::raii::Context& getContext() const { return context; }
		inline const vk::raii::Instance& getInstance() const { return instance; }
		inline const vk::raii::PhysicalDevice& getPhysicalDevice() const { return physicalDevice; }
		inline const vk::raii::Device& getDevice() const { return device; }
		inline const vk::raii::Queue& getQueue() const { return graphicsQueue; }
		inline const vk::raii::SurfaceKHR& getSurface() const { return surface; }
		inline const vk::raii::DebugUtilsMessengerEXT& getDebugMessenger() const { return debugMessenger; }

		uint32_t getGraphicsQueueFamilyIndex() const { return queueIndex; }
	private:

		void createInstance(
			const std::vector<char const*>& validationLayers,
			const std::vector<const char*>& requiredInstanceExtension
		);

		void setupDebugMessenger();
		void pickPhysicalDevice();
		void createLogicalDevice();
		void createSurface(Window& window);

		vk::raii::Context context;
		vk::raii::Instance instance = nullptr;
		vk::raii::DebugUtilsMessengerEXT	debugMessenger = nullptr;
		vk::raii::SurfaceKHR				surface = nullptr;

		vk::raii::PhysicalDevice			physicalDevice = nullptr;
		vk::raii::Device					device = nullptr;
		vk::raii::Queue						graphicsQueue = nullptr;
		uint32_t queueIndex;

#ifdef NDEBUG
		constexpr static bool enableValidationLayers = false;
#else
		constexpr static bool enableValidationLayers = true;
#endif
	};
}