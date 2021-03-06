#version 450

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec2 inUV;

layout (binding = 1) uniform sampler2D tex;


layout (binding = 0) uniform UBO
{
	mat4 projection;
	mat4 modelview;
	float time;
} ubo;

layout (location = 0) out vec4 outColor;


void main() {
	vec4 color = texture(tex, vec2(inUV.x, 1.0 - inUV.y));
//	color = vec4(1);

	vec3 N = (inNormal + vec3(1)) * 0.5;

//	outColor = vec4(inNormal.xyz, 1.0);
	outColor = color * inNormal.z;

}
