#include "../../Public/Utils/IOUtils.hpp"

std::vector<char> ShaderExtensions::ReadShaderFile(const std::string& shaderFileName)
{
	std::ifstream shaderFile(shaderFileName, std::ios::ate | std::ios::binary);

	if (!shaderFile.is_open())
	{
		throw std::runtime_error("shader file" + shaderFileName + " not found");
	}

	const size_t fileSize = static_cast<size_t>(shaderFile.tellg());

	std::vector<char> buffer(fileSize + (4 - fileSize % 4));

	shaderFile.seekg(0);
	shaderFile.read(buffer.data(), fileSize);

	// unify shader length to be in power of 4
	//const uint8_t remainder = fileSize % 4;
	//if (buffer.empty() != 0)
	//{
	//	for (uint8_t index = 0; index <= remainder; ++index)
	//	{
	//		buffer.push_back(' ');
	//	}
	//}

	shaderFile.close();

	return buffer;
}

VkShaderModule ShaderExtensions::CreateShaderModule(const VkDevice& device, const std::vector<char>& shaderText)
{
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = shaderText.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderText.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create shader module");
	}

	return shaderModule;
}

stbi_uc* ShaderExtensions::CreateTextureImage(const char* filePath, int* textureWidth, int* textureHeight, int* textureChannels)
{
	stbi_uc* pixels = stbi_load(filePath, textureWidth, textureHeight, textureChannels, STBI_rgb_alpha);

	VkDeviceSize imageSize = *textureWidth * *textureHeight * *textureChannels;

	if (!pixels) {
		throw std::runtime_error("failed to load texture image");
	}

	return pixels;
}

void MeshExtensions::LoadModelToMemoryBuffer(
	const char* modelPath,
	VkDevice device,
	VkPhysicalDevice physicalDevice,
	VkCommandPool commandPool,
	VkQueue graphicsQueue,
	VkBuffer& vertexBuffer,
	VkDeviceMemory& vertexBufferMemory,
	VkBuffer& indexBuffer,
	VkDeviceMemory& indexBufferMemory)
{
	std::vector<Vertex> vertices; // = Vertex::GetSampleVertexMatrix();
	std::vector<uint32_t> indices; // = Vertex::GetSampleVertexIndices();

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, modelPath)) {
		throw std::runtime_error(err);
	}

	std::unordered_map<Vertex, uint32_t> uniqueVertices = {};

	for (const auto& shape : shapes) {
		for (const auto& index : shape.mesh.indices) {
			Vertex vertex = {};

			vertex.position = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			vertex.textureCoords = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
			};

			vertex.color = { 1.0f, 1.0f, 1.0f };

			if (uniqueVertices.count(vertex) == 0) {
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}

			vertices.push_back(vertex);
			indices.push_back(uniqueVertices[vertex]);
		}
	}

	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	MemoryUtils::CreateBuffer(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		device,
		physicalDevice,
		stagingBuffer,
		stagingBufferMemory);

	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
	vkUnmapMemory(device, stagingBufferMemory);

	MemoryUtils::CreateBuffer(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		device,
		physicalDevice,
		vertexBuffer,
		vertexBufferMemory);

	MemoryUtils::CopyBuffer(
		stagingBuffer,
		vertexBuffer,
		bufferSize,
		commandPool,
		device,
		graphicsQueue);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);

	bufferSize = sizeof(indices[0]) * indices.size();

	stagingBuffer = nullptr;
	stagingBufferMemory = nullptr;

	MemoryUtils::CreateBuffer(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		device,
		physicalDevice,
		stagingBuffer,
		stagingBufferMemory);

	data = nullptr;
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
	vkUnmapMemory(device, stagingBufferMemory);

	MemoryUtils::CreateBuffer(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		device,
		physicalDevice,
		indexBuffer,
		indexBufferMemory);

	MemoryUtils::CopyBuffer(
		stagingBuffer,
		indexBuffer,
		bufferSize,
		commandPool,
		device,
		graphicsQueue);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
}
