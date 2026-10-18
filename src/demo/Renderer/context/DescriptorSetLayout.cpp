#include "DescriptorSetLayout.h"

namespace HelloVulkan {

	void DescriptorSetLayout::addUniformBufferBinding(uint32_t binding, vk::ShaderStageFlags stages)
	{
		vk::DescriptorSetLayoutBinding uboLayoutBinding{
				.binding = binding, .descriptorType = vk::DescriptorType::eUniformBuffer, .descriptorCount = 1, .stageFlags = stages
		};
		bindings.push_back(uboLayoutBinding);
	}

	void DescriptorSetLayout::addCombinedImageSamplerBinding(uint32_t binding, vk::ShaderStageFlags stages)
	{
		vk::DescriptorSetLayoutBinding samplerLayoutBinding{
			.binding = binding, .descriptorType = vk::DescriptorType::eCombinedImageSampler, .descriptorCount = 1, .stageFlags = stages
		};
		bindings.push_back(samplerLayoutBinding);
	}

	void DescriptorSetLayout::Create(VulkanEngine& engine)
	{
		vk::DescriptorSetLayoutCreateInfo layoutInfo{
				.bindingCount = static_cast<uint32_t>(bindings.size()), .pBindings = bindings.data()
		};
		layout = vk::raii::DescriptorSetLayout(engine.getDevice(), layoutInfo);
	}


}
