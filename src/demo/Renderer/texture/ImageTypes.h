#pragma once

#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

#include <cstdint>

namespace HelloVulkan {
	struct ImageCreateContext 
	{
		std::uint32_t width;
		std::uint32_t height;

		vk::Format format{};
		vk::ImageUsageFlags usage{};
		vk::MemoryPropertyFlags memoryProperties{};
		vk::ImageAspectFlags aspects{};
		vk::ImageTiling tiling = vk::ImageTiling::eOptimal;
	};
}