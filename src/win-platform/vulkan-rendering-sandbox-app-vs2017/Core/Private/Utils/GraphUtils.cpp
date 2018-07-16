#include "../../Public/Utils/GraphUtils.hpp"

std::array<VkVertexInputAttributeDescription, 2> Vertex::GetAttributeDescriptions()
{
	std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions = {};

	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(Vertex, position);

	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Vertex, color);

	return attributeDescriptions;
}

VkVertexInputBindingDescription Vertex::GetBindingDescription()
{
	VkVertexInputBindingDescription bindDescription = {};
	bindDescription.binding = 0;
	bindDescription.stride = sizeof(Vertex);
	bindDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return bindDescription;
}

std::vector<Vertex> Vertex::GetSampleVertexMatrix()
{
	return {
		{ { -1.0f, -0.5f },{ 1.0f, 0.0f, 0.0f } },
	{ { 1.0f, -1.0f },{ 0.0f, 1.0f, 0.0f } },
	{ { 1.0f, 1.0f },{ 0.0f, 0.0f, 1.0f } },
	{ { -1.0f, 1.0f },{ 1.0f, 1.0f, 1.0f } }
	};
}

std::vector<uint16_t> Vertex::GetSampleVertexIndices()
{
	return {
		0, 1, 2, 2, 3, 0
	};
}
