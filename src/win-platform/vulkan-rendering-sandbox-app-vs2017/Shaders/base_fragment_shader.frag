#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in mat4 resultMat;
layout(location = 5) in vec2 fragTextureCoord;
layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(texSampler, fragTextureCoord * 0.75);
}
