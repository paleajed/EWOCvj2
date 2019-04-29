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
#include "IL/il.h"

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
	ELEM_IMAGE = 2,
	ELEM_DECK = 3,
	ELEM_MIX = 4,
	ELEM_LIVE = 5,
} ELEM_TYPE;

typedef enum
{
	RATIO_OUTPUT = 0,
	RATIO_ORIGINAL_INSIDE = 1,
	RATIO_ORIGINAL_OUTSIDE = 2,
} RATIO_TYPE;

struct frame_result {
	char *data = nullptr;
	int height = 0;
	int width = 0;
	int size = 0;
	unsigned char compression = 0;
};

class Button;
class Shelf;

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
		bool deck = 0;
		bool comp = true;
		bool clonedeck = -1;
		int clonepos = -1;
		std::vector<Clip*> clips;
		ELEM_TYPE type = ELEM_FILE;
		RATIO_TYPE aspectratio = RATIO_OUTPUT;
		bool queueing = false;
		int queuescroll = 0;
		float scrollcol[4] = {0.5f, 0.5f, 0.5f, 0.0f};
		Button *mutebut;
		Button *solobut;
		bool muting = false;
		bool soloing = false;
		int numefflines[2] = {0,0};
		int effscroll[2] = {0,0};
		std::vector<Effect*> effects[2];
		Effect *add_effect(EFFECT_TYPE type, int pos);
		Effect *replace_effect(EFFECT_TYPE type, int pos);
		void delete_effect(int pos);
		std::vector<Effect*>& choose_effects();
		void set_clones();
		void mute_handle();
		void set_aspectratio(int lw, int lh);
		void open_image(const std::string &path);
		void initialize(int w, int h);
		void initialize(int w, int h, int compression);
		Layer *next();
		Layer *prev();
		Layer();
		Layer(bool comp);
		Layer(const Layer &lay);
		~Layer();
		
		bool initialized = true;
		float frame = 0.0f;
		int prevframe = -1;
		int numf = 0;
		int video_duration;
		float millif = 0.0f;
		std::chrono::high_resolution_clock::time_point prevtime;
		bool timeinit = false;
		int fps[25];
		int fpscount = 0;
		Box *mixbox;
		Box *colorbox;
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
		int iw = 1;
		int ih = 1;
		Param *shiftx;
		Param *shifty;
		Param *scale;
		float oldscale = 1.0f;
		float scratch = 0.0f;
		bool scratchtouch = 0;
		bool vidmoving = false;
		bool live = false;
		Layer *liveinput = nullptr;
		int imagenum = 0;
		ILuint boundimage = -1;
		
		bool dummy = 0;
		std::mutex startlock;
		std::mutex enddecodelock;
		std::mutex endopenlock;
		std::mutex chunklock;
		std::mutex endchunklock;
		std::mutex protect;
		std::condition_variable startdecode;
		std::condition_variable enddecodevar;
		std::condition_variable endopenvar;
		std::condition_variable newchunk;
		std::condition_variable endchunk;
		bool processed = false;
		bool opened = false;
		bool ready = false;
		bool chready = false;
		bool endready = false;
		bool closethread = false;
		bool waiting = true;
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
		std::list<char*> snippets;
		std::list<int> pslens;
		GLuint texture;
		GLuint comptexture;
		GLuint fbotex;
		GLuint fbo;
		GLuint fbotex2;
		GLuint fbo2;
		GLuint texpos = 0;
		GLuint vbuf;
		GLuint tbuf;
		GLuint vao;
		GLuint endtex;
		GLuint endbuf;
		
		Box *vidbox;
		bool changed;
		VideoNode *node = nullptr;
		Node *lasteffnode[2] = {nullptr, nullptr};
		BlendNode *blendnode = nullptr;
		
		std::string filename = "";
		AVFormatContext *video = nullptr;
		AVFrame *rgbframe = nullptr;
		AVFrame *decframe = nullptr;
		AVFrame *audioframe = nullptr;
		AVPacket decpkt;
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
		int oldvidformat = -2;  //different from -1
		int oldcompression = -1;
		std::vector<char*> audio_chunks;
		ALuint sample_rate;
		int channels;
		ALuint sampleformat;
		
		std::unordered_map<EFFECT_TYPE, int> numoftypemap;
		int clonesetnr = -1;
	
	private:
		void decode_frame();
};

class Scene {
	public:
		bool deck;
		Box* box;
		Button* button;
		std::vector<Layer*> nbframes;
		std::vector<Layer*> tempnbframes;
		int scrollpos = 0;
};

class Mixer {
	private:
		void do_deletelay(Layer *testlay, std::vector<Layer*> &layers, bool add);
		void delete_layers(std::vector<Layer*> &layers, bool alive);
		void set_values(Layer *clay, Layer *lay);
		void loopstation_copy(bool comp);
		void clonesets_copy(bool comp);
		void event_write(std::ostream &wfile, Param *par);
		void event_read(std::istream &rfile, Param *par, Layer *lay);
	public:
		std::vector<Layer*> layersAcomp;
		std::vector<Layer*> layersBcomp;
		std::vector<Layer*> layersA;
		std::vector<Layer*> layersB;
		std::vector<Scene*> scenes[2];
		Layer *currlay = nullptr;
		Layer *add_layer(std::vector<Layer*> &layers, int pos);
		void delete_layer(std::vector<Layer*> &layers, Layer *lay, bool add);
		Layer* clone_layer(std::vector<Layer*> &lvec, Layer* slay);
		void lay_copy(std::vector<Layer*> &slayers, std::vector<Layer*> &dlayers, bool comp);
		void copy_to_comp(std::vector<Layer*> &sourcelayersA, std::vector<Layer*> &destlayersA, std::vector<Layer*> &sourcelayersB, std::vector<Layer*> &destlayersB, std::vector<Node*> &sourcenodes, std::vector<Node*> &destnodes, std::vector<MixNode*> &destmixnodes, bool comp);
		void record_video();
		void new_file(int decks, bool alive);
		void save_layerfile(const std::string &path, Layer* lay, bool doclips);
		void save_mix(const std::string &path);
		void save_deck(const std::string &path);
		void open_layerfile(const std::string &path, Layer *lay, bool loadevents, bool doclips);
		void open_mix(const std::string &path);
		void open_deck(const std::string &path, bool alive);
		void new_state();
		void open_state(const std::string& path);
		void save_state(const std::string &path);
		std::vector<std::string> write_layer(Layer *lay, std::ostream& wfile, bool doclips);
		int read_layers(std::istream &rfile, const std::string &result, std::vector<Layer*> &layers, bool deck, int type, bool doclips, bool concat, bool load, bool loadevents);
		void start_recording();
		void cloneset_destroy(std::unordered_set<Layer*>* cs);
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
		
		Box *modebox;
		int mode = 0;
		bool staged = true;
		Button *genmidi[2];
		Button *recbut;
		Param *crossfade;
		Param *crossfadecomp;
		int numaudiochannels = 0;
		
		int currscene[2] = {0, 0};
		bool deck = 0;
		int scrollon = 0;
		int scrollmx;
		float scrolltime = 0.0f;
		int mouseeffect = -1;
		Layer *mouselayer;
		bool mousedeck = -1;
		Shelf *mouseshelf;
		int mouseshelfelem;
		bool insert;
		Node *mousenode = nullptr;
		Param *learnparam;
		Button *learnbutton;
		bool learn = false;
		float midi2;
		Button *midibutton = nullptr;
		Button *midishelfbutton = nullptr;
		Param *midiparam = nullptr;
		Param *adaptparam = nullptr;
		Param *adaptnumparam = nullptr;
		bool midiisspeed = false;
		int prevx;
		GLuint mixbackuptex;
		int wipe[2] = {-1, -1};
		int wipedir[2] = {0, 0};
		bool moving = false;
		float wipex[2];
		float wipey[2];
		
		float time = 0;
		float cbduration = 0.0f;
		
		std::vector<GLuint> fbotexes;
		GLuint tempbuf, temptex;
		
		std::vector<std::unordered_set<Layer*>*> clonesets;
		std::unordered_map<int, Layer*> firstlayers;  //first decompressed layer per cloneset
};