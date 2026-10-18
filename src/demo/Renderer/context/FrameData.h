#pragma once

#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

namespace HelloVulkan {
	struct FrameData {
		vk::raii::CommandBuffer commandBuffer = nullptr;
		vk::raii::Semaphore presentCompleteSemaphore = nullptr;
		vk::raii::Fence inFlightFence = nullptr;
	};
}