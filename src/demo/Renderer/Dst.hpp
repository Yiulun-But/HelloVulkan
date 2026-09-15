#include <vector>
#include <array>
#include <stdexcept>
#include <glm/glm.hpp>

namespace HelloVulkan {
	struct Vertex
	{
		glm::vec4 pos;
		glm::vec4 col;

		static vk::VertexInputBindingDescription getBindingDescription() {
			return { .binding = 0, .stride = sizeof(Vertex), .inputRate = vk::VertexInputRate::eVertex };
		}

		static std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescriptions() {
			return { {
				{.location = 0, .binding = 0, .format = vk::Format::eR32G32B32A32Sfloat,	.offset = offsetof(Vertex, pos) },
				{.location = 1, .binding = 0, .format = vk::Format::eR32G32B32A32Sfloat,	.offset = offsetof(Vertex, col) }
			} };
		}
	};

	struct UniformBufferObject {
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
	};
}