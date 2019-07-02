#version 330

layout (location = 0) in vec2 Position;
layout (location = 1) in vec2 TexCoord;

out vec2 TexCoord0;

uniform vec2 translation = vec2(0.0f, 0.0f);

void main()
{
   gl_Position = vec4(Position + translation, 1.0f, 1.0f);
   TexCoord0 = TexCoord;
};