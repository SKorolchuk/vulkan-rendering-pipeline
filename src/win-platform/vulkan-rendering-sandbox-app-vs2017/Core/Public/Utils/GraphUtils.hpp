#ifndef _GRAPH_UTILS_HPP_
#define	_GRAPH_UTILS_HPP_

#include <vulkan/vulkan.h>
#include <array>
#include <glm/glm.hpp>
#include <vector>

struct Vertex
{
	glm::vec2 position;
	glm::vec3 color;

	static VkVertexInputBindingDescription GetBindingDescription();
	static std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescriptions();
	static std::vector<Vertex> GetSampleVertexMatrix();
};

#endif