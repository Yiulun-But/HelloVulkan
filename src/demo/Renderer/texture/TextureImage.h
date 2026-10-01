#pragma once 

#include "Image.h"

namespace HelloVulkan {

	class Buffer;

	class TextureImage : public Image{
	public:
		TextureImage(
			const vk::raii::Device& device,
			const vk::raii::PhysicalDevice& physicalDevice,
			vk::raii::CommandBuffer& commandBuffer,
			const ImageCreateContext& imageContext,
			Buffer& stagingBuffer
		);

	private:
		uint32_t width;
		uint32_t height;
	};
}