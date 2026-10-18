#include "DescriptorPool.h"

namespace HelloVulkan {
	DescriptorPool::DescriptorPool(
		VulkanEngine& engine, 
		uint32_t maxSets, 
		const std::vector<vk::DescriptorPoolSize>& poolSizes
	)
	{
		vk::DescriptorPoolCreateInfo poolInfo{ 
			.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 
			.maxSets = maxSets, .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
			.pPoolSizes = poolSizes.data()
		};

		pool = std::move(vk::raii::DescriptorPool(engine.getDevice(), poolInfo));
	}
}