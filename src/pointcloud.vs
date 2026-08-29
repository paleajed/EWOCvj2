layout (location = 0) in vec3 Position;
layout (location = 1) in vec4 Color;

uniform mat4 MVP;
uniform float PointSize;

out vec4 Color0;

void main()
{
	gl_Position = MVP * vec4(Position, 1.0);
	gl_PointSize = PointSize;
	Color0 = Color;
}
