#include <string>
#include <vector>
#include <mutex>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#ifdef WINDOWS
#include <windows.h>
#endif
#include "FFGLHost.h"
#include "ISFLoader.h"


typedef enum
{
    BLUR = 0,
    BRIGHTNESS = 1,
    CHROMAROTATE = 2,
    CONTRAST = 3,
    DOT = 4,
    GLOW = 5,
    RADIALBLUR = 6,
    SATURATION = 7,
    SCALE = 8,
    SWIRL = 9,
    OLDFILM = 10,
    RIPPLE = 11,
    FISHEYE = 12,
    TRESHOLD = 13,
    STROBE = 14,
    POSTERIZE = 15,
    PIXELATE = 16,
    CROSSHATCH = 17,
    INVERT = 18,
    ROTATE = 19,
    EMBOSS = 20,
    ASCII = 21,
    SOLARIZE = 22,
    VARDOT = 23,
    CRT = 24,
    EDGEDETECT = 25,
    KALEIDOSCOPE = 26,
    HTONE = 27,
    CARTOON = 28,
    CUTOFF = 29,
    GLITCH = 30,
    COLORIZE = 31,
    NOISE = 32,
   	GAMMA = 33,
   	THERMAL = 34,
   	BOKEH = 35,
   	SHARPEN = 36,
   	DITHER = 37,
   	FLIP = 38,
	MIRROR = 39,
    BOXBLUR = 40,
    CHROMASTRETCH = 41,
    PASS = 42,
    FFGL = 1000,
} EFFECT_TYPE;

typedef enum
{
	NO_IMP = 0,
    TEXCOORD = 1,
    COLOR = 2,
    FBO = 3,
} IMPLEMENTATION;


class Boxx;
class Button;
class Effect;
class Layer;
class EffectNode;
class MidiNode;

class Param {
	public:
		std::string name;
        FFUInt32 type = FF_TYPE_STANDARD;
        std::vector<std::string> options;
        float value = 0.0f;
        char *valuechar;
        std::string valuestr;
        float oldvalue = 0.0f;
		float deflt = 0.0f;
        char *defltchar;
		float range[2] = {0.0f, 1.0f};
		int midi[2] = {-1, -1};
		std::string midiport;
		bool sliding = true;
		bool powertwo = false;
        bool powerfour100 = false;
		std::string shadervar;
		Effect *effect = nullptr;
		Boxx *box = nullptr;
		MidiNode *node = nullptr;
        Layer *layer = nullptr;
		bool nextrow = false;
		std::vector<std::string> oscpaths;
        std::chrono::system_clock::time_point midistarttime;
        bool midistarted = false;
        void handle(bool smallxpad = false);
        void deautomate();
        void register_midi();
        void unregister_midi();
        void lpst_replace_with(Param* cpar);
        int ffglset_parameter_to(FFGLParameter &par, int cnt);
        std::vector<Param*> isfset_parameter_to(ISFLoader::ParamInfo &par, int pos, bool calling = false);
		Param();
		~Param();
};

class Effect {
	public:
		EFFECT_TYPE type;
		int pos;
		Layer *layer;
        GLuint fbo = -1;
        GLuint fbotex = -1;
        GLuint tempfbo = -1;
        GLuint tempfbotex = -1;
		EffectNode *node = NULL;
		std::vector<Param*> params;
        int ffglnr = -1;
        FFInstanceID ffglinstancenr;
        int isfnr = -1;
        int isfpluginnr = -1;
        int isfinstancenr = -1;
        virtual float get_speed() { return -1; };
		virtual float get_ripplecount() { return -1; };
		virtual void set_ripplecount(float count) { return; };
		std::string get_namestring();
		Effect();
		~Effect();
		
		Boxx *box;
		Param *drywet;
		Button *onoffbutton;
		int numrows;
};

class BlurEffect: public Effect {
	public:
		int times;
		BlurEffect();
};

class BrightnessEffect: public Effect {
	public:
		BrightnessEffect();
};

class ChromarotateEffect: public Effect {
	public:
		ChromarotateEffect();
};

class ContrastEffect: public Effect {
	public:
		ContrastEffect();
};

class DotEffect: public Effect {
	public:
		DotEffect();
};

class GlowEffect: public Effect {
	public:
		GlowEffect();
};

class RadialblurEffect: public Effect {
	public:
		RadialblurEffect();
};

class SaturationEffect: public Effect {
	public:
		SaturationEffect();
};

class ScaleEffect: public Effect {
	public:
		ScaleEffect();
};

class SwirlEffect: public Effect {
	public:
		SwirlEffect();
};

class OldFilmEffect: public Effect {
	public:
		OldFilmEffect();
};

class RippleEffect: public Effect {
	public:
		float ripplecount = 0.0f;
		float speed = 0.0f;
		float get_speed();
		float get_ripplecount();
		void set_ripplecount(float count);
		RippleEffect();
};

class FishEyeEffect: public Effect {
	public:
		FishEyeEffect();
};

class TresholdEffect: public Effect {
	public:
		TresholdEffect();
};

class StrobeEffect: public Effect {
	public:
		int phase;
		int get_phase();
		void set_phase(int phase);
		StrobeEffect();
};

class PosterizeEffect: public Effect {
	public:
		PosterizeEffect();
};

class PixelateEffect: public Effect {
	public:
		PixelateEffect();
};

class CrosshatchEffect: public Effect {
	public:
		CrosshatchEffect();
};

class InvertEffect: public Effect {
	public:
		InvertEffect();
};

class RotateEffect: public Effect {
	public:
		RotateEffect();
};

class EmbossEffect: public Effect {
	public:
		EmbossEffect();
};

class AsciiEffect: public Effect {
	public:
		AsciiEffect();
};

class SolarizeEffect: public Effect {
	public:
		SolarizeEffect();
};

class VarDotEffect: public Effect {
	public:
		VarDotEffect();
};

class CRTEffect: public Effect {
	public:
		CRTEffect();
};

class EdgeDetectEffect: public Effect {
	public:
		int thickness = 0;
		EdgeDetectEffect();
};

class KaleidoScopeEffect: public Effect {
	public:
		KaleidoScopeEffect();
};

class HalfToneEffect: public Effect {
	public:
		HalfToneEffect();
};

class CartoonEffect: public Effect {
	public:
		CartoonEffect();
};

class CutoffEffect: public Effect {
	public:
		CutoffEffect();
};

class GlitchEffect: public Effect {
	public:
		GlitchEffect();
};

class ColorizeEffect: public Effect {
	public:
		ColorizeEffect();
};

class NoiseEffect: public Effect {
	public:
		NoiseEffect();
};

class GammaEffect: public Effect {
	public:
		GammaEffect();
};

class ThermalEffect: public Effect {
	public:
		ThermalEffect();
};

class BokehEffect: public Effect {
	public:
		BokehEffect();
};

class SharpenEffect: public Effect {
	public:
		SharpenEffect();
};

class DitherEffect: public Effect {
	public:
		DitherEffect();
};

class FlipEffect: public Effect {
	public:
		FlipEffect();
};

class MirrorEffect : public Effect {
public:
	MirrorEffect();
};

class BoxblurEffect : public Effect {
public:
    BoxblurEffect();
};

class ChromastretchEffect : public Effect {
public:
    ChromastretchEffect();
};

class FFGLEffect : public Effect {
public:
    std::string name;
    FFGLInstanceHandle instance;

    FFGLEffect(Layer *lay, int ffglnr);
};

class ISFEffect : public Effect {
public:
    std::string name;

    ISFEffect(Layer *lay, int isfnr);
};

