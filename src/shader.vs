layout (location = 0) in vec3 Position;
layout (location = 1) in vec2 TexCoord;
// (half width, half height, roundness, rounded-corner bitmask) of the box; per-vertex buffer in the batch
// VAO, constant vertex attribute for draw_direct(), unset (roundness 0, no rounding) everywhere else
layout (location = 2) in vec4 BoxRound;

out vec2 TexCoord0;
flat out int Vertex0;
out vec2 BoxQ;
flat out vec4 BoxRoundInfo;


void main()
{
	gl_Position = vec4(Position, 1.0f);
	Vertex0 = gl_VertexID;
	TexCoord0 = TexCoord;
	// corner of the quad in [-1,1]; quad vertex order is TL, BL, TR, BR
	BoxQ = vec2(((gl_VertexID & 2) != 0) ? 1.0f : -1.0f, ((gl_VertexID & 1) != 0) ? -1.0f : 1.0f);
	BoxRoundInfo = BoxRound;
}
