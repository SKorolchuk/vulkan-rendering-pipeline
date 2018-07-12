#pragma once

struct Vertex
{
	glm::vec2 position;
	glm::vec3 color;

	static VkVertexInputBindingDescription GetBindingDescription();
	static std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescriptions();
	static std::vector<Vertex> GetSampleVertexMatrix();
};