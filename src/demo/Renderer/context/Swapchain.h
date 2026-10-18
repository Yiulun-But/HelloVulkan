#pragma once

#include "VulkanEngine.h"

namespace HelloVulkan {
	class Swapchain {
	public:
		Swapchain(VulkanEngine& vulkanEngine, std::pair<int, int> pixelSize);

		inline vk::raii::SwapchainKHR& getSwapchain() { return swapchain; }
		inline vk::SurfaceFormatKHR& getSwapchainSurfaceFormat() { return swapchainSurfaceFormat; }
		inline vk::Extent2D& getSwapchainExtent() { return swapchainExtent; }
		inline std::vector<vk::Image>& getSwapchainImages() { return swapchainImages; }
		inline std::vector<vk::raii::ImageView>& getSwapchainImageViews() { return swapchainImageViews; }

		void recreate(VulkanEngine& vulkanEngine, std::pair<int, int> pixelSize);
	private:

		void createSwapchain(VulkanEngine& vulkanEngine, std::pair<int, int> pixelSize);
		void createImageViews(VulkanEngine& vulkanEngine);

		vk::raii::SwapchainKHR				swapchain = nullptr;

		std::vector<vk::Image> swapchainImages;
		std::vector<vk::raii::ImageView> swapchainImageViews;
		vk::SurfaceFormatKHR swapchainSurfaceFormat;
		vk::Extent2D swapchainExtent;
	};
}