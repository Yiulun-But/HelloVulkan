#pragma once

#include "renderer/context/VulkanEngine.h"

namespace HelloVulkan {
	class DescriptorSetLayout {
	public:
		void addUniformBufferBinding(
			uint32_t binding,
			vk::ShaderStageFlags stages
		);

		void addCombinedImageSamplerBinding(
			uint32_t binding,
			vk::ShaderStageFlags stages
		);

		void Create(VulkanEngine& engine);

		inline vk::raii::DescriptorSetLayout& get() {
			return layout;
		};

	private:
		std::vector<vk::DescriptorSetLayoutBinding> bindings;
		vk::raii::DescriptorSetLayout layout = nullptr;
	};
}