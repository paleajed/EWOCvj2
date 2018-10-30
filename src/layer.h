#include <thread>
#include <mutex>
#include <condition_variable>
#include <list>
#include <istream>
#include <ostream>
#include <unordered_set>
#include <unordered_map>

#include "SDL2/SDL.h"
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
		bool clonedeck = -1;
		int clonepos = -1;
		std::vector<Clip*> clips;
		ELEM_TYPE currcliptype = ELEM_FILE;
		bool queueing = false;
		int queuescroll = 0;
		float scrollcol[4] = {0.5f, 0.5f, 0.5f, 1.0f};
		int numefflines = 0;
		int effscroll = 0;
		std::vector<Effect*> effects;
		Effect *add_effect(EFFECT_TYPE type, int pos);
		Effect *replace_effect(EFFECT_TYPE type, int pos);
		void delete_effect(int pos);
		void set_clones();
		void set_blendnode(BlendNode *bnode);
		Layer *next();
		Layer();
		Layer(bool comp);
		Layer(const Layer &lay);
		~Layer();
		
		bool onoff = true;
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
		VideoNode *node = nullptr;
		Node *lasteffnode = nullptr;
		BlendNode *blendnode = nullptr;
		
		std::string filename = "";
		AVFormatContext *video = nullptr;
		AVFrame *rgbframe = nullptr;
		AVFrame *decframe = nullptr;
		AVFrame *audioframe = nullptr;
		AVPacket decpkt;
		bool pktloaded = false;
		int reset = 0;
		AVPacket audiopkt;
		AVCodecContext *video_dec_ctx = nullptr;
		AVCodecContext *audio_dec_ctx = nullptr;
		AVStream *video_stream = nullptr;
		AVStream *audio_stream = nullptr;
		int video_stream_idx = -1;
		int audio_stream_idx = -1;
		struct SwsContext *sws_ctx = nullptr;
		uint8_t *avbuffer = nullptr;
		char *databuf = nullptr;
		int vidformat = -1;
		int dataformat = -1;
		std::vector<char*> audio_chunks;
		ALuint sample_rate;
		int channels;
		ALuint sampleformat;
		
		std::unordered_map<EFFECT_TYPE, int> numoftypemap;
	
	private:
		void decode_frame();
};

class Mixer {
	private:
		void do_deletelay(Layer *testlay, std::vector<Layer*> &layers, bool add);
		void set_values(Layer *clay, Layer *lay);
		void loopstation_copy(bool comp);
		void event_write(std::ostream &wfile, Param *par);
		void event_read(std::istream &rfile, Param *par, Layer *lay);
	public:
		std::vector<Layer*> layersAcomp;
		std::vector<Layer*> layersBcomp;
		std::vector<Layer*> layersA;
		std::vector<Layer*> layersB;
		Layer *currlay = nullptr;
		Layer *add_layer(std::vector<Layer*> &layers, int pos);
		void delete_layer(std::vector<Layer*> &layers, Layer *lay, bool add);
		Layer* clone_layer(std::vector<Layer*> &lvec, Layer* slay);
		void lay_copy(std::vector<Layer*> &slayers, std::vector<Layer*> &dlayers, bool comp);
		void copy_to_comp(std::vector<Layer*> &sourcelayersA, std::vector<Layer*> &destlayersA, std::vector<Layer*> &sourcelayersB, std::vector<Layer*> &destlayersB, std::vector<Node*> &sourcenodes, std::vector<Node*> &destnodes, std::vector<MixNode*> &destmixnodes, bool comp);
		void record_video();
		void save_layerfile(const std::string &path, Layer* lay, bool doclips);
		void save_mix(const std::string &path);
		void save_deck(const std::string &path);
		void open_layerfile(const std::string &path, Layer *lay, bool loadevents, bool doclips);
		void open_mix(const std::string &path);
		void open_deck(const std::string &path, bool alive);
		void open_state(const std::string &path);
		void save_state(const std::string &path);
		std::vector<std::string> write_layer(Layer *lay, std::ostream& wfile, bool doclips);
		int read_layers(std::istream &rfile, const std::string &result, std::vector<Layer*> &layers, bool deck, int type, bool doclips, bool concat, bool load, bool loadevents);
		void start_recording();
		Mixer();
		
		std::mutex recordlock;
		std::condition_variable startrecord;
		std::thread recording_video;
		bool recordnow = false;
		bool recording = false;
		bool donerec = true;
		struct SwsContext *sws_ctx = nullptr;
		uint8_t *avbuffer = nullptr;
		AVFrame *yuvframe = nullptr;
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
		int mouseshelfelem;
		bool insert;
		Node *mousenode = nullptr;
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
		
		std::unordered_map<Layer*, std::unordered_set<Layer*>*> clonemap;
};