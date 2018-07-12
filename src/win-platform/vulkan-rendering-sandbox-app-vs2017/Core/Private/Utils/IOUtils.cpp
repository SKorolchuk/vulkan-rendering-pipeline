#include "../../Public/Utils/IOUtils.hpp"

std::vector<char> ShaderExtensions::ReadShaderFile(const std::string& shaderFileName)
{
	std::ifstream shaderFile(shaderFileName, std::ios::ate | std::ios::binary);

	if (!shaderFile.is_open())
	{
		throw std::runtime_error("shader file" + shaderFileName + " not found");
	}

	size_t fileSize = static_cast<size_t>(shaderFile.tellg());

	std::vector<char> buffer(fileSize);

	shaderFile.seekg(0);
	shaderFile.read(buffer.data(), fileSize);

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
