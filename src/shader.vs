#version 430 core


layout (location = 0) in vec3 Position;
layout (location = 1) in vec2 TexCoord;

out vec2 TexCoord0;
out flat int Vertex0;


void main()
{
	gl_Position = vec4(Position, 1.0f);
	Vertex0 = gl_VertexID;
	TexCoord0 = TexCoord;
};