#version 430 core
 
in vec2 TexCoord0;
in flat int Vertex0;

layout(location = 0) out vec4 FragColor;

uniform sampler2D Sampler0;
uniform sampler2D endSampler0, endSampler1;
uniform sampler2D fboSampler;
uniform sampler2D boxSampler[24];   // reminder: number ~ card
uniform samplerBuffer boxcolSampler;
uniform usamplerBuffer boxtexSampler;
uniform samplerBuffer boxbrdSampler;
uniform samplerBuffer texSampler;
uniform float cf = 0.5f;
uniform float opacity = 1.0f;
uniform float satamount = 4.0f;
uniform float colorrot = 0.5f;
uniform float dotsize = 300.0f;
uniform int fbowidth = 1920;
uniform int fboheight = 1080;
uniform float globw = 1920.0;
uniform float globh = 1080.0;
uniform bool inverted = false;
uniform float fcdiv = 1.0f;
uniform int numverts = 0;
uniform int preff = 1;
uniform float drywet = 1.0f;
uniform vec4 color = vec4(0.0f, 0.0f, 0.0f, 1.0f);
uniform vec4 lcolor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
uniform bool horizontal;
uniform float BlurStart = 0.0f;
uniform float radialwidth = 0.6f; 
uniform float radialX = 0.0f; 
uniform float radialY = 0.0f; 
uniform float nSamples = 10.0f;
uniform float glowblur = 10.0f;  
uniform float jump = 20;  
uniform float glowfac = 1.2f;
uniform float contrastamount = 2.0f;
uniform float brightamount = 1.0f;    
uniform float scalefactor = 0.25f;
uniform float swradius = 0.5f;
uniform float swirlangle = 0.8f;
uniform float swirlx = 0.5f;
uniform float swirly = 0.5f;
uniform int fxid = -1;
uniform int mixmode = 0;
uniform int singlelayer = 0;
uniform int down = 0;
uniform int circle = 0;
uniform float cirx = 0.0f;
uniform float ciry = 0.0f;
uniform float circleradius = 0.0f;
uniform int thumb = 0;
uniform int glbox = 0;
uniform int box = 0;
uniform int border = 0;
uniform int batch = 0;
uniform int linetriangle = 0;
uniform int textmode = 0;
uniform int orquad = 0;
uniform float pixelw;
uniform float pixelh;
uniform int numquads;
uniform float bdw;
uniform float bdh;
uniform int istex = 0;
uniform int interm = 0;
uniform float riptime = 0.01f;
uniform float fishamount = 0.7f;
uniform float treshheight = 0.5f;
uniform float treshred1 = 1.0f;
uniform float treshgreen1 = 1.0f;
uniform float treshblue1 = 1.0f;
uniform float treshred2 = 0.0f;
uniform float treshgreen2 = 0.0f;
uniform float treshblue2 = 0.0f;
uniform vec4 treshlowcol = vec4(0.0f, 0.0f, 0.0f, 1.0f);
uniform float strobephase = 0.0f;
uniform float strobered = 1.0f; 
uniform float strobegreen = 1.0f;
uniform float strobeblue = 1.0f;
uniform float gamma = 0.6f;
uniform float numColors = 8.0f;
uniform float pixel_w = 32.0f;
uniform float pixel_h = 32.0f;
uniform float rotamount = 45.0f;
uniform float iGlobalTime = 0.0f;
uniform float asciisize = 50.0f;
uniform float vardotsize = 0.1f;

uniform float hatchsize = 10.0f;
uniform float hatch_y_offset = 5.0f;
uniform float lum_threshold_1 = 1.0f;
uniform float lum_threshold_2 = 0.7f;
uniform float lum_threshold_3 = 0.5f;
uniform float lum_threshold_4 = 0.3f;

uniform float SepiaValue = 2.0f;
uniform float filmnoise = 2.0f;
uniform float ScratchValue = 2.0f;
uniform float InnerVignetting = 0.7f;
uniform float OuterVignetting = 1.0f;
uniform float RandomValue = 2.0f;
uniform float TimeLapse = 0.1f;

uniform bool wipe = false;
uniform int wkind = -1;
uniform int dir = 0;
uniform float xpos = 0.0f;
uniform float ypos = 0.0f;

uniform float colortol = 0.8f;
uniform bool chdir = false;
uniform bool chinv = false;
uniform float mx = 0.0f;
uniform float my = -1.0f;
uniform float cwx = 0.0f;
uniform float cwy = 0.0f;
uniform bool cwon = false;
uniform bool cwmouse = false;
uniform float chred = 0.0f;
uniform float chgreen = 0.0f;
uniform float chblue = 0.0f;
uniform float feather = 2.0f;
uniform float mixfac = 0.5f;
	
uniform float htdotsize = 1.48f;
uniform float hts = 100.0f;
uniform float cut = 0.5f;
uniform float glitchstr = 1.0f;
uniform float glitchspeed = 1.0f;
uniform float colhue = 0.5f;
uniform float noiselevel = 0.5f;
uniform float gammaval = 2.2f;
uniform float bokehrad = 1.0f;
uniform bool inlayer = false;
uniform bool xflip = true;
uniform bool yflip = false;
uniform float xmirror = 0;
uniform float ymirror = 1;

/// Computes the overlay between the source and destination colours.
vec3 Overlay (vec3 src, vec3 dst)
{
	return vec3((dst.x <= 0.5) ? (2.0 * src.x * dst.x) : (1.0 - 2.0 * (1.0 - dst.x) * (1.0 - src.x)),
	(dst.y <= 0.5) ? (2.0 * src.y * dst.y) : (1.0 - 2.0 * (1.0 - dst.y) * (1.0 - src.y)),
	(dst.z <= 0.5) ? (2.0 * src.z * dst.z) : (1.0 - 2.0 * (1.0 - dst.z) * (1.0 - src.z)));
}
 
vec4 mod289(vec4 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec3 mod289(vec3 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec2 mod289(vec2 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec3 permute(vec3 x) { return mod289(((x*34.0)+1.0)*x); }
 
float snoise2 (vec2 v)
{
	const vec4 C = vec4(0.211324865405187,  // (3.0-sqrt(3.0))/6.0
	0.366025403784439,  // 0.5*(sqrt(3.0)-1.0)
	-0.577350269189626, // -1.0 + 2.0 * C.x
	0.024390243902439); // 1.0 / 41.0
	 
	// First corner
	vec2 i  = floor(v + dot(v, C.yy) );
	vec2 x0 = v -   i + dot(i, C.xx);
	 
	// Other corners
	vec2 i1;
	i1 = (x0.x > x0.y) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);
	vec4 x12 = x0.xyxy + C.xxzz;
	x12.xy -= i1;
	 
	// Permutations
	i = mod289(i); // Avoid truncation effects in permutation
	vec3 p = permute( permute( i.y + vec3(0.0, i1.y, 1.0 ))
	+ i.x + vec3(0.0, i1.x, 1.0 ));
	 
	vec3 m = max(0.5 - vec3(dot(x0,x0), dot(x12.xy,x12.xy), dot(x12.zw,x12.zw)), 0.0);
	m = m*m ;
	m = m*m ;
	 
	// Gradients: 41 points uniformly over a line, mapped onto a diamond.
	// The ring size 17*17 = 289 is close to a multiple of 41 (41*7 = 287)
	 
	vec3 x = 2.0 * fract(p * C.www) - 1.0;
	vec3 h = abs(x) - 0.5;
	vec3 ox = floor(x + 0.5);
	vec3 a0 = x - ox;
	 
	// Normalise gradients implicitly by scaling m
	// Approximation of: m *= inversesqrt( a0*a0 + h*h );
	m *= 1.79284291400159 - 0.85373472095314 * ( a0*a0 + h*h );
	 
	// Compute final noise value at P
	vec3 g;
	g.x  = a0.x  * x0.x  + h.x  * x0.y;
	g.yz = a0.yz * x12.xz + h.yz * x12.yw;
	return 130.0 * dot(m, g);
}
 
 
// Sepia RGB value
vec3 sepia = vec3(112.0 / 255.0, 66.0 / 255.0, 20.0 / 255.0);

vec4 oldfilm(vec2 texco) { //devmaster.net seems free
	// Step 1: Convert to grayscale
	vec3 colour = texture2D(Sampler0, texco).xyz;
	float gray = (colour.x + colour.y + colour.z) / 3.0;
	vec3 grayscale = vec3(gray);
	 
	// Step 2: Appy sepia overlay
	vec3 finalColour = Overlay(sepia, grayscale);
	 
	// Step 3: Lerp final sepia colour
	finalColour = grayscale + SepiaValue * (finalColour - grayscale);
	 
	// Step 4: Add noise
	float noise = snoise2(texco * vec2(1024.0 + RandomValue * 512.0, 1024.0 + RandomValue * 512.0)) * 0.5;
	finalColour += noise * filmnoise;
	 
	// Optionally add noise as an overlay, simulating ISO on the camera
	//vec3 noiseOverlay = Overlay(finalColour, vec3(noise));
	//finalColour = finalColour + filmnoise * (finalColour - noiseOverlay);
	 
	// Step 5: Apply scratches
	if ( RandomValue < ScratchValue )
	{
	// Pick a random spot to show scratches
		float dist = 1.0 / ScratchValue;
		float d = distance(texco, vec2(RandomValue * dist, RandomValue * dist));
		if ( d < 0.4 )
		{
			// Generate the scratch
			float xPeriod = 8.0;
			float yPeriod = 1.0;
			float pi = 3.141592;
			float phase = TimeLapse;
			float turbulence = snoise2(texco * 2.5);
			float vScratch = 0.5 + (sin(((texco[0] * xPeriod + texco[1] * yPeriod + turbulence)) * pi + phase) * 0.5);
			vScratch = clamp((vScratch * 10000.0) + 0.35, 0.0, 1.0);
			 
			finalColour.xyz *= vScratch;
		}
	}
	 
	// Step 6: Apply vignetting
	// Max distance from centre to corner is ~0.7. Scale that to 1.0.
	float d = distance(vec2(0.5, 0.5), texco) * 1.414213;
	float vignetting = clamp((OuterVignetting - d) / (OuterVignetting - InnerVignetting), 0.0, 1.0);
	finalColour.xyz *= vignetting;
	 
	// Apply colour
	return vec4(finalColour, 1.0);
}


vec3 rgb2hsv(vec3 c)  //theres many on kylemcdonald/hsv2rgb.glsl
{
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec4 saturation(vec4 color)  //selfmade
{
	vec3 hsv = rgb2hsv(color.rgb);
	hsv.y *= satamount;
	vec3 rgb = hsv2rgb(hsv);
	return vec4(rgb.x, rgb.y, rgb.z, color.a);
}

vec4 chromarotate(vec4 color)  //selfmade
{
	vec3 hsv = rgb2hsv(color.rgb);
	hsv.x += colorrot;
	if (hsv.x < 0) hsv.x += 1;
	vec3 rgb = hsv2rgb(hsv);
	return vec4(rgb.x, rgb.y, rgb.z, color.a);
}

vec4 colorize(vec4 color)  //selfmade
{
	vec3 hsv = rgb2hsv(color.rgb);
	hsv.x = colhue;
	hsv.y = hsv.y - ((hsv.y - 1.0f) / 2.0f);
	if (hsv.x < 0) hsv.x += 1;
	vec3 rgb = hsv2rgb(hsv);
	return vec4(rgb.x, rgb.y, rgb.z, color.a);
}

float normpdf(in float x, in float sigma)
{
	return 0.39894*exp(-0.5*x*x/(sigma*sigma))/sigma;
}

const float offset[3] = float[]( 0.0, 1.3846153846, 3.2307692308 );
const float weight[3] = float[]( 0.2270270270, 0.3162162162, 0.0702702703 );

vec4 blur(vec2 texc) //tutorial on rastergrid seems free
{
  	vec3 tc = vec3(1.0, 0.0, 0.0);
    vec2 uv = texc.xy;
    vec2 size = textureSize(fboSampler, 0) / 4.0f;
    tc = texture2D(fboSampler, uv).rgb * weight[0];
    if (!horizontal) {
		for (int i=1; i<3; i++) 
		{
		  tc += texture2D(fboSampler, uv + vec2(0.0, offset[i] * fcdiv / size.y)).rgb * weight[i];
		  tc += texture2D(fboSampler, uv - vec2(0.0, offset[i] * fcdiv / size.y)).rgb * weight[i];
		}
	}
	else {
		for (int i=1; i<3; i++) 
		{
		  tc += texture2D(fboSampler, uv + vec2(offset[i]) * fcdiv / size.x, 0.0).rgb * weight[i];
		  tc += texture2D(fboSampler, uv - vec2(offset[i]) * fcdiv / size.x, 0.0).rgb * weight[i];
		}
	}
  	return vec4(tc, texture2D(fboSampler, uv).a);
}

vec4 boxblur(vec2 texc)  //blog.trsquarelab.com free
{
	float alpha = texture2D(fboSampler, texc).a;
	vec2 size = textureSize(fboSampler, 0);
	vec2 point;
	vec4 finalcol = vec4(0.0, 0.0, 0.0, 0.0);
	int count = 0;
    float pxdistX = 1 / float(fbowidth);
    float pxdistY = 1 / float(fboheight);
    if (!horizontal) {
		for (int i = int(-glowblur); i < glowblur; i+= int(41 - jump)) 
		{
      		point.x = texc.x;
            point.y = texc.y  + i * pxdistY;
            ++count;
            finalcol += texture2D(fboSampler, point.xy);
		}
		finalcol /= float(count);
	}
	else {
		for (int i = int(-glowblur); i < glowblur; i+= int(41 - jump)) 
		{
     		point.x = texc.x  + i * pxdistX;
            point.y = texc.y;
            ++count;
           finalcol += texture2D(fboSampler, point.xy);
		}
		finalcol /= float(count);
	}
  	return vec4(finalcol.rgb, alpha);
}


uniform float radstrength;
uniform float radinner;
uniform float radrad = -1;

const float MAX_KERNEL_SIZE = 32.0;

float random(vec3 scale, float seed) {
    // use the fragment position for a different seed per-pixel
    return fract(sin(dot(gl_FragCoord.xyz + seed, scale)) * 43758.5453 + seed);
}

vec4 radialblur(vec2 texco) {  //pixi-filters mit

	vec2 uCenter = vec2(radialX * fbowidth, radialY * fboheight);	

    float minGradient = radinner * fbowidth * 0.3;
    float innerRadius = (radinner * fbowidth + minGradient * 0.5) / fbowidth;

    float gradient = radrad * 0.3;
    float radius = (radrad - gradient * 0.5) / fbowidth;

    float countLimit = MAX_KERNEL_SIZE;

    vec2 dir = vec2(uCenter.xy / vec2(fbowidth, fboheight) - texco);
    float dist = length(vec2(dir.x, dir.y * fboheight / fbowidth));

    float strength = radstrength;

    float delta = 0.0;
    float gap;
    if (dist < innerRadius) {
        delta = innerRadius - dist;
        gap = minGradient;
    } else if (radius >= 0.0 && dist > radius) { // radius < 0 means it's infinity
        delta = dist - radius;
        gap = gradient;
    }

    if (delta > 0.0) {
        float normalCount = gap / fbowidth;
        delta = (normalCount - delta) / normalCount;
        countLimit *= delta;
        strength *= delta;
        if (countLimit < 1.0)
        {
            return texture2D(Sampler0, texco);
        }
    }

    // randomize the lookup values to hide the fixed number of samples
    float offset = random(vec3(12.9898, 78.233, 151.7182), 0.0);

    float total = 0.0;
    vec4 color = vec4(0.0);

    dir *= strength;

    for (float t = 0.0; t < MAX_KERNEL_SIZE; t++) {
        float percent = (t + offset) / MAX_KERNEL_SIZE;
        float weight = 4.0 * (percent - percent * percent);
        vec2 p = texco + dir * percent;
        vec4 smple = texture2D(Sampler0, p);

        // switch to pre-multiplied alpha to correctly blur transparent images
        // smple.rgb *= smple.a;

        color += smple * weight;
        total += weight;

        if (t > countLimit){
            break;
        }
    }

    color /= total;
    // switch back from pre-multiplied alpha
    color.rgb /= color.a + 0.00001;

    return color;
}


vec4 dotf(vec2 texco) {  //selfmade
	vec2 normco = vec2((texco[0] - 0.5f) * 2.0f, (texco[1] - 0.5f) * 2.0f * fboheight / fbowidth);
	float dot = dotsize / fbowidth * 2.0f;
	float xint = normco[0] - dot/2.0f;
	if (xint > 0) xint += dot;
	float circlex = int(xint / dot) * dot;
	float yint = normco[1] - dot/2.0f;
	if (yint > 0) yint += dot; 
	float circley = int(yint / dot) * dot;
	if (distance(normco.xy, vec2(circlex, circley)) > dot * 0.4) return vec4(0.0, 0.0 ,0.0, 1.0);
	else return texture2D(Sampler0, texco);
}

vec4 brightness(vec4 pxcol) {  //selfmade
	pxcol.rgb /= pxcol.a;
	pxcol.rgb += brightamount;
	pxcol.rgb *= pxcol.a;
	return pxcol;
}
  
vec4 contrast(vec4 pxcol) {  //selfmade
	pxcol.rgb /= pxcol.a;
	pxcol.rgb = (pxcol.rgb - 0.5f) * contrastamount + 0.5f;
	pxcol.rgb *= pxcol.a;
	return pxcol;
}

vec2 scale(vec2 texc) {  //selfmade
	float fac = scalefactor;
	fac = fac * fac * fac * fac * fac * fac * fac * fac;
	vec2 center = vec2(0.5, 0.5);
	texc = fract((texc - center) / fac + center);
	return texc;
}

vec4 swirl(vec2 texco)  //geeks3D seems free
{
	vec2 normtc = texco - 0.5f;
	normtc.y = normtc.y * fboheight / fbowidth;
	vec2 center = ((vec2(swirlx, swirly)) - 0.5f) * 2.0f;
	float radius = swradius * 2.0f;
	normtc -= center;
	float dist = length(normtc);
	if (dist < radius) 
	{
	float percent = (radius - dist) / radius;
	float theta = percent * percent * swirlangle * 8.0;
	float s = sin(theta);
	float c = cos(theta);
	normtc = vec2(dot(normtc, vec2(c, -s)), dot(normtc, vec2(s, c)));
	}
	normtc += center;
	normtc.y = normtc.y * fbowidth / fboheight;
	return texture2D(Sampler0, (normtc + 0.5f));
}

vec4 ripple() {  //geeks3d seems free
  	vec2 tc = TexCoord0.xy;
  	vec2 p = -1.0 + 2.0 * tc;
  	float len = length(p);
  	vec2 uv = tc + (p / len) * cos(len * 12.0 - riptime * 4.0) * 0.03;
  	vec3 col = texture2D(Sampler0, uv).xyz;
  	return vec4(col,1.0);
}

vec4 fisheye(vec2 texco)  //selfmade
{
  // (0, 0) to center
  vec2 c = texco - 0.5f;
  float dist = length(c);
  float ang = atan(c.y, c.x);
  
  float power = fishamount * 2.0f;
  dist = pow(dist, power) * fishamount * 3.0f;
  
  c = dist * vec2(cos(ang)*0.5f, sin(ang)*0.5f);
  
  return texture2D(Sampler0, c + 0.5f);
}

vec4 treshold(vec4 pxcol) {  //selfmade
	float bright = (pxcol.r + pxcol.g + pxcol.b) / 3.0;
	if (bright > treshheight) return vec4(treshred1, treshgreen1, treshblue1, pxcol.a);
	else return vec4(treshred2, treshgreen2, treshblue2, pxcol.a);
}
  
vec4 posterize(vec4 col)  //geeks3d seems free
{ 
  vec3 c = col.rgb;
  c = pow(c, vec3(gamma, gamma, gamma));
  c = c * float(numColors);
  c = floor(c);
  c = c / float(numColors);
  c = pow(c, vec3(1.0 / gamma));
  return vec4(c, col.a);
}

vec4 pixelate(vec2 uv)  //selfmade
{
	int pw = int(pixel_w * pixel_w);
	int ph = int(pixel_h * pixel_h);
    float dx = 1.0f / pw;
    float dy = 1.0f / ph;
    vec2 coord = vec2(dx * (floor(uv.x / dx) + 0.5f), dy * (floor(uv.y / dy) + 0.5f));
    return texture2D(Sampler0, coord);
}

vec4 crosshatch(vec4 col, vec2 uv)  //geeks3D
{ 
    float lum = length(col.rgb);
    vec3 tc = vec3(1.0, 1.0, 1.0);
  
    if (lum < lum_threshold_1) 
    {
      	if (mod(int(uv.x * fboheight) + int(uv.y * fboheight) - int(hatch_y_offset), int(hatchsize)) == 0.0) 
        	tc = vec3(0.0, 0.0, 0.0);
    }
  
    if (lum < lum_threshold_2) 
    {
      	if (mod(int(uv.x * fboheight) - int(uv.y * fboheight) - int(hatch_y_offset), int(hatchsize)) == 0.0) 
        	tc = vec3(0.0, 0.0, 0.0);
    }  
  
    if (lum < lum_threshold_3) 
    {
      	if (mod(int(uv.x * fboheight) + int(uv.y * fboheight) - int(hatch_y_offset), int(hatchsize)) == 0.0) 
        	tc = vec3(0.0, 0.0, 0.0);
    }  
  
    if (lum < lum_threshold_4) 
    {
      	if (mod(int(uv.x * fboheight) - int(uv.y * fboheight) - int(hatch_y_offset), int(hatchsize)) == 0.0) 
        	tc = vec3(0.0, 0.0, 0.0);
  	}
 	return vec4(tc, col.a);
}

vec4 rotate(vec2 texco)  // selfmade
{
    float rot = radians(rotamount);
    texco -= 0.5f;
    texco.y *= fbowidth / fboheight;
    //rotation matrix
    mat2 m = mat2(cos(rot), -sin(rot), sin(rot), cos(rot));
   	texco  = m * texco;
    //texco.y *= fboheight / fbowidth;
    texco += 0.5f;
    
    return texture2D(Sampler0, texco);
}


uniform float embstrength;

vec4 emboss(vec2 texco)  //pixi-filters mit
{
	vec2 onePixel = vec2(1.0 / fboheight);

	vec4 color;

	color.rgb = vec3(0.5);

	color -= texture2D(Sampler0, texco - onePixel) * embstrength;
	color += texture2D(Sampler0, texco + onePixel) * embstrength;

	color.rgb = vec3((color.r + color.g + color.b) / 3.0);

	float alpha = texture2D(Sampler0, texco).a;

	return vec4(color.rgb * alpha, alpha);
}


float character(float n, vec2 p)
{
	p = floor(p * vec2(4.0, -4.0) + 2.5);
	if (clamp(p.x, 0.0, 4.0) == p.x && clamp(p.y, 0.0, 4.0) == p.y)
	{
		if (int(mod(n / exp2(p.x + 5.0*p.y), 2.0)) == 1) return 1.0;
	}	
	return 0.0;
}

vec4 ascii(vec2 texco)  //pixi-filters mit
{
	float detail = 8.0 / fbowidth;
	vec3 col = texture2D(Sampler0, floor(texco / detail) * detail).rgb;
	
	float gray = 0.3 * col.r + 0.59 * col.g + 0.11 * col.b;
	float n = 65536.0;              // .
	if (gray > 0.2) n = 65600.0;    // :
	if (gray > 0.3) n = 332772.0;   // *
	if (gray > 0.4) n = 15255086.0; // o 
	if (gray > 0.5) n = 23385164.0; // &
	if (gray > 0.6) n = 15252014.0; // 8
	if (gray > 0.7) n = 13199452.0; // @
	if (gray > 0.8) n = 11512810.0; // #
	
	vec2 p = mod(vec2(texco.x * fbowidth, texco.y * fboheight) / asciisize, 2.0) - vec2(1.0);
	col = col * character(n, p);
	
	return vec4(col, texture2D(Sampler0, texco).a);
}

#define THRESHOLD vec3(1.,.92,.1)

vec4 solarize(vec2 texco)  //unknown problem 3?
{
    vec3 rgb = texture2D(Sampler0, texco).xyz;
    if (rgb.r < 1.0f) rgb.r = 1.0f - rgb.r;
    if (rgb.g < 0.92f) rgb.g = 1.0f - rgb.g;
    if (rgb.b < 0.1f) rgb.b = 1.0f - rgb.b;
    
    return vec4(rgb, texture2D(Sampler0, texco).a);
}


uniform float vardotangle = 0.0f;

float pattern(vec2 texco)
{
   float s = sin(vardotangle), c = cos(vardotangle);
   vec2 tex = texco * fboheight;
   vec2 point = vec2(
       c * tex.x - s * tex.y,
       s * tex.x + c * tex.y
   ) * (0.2f - vardotsize);
   return ((sin(point.x) * sin(point.y)) - 0.3f) / 2.0f;
}

vec4 vardot(vec2 texco)
{
   vec4 color = texture2D(Sampler0, texco);
   float p = pattern(texco);
   return vec4(color.r + p, color.g + p, color.b + p, color.a);
}


const float SQRT_2 = 1.414213;
const float light = 1.0;

uniform float crtcurvature;
uniform float crtlineWidth;
uniform float crtlineContrast;
uniform bool verticalLine = false;
uniform float crtnoise;
uniform float crtnoiseSize;

uniform float crtvignetting;
uniform float crtvignettingAlpha;
uniform float crtvignettingBlur;

float rand2(vec2 co) {
    return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

vec4 crt(vec2 texco)
{
    vec2 pixelCoord = texco * vec2(fbowidth, fboheight);
    vec2 coord = texco;

    vec2 dir = vec2(coord - vec2(0.5, 0.5));

    float _c = crtcurvature > 0. ? crtcurvature : 1.;
    float k = crtcurvature > 0. ?(length(dir * dir) * 0.25 * _c * _c + 0.935 * _c) : 1.;
    vec2 uv = dir * k;

    vec4 rgba = texture2D(Sampler0, texco);
    vec3 rgb = rgba.rgb;

    if (crtnoise > 0.0 && crtnoiseSize > 0.0)
    {
        pixelCoord.x = floor(pixelCoord.x / crtnoiseSize);
        pixelCoord.y = floor(pixelCoord.y / crtnoiseSize);
        float _noise = rand2(pixelCoord * crtnoiseSize * iGlobalTime) - 0.5;
        rgb += _noise * crtnoise;
    }

    if (crtlineWidth > 0.0) {
        float v = (verticalLine ? uv.x * fbowidth : uv.y * fboheight) * min(1.0, 2.0 / crtlineWidth ) / _c;
        float j = 1. + cos(v * 1.2 - iGlobalTime) * 0.5 * crtlineContrast;
        rgb *= j;
        float segment = verticalLine ? mod((dir.x + .5) * fbowidth, 4.) : mod((dir.y + .5) * fboheight, 4.);
        rgb *= 0.99 + ceil(segment) * 0.015;
    }

    if (crtvignetting > 0.0)
    {
        float outter = SQRT_2 - crtvignetting * SQRT_2;
        float darker = clamp((outter - length(dir) * SQRT_2) / ( 0.00001 + crtvignettingBlur * SQRT_2), 0.0, 1.0);
        rgb *= darker + (1.0 - darker) * (1.0 - crtvignettingAlpha);
    }

    return vec4(rgb, rgba.a);
}


uniform mat3 G[9] = mat3[](
	1.0/(2.0*sqrt(2.0)) * mat3( 1.0, sqrt(2.0), 1.0, 0.0, 0.0, 0.0, -1.0, -sqrt(2.0), -1.0 ),
	1.0/(2.0*sqrt(2.0)) * mat3( 1.0, 0.0, -1.0, sqrt(2.0), 0.0, -sqrt(2.0), 1.0, 0.0, -1.0 ),
	1.0/(2.0*sqrt(2.0)) * mat3( 0.0, -1.0, sqrt(2.0), 1.0, 0.0, -1.0, -sqrt(2.0), 1.0, 0.0 ),
	1.0/(2.0*sqrt(2.0)) * mat3( sqrt(2.0), -1.0, 0.0, -1.0, 0.0, 1.0, 0.0, 1.0, -sqrt(2.0) ),
	1.0/2.0 * mat3( 0.0, 1.0, 0.0, -1.0, 0.0, -1.0, 0.0, 1.0, 0.0 ),
	1.0/2.0 * mat3( -1.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, -1.0 ),
	1.0/6.0 * mat3( 1.0, -2.0, 1.0, -2.0, 4.0, -2.0, 1.0, -2.0, 1.0 ),
	1.0/6.0 * mat3( -2.0, 1.0, -2.0, 1.0, 4.0, 1.0, -2.0, 1.0, -2.0 ),
	1.0/3.0 * mat3( 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0 )
);

vec4 edgedetect2(vec2 texco)  //rastergrid seems free
{
	mat3 I;
	float cnv[9];
	vec3 smple;

	/* fetch the 3x3 neighbourhood and use the RGB vector's length as intensity value */
	for (int i=0; i<3; i++)
	for (int j=0; j<3; j++) {
		smple = texture2D(Sampler0, texco + vec2(float(i-1) / float(fbowidth), float(j-1) / float(fboheight))).rgb;
		I[i][j] = length(smple) * 20.0f; 
	}
	
	/* calculate the convolution values for all the masks */
	for (int i=0; i<9; i++) {
		float dp3 = dot(G[i][0], I[0]) + dot(G[i][1], I[1]) + dot(G[i][2], I[2]);
		cnv[i] = dp3 * dp3; 
	}

	float M = (cnv[0] + cnv[1]) + (cnv[2] + cnv[3]);
	float S = (cnv[4] + cnv[5]) + (cnv[6] + cnv[7]) + (cnv[8] + M); 
	
	vec4 col = vec4(sqrt(M/S));
	return vec4(col.rgb * 2.0f, texture2D(Sampler0, texco).a);
}


#define PI 3.141592653589793
uniform float kalslices = 16.0;

vec4 kaleidoscope(vec2 texco) {  //selfmade
  int slices = int(kalslices);

  vec2 pos = texco - 0.5;
  
  float len = length(pos);
  float ang = atan(pos.y, pos.x);
  
  ang = mod(ang, PI * 2.0f / slices);
  ang = abs(ang - PI / slices);
  
  pos = len * vec2(cos(ang), sin(ang));
  
  return texture2D(Sampler0, pos + 0.5);
}


float aastep(float threshold, float value) {
  #ifdef GL_OES_standard_derivatives
    float afwidth = length(vec2(dFdx(value), dFdy(value))) * 0.70710678118654757;
    return smoothstep(threshold-afwidth, threshold+afwidth, value);
  #else
    return step(threshold, value);
  #endif
}

vec4 halftone(vec4 rgba, vec2 st) {  //glslify mit
	vec3 texcolor = rgba.rgb;
  st.y = st.y * fboheight /fbowidth;
  float frequency = (201.0f - hts);
  float n = 0.1*snoise2(st*200.0); // Fractal noise
  n += 0.05*snoise2(st*400.0);
  n += 0.025*snoise2(st*800.0);
  vec3 white = vec3(n*0.2 + 0.97);
  vec3 black = vec3(n + 0.1);

  // Perform a rough RGB-to-CMYK conversion
  vec4 cmyk;
  cmyk.xyz = 1.0 - texcolor;
  cmyk.w = min(cmyk.x, min(cmyk.y, cmyk.z)); // Create K
  cmyk.xyz -= cmyk.w; // Subtract K equivalent from CMY

  // Distance to nearest point in a grid of
  // (frequency x frequency) points over the unit square
  vec2 Kst = frequency*mat2(0.707, -0.707, 0.707, 0.707)*st;
  vec2 Kuv = 2.0*fract(Kst)-1.0;
  float k = aastep(0.0, sqrt(cmyk.w)-length(Kuv)+n);
  vec2 Cst = frequency*mat2(0.966, -0.259, 0.259, 0.966)*st;
  vec2 Cuv = 2.0*fract(Cst)-1.0;
  float c = aastep(0.0, sqrt(cmyk.x)-length(Cuv)+n);
  vec2 Mst = frequency*mat2(0.966, 0.259, -0.259, 0.966)*st;
  vec2 Muv = 2.0*fract(Mst)-1.0;
  float m = aastep(0.0, sqrt(cmyk.y)-length(Muv)+n);
  vec2 Yst = frequency*st; // 0 deg
  vec2 Yuv = 2.0*fract(Yst)-1.0;
  float y = aastep(0.0, sqrt(cmyk.z)-length(Yuv)+n);

  vec3 rgbscreen = 1.0 - 0.9*vec3(c,m,y) + n;
  return vec4(mix(rgbscreen, black, 0.85*k + 0.3*n), rgba.a);
}


uniform float edge_thres = 0.2f; // 0.2;
uniform float edge_thres2 = 5.0f; // 5.0;
uniform int edgethickmode;


// averaged pixel intensity from 3 color channels
float avg_intensity(vec4 pix) 
{
 return (pix.r + pix.g + pix.b)/3.;
}

vec4 get_pixel(vec2 coords, float dx, float dy) 
{
 return texture2D(Sampler0,coords + vec2(dx, dy));
}

// returns pixel color
float IsEdge(in vec2 coords)
{
  float dxtex = 1.0 /float(textureSize(Sampler0,0)) ;
  float dytex = 1.0 /float(textureSize(Sampler0,0));
  float pix[9];
  int k = -1;
  float delta;

  // read neighboring pixel intensities
  for (int i=-1; i<2; i++) {
   for(int j=-1; j<2; j++) {
    k++;
    pix[k] = avg_intensity(get_pixel(coords,float(i)*dxtex,
                                          float(j)*dytex));
   }
  }

  // average color differences around neighboring pixels
  delta = (abs(pix[1]-pix[7])+
          abs(pix[5]-pix[3]) +
          abs(pix[0]-pix[8])+
          abs(pix[2]-pix[6])
           )/4.;

  //return clamp(5.5*delta,0.0,1.0);
  return clamp(edge_thres2*delta,0.0,1.0);
}

vec4 edgedetect(vec2 texco)
{
  vec4 color = vec4(0.0,0.0,0.0,1.0);
  float gray = IsEdge(texco.xy);
  vec3 vRGB = (gray >= edge_thres)? vec3(0.0,0.0,0.0):vec3(gray ,gray ,gray);
    
  return vec4(vRGB.x,vRGB.y,vRGB.z, texture2D(fboSampler, texco).a);  
}

vec4 cartoon(vec2 texco)  //geeks3d free
{
	//vec4 tc = vec4(1.0, 0.0, 0.0, 1.0);
    vec4 colorOrg = texture2D(Sampler0, texco).rgba;
    vec3 vHSV =  rgb2hsv(colorOrg.rgb);
    vHSV.x = floor(vHSV.x * 25.0f) / 25.0f;
    vHSV.y = floor(vHSV.y * 7.0f + 1) / 7.0f;
    vHSV.z = floor(vHSV.z * 4.0f + 1) / 4.0f;
    float edg = IsEdge(texco);
    vec3 vRGB = (edg >= edge_thres)? vec3(0.0,0.0,0.0):hsv2rgb(vec3(vHSV.x,vHSV.y,vHSV.z));
    
  	return vec4(vRGB.x,vRGB.y,vRGB.z, texture2D(Sampler0, texco).a);  
}

vec4 cutoff(vec2 texco)  //selfmade
{
    vec4 color = texture2D(Sampler0, texco);
    
	float luma = dot(vec3(0.30, 0.59, 0.11), color.rgb);

	if (luma > cut) return color;
	else return vec4(0.1f, 0.1f, 0.1f, color.a);
}


// Glitch
float random2(vec2 c){
return fract(sin(dot(c.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

//
// Description : Array and textureless GLSL 2D/3D/4D simplex
//               noise functions.
//      Author : Ian McEwan, Ashima Arts.
//  Maintainer : ijm
//     Lastmod : 20110822 (ijm)
//     License : Copyright (C) 2011 Ashima Arts. All rights reserved.
//               Distributed under the MIT License. See LICENSE file.
//               https://github.com/ashima/webgl-noise
//

vec4 permute4(vec4 x) {
   return mod289(((x*34.0)+1.0)*x);
}

vec4 taylorInvSqrt(vec4 r)
{
	return 1.79284291400159 - 0.85373472095314 * r;
}

float snoise3(vec3 v)
{
	const vec2  C = vec2(1.0/6.0, 1.0/3.0) ;
	const vec4  D = vec4(0.0, 0.5, 1.0, 2.0);
	
	// First corner
	vec3 i  = floor(v + dot(v, C.yyy) );
	vec3 x0 =   v - i + dot(i, C.xxx) ;
	
	// Other corners
	vec3 g = step(x0.yzx, x0.xyz);
	vec3 l = 1.0 - g;
	vec3 i1 = min( g.xyz, l.zxy );
	vec3 i2 = max( g.xyz, l.zxy );
	
	//   x0 = x0 - 0.0 + 0.0 * C.xxx;
	//   x1 = x0 - i1  + 1.0 * C.xxx;
	//   x2 = x0 - i2  + 2.0 * C.xxx;
	//   x3 = x0 - 1.0 + 3.0 * C.xxx;
	vec3 x1 = x0 - i1 + C.xxx;
	vec3 x2 = x0 - i2 + C.yyy; // 2.0*C.x = 1/3 = C.y
	vec3 x3 = x0 - D.yyy;      // -1.0+3.0*C.x = -0.5 = -D.y
	
	// Permutations
	i = mod289(i);
	vec4 p = permute4( permute4( permute4(
			   i.z + vec4(0.0, i1.z, i2.z, 1.0 ))
			 + i.y + vec4(0.0, i1.y, i2.y, 1.0 ))
			 + i.x + vec4(0.0, i1.x, i2.x, 1.0 ));
	
	// Gradients: 7x7 points over a square, mapped onto an octahedron.
	// The ring size 17*17 = 289 is close to a multiple of 49 (49*6 = 294)
	float n_ = 0.142857142857; // 1.0/7.0
	vec3  ns = n_ * D.wyz - D.xzx;
	
	vec4 j = p - 49.0 * floor(p * ns.z * ns.z);  //  mod(p,7*7)
	
	vec4 x_ = floor(j * ns.z);
	vec4 y_ = floor(j - 7.0 * x_ );    // mod(j,N)
	
	vec4 x = x_ *ns.x + ns.yyyy;
	vec4 y = y_ *ns.x + ns.yyyy;
	vec4 h = 1.0 - abs(x) - abs(y);
	
	vec4 b0 = vec4( x.xy, y.xy );
	vec4 b1 = vec4( x.zw, y.zw );
	
	//vec4 s0 = vec4(lessThan(b0,0.0))*2.0 - 1.0;
	//vec4 s1 = vec4(lessThan(b1,0.0))*2.0 - 1.0;
	vec4 s0 = floor(b0)*2.0 + 1.0;
	vec4 s1 = floor(b1)*2.0 + 1.0;
	vec4 sh = -step(h, vec4(0.0));
	
	vec4 a0 = b0.xzyw + s0.xzyw*sh.xxyy ;
	vec4 a1 = b1.xzyw + s1.xzyw*sh.zzww ;
	
	vec3 p0 = vec3(a0.xy,h.x);
	vec3 p1 = vec3(a0.zw,h.y);
	vec3 p2 = vec3(a1.xy,h.z);
	vec3 p3 = vec3(a1.zw,h.w);
	
	//Normalise gradients
	vec4 norm = taylorInvSqrt(vec4(dot(p0,p0), dot(p1,p1), dot(p2, p2), dot(p3,p3)));
	p0 *= norm.x;
	p1 *= norm.y;
	p2 *= norm.z;
	p3 *= norm.w;
	
	// Mix final noise value
	vec4 m = max(0.6 - vec4(dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), 0.0);
	m = m * m;
	return 42.0 * dot( m*m, vec4( dot(p0,x0), dot(p1,x1),
								  dot(p2,x2), dot(p3,x3) ) );
}
		
const float interval = 3.0;

vec4 glitch(vec2 texco) {
	float time = iGlobalTime * glitchspeed;
	float strength = smoothstep(interval * 0.5, interval, interval - mod(time, interval));
	vec2 shake = vec2(strength * 8.0 + 0.5) * vec2(
	  random2(vec2(time)) * 2.0 - 1.0,
	  random2(vec2(time * 2.0)) * 2.0 - 1.0
	) / vec2(fbowidth, fboheight);
	
	float y = texco.y * fboheight;
	float rgbWave = (
		snoise3(vec3(0.0, y * 0.01, time * 400.0)) * (2.0 + strength * 32.0)
		* snoise3(vec3(0.0, y * 0.02, time * 200.0)) * (1.0 + strength * 4.0)
		+ step(0.9995, sin(y * 0.005 + time * 1.6)) * 12.0
		+ step(0.9999, sin(y * 0.005 + time * 2.0)) * -18.0
	  ) / fbowidth;
	float rgbDiff = (6.0 + sin(time * 500.0 + texco.y * 40.0) * (20.0 * strength + 1.0)) / fbowidth;
	float rgbUvX = texco.x + rgbWave;
	float r = texture2D(Sampler0, vec2(rgbUvX + rgbDiff, texco.y) + shake).r;
	float g = texture2D(Sampler0, vec2(rgbUvX, texco.y) + shake).g;
	float b = texture2D(Sampler0, vec2(rgbUvX - rgbDiff, texco.y) + shake).b;
	
	float whiteNoise = (random2(texco + mod(time, 10.0)) * 2.0 - 1.0) * (0.15 + strength * 0.15);
	
	float bnTime = floor(time * 20.0) * 200.0;
	float noiseX = step((snoise3(vec3(0.0, texco.x * 3.0, bnTime)) + 1.0) / 2.0, 0.12 + strength * 0.3);
	float noiseY = step((snoise3(vec3(0.0, texco.y * 3.0, bnTime)) + 1.0) / 2.0, 0.12 + strength * 0.3);
	float bnMask = noiseX * noiseY;
	float bnUvX = texco.x + sin(bnTime) * 0.2 + rgbWave;
	float bnR = texture2D(Sampler0, vec2(bnUvX + rgbDiff, texco.y)).r * bnMask;
	float bnG = texture2D(Sampler0, vec2(bnUvX, texco.y)).g * bnMask;
	float bnB = texture2D(Sampler0, vec2(bnUvX - rgbDiff, texco.y)).b * bnMask;
	vec4 blockNoise = vec4(bnR, bnG, bnB, 1.0);
	
	float bnTime2 = floor(time * 25.0) * 300.0;
	float noiseX2 = step((snoise3(vec3(0.0, texco.x * 2.0, bnTime2)) + 1.0) / 2.0, 0.12 + strength * 0.5);
	float noiseY2 = step((snoise3(vec3(0.0, texco.y * 8.0, bnTime2)) + 1.0) / 2.0, 0.12 + strength * 0.3);
	float bnMask2 = noiseX2 * noiseY2;
	float bnR2 = texture2D(Sampler0, vec2(bnUvX + rgbDiff, texco.y)).r * bnMask2;
	float bnG2 = texture2D(Sampler0, vec2(bnUvX, texco.y)).g * bnMask2;
	float bnB2 = texture2D(Sampler0, vec2(bnUvX - rgbDiff, texco.y)).b * bnMask2;
	vec4 blockNoise2 = vec4(bnR2, bnG2, bnB2, 1.0);
	
	float waveNoise = (sin(texco.y * 1200.0) + 1.0) / 2.0 * (0.15 + strength * 0.2);
	
	return vec4(r, g, b, 1.0) * (1.0 - bnMask - bnMask2) + (whiteNoise + blockNoise + blockNoise2 - waveNoise);
}

vec4 binary_glitch(vec2 uv)  //binary glitch shadertoy  waiting for permission - this function is CC licenced at the moment!
{
	return vec4(0, 0, 0, 0);
    float x = uv.s;
    float y = uv.t;
        
    // get snapped position
    float psize = 0.04 * glitchstr;
    float psq = 1.0 / psize;

    float px = floor(x * psq + 0.5) * psize;
    float py = floor(y * psq + 0.5) * psize;
    
	vec4 colSnap = texture2D(Sampler0, vec2( px,py) );
    
	float lum = pow( 1.0 - (colSnap.r + colSnap.g + colSnap.b) / 3.0, glitchstr); // remove the minus one if you want to invert luma
    
    // do move with lum as multiplying factor
    float qsize = psize * lum;
    
    float qsq = 1.0 / qsize;

    float qx = floor(x * qsq + 0.5f) * qsize;
    float qy = floor(y * qsq + 0.5f) * qsize;

    float rx = (px - qx) * lum + x;
    float ry = (py - qy) * lum + y;
    
	vec4 colMove = texture2D(Sampler0, vec2( rx,ry));
    
    // final color
    return colMove;
}

float rand(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453) * 0.5 - 0.25;
}

vec4 noise(vec2 uv)  //ok
{
    vec4 color = texture2D(Sampler0, uv);
	return vec4(max(min(color.rgb + vec3(rand(uv) * noiselevel), vec3(1.0f)), vec3(0.0f)), color.a);
}


vec3 gammafunc(vec3 col, float g)
{
    float i = 1.0f / g;
    return vec3(pow(col.x, i), pow(col.y, i), pow(col.z, i));
}

vec3 texfilter(vec2 texco)
{
    vec3 val = texture2D(Sampler0, texco).xyz;    
	return gammafunc(val, gammaval);
}

vec4 gammamain(vec2 texco)  //shadertoy WTFPL
{
    vec3 cf = texfilter(texco);
   
    return vec4(cf, 1);
}


vec4 thermal(vec2 texco)  //geeks3d free
{ 
    vec3 tc = vec3(1.0, 0.0, 0.0);
    vec4 pixcol = texture2D(Sampler0, texco);
    vec3 colors[3];
    colors[0] = vec3(0.,0.,1.);
    colors[1] = vec3(1.,1.,0.);
    colors[2] = vec3(1.,0.,0.);
	float lum = dot(vec3(0.30, 0.59, 0.11), pixcol.rgb);
    int ix = (lum < 0.5)? 0:1;
    tc = mix(colors[ix],colors[ix+1],(lum-float(ix)*0.5)/0.5);

    return vec4(tc, pixcol.a);
}


#define GOLDEN_ANGLE 2.39996
#define ITERATIONS 150
mat2 rot = mat2(cos(GOLDEN_ANGLE), sin(GOLDEN_ANGLE), -sin(GOLDEN_ANGLE), cos(GOLDEN_ANGLE));

vec3 Bokeh(sampler2D tex, vec2 uv, float radius)  // kindly shared by Dave H.
{
	vec3 acc = vec3(0), div = acc;
    float r = 1.;
    vec2 vangle = vec2(0.0,radius*.01 / sqrt(float(ITERATIONS)));
    
	for (int j = 0; j < ITERATIONS; j++)
    {  
        // the approx increase in the scale of sqrt(0, 1, 2, 3...)
        r += 1. / r;
	    vangle = rot * vangle;
	    vec2 angle = (r-1.) * vangle;
	    angle.y *= float(fbowidth) / float(fboheight);
        vec3 col = texture2D(tex, uv + angle).xyz; /// ... Sample the image
        col = col * col *1.8; // ... Contrast it for better highlights - leave this out elsewhere.
		vec3 bokeh = pow(col, vec3(4));
		acc += col * bokeh;
		div += bokeh;
	}
	return acc / div;
}

vec4 bokehmain(in vec2 uv)
{
    return vec4(Bokeh(Sampler0, uv, bokehrad), 1.0);
}


vec3 getHueColor(vec2 pos)
{
	float theta = 3.0 + 3.0 * atan(pos.x, pos.y) / PI;
	vec3 color = vec3(0.0);
	return clamp(abs(mod(theta + vec3(0.0, 4.0, 2.0), 6.0) - 3.0) - 1.0, 0.0, 1.0);
}

void colorwheel()
{
	vec2 uv = vec2(8.0, -8.0) * (gl_FragCoord.xy - vec2(cwx, cwy) * vec2(globw, globh)) / float(globh);
	vec2 mouse = vec2(8.0, -8.0) * (vec2(mx, globh - my) - vec2(cwx, cwy) * vec2(globw, globh)) / globh;
	
	float l = length(uv);
	float m = length(mouse);
	
	FragColor = vec4(0.0, 0.0f, 0.0f, 0.0f);

	if (l >= 0.75 && l <= 1.0)
	{
		l = 1.0 - abs((l - 0.875) * 8.0);
		l = clamp(l * fboheight * 0.0625, 0.0, 1.0); // Antialiasing approximation
		
		FragColor = vec4(l * getHueColor(uv), l);
	}
	else if (l < 0.75)
	{
		vec3 pickedHueColor;
		
		if (!cwmouse)
		{
			mouse = vec2(0.0, -1.0);
			pickedHueColor = vec3(1.0, 0.0, 0.0);
		}
		else
		{
			pickedHueColor = getHueColor(mouse);
		}
		
		uv = uv / 0.75;
		mouse = normalize(mouse);
		
		float sat = 1.5 - (dot(uv, mouse) + 0.5); // [0.0,1.5]
		
		if (sat < 1.5)
		{
			float h = sat / sqrt(3.0);
			vec2 om = vec2(cross(vec3(mouse, 0.0), vec3(0.0, 0.0, 1.0)));
			float lum = dot(uv, om);
			
			if (abs(lum) <= h)
			{
				l = clamp((h - abs(lum)) * fboheight * cwx, 0.0, 1.0) * clamp((1.5 - sat) / 1.5 * fboheight * cwy, 0.0, 1.0); // Fake antialiasing
				FragColor = vec4(l * mix(pickedHueColor, vec3(0.5 * (lum + h) / h), sat / 1.5), l);
			}
		}
	}
}


//sharpen start
//#pragma parameter curve_height "AS Sharpness" 0.8 0.1 2.0 0.1

#define VIDEO_LEVEL_OUT 0.0

#define video_level_out VIDEO_LEVEL_OUT      // True to preserve BTB & WTW (minor summation error)
                                             // Normally it should be set to false

uniform float curve_height = 0.8f;
float curveslope = (curve_height*1.5f);   // Sharpening curve slope, edge region
uniform float D_overshoot = 0.016f;              // Max dark overshoot before max compression
uniform float D_comp_ratio = 0.250f;               // Max compression ratio, dark overshoot (1/0.25=4x)
uniform float L_overshoot = 0.004f;                // Max light overshoot before max compression
uniform float L_comp_ratio = 0.167f;                // Max compression ratio, light overshoot (1/0.167=6x)
uniform float max_scale_lim = 10.0f;                // Abs change before max compression (1/10=�10%)

// Colour to greyscale, fast approx gamma
float CtG(vec3 RGB) { return  sqrt( (1.0/3.0)*((RGB*RGB).r + (RGB*RGB).g + (RGB*RGB).b) ); }

vec4 sharpen(vec2 texco)    //https://github.com/libretro/glsl-shaders - the following disclaimer belongs to this functions code
{
// Copyright (c) 2015, bacondither
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer
//    in this position and unchanged.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
// NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */	

	vec2	tex	=	texco;
	
	float	px	=	1.0f / fbowidth;
	float	py	=	1.0f / fboheight;

// Get points and saturate out of range values (BTB & WTW)
// [                c22               ]
// [           c24, c9,  c23          ]
// [      c21, c1,  c2,  c3, c18      ]
// [ c19, c10, c4,  c0,  c5, c11, c16 ]
// [      c20, c6,  c7,  c8, c17      ]
// [           c15, c12, c14          ]
// [                c13               ]
	vec3	 c19	=	clamp( texture2D(Sampler0, texco + vec2(-3.*px,   0.)).rgb, 0.0, 1.0);
	vec3	 c21	=	clamp( texture2D(Sampler0, texco + vec2(-2.*px,  -py)).rgb, 0.0, 1.0);
	vec3	 c10	=	clamp( texture2D(Sampler0, texco + vec2(-2.*px,   0.)).rgb, 0.0, 1.0);
	vec3	 c20	=	clamp( texture2D(Sampler0, texco + vec2(-2.*px,   py)).rgb, 0.0, 1.0);
	vec3	 c24	=	clamp( texture2D(Sampler0, texco + vec2(  -px,-2.*py)).rgb, 0.0, 1.0);
	vec3	 c1 	=	clamp( texture2D(Sampler0, texco + vec2(  -px,  -py)).rgb, 0.0, 1.0);
	vec3	 c4 	=	clamp( texture2D(Sampler0, texco + vec2(  -px,   0.)).rgb, 0.0, 1.0);
	vec3	 c6 	=	clamp( texture2D(Sampler0, texco + vec2(  -px,   py)).rgb, 0.0, 1.0);
	vec3	 c15	=	clamp( texture2D(Sampler0, texco + vec2(  -px, 2.*py)).rgb, 0.0, 1.0);
	vec3	 c22	=	clamp( texture2D(Sampler0, texco + vec2(   0., -3.*py)).rgb, 0.0, 1.0);
	vec3	 c9 	=	clamp( texture2D(Sampler0, texco + vec2(   0., -2.*py)).rgb, 0.0, 1.0);
	vec3	 c2 	=	clamp( texture2D(Sampler0, texco + vec2(   0.,   -py)).rgb, 0.0, 1.0);
	vec3	 c0 	=	clamp( texture2D(Sampler0, texco).rgb, 0.0, 1.0);
	vec3	 c7 	=	clamp( texture2D(Sampler0, texco + vec2(   0.,    py)).rgb, 0.0, 1.0);
	vec3	 c12	=	clamp( texture2D(Sampler0, texco + vec2(   0.,  2.*py)).rgb, 0.0, 1.0);
	vec3	 c13	=	clamp( texture2D(Sampler0, texco + vec2(   0.,  3.*py)).rgb, 0.0, 1.0);
	vec3	 c23	=	clamp( texture2D(Sampler0, texco + vec2(   px,-2.*py)).rgb, 0.0, 1.0);
	vec3	 c3 	=	clamp( texture2D(Sampler0, texco + vec2(   px,  -py)).rgb, 0.0, 1.0);
	vec3	 c5 	=	clamp( texture2D(Sampler0, texco + vec2(   px,   0.)).rgb, 0.0, 1.0);
	vec3	 c8 	=	clamp( texture2D(Sampler0, texco + vec2(   px,   py)).rgb, 0.0, 1.0);
	vec3	 c14	=	clamp( texture2D(Sampler0, texco + vec2(   px, 2.*py)).rgb, 0.0, 1.0);
	vec3	 c18	=	clamp( texture2D(Sampler0, texco + vec2( 2.*px,  -py)).rgb, 0.0, 1.0);
	vec3	 c11	=	clamp( texture2D(Sampler0, texco + vec2( 2.*px,   0.)).rgb, 0.0, 1.0);
	vec3	 c17	=	clamp( texture2D(Sampler0, texco + vec2( 2.*px,   py)).rgb, 0.0, 1.0);
	vec3	 c16	=	clamp( texture2D(Sampler0, texco + vec2( 3.*px,   0.)).rgb, 0.0, 1.0 );
	
// Blur, gauss 3x3
	vec3	blur	=	(2.*(c2 + c4 + c5 + c7) + (c1 + c3 + c6 +c8) + 4.*c0)/16.;
	float	blur_Y	=	(blur.r*(1.0/3.0) + blur.g*(1.0/3.0) + blur.b*(1.0/3.0));
	
// Edge detection
// Matrix, relative weights
// [           1          ]
// [       4,  4,  4      ]
// [   1,  4,  4,  4,  1  ]
// [       4,  4,  4      ]
// [           1          ]
	float	edge	=	length( abs(blur-c0) + abs(blur-c1) + abs(blur-c2) + abs(blur-c3)
					+	abs(blur-c4) + abs(blur-c5) + abs(blur-c6) + abs(blur-c7) + abs(blur-c8)
					+	0.25*(abs(blur-c9) + abs(blur-c10) + abs(blur-c11) + abs(blur-c12)) )*(1.0/3.0);

// Edge detect contrast compression, center = 0.5
	edge	*=	min((0.8+2.7*pow(2., (-7.4*blur_Y))), 3.2);
	
// RGB to greyscale
	float	c0_Y	=	CtG(c0);
	
	float	kernel[25]	=	float[]( c0_Y,  CtG(c1), CtG(c2), CtG(c3), CtG(c4), CtG(c5), CtG(c6), CtG(c7), CtG(c8),
							CtG(c9), CtG(c10), CtG(c11), CtG(c12), CtG(c13), CtG(c14), CtG(c15), CtG(c16),
							CtG(c17), CtG(c18), CtG(c19), CtG(c20), CtG(c21), CtG(c22), CtG(c23), CtG(c24) );
			
// Partial laplacian outer pixel weighting scheme
	float	mdiff_c0	=	0.03 + 4.*( abs(kernel[0]-kernel[2]) + abs(kernel[0]-kernel[4])
						+	abs(kernel[0]-kernel[5]) + abs(kernel[0]-kernel[7])
						+	0.25*(abs(kernel[0]-kernel[1]) + abs(kernel[0]-kernel[3])
						+	abs(kernel[0]-kernel[6]) + abs(kernel[0]-kernel[8])) );
								  
	float	mdiff_c9	=	( abs(kernel[9]-kernel[2])   + abs(kernel[9]-kernel[24])
						+	abs(kernel[9]-kernel[23])  + abs(kernel[9]-kernel[22])
						+	0.5*(abs(kernel[9]-kernel[1]) + abs(kernel[9]-kernel[3])) );

	float	mdiff_c10	=	( abs(kernel[10]-kernel[20]) + abs(kernel[10]-kernel[19])
						+	abs(kernel[10]-kernel[21]) + abs(kernel[10]-kernel[4])
						+	0.5*(abs(kernel[10]-kernel[1]) + abs(kernel[10]-kernel[6])) );

	float	mdiff_c11	=	( abs(kernel[11]-kernel[17]) + abs(kernel[11]-kernel[5])
						+	abs(kernel[11]-kernel[18]) + abs(kernel[11]-kernel[16])
						+	0.5*(abs(kernel[11]-kernel[3]) + abs(kernel[11]-kernel[8])) );

	float	mdiff_c12	=	( abs(kernel[12]-kernel[13]) + abs(kernel[12]-kernel[15])
						+	abs(kernel[12]-kernel[7])  + abs(kernel[12]-kernel[14])
						+	0.5*(abs(kernel[12]-kernel[6]) + abs(kernel[12]-kernel[8])) );

	vec4	weights		=	vec4( (min((mdiff_c0/mdiff_c9), 2.0)), (min((mdiff_c0/mdiff_c10),2.0)),
							(min((mdiff_c0/mdiff_c11),2.0)), (min((mdiff_c0/mdiff_c12),2.0)) );
						  
// Negative laplace matrix
 // Matrix, relative weights, *Varying 0<->8
 // [          8*         ]
 // [      4,  1,  4      ]
 // [  8*, 1,      1,  8* ]
 // [      4,  1,  4      ]
 // [          8*         ]
	float	neg_laplace	=	( 0.25 * (kernel[2] + kernel[4] + kernel[5] + kernel[7])
						+	(kernel[1] + kernel[3] + kernel[6] + kernel[8])
						+	((kernel[9]*weights.x) + (kernel[10]*weights.y)
						+	(kernel[11]*weights.z) + (kernel[12]*weights.w)) )
						/	(5. + weights.x + weights.y + weights.z + weights.w);
						
 // Compute sharpening magnitude function, x = edge mag, y = laplace operator mag
	float	sharpen_val	=	0.01 + (curve_height/(curveslope*pow(edge, 3.5) + 0.5))
						-	(curve_height/(8192.*pow((edge*2.2), 4.5) + 0.5));

 // Calculate sharpening diff and scale
	float	sharpdiff	=	(c0_Y - neg_laplace)*(sharpen_val*0.8);
	
// Calculate local near min & max, partial cocktail sort (No branching!)
	for	(int i = 0; i < 2; ++i)
	{
		for	(int i1 = 1+i; i1 < 25-i; ++i1)
		{
			float temp		=	kernel[i1-1];
			kernel[i1-1]	=	min(kernel[i1-1], kernel[i1]);
			kernel[i1]		=	max(temp, kernel[i1]);
		}

		for	(int i2 = 23-i; i2 > i; --i2)
		{
			float temp		=	kernel[i2-1];
			kernel[i2-1]	=	min(kernel[i2-1], kernel[i2]);
			kernel[i2]		=	max(temp, kernel[i2]);
		}
	}
	
	float	nmax		=	max(((kernel[23] + kernel[24])/2.), c0_Y);
	float	nmin		=	min(((kernel[0]  + kernel[1])/2.),  c0_Y);

// Calculate tanh scale factor, pos/neg
	float	nmax_scale	=	max((1./((nmax - c0_Y) + L_overshoot)), max_scale_lim);
	float	nmin_scale	=	max((1./((c0_Y - nmin) + D_overshoot)), max_scale_lim);

// Soft limit sharpening with tanh, mix to control maximum compression
	sharpdiff			=	mix( (tanh((max(sharpdiff, 0.0))*nmax_scale)/nmax_scale), (max(sharpdiff, 0.0)), L_comp_ratio )
						+	mix( (tanh((min(sharpdiff, 0.0))*nmin_scale)/nmin_scale), (min(sharpdiff, 0.0)), D_comp_ratio );

   return vec4(c0.rgb, texture2D(Sampler0, texco).a) + sharpdiff;
}


// dither stuff
vec4 outsize = vec4(vec2(fbowidth, fboheight), 1.0 / vec2(fbowidth, fboheight));

uniform float animate = 0.0f;
uniform float dither_size = 0.95f;

float find_closest(int x, int y, float c0)
{
int dither[8][8] = {
{ 0, 32, 8, 40, 2, 34, 10, 42}, /* 8x8 Bayer ordered dithering */
{48, 16, 56, 24, 50, 18, 58, 26}, /* pattern. Each input pixel */
{12, 44, 4, 36, 14, 46, 6, 38}, /* is scaled to the 0..63 range */
{60, 28, 52, 20, 62, 30, 54, 22}, /* before looking in this table */
{ 3, 35, 11, 43, 1, 33, 9, 41}, /* to determine the action. */
{51, 19, 59, 27, 49, 17, 57, 25},
{15, 47, 7, 39, 13, 45, 5, 37},
{63, 31, 55, 23, 61, 29, 53, 21} }; 

float limit = 0.0;
if(x < 8)
{
limit = (dither[x][y]+1)/64.0;
}

if(c0 < limit)
return 0.0;
return 1.0;
}

vec4 dither(vec2 texco)  //https://github.com/libretro/glsl-shaders/blob/master/dithering/shaders/bayer-matrix-dithering.glsl  free!
{
	float Scale = 3.0 + mod(2.0 * iGlobalTime, 32.0) * animate + dither_size;
	vec4 lum = vec4(0.299, 0.587, 0.114, 0);
	float grayscale = dot(texture2D(Sampler0, texco), lum);
	vec3 rgb = texture2D(Sampler0, texco).rgb;
	
	vec2 xy = (texco * outsize.xy) * Scale;
	int x = int(mod(xy.x, 8));
	int y = int(mod(xy.y, 8));
	
	vec3 finalRGB;
	finalRGB.r = find_closest(x, y, rgb.r);
	finalRGB.g = find_closest(x, y, rgb.g);
	finalRGB.b = find_closest(x, y, rgb.b);
	
	float final = find_closest(x, y, grayscale);

   return vec4(finalRGB, 1.0);
}


vec4 flip(vec2 texco)  //selfmade
{
	if (xflip) texco.x = 1.0f - texco.x;
	if (yflip) texco.y = 1.0f - texco.y;
	return texture2D(Sampler0, texco);
}


vec4 mirror(vec2 texco)  //selfmade
{
	int xm = int(xmirror + 0.5f);
	int ym = int(ymirror + 0.5f);
	if (xm == 0) {
		if (texco.x > 0.5f) texco.x = 1.0f - texco.x;
	}
	else if (xm == 2) {
		if (texco.x <= 0.5f) texco.x = 1.0f - texco.x;
	}
	if (ym == 2) {
		if (texco.y > 0.5f) texco.y = 1.0f - texco.y;
	}
	else if (ym == 0) {
		if (texco.y <= 0.5f) texco.y = 1.0f - texco.y;
	}
	return texture2D(Sampler0, texco);
}





void main()
{
	float cf2 = 1.0f - cf;
    vec4 intcol, texcol;
    vec4 tex0, tex1;
    vec4 fc;
    int brk = 0;
	vec2 texco;
	texco = TexCoord0.st;
    if (interm == 1) {
		texcol = texture2D(Sampler0, texco);
		switch (fxid) {
			case 0:
				intcol = blur(texco);
				break;
			case 5:
				intcol = boxblur(texco);
				intcol = vec4(intcol.rgb * glowfac, intcol.a);
				break;
			case 1:
				intcol = brightness(texcol); break;
			case 2:
				intcol = chromarotate(texcol); break;
			case 3:
				intcol = contrast(texcol); break;
			case 4:
				intcol = dotf(texco); break;
			case 6:
				intcol = radialblur(texco); break;
			case 7:
				intcol = saturation(texcol); break;
			case 8:
				intcol = texture2D(Sampler0, scale(texco)); break;
			case 9:
				intcol = swirl(texco); break;
			case 10:
				intcol = oldfilm(texco); break;
			case 11:
				intcol = ripple(); break;
			case 12:
				intcol = fisheye(texco); break;
			case 13:
				intcol = treshold(texcol); break;
			case 14:
				if (strobephase == 0) intcol = texcol;
				else intcol = vec4(strobered, strobegreen, strobeblue, 1.0); break;
			case 15:
				intcol = posterize(texcol); break;
			case 16:
				intcol = pixelate(texco); break;
			case 17:
				intcol = crosshatch(texcol, texco); break;
			case 18:
				// Invert
				intcol = vec4(1.0 - texcol.r, 1.0 - texcol.g, 1.0 - texcol.b, 1.0); break;
			case 19:
				intcol = rotate(texco); break;
			case 20:
				intcol = emboss(texco); break;
			case 21:
				intcol = ascii(texco); break;
			case 22:
				intcol = solarize(texco); break;
			case 23:
				intcol = vardot(texco); break;
			case 24:
				intcol = crt(texco); break;
			case 25:
				intcol = edgedetect(texco); break;
			case 26:
				intcol = kaleidoscope(texco); break;
			case 27:
				intcol = halftone(texcol, texco); break;
			case 28:
				intcol = cartoon(texco); break;
			case 29:
				intcol = cutoff(texco); break;
			case 30:
				intcol = glitch(texco); break;
			case 31:
				intcol = colorize(texcol); break;
			case 32:
				intcol = noise(texco); break;
			case 33:
				intcol = gammamain(texco); break;
			case 34:
				intcol = thermal(texco); break;
			case 35:
				intcol = bokehmain(texco); break;
			case 36:
				intcol = sharpen(texco); break;
			case 37:
				intcol = dither(texco); break;
			case 38:
				intcol = flip(texco); break;
			case 39:
				intcol = mirror(texco); break;
			case 40:
				intcol = boxblur(texco); break;
		}
    	FragColor = vec4(intcol.rgb * drywet + (1.0f - drywet) * texcol.rgb, intcol.a * opacity);
    	return;
	}
	else if (mixmode > 0) {
		//vec2 size0 = textureSize(endSampler0, 0);
		//vec2 size1 = textureSize(endSampler1, 0);
		//tex0 = texture2D(endSampler0, vec2((texco.x - 0.5f) * fbowidth * fcdiv / size0.x + 0.5f, (texco.y - 0.5f) * fboheight * fcdiv / size0.y + 0.5f));
		//tex1 = texture2D(endSampler1, vec2((texco.x - 0.5f) * fbowidth * fcdiv / size1.x + 0.5f, (texco.y - 0.5f) * fboheight * fcdiv / size1.y + 0.5f));
		tex0 = texture2D(endSampler0, texco);
		tex1 = texture2D(endSampler1, texco);
		//tex0 = vec4(tex0.rgb * tex0.a, tex0.a);
		//tex1 = vec4(tex1.rgb * tex1.a, tex1.a);
	}
	if (cwon) {
		colorwheel();
		return;
	}
	else if (mixmode == 1) {
		//MIX alpha
		float mf = mixfac;
		float fac1 = clamp((1.0f - mf) * 2.0f, 0.0f, 1.0f);
		float fac2 = clamp(mf * 2.0f, 0.0f, 1.0f);
		float term0 = (1.0f - fac2 * tex1.a / 2.0f) * fac1;
		float term1 = (1.0f - fac1 * tex0.a / 2.0f) * fac2;
		fc = vec4((tex0.rgb * (term0 + (1.0f - tex1.a) * (1.0f - term0)) + tex1.rgb * (term1 + (1.0f - tex0.a) * (1.0f - term1))), max(tex0.a, tex1.a));
	}
	else if (mixmode == 2) {
		//MULTIPLY alpha
		fc = vec4(tex0.rgb * vec3(1.0f, 1.0f, 1.0f) + ((tex1.rgb - vec3(1.0f, 1.0f, 1.0f)) * tex1.a), max(tex0.a, tex1.a));
	}
	else if (mixmode == 3) {
		//SCREEN alpha
		fc = 1.0 - (1.0 - tex0) * (1.0 - tex1);
	}
	else if (mixmode == 4) {
		//OVERLAY  alpha
		if ((tex0.r + tex0.g + tex0.b) / 3.0 < 0.5) {
			fc = vec4(2.0f * tex0.rgb * vec3(0.5f, 0.5f, 0.5f) + ((tex1.rgb - vec3(0.5f, 0.5f, 0.5f)) * tex1.a), max(tex0.a, tex1.a));
		}
		else fc = 1.0 - 2 * ((tex1.a / 2.0f) + 0.5f) * (1.0 - tex0) * (1.0 - tex1);
	}
	else if (mixmode == 5) {
		//HARD LIGHT alpha
		if ((tex0.r + tex0.g + tex0.b) / 3.0 >= 0.5) {
			fc = vec4(2.0f * tex0.rgb * vec3(0.5f, 0.5f, 0.5f) + ((tex1.rgb - vec3(0.5f, 0.5f, 0.5f)) * tex1.a), max(tex0.a, tex1.a));
		}
		else fc = 1.0 - 2 * ((tex1.a / 2.0f) + 0.5f) * (1.0 - tex0) * (1.0 - tex1);
	}
	else if (mixmode == 6) {
		//SOFT LIGHT
		fc = (1.0 - 2.0 * tex1) * tex0 * tex0 + 2 * tex1 * tex0;
	}
	else if (mixmode == 7) {
		//DIVIDE alpha
		fc = vec4(tex0.rgb / (vec3(1.0f, 1.0f, 1.0f) + ((tex1.rgb - vec3(1.0f, 1.0f, 1.0f)) * tex1.a)), max(tex0.a, tex1.a));
	}
	else if (mixmode == 8) {
		//ADD alpha
		fc = vec4(tex0.rgb + tex1.rgb, tex0.a);
	}
	else if (mixmode == 9) {
		//SUBTRACT alpha
		fc = vec4(tex0.rgb - tex1.rgb, tex0.a);
	}
	else if (mixmode == 10) {
		//DIFF alpha
		fc = vec4(abs(tex0.rgb - tex1.rgb), max(tex0.a, tex1.a));
	}
	else if (mixmode == 11) {
		//DODGE alpha
		fc = vec4(tex0.rgb / (vec3(1.0f, 1.0f, 1.0f) - ((vec3(1.0f, 1.0f, 1.0f) - tex1.rgb) * tex1.a)), max(tex0.a, tex1.a));
		//fc = tex0 / (1.0 - tex1);
	}
	else if (mixmode == 12) {
		//COLOR_BURN
		fc = 1.0 - ((1.0 - tex0) / tex1);
	}
	else if (mixmode == 13) {
		//LINEAR_BURN
		fc = (tex0 + tex1) - 1.0;
	}
	else if (mixmode == 14) {
		//VIVID LIGHT
		if ((tex0.r + tex0.g + tex0.b) / 3.0 >= 0.5) fc = vec4(tex0.rgb / (1.0 - tex1.rgb), 1.0);
		else fc = 1.0 - ((1.0 - tex0) / tex1);
	}
	else if (mixmode == 15) {
		// LINEAR_LIGHT
		if ((tex0.r + tex0.g + tex0.b) / 3.0 >= 0.5) fc = tex0 + tex1;
		else fc = (tex0 + tex1) - 1.0;
	}
	else if (mixmode == 16) {
		//DARKEN_ONLY alpha
		fc = vec4(min(tex0.r, tex1.r + (1.0f - tex1.a)), min(tex0.g, tex1.g + (1.0f - tex1.a)), min(tex0.b, tex1.b + (1.0f - tex1.a)), max(tex0.a, tex1.a));
	}
	else if (mixmode == 17) {
		//LIGHTEN_ONLY alpha
		fc = vec4(max(tex0.r, tex1.r), max(tex0.g, tex1.g), max(tex0.b, tex1.b), max(tex0.a, tex1.a));
	}
	else if (mixmode == 22) {
		//DISPLACEMENT alpha
		fc = texture2D(endSampler0, (TexCoord0.st+(texture2D(endSampler1, TexCoord0.st).rb)*.1*tex1.a) * 0.91f);
	}
	else if (mixmode == 23) {
		//CROSSFADING alpha
		float fac1 = clamp(cf2 * 2.0f, 0.0f, 1.0f);
		float fac2 = clamp(cf * 2.0f, 0.0f, 1.0f);
		fc = vec4((tex0.rgb * (1.0f - fac2 * tex1.a / 2.0f) * fac1 + tex1.rgb * (1.0f - fac1 * tex0.a / 2.0f) * fac2), max(fac1 * tex0.a, fac2 * tex1.a));
	}
	else if (mixmode == 19) {
		//COLORKEY alpha
		if (chdir) {
			vec4 bu;
			bu = tex0;
			tex0 = tex1;
			tex1 = bu;
		}
		float totdiff = (abs(chred - tex1.r) + abs(chgreen - tex1.g) + abs(chblue - tex1.b)) * tex1.a;
		if (totdiff > colortol) {
			if (chinv) fc = vec4(tex0.rgb, 1.0f);
			else fc = vec4(tex1.rgb, 1.0f);
		}
		else {
			float ia = (colortol - totdiff) * -(feather - 5.2f);
			if (ia > 1.0f) ia = 1.0f;
			float a = 1.0f - ia;
			if (chinv) fc = vec4(tex1.rgb * ia + tex0.rgb * a, max(tex0.a, tex1.a));
			else fc = vec4(tex0.rgb * ia + tex1.rgb * a, max(tex0.a, tex1.a));
		}
	}
	else if (mixmode == 20) {
		//CHROMAKEY alpha
		if (chdir) {
			vec4 bu;
			bu = tex0;
			tex0 = tex1;
			tex1 = bu;
		}

		float huetol = colortol / 4.0f;

		float huediff = abs(rgb2hsv(vec3(chred, chgreen, chblue)).x - rgb2hsv(vec3(tex1.r, tex1.g, tex1.b)).x);
		if (huediff > 0.5f) huediff = 1.0f - huediff;
		if (huediff > huetol) {
			if (chinv) fc = vec4(tex0.rgb, 1.0f);
			else fc = vec4(tex1.rgb, 1.0f);
		}
		else {
			float ia = ((huetol - huediff) * (-(feather - 5.2f) / 4.0f)) * 4.0f;
			if (ia > 1.0f) ia = 1.0f;
			float a = 1.0f - ia;
		    if (chinv) fc = vec4(tex1.rgb * ia + tex0.rgb * a, max(tex0.a, tex1.a));
			else fc = vec4(tex0.rgb * ia + tex1.rgb * a, max(tex0.a, tex1.a));
		}
	}
	else if (mixmode == 21) {
		//LUMAKEY alpha
		if (chdir) {
			vec4 bu;
			bu = tex0;
			tex0 = tex1;
			tex1 = bu;
		}

		float lumtol = colortol / 3.0f;

		float lumdiff = abs(rgb2hsv(vec3(chred, chgreen, chblue)).z - rgb2hsv(vec3(tex1.r, tex1.g, tex1.b)).z);
		if (lumdiff > lumtol) {
			if (chinv) fc = vec4(tex0.rgb, 1.0f);
			else fc = vec4(tex1.rgb, 1.0f);
		}
		else {
			float ia = ((lumtol - lumdiff) * (-(feather - 5.2f) / 3.0f)) * 3.0f;
			if (ia > 1.0f) ia = 1.0f;
			float a = 1.0f - ia;
			if (chinv) fc = vec4(tex1.rgb * ia + tex0.rgb * a, max(tex0.a, tex1.a));
			else fc = vec4(tex0.rgb * ia + tex1.rgb * a, max(tex0.a, tex1.a));
		}
	}

	if (mixmode > 0) {
		//alpha demultiplying
		FragColor = vec4(fc.rgb, fc.a);
	}
	else if (textmode == 1) {
		float c = texture2D(Sampler0, vec2(TexCoord0.s, TexCoord0.t)).r;
		FragColor = vec4(color.rgb, c);
		return;
	}
	else if (edgethickmode == 1) {
		float thx = 1.0f / fbowidth;
		float thy = 1.0f / fboheight;
		float left = texture2D(fboSampler, vec2(TexCoord0.s - thx, TexCoord0.t)).r;
		float right = texture2D(fboSampler, vec2(TexCoord0.s + thx, TexCoord0.t)).r;
		float above = texture2D(fboSampler, vec2(TexCoord0.s, TexCoord0.t - thy)).r;
		float under = texture2D(fboSampler, vec2(TexCoord0.s, TexCoord0.t + thy)).r;
		float border = max(max(max(left, right), above), under);
		FragColor = vec4(border, border, border, texture2D(fboSampler, TexCoord0).a);
	}
	else if (thumb == 1) {
		FragColor = vec4(texture2D(Sampler0, TexCoord0.st).rgb, 0.7f);
	}
	else if (singlelayer == 1) {
		//vec2 size0 = textureSize(Sampler0, 0);
		//vec4 ic = texture2D(Sampler0, vec2((texco.x - 0.5f) * fbowidth * fcdiv / size0.x + 0.5f, (texco.y - 0.5f) * fboheight * fcdiv / size0.y + 0.5f));
		vec4 ic = texture2D(Sampler0, texco);
		FragColor = vec4(ic.r, ic.g, ic.b, ic.a * opacity);
	}
	else if (down == 1) {
		vec4 ic = texture2D(Sampler0, TexCoord0.st).rgba;
		if (!inverted) FragColor = vec4(ic.r, ic.g, ic.b, ic.a * opacity);
		else FragColor = vec4(1.0f - ic.r, 1.0f - ic.g, 1.0f - ic.b, ic.a * opacity);
	}
	else if (circle == 1) {
		if (distance(vec2(cirx, ciry), gl_FragCoord.xy) < circleradius - 1.0f) {
			FragColor = color;
		}
		else discard;
	}
	else if (circle == 2) {
		if (distance(vec2(cirx, ciry), gl_FragCoord.xy) < circleradius - 1.0f) {
			if (distance(vec2(cirx, ciry), gl_FragCoord.xy) > circleradius - 3.0f) {
				FragColor = color;
			}
			else discard;
		}
		else discard;
	}
	else if (linetriangle == 1) {
		FragColor = color;
	}
	else if (box == 1 || pixelw != 0.0f) {
		if (pixelw != 0.0f) {
			float maxX;
			float minX;
			float maxY;
			float minY;
			if (box == 1) {
				maxX = 1.0 - pixelw;
				minX = pixelw;
				maxY = 1.0 - pixelh;
				minY = pixelh;
			}
			else {
				vec2 size = textureSize(Sampler0, 0);
				float pw = 1.0f / size.x;
				float ph = 1.0f / size.y;
				maxX = 1.0 - pw;
				minX = pw;
				maxY = 1.0 - ph;
				minY = ph;
			}

			if (TexCoord0.x < maxX && TexCoord0.x > minX && TexCoord0.y < maxY && TexCoord0.y > minY) {
				if (box == 1) {
					if (color.a == 0.0) discard;
					FragColor = color;
				}
				else FragColor = texture2D(Sampler0, TexCoord0.st).rgba;
			} 
			else {
				FragColor = lcolor;
			}
		}
		else {
			if (box == 1) FragColor = color;
			else FragColor = texture2D(Sampler0, TexCoord0.st).rgba;
		}
	}
	if (glbox == 1) {
		int quadnr;
		if (orquad != 0) quadnr = orquad;
		else quadnr = Vertex0 / 4;
		uint Tex0 = texelFetch(boxtexSampler, quadnr).r;
		if (Tex0 > 127) {
			// text
			float c = texture2D(boxSampler[Tex0 - 128], vec2(TexCoord0.s, TexCoord0.t)).r;
			if (c == 0.0) discard;
			vec4 sam = texelFetch(boxcolSampler, quadnr).rgba;
			FragColor = vec4(sam.rgb, 1.0);
		}
		else if (Tex0 != 127) {
			// image
			FragColor = vec4(texture2D(boxSampler[Tex0], vec2(TexCoord0.s, TexCoord0.t)).rgb, 1.0f);
		}
		else {
			// flat
			FragColor = texelFetch(boxcolSampler, quadnr).rgba;
		}
	}

	if (wipe) {
		if (mixmode == 18) {
			float xamount = cf;
			if (inlayer) xamount = 1.0f - mixfac;
			float xpix = int(xamount * fbowidth * fcdiv);
			float ypix = int(xamount * fboheight * fcdiv);
			float xxpos = xpos;
			float xypos = ypos;
			float xc, yc;
			float offsx, offsy;
			bool cond, cond1, cond2;
			float aminv, minaminv;
			float offset11, offset12, offset01, offset02;
			float xam;
			float angle, sa, ca;
			float q;
			float xt, yt, saxt, cayt;
			float xl, xh, yl, yh;
			float parm;
			float a, b;
			float fardist, dist;
			
			vec4 data0 = texture2D(endSampler0, TexCoord0.st);	
			vec4 data1 = texture2D(endSampler1, TexCoord0.st);
			vec4 data;
			
			switch (wkind) {
				case 0:  //classic
					if (dir == 1) xpix = fbowidth * fcdiv - xpix;
					else if (dir == 3) ypix = fboheight * fcdiv - ypix;
					if (dir % 2 == 1) {
						data = data0;
						data0 = data1;
						data1 = data;
					}
					vec2 tc = TexCoord0.st;
					yc = tc.y * fboheight * fcdiv;
					xc = tc.x * fbowidth * fcdiv;
					if (dir < 2) cond=(xc < xpix);
					else cond=(yc < ypix);
					if (cond) {
						FragColor = vec4((data1.rgb * data1.a + data0.rgb * (1.0f - data1.a)), 1.0f);
					}
					else {
						FragColor = vec4((data0.rgb * data0.a + data1.rgb * (1.0f - data0.a)), 1.0f);
					}
					 break;
				case 1:  //pushpull
					if (dir % 2 == 1) {
						data = data0;
						data0 = data1;
						data1 = data;
					}
					tc = TexCoord0.st;
					yc = tc.y * fboheight * fcdiv;
					xc = tc.x * fbowidth * fcdiv;
					if (dir == 0) {
						tc.x -= xpix / (fbowidth * fcdiv);
						if (xc < xpix) {
							tc.x = 1.0f + tc.x;
							FragColor = vec4(texture2D(endSampler1, tc).rgb, 1.0f);
						}
						else {
							FragColor = vec4(texture2D(endSampler0, tc).rgb, 1.0f);
						}
					}
					else if (dir == 2) {
						tc.y -= ypix / (fboheight * fcdiv);
						if (yc < ypix) {
							tc.y = 1.0f + tc.y;
							FragColor = vec4(texture2D(endSampler1, tc).rgb, 1.0f);
						}
						else {
							FragColor = vec4(texture2D(endSampler0, tc).rgb, 1.0f);
						}
					}
					if (dir == 1) {
						tc.x -= (fbowidth * fcdiv - xpix) / (fbowidth * fcdiv);
						if (xc > fbowidth * fcdiv - xpix) {
							FragColor = vec4(texture2D(endSampler1, tc).rgb, 1.0f);
						}
						else {
							tc.x = 1.0f + tc.x;
							FragColor = vec4(texture2D(endSampler0, tc).rgb, 1.0f);
						}
					}
					else if (dir == 3) {
						tc.y -= (fboheight * fcdiv - ypix) / (fboheight * fcdiv);
						if (yc > fboheight * fcdiv - ypix) {
							FragColor = vec4(texture2D(endSampler1, tc).rgb, 1.0f);
						}
						else {
							tc.y = 1.0f + tc.y;
							FragColor = vec4(texture2D(endSampler0, tc).rgb, 1.0f);
						}
					}
					break;
				case 2:  //squashed
					if (dir % 2 == 1) {
						data = data0;
						data0 = data1;
						data1 = data;
					}
					if (dir == 1) xpix = fbowidth * fcdiv - xpix;
					else if (dir == 3) ypix = fboheight * fcdiv - ypix;
					tc = TexCoord0.st;
					yc = tc.y * fboheight * fcdiv;
					xc = tc.x * fbowidth * fcdiv;
					if (dir == 0) {
						if (xc < xpix) {
							tc.x = xc / xpix;
							FragColor = vec4(texture2D(endSampler1, tc).rgb, 1.0f);
						}
						else {
							tc.x = (fbowidth * fcdiv - xc) / (fbowidth * fcdiv - xpix);
							FragColor = vec4(texture2D(endSampler0, tc).rgb, 1.0f);
						}
					}
					else if (dir == 1) {
						if (xc < xpix) {
							tc.x = xc / xpix;
							FragColor = vec4(texture2D(endSampler0, tc).rgb, 1.0f);
						}
						else {
							tc.x = (fbowidth * fcdiv - xc) / (fbowidth * fcdiv - xpix);
							FragColor = vec4(texture2D(endSampler1, tc).rgb, 1.0f);
						}
					}
					else if (dir == 2) {
						if (yc < ypix) {
							tc.y = yc / ypix;
							FragColor = vec4(texture2D(endSampler1, tc).rgb, 1.0f);
						}
						else {
							tc.y = (fboheight * fcdiv - yc) / (fboheight * fcdiv - ypix);
							FragColor = vec4(texture2D(endSampler0, tc).rgb, 1.0f);
						}
					}
					else if (dir == 3) {
						if (yc < ypix) {
							tc.y = yc / ypix;
							FragColor = vec4(texture2D(endSampler0, tc).rgb, 1.0f);
						}
						else {
							tc.y = (fboheight * fcdiv - yc) / (fboheight * fcdiv - ypix);
							FragColor = vec4(texture2D(endSampler1, tc).rgb, 1.0f);
						}
					}
					break;
				case 3:  //ellipse
					if (xxpos < 0.5f) a = 1.0f - xxpos;
					else a = xxpos;
					if (xypos < 0.5f) b = 1.0f - xypos;
					else b = xypos;
					if (dir == 1) xamount = 0.5f + xamount * 0.5f;
					else xamount = xamount * 0.5f;
					fardist = sqrt(a * a + b * b);
					if (dir == 0) dist = fardist * xamount;
					else dist = fardist * (1 - xamount);
					tc = TexCoord0.st;
					if (dir == 0) {
                        a = abs((xxpos - tc.x) * (1.0 - xamount));
                        b = abs((xypos - tc.y) * (1.0 - xamount));
					}
					else {
                        a = abs((xxpos - tc.x) * (xamount));
                        b = abs((xypos - tc.y) * (xamount));
					}
					if (dir == 0) cond = sqrt(a * a + b * b) <= dist;
					else cond = sqrt(a * a + b * b) >= dist;
					if (cond) {
						FragColor = vec4((data1.rgb * data1.a + data0.rgb * (1.0f - data1.a)), max(data0.a, data1.a)) * (1 - dir) + data1 * dir;
					}
					else {
						FragColor = vec4((data0.rgb * data0.a + data1.rgb * (1.0f - data0.a)), max(data0.a, data1.a)) * dir + data0 * (1 - dir);
					}
					break;
				case 4:  //rectangle
					if (dir == 0) {
						xl = (xxpos * (1 - xamount));
						xh = (xxpos + (1 - xxpos) * (xamount));
						yl = (xypos * (1 - xamount));
						yh = (xypos + (1 - xypos) * (xamount));
					}
					else {
						xl = (xxpos * (xamount));
						xh = (xxpos + (1 - xxpos) * (1 - xamount));
						yl = (xypos * (xamount));
						yh = (xypos + (1 - xypos) * (1 - xamount));
					}
					tc = TexCoord0.st;
					if (dir == 0) cond = (tc.x > xl) && (tc.x < xh) && (tc.y > yl) && (tc.y < yh);
					else cond = (tc.x < xl) || (tc.x > xh) || (tc.y < yl) || (tc.y > yh);
					if (cond) {
						FragColor = vec4((data1.rgb * data1.a + data0.rgb * (1.0f - data1.a)), max(data0.a, data1.a)) * (1 - dir) + data1 * dir;
					}
					else {
						FragColor = vec4((data0.rgb * data0.a + data1.rgb * (1.0f - data0.a)), max(data0.a, data1.a)) * dir + data0 * (1 - dir);
					}
					break;
				case 5:  //zoomed rectangle
					if (dir == 0) {
						xl = (xxpos * (1 - xamount));
						xh = (xxpos + (1 - xxpos) * (xamount));
						yl = (xypos * (1 - xamount));
						yh = (xypos + (1 - xypos) * (xamount));
					}
					else {
						xl = (xxpos * (xamount));
						xh = (xxpos + (1 - xxpos) * (1 - xamount));
						yl = (xypos * (xamount));
						yh = (xypos + (1 - xypos) * (1 - xamount));
					}
					tc = TexCoord0.st;
					cond = (tc.x > xl) && (tc.x < xh) && (tc.y > yl) && (tc.y < yh);
					if (cond) {
						tc.x = ((tc.x - xl) / (xh - xl));
						tc.y = ((tc.y - yl) / (yh - yl));
						if (dir == 0) {
							data1 = vec4(texture2D(endSampler1, tc).rgba);
						}
						else {
							data0 = data1;
							data1 = vec4(texture2D(endSampler0, tc).rgba);
						}
						FragColor = vec4((data1.rgb * data1.a + data0.rgb * (1.0f - data1.a)), max(data0.a, data1.a));
					}
					else {
						if (dir == 0) {
							FragColor = data0;
						}
						else {
							FragColor = data1;
						}
					}                        
					break;
				case 6:  //clock
					if (dir % 2 == 1) {
						data = data0;
						data0 = data1;
						data1 = data;
					}
					if (dir % 2 == 0) xam = xamount;
					else xam = -1 * xamount;
					angle = ((PI * 2 * xam) - PI / 2) + ((dir / 2) * (PI / 2));
					sa = sin(angle);
					ca = cos(angle);
					q = int(xamount / 0.25f) + 1;
					if (q == 5) q = 4;
					if (dir % 2 == 1) q = 5 - q;
				
					tc = TexCoord0.st;
					yc = tc.y * fboheight * fcdiv;
					yt = yc - fboheight * fcdiv / 2;
					cayt = ca*yt;
					xc = tc.x * fbowidth * fcdiv;
					xt = xc - fbowidth * fcdiv / 2;
					saxt = sa*xt;
		
					cond2 = (saxt - cayt > 0);
					switch (int(q)) {
						case 1:
							if ((dir / 2) == 0) {
								cond1 = (xc <= fbowidth * fcdiv / 2);
							}
							else if ((dir / 2) == 1) {
								cond1 = (yc <= fboheight * fcdiv / 2);
							}
							else if ((dir / 2) == 2) {
								cond1 = (xc > fbowidth * fcdiv / 2);
							}
							else if ((dir / 2) == 3) {
								cond1 = (yc > fboheight * fcdiv / 2);
							}                            
							if (cond1) FragColor = vec4((data0.rgb * data0.a + data1.rgb * (1.0f - data0.a)), 1.0f);
							else if (cond2) FragColor = vec4((data1.rgb * data1.a + data0.rgb * (1.0f - data1.a)), 1.0f);
							else FragColor = vec4((data0.rgb * data0.a + data1.rgb * (1.0f - data0.a)), 1.0f);
							break;
						case 2:
							if ((dir /2) == 0) {
								cond1 = (xc <= fbowidth * fcdiv / 2);
							}
							else if ((dir / 2) == 1) {
								cond1 = (yc <= fboheight * fcdiv / 2);
						   }
							else if ((dir/2) == 2) {
								cond1 = (xc > fbowidth * fcdiv / 2);
							}
							else if ((dir / 2) == 3) {
								cond1 = (yc > fboheight * fcdiv / 2);
							}                            
							if (cond1) FragColor = vec4((data0.rgb * data0.a + data1.rgb * (1.0f - data0.a)), 1.0f);
							else if (cond2) FragColor = vec4((data1.rgb * data1.a + data0.rgb * (1.0f - data1.a)), 1.0f);
							else FragColor = vec4((data0.rgb * data0.a + data1.rgb * (1.0f - data0.a)), 1.0f);
							break;
						case 3:
							if ((dir / 2) == 0) {
								cond1 = (xc > fbowidth * fcdiv / 2);
							}
							else if ((dir / 2) == 1) {
								cond1 = (yc > fboheight * fcdiv / 2);
							}
							else if ((dir / 2) == 2) {
								cond1 = (xc <= fbowidth * fcdiv / 2);
							}
							else if ((dir / 2) == 3) {
								cond1 = (yc <= fboheight * fcdiv / 2);
							 }                            
							if (cond1) FragColor = vec4((data1.rgb * data1.a + data0.rgb * (1.0f - data1.a)), 1.0f);
							else if (cond2) FragColor = vec4((data1.rgb * data1.a + data0.rgb * (1.0f - data1.a)), 1.0f);
							else FragColor = vec4((data0.rgb * data0.a + data1.rgb * (1.0f - data0.a)), 1.0f);
							break;
						case 4:
							if ((dir /2) == 0) {
								cond1 = (xc>= fbowidth * fcdiv / 2);
							}
							else if ((dir / 2) == 1) {
								cond1 = (yc >= fboheight * fcdiv / 2);
						   }
							else if ((dir / 2) == 2) {
								cond1 = (xc <= fbowidth * fcdiv / 2);
							}
							else if ((dir / 2) == 3) {
								cond1 = (yc <= fboheight * fcdiv / 2);
							}
							if (cond1) FragColor = vec4((data1.rgb * data1.a + data0.rgb * (1.0f - data1.a)), 1.0f);
							else if (cond2) FragColor = vec4((data1.rgb * data1.a + data0.rgb * (1.0f - data1.a)), 1.0f);
							else FragColor = vec4((data0.rgb * data0.a + data1.rgb * (1.0f - data0.a)), 1.0f);
							break;
					}
					break;
				case 7:  //doubleclock
					if (dir % 2 == 0) angle = ((PI * xamount) - PI / 2) + (int(dir / 2) * (PI / 2));
					else angle = (-(PI * xamount) - PI / 2) + (int(dir / 2) * (PI / 2));
					sa = sin(angle);
					ca = cos(angle);
					tc = TexCoord0.st;
					yc = tc.y * fboheight * fcdiv;
					yt = yc - fboheight * fcdiv / 2;
					cayt = ca * yt;
					xc = tc.x * fbowidth * fcdiv;
					xt = xc - fbowidth * fcdiv / 2;
					saxt = sa * xt;
					if (xamount == 1) FragColor = vec4((data1.rgb * data1.a + data0.rgb * (1.0f - data1.a)), 1.0f);
					else {
						if (int(dir / 2) == 0) cond = (xc <= fbowidth * fcdiv / 2);
						else cond = (yc <= fboheight * fcdiv / 2);
						if (cond) {
							if (saxt - cayt < 0) FragColor = vec4((data1.rgb * data1.a + data0.rgb * (1.0f - data1.a)), 1.0f);
							else FragColor = vec4((data0.rgb * data0.a + data1.rgb * (1.0f - data0.a)), 1.0f);
						}
						else {
							if (saxt - cayt > 0) FragColor = vec4((data1.rgb * data1.a + data0.rgb * (1.0f - data1.a)), 1.0f);
							else FragColor = vec4((data0.rgb * data0.a + data1.rgb * (1.0f - data0.a)), 1.0f);
						}
					}
					break;
				case 8:  //bars
					if (xxpos >= 1) xl = fbowidth * fcdiv / xxpos;
					else xl = fbowidth * fcdiv;
					if (xypos >= 1) yl = fboheight * fcdiv / xypos;
					else yl = fboheight * fcdiv;
					tc = TexCoord0.st;
					yc = tc.y * fboheight * fcdiv;
					xc = tc.x * fbowidth * fcdiv;
					if (dir == 0) cond = ((xc - xl * int((xc / xl))) > (xl * xamount));
					else if (dir==1) cond = ((xc - xl * int((xc / xl))) < xl - (xl * xamount));
					else if (dir==2) cond = (((xc - xl * int((xc / xl))) > (xl * xamount) / 2)&&((xc - xl * int((xc / xl))) < xl - (xl * xamount) / 2));
					else if (dir==3) cond = ((yc - yl * int((yc / yl))) >= (yl * xamount));
					else if (dir==4) cond = ((yc - yl * int((yc / yl))) < yl - (yl * xamount));
					else if (dir==5) cond = (((yc - yl * int((yc / yl))) > (yl * xamount) / 2)&&((yc - yl * int((yc / yl))) < yl - (yl *xamount) / 2));
					if (cond || xamount == 0) {
						FragColor = vec4((data0.rgb * data0.a + data1.rgb * (1.0f - data0.a)), 1.0f);
					}
					else {
						FragColor = vec4((data1.rgb * data1.a + data0.rgb * (1.0f - data1.a)), 1.0f);
					}
					break;
				case 9:  //pattern
					if (xxpos >= 1) xl = fbowidth * fcdiv / xxpos;
					else xl = fbowidth * fcdiv;
					if (xypos >= 1) yl = fboheight * fcdiv / xypos;
					else yl = fboheight * fcdiv;
					tc = TexCoord0.st;
					yc = tc.y * fboheight * fcdiv;
					xc = tc.x * fbowidth * fcdiv;
					offsx = ((int((yc / yl)) % 2) * xl) / 2;
					offsy = ((int((xc / xl)) % 2) * yl) / 2;
					if (dir == 0) cond = ((xc + offsx - xl * int(((xc + offsx) / xl))) >= (xl * xamount));
					else if (dir == 1) cond = ((xc + offsx - xl * int(((xc + offsx) / xl))) < xl - (xl * xamount));
					else if (dir == 2) cond = (((xc + offsx - xl * int(((xc + offsx) / xl))) > (xl * xamount) / 2) && ((xc + offsx - xl * int(((xc + offsx) / xl))) < xl - (xl * xamount) / 2));
					else if (dir == 3) cond = ((yc + offsy - yl * int(((yc + offsy) / yl))) >= (yl * xamount));
					else if (dir == 4) cond = ((yc + offsy - yl * int(((yc + offsy) / yl))) < yl - (yl * xamount));
					else if (dir == 5) cond = (((yc + offsy - yl * int(((yc + offsy) / yl))) > (yl * xamount) / 2) && ((yc + offsy - yl * int(((yc + offsy) / yl))) < yl - (yl * xamount) / 2));
					if (cond || xamount == 0) {
						FragColor = vec4((data0.rgb * data0.a + data1.rgb * (1.0f - data0.a)), 1.0f);
					}
					else {
						FragColor = vec4((data1.rgb * data1.a + data0.rgb * (1.0f - data1.a)), 1.0f);
					}
					break;
				case 10:  //repel - alpha? reminder
					float rad = xamount / 1.7f;
					if (dir == 1) rad = (1.0f - xamount) / 1.7f;
					xxpos = 0.5f + (xxpos - 0.5f) * (1.0f - xamount);
					xypos = 0.5f + (xypos - 0.5f) * (1.0f - xamount);
					tc = TexCoord0.st;
					tc.y = (tc.y - 0.5f) * float(fboheight) / float(fbowidth) + 0.5f;
					dist = distance(tc, vec2(xxpos, xypos));
					cond = (dist < rad);
					if (cond) {
						if (dir == 0) {
							FragColor = vec4((data1.rgb * data1.a + data0.rgb * (1.0f - data1.a)), max(data0.a, data1.a)) * (1 - dir) + data1 * dir;
						}
						else FragColor = vec4((data0.rgb * data0.a + data1.rgb * (1.0f - data0.a)), max(data1.a, data0.a)) * dir + data1 * (1 - dir);
					}
					else {
						if (dir == 0) {
							FragColor = vec4((data0.rgb * data0.a + data1.rgb * (1.0f - data0.a)), max(data0.a, data1.a)) * dir + data0 * (1 - dir);
						}
						else {
						    FragColor = vec4((data1.rgb * data1.a + data0.rgb * (1.0f - data1.a)), max(data1.a, data0.a)) * (1 - dir) + data1 * dir;
						}
						cond = (dist < rad * 2.0f);
						if (dist < rad * 2.0f) {
							vec2 distvec = vec2(xxpos - tc.x, xypos - tc.y);
							distvec *= (dist - 2.0f * rad) / rad;
							//distvec.y *= fbowidth / fboheight;
							vec2 newtexco = TexCoord0.st - distvec;
							if (dir == 0) FragColor = texture2D(endSampler0, newtexco);
							else FragColor = texture2D(endSampler1, newtexco);
						}
					}
					break;
			}
		}
	}
}