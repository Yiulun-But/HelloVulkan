#pragma once 

#include "renderer/context/VulkanEngine.h"
#include "Image.h"

namespace HelloVulkan {

	class Buffer;

	class TextureImage : public Image{
	public:
		TextureImage(
			VulkanEngine& engine,
			vk::raii::CommandBuffer& commandBuffer,
			const ImageCreateContext& imageContext,
			Buffer& stagingBuffer
		);

		inline const vk::raii::ImageView& getTextureImageView() { return textureImageView; };
		inline const vk::raii::Sampler& getTextureSampler() { return textureSampler; };

	private:
		void generateMipmaps(
			vk::raii::CommandBuffer& commandBuffer,
			int32_t width,
			int32_t height,
			uint32_t mipLevels
		);

		void createTextureSampler(
			const vk::raii::Device& device, 
			const vk::raii::PhysicalDevice& physicalDevice
		);

		uint32_t width;
		uint32_t height;

		vk::raii::ImageView textureImageView = nullptr;
		vk::raii::Sampler textureSampler = nullptr;
	};
}