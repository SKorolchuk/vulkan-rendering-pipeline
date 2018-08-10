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
