#include "DescriptorSet.h"

namespace HelloVulkan {
	DescriptorSet::DescriptorSet(
		VulkanEngine& engine, 
		DescriptorSetLayout& layout, 
		DescriptorPool& pool) : engine(engine)
	{
        vk::DescriptorSetLayout rawLayout = *layout.get();

        vk::DescriptorSetAllocateInfo allocInfo{
            .descriptorPool = *pool.get(),
            .descriptorSetCount = 1,
            .pSetLayouts = &rawLayout
        };

        auto sets =
            engine.getDevice().allocateDescriptorSets(allocInfo);

        descriptorSet = std::move(sets.front());
	}

    void DescriptorSet::writeUniformBuffer(
        uint32_t binding,
        UniformBuffer& buffer)
    {
        vk::DescriptorBufferInfo bufferInfo{
            .buffer = buffer.getBuffer(),
            .offset = 0,
            .range = buffer.getSize()
        };

        vk::WriteDescriptorSet write{
            .dstSet = *descriptorSet,
            .dstBinding = binding,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .pBufferInfo = &bufferInfo
        };

        engine.getDevice().updateDescriptorSets(write, {});
    }

    void DescriptorSet::writeCombinedImageSampler(
        uint32_t binding,
        TextureImage& texture)
    {
        vk::DescriptorImageInfo imageInfo{
            .sampler = *texture.getTextureSampler(),
            .imageView = *texture.getTextureImageView(),
            .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
        };

        vk::WriteDescriptorSet write{
            .dstSet = *descriptorSet,
            .dstBinding = binding,
            .descriptorCount = 1,
            .descriptorType =
                vk::DescriptorType::eCombinedImageSampler,
            .pImageInfo = &imageInfo
        };

        engine.getDevice().updateDescriptorSets(write, {});
    }
}