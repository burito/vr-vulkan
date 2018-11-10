#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;

layout (binding = 0) uniform UBO
{
	mat4 projection;
	mat4 modelview;
} ubo;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec2 outUV;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main() {

//	outNormal = inNormal;
	outUV = inUV;
	
	gl_Position = ubo.projection * ubo.modelview * vec4( inPos.xyz, 1.0 );
	outNormal = mat3(ubo.modelview) * inNormal;
}