#pragma once

class ShaderExtensions
{
public:
	static std::vector<char> ReadShaderFile(const std::string& fileName);
	static VkShaderModule CreateShaderModule(const VkDevice& device, const std::vector<char>& shaderText);
};