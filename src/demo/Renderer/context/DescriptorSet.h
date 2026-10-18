#pragma once

#include "renderer/context/VulkanEngine.h"
#include "renderer/context/DescriptorSetLayout.h"
#include "renderer/context/DescriptorPool.h"
#include "renderer/buffers/UniformBuffer.h"
#include "renderer/texture/TextureImage.h"

namespace HelloVulkan{
	class DescriptorSet {
	public:
		DescriptorSet(
			VulkanEngine& engine,
			DescriptorSetLayout& layout,
			DescriptorPool& pool
		);

		void writeUniformBuffer(
			uint32_t binding,
			UniformBuffer& buffer
		);

		void writeCombinedImageSampler(
			uint32_t binding,
			TextureImage& texture
		);

		vk::raii::DescriptorSet& get()
		{
			return descriptorSet;
		}

	private:
		VulkanEngine& engine;
		vk::raii::DescriptorSet descriptorSet = nullptr;
	};
}