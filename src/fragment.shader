#version 450

layout (location = 0) out vec4 ColourOut;

layout (binding = 0) uniform UBO
{
	float time;
} ubo;


void main() {
	ColourOut = vec4( 0.0, mod(ubo.time, 1.0), 1.0, 1.0 );
}