#version 450

void main() {
	vec2 p[4] = vec2[4]( vec2(-1.0, 1.0), vec2(1.0, 1.0),
				vec2(-1.0, -1.0), vec2(1.0, -1.0)
		);
	gl_Position = vec4( p[gl_VertexIndex], 0.0, 1.0 );
}