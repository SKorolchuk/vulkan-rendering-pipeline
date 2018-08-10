#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
	mat4 model;
	mat4 view;
	mat4 projection;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTextureCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out mat4 resultMat;
layout(location = 5) out vec2 fragTextureCoord;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	resultMat = ubo.projection * ubo.view * ubo.model;

	gl_Position = resultMat * vec4(inPosition, 1.0);
	fragColor = inColor;
	fragTextureCoord = inTextureCoord;
}