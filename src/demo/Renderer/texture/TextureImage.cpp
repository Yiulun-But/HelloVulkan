#include "TextureImage.h"

#include "renderer/buffers/Buffer.h"

namespace HelloVulkan {
	

	static void transitionImageLayout(const vk::raii::CommandBuffer& commandBuffer, vk::raii::Image& image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t mipLevels)
	{
		vk::ImageMemoryBarrier barrier{
			.oldLayout = oldLayout,
			.newLayout = newLayout,
			.srcQueueFamilyIndex = vk::QueueFamilyIgnored,
			.dstQueueFamilyIndex = vk::QueueFamilyIgnored,
			.image = image,
			.subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor, .levelCount = mipLevels, .layerCount = 1 }
		};
		vk::PipelineStageFlags sourceStage;
		vk::PipelineStageFlags destinationStage;

		if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal)
		{
			barrier.srcAccessMask = {};
			barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

			sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
			destinationStage = vk::PipelineStageFlagBits::eTransfer;
		}
		else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
		{
			barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
			barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

			sourceStage = vk::PipelineStageFlagBits::eTransfer;
			destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
		}
		else
		{
			throw std::invalid_argument("unsupported layout transition!");
		}
		commandBuffer.pipelineBarrier(sourceStage, destinationStage, {}, {}, nullptr, barrier);
	}

	static void copyBufferToImage(vk::raii::CommandBuffer& commandBuffer, const vk::raii::Buffer& buffer, vk::raii::Image& image, uint32_t width, uint32_t height)
	{
		vk::BufferImageCopy region{
			.bufferOffset = 0,
			.bufferRowLength = 0,
			.bufferImageHeight = 0,
			.imageSubresource = {.aspectMask = vk::ImageAspectFlagBits::eColor, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1 },
			.imageOffset = { 0, 0, 0 },
			.imageExtent = { width, height, 1 }
		};

		commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, region);
	}

	static void validateLinearBlitSupport(const vk::raii::PhysicalDevice& physicalDevice, vk::Format imageFormat) 
	{
		vk::FormatProperties formatProperties = physicalDevice.getFormatProperties(imageFormat);

		if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear))
		{
			throw std::runtime_error("texture image format does not support linear blitting!");
		}
	}


	TextureImage::TextureImage(
		VulkanEngine& engine,
		vk::raii::CommandBuffer& commandBuffer,
		const ImageCreateContext& imageContext,
		Buffer& stagingBuffer
	) : Image(engine, imageContext), width(imageContext.width), height(imageContext.height) {

		transitionImageLayout(commandBuffer, image, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, imageContext.mipLevels);
		copyBufferToImage(commandBuffer, stagingBuffer.getBuffer(), image, width, height);
		validateLinearBlitSupport(engine.getPhysicalDevice(), imageContext.format);
		generateMipmaps(commandBuffer, imageContext.width, imageContext.height, imageContext.mipLevels);
		textureImageView = createImageView(engine.getDevice(), image, imageContext);
		createTextureSampler(engine.getDevice(), engine.getPhysicalDevice());
	}

	void TextureImage::generateMipmaps(
		vk::raii::CommandBuffer& commandBuffer,
		int32_t width,
		int32_t height,
		uint32_t mipLevels
	) {

		vk::ImageMemoryBarrier barrier = { .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
										  .dstAccessMask = vk::AccessFlagBits::eTransferRead,
										  .oldLayout = vk::ImageLayout::eTransferDstOptimal,
										  .newLayout = vk::ImageLayout::eTransferSrcOptimal,
										  .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
										  .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
										  .image = image,
										  .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor, .levelCount = 1, .layerCount = 1} };

		int32_t mipWidth = width;
		int32_t mipHeight = height;

		for (uint32_t i = 1; i < mipLevels; i++)
		{
			barrier.subresourceRange.baseMipLevel = i - 1;
			barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
			barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
			barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
			barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

			commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, barrier);

			vk::ImageBlit blit = { 
				.srcSubresource = {.aspectMask = vk::ImageAspectFlagBits::eColor, .mipLevel = i - 1, .layerCount = 1},
				.srcOffsets = std::array<vk::Offset3D, 2>({{}, {mipWidth, mipHeight, 1}}),
				.dstSubresource = {.aspectMask = vk::ImageAspectFlagBits::eColor, .mipLevel = i, .layerCount = 1},
				.dstOffsets = std::array<vk::Offset3D, 2>({{}, {1 < mipWidth ? mipWidth / 2 : 1, 1 < mipHeight ? mipHeight / 2 : 1, 1}}) 
			};
			commandBuffer.blitImage(image, vk::ImageLayout::eTransferSrcOptimal, image, vk::ImageLayout::eTransferDstOptimal, blit, vk::Filter::eLinear);

			barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
			barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
			barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

			commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, barrier);

			if (1 < mipWidth)
			{
				mipWidth /= 2;
			}
			if (1 < mipHeight)
			{
				mipHeight /= 2;
			}
		}

		barrier.subresourceRange.baseMipLevel = mipLevels - 1;
		barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
		barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

		commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, barrier);
	}

	void TextureImage::createTextureSampler(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice)
	{
		vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();

		vk::SamplerCreateInfo samplerInfo{
			.magFilter = vk::Filter::eLinear,
			.minFilter = vk::Filter::eLinear,
			.mipmapMode = vk::SamplerMipmapMode::eLinear,
			.addressModeU = vk::SamplerAddressMode::eRepeat,
			.addressModeV = vk::SamplerAddressMode::eRepeat,
			.addressModeW = vk::SamplerAddressMode::eRepeat,
			.mipLodBias = 0.0f,
			.anisotropyEnable = vk::True,
			.maxAnisotropy = properties.limits.maxSamplerAnisotropy,
			.compareEnable = vk::False,
			.compareOp = vk::CompareOp::eAlways,
			.minLod = 0.0f,
			.maxLod = vk::LodClampNone
		};

		textureSampler = std::move(vk::raii::Sampler(device, samplerInfo));
	}



}