#include <vector>
#include <array>
#include <stdexcept>
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

namespace HelloVulkan {
	struct Vertex
	{
		glm::vec3 pos;
		glm::vec3 col;
		glm::vec2 texCoord;

		bool operator==(const Vertex& other) const {
			return (pos == other.pos) && (col == other.col) && (texCoord == other.texCoord);
		}

		static vk::VertexInputBindingDescription getBindingDescription() {
			return { .binding = 0, .stride = sizeof(Vertex), .inputRate = vk::VertexInputRate::eVertex };
		}

		static std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescriptions() {
			return { {
				{.location = 0, .binding = 0, .format = vk::Format::eR32G32B32Sfloat,	.offset = offsetof(Vertex, pos) },
				{.location = 1, .binding = 0, .format = vk::Format::eR32G32B32Sfloat,	.offset = offsetof(Vertex, col) },
				{.location = 2, .binding = 0, .format = vk::Format::eR32G32Sfloat,			.offset = offsetof(Vertex, texCoord) }
			} };
		}
	};

	struct UniformBufferObject {
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
	};


}

namespace std
{
	template<> struct hash<HelloVulkan::Vertex>
	{
		size_t operator()(HelloVulkan::Vertex const& vertex) const
		{
			return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.col) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
}