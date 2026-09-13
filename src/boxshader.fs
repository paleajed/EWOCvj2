in vec2 TexCoord0;
flat in int Vertex0;

layout(location = 0) out vec4 FragColor;

#ifdef GLES
uniform sampler2D boxSampler[BOXTEX_COUNT];
#else
uniform sampler2D boxSampler[BOXTEX_COUNT];
#endif
uniform sampler2D boxcolSampler;
uniform usampler2D boxtexSampler;
uniform int orquad;
uniform int textmode;
uniform int baseQuad;

#ifdef GLES
vec4 sampleFromBox(int idx, vec2 tc) {
	switch(idx) {
#if BOXTEX_COUNT > 0
		case 0: return texture(boxSampler[0], tc);
#endif
#if BOXTEX_COUNT > 1
		case 1: return texture(boxSampler[1], tc);
#endif
#if BOXTEX_COUNT > 2
		case 2: return texture(boxSampler[2], tc);
#endif
#if BOXTEX_COUNT > 3
		case 3: return texture(boxSampler[3], tc);
#endif
#if BOXTEX_COUNT > 4
		case 4: return texture(boxSampler[4], tc);
#endif
#if BOXTEX_COUNT > 5
		case 5: return texture(boxSampler[5], tc);
#endif
#if BOXTEX_COUNT > 6
		case 6: return texture(boxSampler[6], tc);
#endif
#if BOXTEX_COUNT > 7
		case 7: return texture(boxSampler[7], tc);
#endif
#if BOXTEX_COUNT > 8
		case 8: return texture(boxSampler[8], tc);
#endif
#if BOXTEX_COUNT > 9
		case 9: return texture(boxSampler[9], tc);
#endif
#if BOXTEX_COUNT > 10
		case 10: return texture(boxSampler[10], tc);
#endif
#if BOXTEX_COUNT > 11
		case 11: return texture(boxSampler[11], tc);
#endif
#if BOXTEX_COUNT > 12
		case 12: return texture(boxSampler[12], tc);
#endif
#if BOXTEX_COUNT > 13
		case 13: return texture(boxSampler[13], tc);
#endif
		default: return vec4(0.0);
	}
}
#else
vec4 sampleFromBox(int idx, vec2 tc) {
	switch(idx) {
#if BOXTEX_COUNT > 0
		case 0: return texture(boxSampler[0], tc);
#endif
#if BOXTEX_COUNT > 1
		case 1: return texture(boxSampler[1], tc);
#endif
#if BOXTEX_COUNT > 2
		case 2: return texture(boxSampler[2], tc);
#endif
#if BOXTEX_COUNT > 3
		case 3: return texture(boxSampler[3], tc);
#endif
#if BOXTEX_COUNT > 4
		case 4: return texture(boxSampler[4], tc);
#endif
#if BOXTEX_COUNT > 5
		case 5: return texture(boxSampler[5], tc);
#endif
#if BOXTEX_COUNT > 6
		case 6: return texture(boxSampler[6], tc);
#endif
#if BOXTEX_COUNT > 7
		case 7: return texture(boxSampler[7], tc);
#endif
#if BOXTEX_COUNT > 8
		case 8: return texture(boxSampler[8], tc);
#endif
#if BOXTEX_COUNT > 9
		case 9: return texture(boxSampler[9], tc);
#endif
#if BOXTEX_COUNT > 10
		case 10: return texture(boxSampler[10], tc);
#endif
#if BOXTEX_COUNT > 11
		case 11: return texture(boxSampler[11], tc);
#endif
#if BOXTEX_COUNT > 12
		case 12: return texture(boxSampler[12], tc);
#endif
#if BOXTEX_COUNT > 13
		case 13: return texture(boxSampler[13], tc);
#endif
#if BOXTEX_COUNT > 14
		case 14: return texture(boxSampler[14], tc);
#endif
#if BOXTEX_COUNT > 15
		case 15: return texture(boxSampler[15], tc);
#endif
#if BOXTEX_COUNT > 16
		case 16: return texture(boxSampler[16], tc);
#endif
#if BOXTEX_COUNT > 17
		case 17: return texture(boxSampler[17], tc);
#endif
#if BOXTEX_COUNT > 18
		case 18: return texture(boxSampler[18], tc);
#endif
#if BOXTEX_COUNT > 19
		case 19: return texture(boxSampler[19], tc);
#endif
#if BOXTEX_COUNT > 20
		case 20: return texture(boxSampler[20], tc);
#endif
#if BOXTEX_COUNT > 21
		case 21: return texture(boxSampler[21], tc);
#endif
#if BOXTEX_COUNT > 22
		case 22: return texture(boxSampler[22], tc);
#endif
#if BOXTEX_COUNT > 23
		case 23: return texture(boxSampler[23], tc);
#endif
#if BOXTEX_COUNT > 24
		case 24: return texture(boxSampler[24], tc);
#endif
#if BOXTEX_COUNT > 25
		case 25: return texture(boxSampler[25], tc);
#endif
#if BOXTEX_COUNT > 26
		case 26: return texture(boxSampler[26], tc);
#endif
#if BOXTEX_COUNT > 27
		case 27: return texture(boxSampler[27], tc);
#endif
#if BOXTEX_COUNT > 28
		case 28: return texture(boxSampler[28], tc);
#endif
#if BOXTEX_COUNT > 29
		case 29: return texture(boxSampler[29], tc);
#endif
#if BOXTEX_COUNT > 30
		case 30: return texture(boxSampler[30], tc);
#endif
#if BOXTEX_COUNT > 31
		case 31: return texture(boxSampler[31], tc);
#endif
#if BOXTEX_COUNT > 32
		case 32: return texture(boxSampler[32], tc);
#endif
#if BOXTEX_COUNT > 33
		case 33: return texture(boxSampler[33], tc);
#endif
#if BOXTEX_COUNT > 34
		case 34: return texture(boxSampler[34], tc);
#endif
#if BOXTEX_COUNT > 35
		case 35: return texture(boxSampler[35], tc);
#endif
#if BOXTEX_COUNT > 36
		case 36: return texture(boxSampler[36], tc);
#endif
#if BOXTEX_COUNT > 37
		case 37: return texture(boxSampler[37], tc);
#endif
#if BOXTEX_COUNT > 38
		case 38: return texture(boxSampler[38], tc);
#endif
#if BOXTEX_COUNT > 39
		case 39: return texture(boxSampler[39], tc);
#endif
#if BOXTEX_COUNT > 40
		case 40: return texture(boxSampler[40], tc);
#endif
#if BOXTEX_COUNT > 41
		case 41: return texture(boxSampler[41], tc);
#endif
#if BOXTEX_COUNT > 42
		case 42: return texture(boxSampler[42], tc);
#endif
#if BOXTEX_COUNT > 43
		case 43: return texture(boxSampler[43], tc);
#endif
#if BOXTEX_COUNT > 44
		case 44: return texture(boxSampler[44], tc);
#endif
#if BOXTEX_COUNT > 45
		case 45: return texture(boxSampler[45], tc);
#endif
#if BOXTEX_COUNT > 46
		case 46: return texture(boxSampler[46], tc);
#endif
#if BOXTEX_COUNT > 47
		case 47: return texture(boxSampler[47], tc);
#endif
#if BOXTEX_COUNT > 48
		case 48: return texture(boxSampler[48], tc);
#endif
#if BOXTEX_COUNT > 49
		case 49: return texture(boxSampler[49], tc);
#endif
#if BOXTEX_COUNT > 50
		case 50: return texture(boxSampler[50], tc);
#endif
#if BOXTEX_COUNT > 51
		case 51: return texture(boxSampler[51], tc);
#endif
#if BOXTEX_COUNT > 52
		case 52: return texture(boxSampler[52], tc);
#endif
#if BOXTEX_COUNT > 53
		case 53: return texture(boxSampler[53], tc);
#endif
#if BOXTEX_COUNT > 54
		case 54: return texture(boxSampler[54], tc);
#endif
#if BOXTEX_COUNT > 55
		case 55: return texture(boxSampler[55], tc);
#endif
#if BOXTEX_COUNT > 56
		case 56: return texture(boxSampler[56], tc);
#endif
#if BOXTEX_COUNT > 57
		case 57: return texture(boxSampler[57], tc);
#endif
#if BOXTEX_COUNT > 58
		case 58: return texture(boxSampler[58], tc);
#endif
#if BOXTEX_COUNT > 59
		case 59: return texture(boxSampler[59], tc);
#endif
#if BOXTEX_COUNT > 60
		case 60: return texture(boxSampler[60], tc);
#endif
#if BOXTEX_COUNT > 61
		case 61: return texture(boxSampler[61], tc);
#endif
#if BOXTEX_COUNT > 62
		case 62: return texture(boxSampler[62], tc);
#endif
#if BOXTEX_COUNT > 63
		case 63: return texture(boxSampler[63], tc);
#endif
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
