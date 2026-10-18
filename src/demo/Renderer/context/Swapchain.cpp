#include "Swapchain.h"

namespace HelloVulkan {

	static vk::SurfaceFormatKHR chooseSwapSurfaceformat(std::vector<vk::SurfaceFormatKHR> const& availableFormats)
	{
		assert(!availableFormats.empty());
		const auto formatIt = std::ranges::find_if(
			availableFormats,
			[](const auto& format) { return format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;  }
		);
		return formatIt != availableFormats.end() ? *formatIt : availableFormats[0];
	}

	static vk::PresentModeKHR chooseSwapPresentMode(std::vector<vk::PresentModeKHR> const& availablePresentModes)
	{
		assert(std::ranges::any_of(availablePresentModes, [](auto presentMode) {return presentMode == vk::PresentModeKHR::eFifo; }));
		return std::ranges::any_of(availablePresentModes,
			[](const vk::PresentModeKHR value) {return vk::PresentModeKHR::eMailbox == value; }) ?
			vk::PresentModeKHR::eMailbox :
			vk::PresentModeKHR::eFifo;
	}

	static vk::Extent2D chooseSwapExtent(vk::SurfaceCapabilitiesKHR const& capabilities, std::pair<int, int> pixelSize)
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			return capabilities.currentExtent;
		}

		auto [width, height] = pixelSize;

		return {
			std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
			std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
		};
	}

	static uint32_t chooseSwapMinImageCount(vk::SurfaceCapabilitiesKHR const& surfaceCapabilities)
	{
		auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);
		if ((0 < surfaceCapabilities.maxImageCount) && (minImageCount > surfaceCapabilities.maxImageCount))
		{
			minImageCount = surfaceCapabilities.maxImageCount;
		}
		return minImageCount;
	}

	Swapchain::Swapchain(VulkanEngine& vulkanEngine, std::pair<int, int> pixelSize)
	{
		createSwapchain(vulkanEngine, pixelSize);
		createImageViews(vulkanEngine);
	}


	void Swapchain::recreate(VulkanEngine& vulkanEngine, std::pair<int, int> pixelSize)
	{
		swapchainImageViews.clear();
		swapchainImages.clear();
		swapchain = nullptr;

		createSwapchain(vulkanEngine, pixelSize);
		createImageViews(vulkanEngine);
	}

	void Swapchain::createSwapchain(VulkanEngine& vulkanEngine, std::pair<int, int> pixelSize)
	{
		vk::SurfaceCapabilitiesKHR surfaceCapabilities = vulkanEngine.getPhysicalDevice().getSurfaceCapabilitiesKHR(*vulkanEngine.getSurface());
		swapchainExtent = chooseSwapExtent(surfaceCapabilities, pixelSize);
		uint32_t minImageCount = chooseSwapMinImageCount(surfaceCapabilities);

		std::vector<vk::SurfaceFormatKHR> availableFormats = vulkanEngine.getPhysicalDevice().getSurfaceFormatsKHR(*vulkanEngine.getSurface());
		swapchainSurfaceFormat = chooseSwapSurfaceformat(availableFormats);

		std::vector<vk::PresentModeKHR> availablePresentModes = vulkanEngine.getPhysicalDevice().getSurfacePresentModesKHR(*vulkanEngine.getSurface());

		vk::SwapchainCreateInfoKHR swapChainCreateInfo{
			.surface = *vulkanEngine.getSurface(),
			.minImageCount = minImageCount,
			.imageFormat = swapchainSurfaceFormat.format,
			.imageColorSpace = swapchainSurfaceFormat.colorSpace,
			.imageExtent = swapchainExtent,
			.imageArrayLayers = 1,
			.imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
			.imageSharingMode = vk::SharingMode::eExclusive,
			.preTransform = surfaceCapabilities.currentTransform,
			.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
			.presentMode = chooseSwapPresentMode(availablePresentModes),
			.clipped = true,
			.oldSwapchain = nullptr
		};

		swapchain = vk::raii::SwapchainKHR(vulkanEngine.getDevice(), swapChainCreateInfo);
		swapchainImages = swapchain.getImages();
	}

	void Swapchain::createImageViews(VulkanEngine& vulkanEngine)
	{
		assert(swapchainImageViews.empty());

		vk::ImageViewCreateInfo imageViewCreateInfo{
			.viewType = vk::ImageViewType::e2D,
			.format = swapchainSurfaceFormat.format,
			.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 }
		};

		imageViewCreateInfo.components = {
			vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity
		};

		imageViewCreateInfo.subresourceRange = {
			.aspectMask = vk::ImageAspectFlagBits::eColor, .levelCount = 1, .layerCount = 1
		};

		for (auto& image : swapchainImages)
		{
			imageViewCreateInfo.image = image;
			swapchainImageViews.emplace_back(vulkanEngine.getDevice(), imageViewCreateInfo);
		}
	}
}