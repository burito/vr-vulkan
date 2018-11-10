#version 450

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec2 inUV;

//layout (binding = 1) uniform sampler2D texture;


layout (location = 0) out vec4 outColor;


void main() {
//	vec4 color = texture(texture, inUV);

	vec3 N = normalize(inNormal) + vec3(1) * 0.5;

	outColor = vec4(N, 1);
}