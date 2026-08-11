in vec2 TexCoord0;
flat in int Vertex0;

layout(location = 0) out vec4 FragColor;

#ifdef GLES
uniform sampler2D boxSampler[14];
#else
uniform sampler2D boxSampler[64];
#endif
uniform sampler2D boxcolSampler;
uniform usampler2D boxtexSampler;
uniform int orquad;
uniform int textmode;
uniform int baseQuad;

#ifdef GLES
vec4 sampleFromBox(int idx, vec2 tc) {
	switch(idx) {
		case 0: return texture(boxSampler[0], tc);
		case 1: return texture(boxSampler[1], tc);
		case 2: return texture(boxSampler[2], tc);
		case 3: return texture(boxSampler[3], tc);
		case 4: return texture(boxSampler[4], tc);
		case 5: return texture(boxSampler[5], tc);
		case 6: return texture(boxSampler[6], tc);
		case 7: return texture(boxSampler[7], tc);
		case 8: return texture(boxSampler[8], tc);
		case 9: return texture(boxSampler[9], tc);
		case 10: return texture(boxSampler[10], tc);
		case 11: return texture(boxSampler[11], tc);
		case 12: return texture(boxSampler[12], tc);
		case 13: return texture(boxSampler[13], tc);
		default: return vec4(0.0);
	}
}
#else
vec4 sampleFromBox(int idx, vec2 tc) {
	switch(idx) {
		case 0: return texture(boxSampler[0], tc);
		case 1: return texture(boxSampler[1], tc);
		case 2: return texture(boxSampler[2], tc);
		case 3: return texture(boxSampler[3], tc);
		case 4: return texture(boxSampler[4], tc);
		case 5: return texture(boxSampler[5], tc);
		case 6: return texture(boxSampler[6], tc);
		case 7: return texture(boxSampler[7], tc);
		case 8: return texture(boxSampler[8], tc);
		case 9: return texture(boxSampler[9], tc);
		case 10: return texture(boxSampler[10], tc);
		case 11: return texture(boxSampler[11], tc);
		case 12: return texture(boxSampler[12], tc);
		case 13: return texture(boxSampler[13], tc);
		case 14: return texture(boxSampler[14], tc);
		case 15: return texture(boxSampler[15], tc);
		case 16: return texture(boxSampler[16], tc);
		case 17: return texture(boxSampler[17], tc);
		case 18: return texture(boxSampler[18], tc);
		case 19: return texture(boxSampler[19], tc);
		case 20: return texture(boxSampler[20], tc);
		case 21: return texture(boxSampler[21], tc);
		case 22: return texture(boxSampler[22], tc);
		case 23: return texture(boxSampler[23], tc);
		case 24: return texture(boxSampler[24], tc);
		case 25: return texture(boxSampler[25], tc);
		case 26: return texture(boxSampler[26], tc);
		case 27: return texture(boxSampler[27], tc);
		case 28: return texture(boxSampler[28], tc);
		case 29: return texture(boxSampler[29], tc);
		case 30: return texture(boxSampler[30], tc);
		case 31: return texture(boxSampler[31], tc);
		case 32: return texture(boxSampler[32], tc);
		case 33: return texture(boxSampler[33], tc);
		case 34: return texture(boxSampler[34], tc);
		case 35: return texture(boxSampler[35], tc);
		case 36: return texture(boxSampler[36], tc);
		case 37: return texture(boxSampler[37], tc);
		case 38: return texture(boxSampler[38], tc);
		case 39: return texture(boxSampler[39], tc);
		case 40: return texture(boxSampler[40], tc);
		case 41: return texture(boxSampler[41], tc);
		case 42: return texture(boxSampler[42], tc);
		case 43: return texture(boxSampler[43], tc);
		case 44: return texture(boxSampler[44], tc);
		case 45: return texture(boxSampler[45], tc);
		case 46: return texture(boxSampler[46], tc);
		case 47: return texture(boxSampler[47], tc);
		case 48: return texture(boxSampler[48], tc);
		case 49: return texture(boxSampler[49], tc);
		case 50: return texture(boxSampler[50], tc);
		case 51: return texture(boxSampler[51], tc);
		case 52: return texture(boxSampler[52], tc);
		case 53: return texture(boxSampler[53], tc);
		case 54: return texture(boxSampler[54], tc);
		case 55: return texture(boxSampler[55], tc);
		case 56: return texture(boxSampler[56], tc);
		case 57: return texture(boxSampler[57], tc);
		case 58: return texture(boxSampler[58], tc);
		case 59: return texture(boxSampler[59], tc);
		case 60: return texture(boxSampler[60], tc);
		case 61: return texture(boxSampler[61], tc);
		case 62: return texture(boxSampler[62], tc);
		case 63: return texture(boxSampler[63], tc);
		default: return vec4(0.0);
	}
}
#endif

void main()
{
	int quadnr;
	if (orquad != 0) quadnr = orquad;
	else quadnr = baseQuad + Vertex0 / 4;
	uint Tex0 = texelFetch(boxtexSampler, ivec2(quadnr, 0), 0).r;
	if (textmode == 1) {
		float c = sampleFromBox(int(Tex0), vec2(TexCoord0.s, TexCoord0.t)).r;
		vec4 sam = texelFetch(boxcolSampler, ivec2(quadnr, 0), 0).rgba;
		FragColor = vec4(sam.rgb, c);
	}
	else if (Tex0 != 255u) {
		FragColor = sampleFromBox(int(Tex0), vec2(TexCoord0.s, TexCoord0.t));
	}
	else {
		FragColor = texelFetch(boxcolSampler, ivec2(quadnr, 0), 0).rgba;
	}
}
