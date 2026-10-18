#pragma once

#include "renderer/context/VulkanEngine.h"

namespace HelloVulkan {
	class DescriptorPool {
    public:
        DescriptorPool(
            VulkanEngine& engine,
            uint32_t maxSets,
            const std::vector<vk::DescriptorPoolSize>& poolSizes
        );

        vk::raii::DescriptorPool& get()
        {
            return pool;
        }

    private:
        vk::raii::DescriptorPool pool = nullptr;
	};
}