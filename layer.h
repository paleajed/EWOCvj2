#include <thread>
#include <mutex>
#include <condition_variable>
#include <list>

#include "SDL2\SDL.h"
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavutil/avutil.h"
#include "libavutil/pixfmt.h"
};


typedef enum
{
	ELEM_FILE = 0,
	ELEM_LAYER = 1,
	ELEM_DECK = 2,
	ELEM_MIX = 3,
	ELEM_LIVE = 4,
} ELEM_TYPE;

struct frame_result {
	char *data = nullptr;
	int height = 0;
	int width = 0;
	int size = 0;
	unsigned char compression = 0;
};

class Button;

class Clip {
	public:
		std::string path = "";
		ELEM_TYPE type;
		GLuint tex;
		Clip();
		~Clip();
};

class Layer {
	public:
		int pos;
		bool deck;
		std::vector<Clip*> clips;
		ELEM_TYPE currcliptype = ELEM_FILE;
		bool queueing = false;
		int queuescroll = 0;
		int numefflines = 0;
		int effscroll = 0;
		std::vector<Effect*> effects;
		Effect *add_effect(EFFECT_TYPE type, int pos);
		Effect *replace_effect(EFFECT_TYPE type, int pos);
		void delete_effect(int pos);
		Layer();
		Layer(bool comp);
		Layer(const Layer &lay);
		~Layer();
		
		float frame = 0;
		int prevframe = -1;
		int numf = 0;
		float millif = 0.0f;
		std::chrono::high_resolution_clock::time_point prevtime;
		bool timeinit = false;
		int fps[25];
		int fpscount = 0;
		Box *mixbox;
		Box *chromabox;
		Param *chtol;
		Button *chdir;
		Button *chinv;
		bool cwon = false;
		float rgb[4] = {0.0f, 0.0f, 0.0f, 0.0f};
		Param *speed;
		Param *opacity;
		Param *volume;
		Button *playbut;
		Button *revbut;
		Button *bouncebut;
		Button *frameforward;
		bool prevffw = false;
		Button *framebackward;
		bool prevfbw = false;
		Button *genmidibut;
		Box *loopbox;
		int playkind = 0;
		int startframe = 0;
		int endframe = 0;
		int scritching = 0;
		int transforming = 0;
		int transmx;
		int transmy;
		int shiftx = 0;
		int shifty = 0;
		float scale = 1.0f;
		float oldscale = 1.0f;
		float scratch = 0.0f;
		bool scratchtouch = 0;
		bool vidmoving = false;
		bool live = false;
		Layer *liveinput = nullptr;
		
		bool dummy = 0;
		std::mutex startlock;
		std::mutex endlock;
		std::mutex chunklock;
		std::mutex endchunklock;
		std::mutex protect;
		std::condition_variable startdecode;
		std::condition_variable enddecode;
		std::condition_variable newchunk;
		std::condition_variable endchunk;
		bool processed = false;
		bool ready = false;
		bool chready = false;
		bool endready = false;
		bool closethread = false;
		bool building = true;
		bool vidopen = false;
		bool openerr = false;
		bool copying = false;
		bool firsttime = true;
		bool newframe = false;
		frame_result *decresult;
		std::thread decoding;
		void get_frame();
		std::thread audiot;
		void playaudio();
		bool audioplaying = false;
		bool notdecoding = true;
        std::list<int*> snippets;
 		int pslen;
		GLuint texture;
		GLuint fbotex;
		GLuint fbo;
		GLuint texpos = 0;
		GLuint vbuf;
		GLuint tbuf;
		GLuint vao;
		GLuint endtex;
		GLuint endbuf;
		
		Box *vidbox;
		bool changed;
		VideoNode *node;
		Node *lasteffnode = NULL;
		BlendNode *blendnode = NULL;
		
		std::string filename = "";
		AVFormatContext *video = NULL;
		AVFrame *rgbframe = NULL;
		AVFrame *decframe = NULL;
		AVFrame *audioframe = NULL;
		AVPacket decpkt;
		bool pktloaded = false;
		int reset = 0;
		AVPacket audiopkt;
		AVCodecContext *video_dec_ctx = NULL;
		AVCodecContext *audio_dec_ctx = NULL;
		AVStream *video_stream = NULL;
		AVStream *audio_stream = NULL;
		int video_stream_idx = -1;
		int audio_stream_idx = -1;
		struct SwsContext *sws_ctx = NULL;
		uint8_t *avbuffer = NULL;
		char *databuf = nullptr;
		int vidformat = -1;
		int dataformat = -1;
		std::vector<char*> audio_chunks;
		ALuint sample_rate;
		int channels;
		ALuint sampleformat;
	
	private:
		void decode_frame();
};

class Mixer {
	public:
		std::vector<Layer*> layersAcomp;
		std::vector<Layer*> layersBcomp;
		std::vector<Layer*> layersA;
		std::vector<Layer*> layersB;
		Layer *currlay = nullptr;
		Layer *add_layer(std::vector<Layer*> &layers, int pos);
		void delete_layer(std::vector<Layer*> &layers, Layer *lay, bool add);
		void record_video();
		Mixer();
		
		std::mutex recordlock;
		std::condition_variable startrecord;
		std::thread recording_video;
		bool recordnow = false;
		bool recording = false;
		bool donerec = true;
		struct SwsContext *sws_ctx = NULL;
		uint8_t *avbuffer = NULL;
		AVFrame *yuvframe = NULL;
		void *rgbdata = nullptr;
		GLuint ioBuf;
		
		std::vector<Box*> numboxesA;
		std::vector<Box*> numboxesB;
		std::vector<std::vector<Layer*>> nbframesA;
		std::vector<std::vector<Layer*>> nbframesB;
		std::vector<std::vector<Layer*>> tempnbframes;
		Box *modebox;
		int mode = 0;
		bool staged = true;
		Button *genmidi[2];
		Button *recbut;
		Param *crossfade;
		Param *crossfadecomp;
		int numaudiochannels = 0;
		
		float layw = 0.319f;
		int page[2] = {0, 0};
		bool deck = 0;
		float numw = 0.041f;
		float numh = 0.041f;
		int scrollpos[2] = {0, 0};
		int scrollon = 0;
		int scrollmx;
		float scrolltime = 0.0f;
		int mouseeffect = -1;
		Layer *mouselayer;
		bool mousedeck = -1;
		bool insert;
		Node *mousenode = NULL;
		Param *learnparam;
		bool learn = false;
		float midi2;
		Param *midiparam = nullptr;
		Param *adaptparam = nullptr;
		bool midiisspeed = false;
		int prevx;
		bool compon = false;
		bool firststage = true;
		GLuint mixbackuptex;
		int wipe[2] = {-1, -1};
		int wipedir[2] = {0, 0};
		bool moving = false;
		float wipex[2];
		float wipey[2];
		
		float time = 0;
		
		GLuint tempbuf, temptex;
};