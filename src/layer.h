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
	ELEM_NONE = 6,
} ELEM_TYPE;

typedef enum
{
	RATIO_OUTPUT = 0,
	RATIO_ORIGINAL_INSIDE = 1,
	RATIO_ORIGINAL_OUTSIDE = 2,
} RATIO_TYPE;

class Button;
class Shelf;
class ShelfElement;
class LoopStationElement;
class BinElement;
class MidiElement;

struct frame_result {
    char *data = nullptr;
    bool newdata = false;
    int height = 0;
    int width = 0;
    int size = 0;
    unsigned char compression = 0;
    bool hap = false;
};

struct remaining_frames {
    int compression = -1;
    int vidformat = -1;
    bool initialized = false;
    bool liveinput = false;
    bool isclone = false;
    int width = 1;
    int height = 1;
    int bpp = 0;
    int size;
    bool changeinit = false;
};

struct registered_midi {
    Button *but = nullptr;
    Param *par = nullptr;
    MidiElement *midielem = nullptr;
};

class Clip {
	public:
		std::string path = "";
		ELEM_TYPE type;
		GLuint tex = -1;
		int frame = 0.0f;
		Param *startframe = nullptr;
        Param *endframe = nullptr;
		Box* box;
		Layer* layer = nullptr;
		Clip();
		~Clip();
		Clip* copy();
		void insert(Layer* lay, std::vector<Clip*>::iterator it);
		bool get_imageframes();
		bool get_videoframes();
		bool get_layerframes();
		void open_clipfiles();
};

class Layer {
	public:
		int pos;
		bool deck = 0;
		bool comp = true;
		std::vector<Layer*>* layers;
		std::vector<Clip*> clips;
		Clip* currclip = nullptr;
		ELEM_TYPE type = ELEM_FILE;
        ELEM_TYPE oldtype = ELEM_FILE;
		RATIO_TYPE aspectratio = RATIO_OUTPUT;
		bool queueing = false;
		int queuescroll = 0;
		float scrollcol[4] = {0.4f, 0.4f, 0.4f, 0.0f};
		Button *mutebut;
        Button* solobut;
        Button* keepeffbut;
		Button* queuebut;
		bool muting = false;
        bool soloing = false;
        bool keepeffing = false;
		bool mousequeue = false;
		int numefflines[2] = {0,0};
		int effscroll[2] = {0,0};
		std::vector<Effect*> effects[2];
		Box* panbox;
		
		bool initialized = false;
        float frame = 0.0f;
        float oldframe = 0.0f;
		int prevframe = -1;
		int numf = 0;
		int video_duration;
		float millif = 0.0f;
		std::chrono::high_resolution_clock::time_point prevtime;
		bool timeinit = false;
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
        Button *stopbut;
		Button *frameforward;
		bool prevffw = false;
		Button *framebackward;
		bool prevfbw = false;
        Button *lpbut;
        bool onhold = false;
        Button *genmidibut;
		Box *loopbox;
		int playkind = 0;
		Param *startframe = nullptr;
        Param *endframe = nullptr;
        float oldstartframe = 0.0f;
        float oldendframe = 0.0f;
		int scritching = 0;
		int transforming = 0;
		int transmx;
		int transmy;
		int iw = 1;
		int ih = 1;
		Param *shiftx;
		Param *shifty;
        Param *scale;
        Param *scritch;
		float oldscale = 1.0f;
		float scratch = 0.0f;
		bool scratchtouch = 0;
		float olddeckspeed;
		bool vidmoving = false;
		bool live = false;
		Layer *liveinput = nullptr;
		int imagenum = 0;
		ILuint boundimage = -1;
		bool cliploading = false;
		
		bool dummy = 0;
		std::mutex startlock;
		std::mutex enddecodelock;
		std::mutex endopenlock;
		std::mutex chunklock;
		std::condition_variable startdecode;
		std::condition_variable enddecodevar;
		std::condition_variable endopenvar;
		std::condition_variable newchunk;
		std::condition_variable protect;
		bool processed = false;
		bool opened = false;
		bool ready = false;
		bool chready = false;
		bool closethread = false;
		bool waiting = true;
		bool vidopen = false;
		bool copying = false;
		bool firsttime = true;
		bool newframe = false;
		bool newtexdata = false;
		frame_result *decresult;
		remaining_frames *remfr[3];
		int changeinit = -1;
        std::thread decoding;
        std::thread triggering;
		void get_frame();
		std::thread audiot;
		void playaudio();
		bool audioplaying = false;
		std::list<char*> snippets;
		std::list<int> pslens;
		GLuint jpegtex;
		GLuint texture;
		GLuint fbotex;
		GLuint fbo;
		GLuint texpos = 0;
		GLuint vbuf;
		GLuint tbuf;
		GLuint vao;
		GLuint endtex;
        GLuint frb;
        GLuint pbo[3];
		GLubyte* mapptr[3] = {0, 0, 0};
		GLsync syncobj[3] = {nullptr, nullptr, nullptr};
		char pbodi = 0;
		char pboui = 1;
		int bpp;
		bool nonewpbos = false;
		
		Box *vidbox;
		bool changed;
		VideoNode *node = nullptr;
		Node *lasteffnode[2] = {nullptr, nullptr};
		BlendNode *blendnode = nullptr;

		ShelfElement* prevshelfdragelem = nullptr;

		std::string filename = "";
		std::string layerfilepath = "";
		AVFormatContext* video = nullptr;
		AVFormatContext* videoseek = nullptr;
		AVInputFormat *ifmt;
		bool skip = false;
		AVFrame *rgbframe = nullptr;
		AVFrame *decframe = nullptr;
		AVFrame *audioframe = nullptr;
		AVPacket decpkt;
		AVPacket decpktseek;
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
		bool databufready = false;
		int vidformat = -1;
        int oldvidformat = -1;
        int oldcompression = -1;
		bool oldalive = true;
		std::vector<char*> audio_chunks;
		ALuint sample_rate;
		int channels;
		ALuint sampleformat;
		BinElement *hapbinel = nullptr;
		bool encodeload = false;
		bool nopbodel = false;
		bool tobedeleted = true;
		
		std::unordered_map<EFFECT_TYPE, int> numoftypemap;
		int clonesetnr = -1;
        bool isclone = false;
        Layer *isduplay = nullptr;

        bool keyframe = false;

		void display();
		Effect* add_effect(EFFECT_TYPE type, int pos);
		Effect* replace_effect(EFFECT_TYPE type, int pos);
		void delete_effect(int pos);
		void inhibit();
		std::vector<Effect*>& choose_effects();
		Layer* clone();
		void set_clones();
		void mute_handle();
		void set_aspectratio(int lw, int lh);
		bool calc_texture(bool comp, bool alive);
		void load_frame();
		bool exchange(std::vector<Layer*>& slayers, std::vector<Layer*>& dlayers, bool deck);
		void open_files_layers();
		void open_files_queue();
		bool thread_vidopen();
        Layer* open_video(float frame, const std::string& filename, int reset, bool dontdeleffs = false);
        Layer* open_video(float frame, const std::string& filename, int reset, bool copy, bool noeffects);
		void open_image(const std::string& path);
		void initialize(int w, int h);
		void initialize(int w, int h, int compression);
		void clip_display_next(bool startend, bool alive);
		bool find_new_live_base(int pos);
		void set_live_base(std::string livename);
		void deautomate();
        void set_inlayer(Layer* lay, bool pbos);
        void trigger();
        Layer* next();
		Layer* prev();
        void del();
		Layer();
		Layer(bool comp);
		Layer(const Layer& lay);
		//~Layer();


	private:
		bool get_hap_frame();
		void get_cpu_frame(int framenr, int prevframe, int errcount);
		Layer* transfer();
};

class Scene {
	public:
		bool deck;
		Box* box;
		Button* button;
		bool loaded;
		std::vector<Layer*> nblayers;
		std::vector<float> nbframes;
		std::vector<Layer*> tempnblayers;
		std::vector<float> tempnbframes;
		int scrollpos = 0;
};

class Mixer {
	private:
		void do_deletelay(Layer *testlay, std::vector<Layer*> &layers, bool add);
		void event_write(std::ostream &wfile, Param *par, Button *but);
		void event_read(std::istream &rfile, Param *par, Button *but, Layer *lay);
		void add_del_bar();
		void clip_dragging();
		bool clip_drag_per_layervec(std::vector<Layer*>& layers, bool deck);
		void clip_inside_test(std::vector<Layer*>& layers, bool deck);
	public:
		std::vector<Layer*> layersAcomp;
		std::vector<Layer*> layersBcomp;
		std::vector<Layer*> layersA;
		std::vector<Layer*> layersB;
		std::vector<Scene*> scenes[2];
		std::vector<Layer*> bulrs[2];
        std::vector<Layer*> bulrscopy[2];
        std::vector<GLuint> butexes[2];
        std::vector<Layer*> newpathlayers;
        std::vector<Clip*> newpathclips;
        std::vector<ShelfElement*> newpathshelfelems;
        std::vector<BinElement*> newpathbinels;
        std::vector<std::string> *newpaths;
        std::vector<std::string> newlaypaths;
        std::vector<std::string> newclippaths;
        std::vector<std::string> newshelfpaths;
        std::vector<std::string> newbinelpaths;
        int newpathpos = 0;
        int retargetstage = 0;
        bool retargeting = false;
        bool retargetingdone = false;
        bool renaming = false;
		bool bualive;
		Layer *currlay[2] = {nullptr, nullptr};
        std::vector<Layer*> currlays[2];

        Layer *add_layer(std::vector<Layer*> &layers, int pos);
		void delete_layer(std::vector<Layer*> &layers, Layer *lay, bool add);
		void delete_layers(std::vector<Layer*>& layers, bool alive);
		void do_delete_layers(std::vector<Layer*> layers, bool alive);
		void copy_to_comp(bool deckA, bool deckB, bool comp);
        void copy_pbos(Layer *clay, Layer *lay);
        void set_values(Layer* clay, Layer* lay, bool open);
		void copy_effects(Layer* slay, Layer* dlay, bool comp);
        void handle_param(Param* par);
        void handle_adaptparam();
		void handle_clips();
		void record_video(std::string reccod);
		void new_file(int decks, bool alive);
		void save_layerfile(const std::string &path, Layer* lay, bool doclips, bool dojpeg);
		void save_mix(const std::string &path);
		void do_save_mix(const std::string& path, bool modus, bool save);
		void save_deck(const std::string &path);
		void do_save_deck(const std::string& path, bool save, bool doclips);
		Layer* open_layerfile(const std::string &path, Layer *lay, bool loadevents, bool doclips);
		void open_mix(const std::string &path, bool alive);
		void open_deck(const std::string &path, bool alive);
		void new_state();
		void open_state(const std::string& path);
		void save_state(const std::string &path, bool autosave);
		void do_save_state(const std::string& path, bool autosave);
		std::vector<std::string> write_layer(Layer *lay, std::ostream& wfile, bool doclips, bool dojpeg);
		Layer* read_layers(std::istream &rfile, const std::string &result, std::vector<Layer*> &layers, bool deck, int type, bool doclips, bool concat, bool load, bool loadevents, bool save, bool keepeff = false);
		void start_recording();
		void cloneset_destroy(std::unordered_set<Layer*>* cs);
		void handle_genmidibuttons();
		bool set_prevshelfdragelem(Layer *lay);
		void vidbox_handle();
		void outputmonitors_handle();
		void layerdrag_handle();
		void deckmixdrag_handle();
        void open_dragbinel(Layer *lay, int i);
        void open_dragbinel(Layer *lay);
        void reconnect_all(std::vector<Layer*> &layers);
		Mixer();
		
		std::mutex recordlock[2];
		std::condition_variable startrecord[2];
		std::thread recording_video[2];
        bool recordnow[2] = {false, false};
        bool recording[2] = {false, false};
        bool donerec[2] = {true, true};
        std::string reccodec;
        Button *recbutQ;
        Button *recbutS;
        GLuint recSthumb = -1;
        GLuint recQthumb = -1;
        GLuint recSthumbshow = -1;
        GLuint recQthumbshow = -1;
        std::string recpath[2];
        bool recswitch[2] = {false, false};
		uint8_t *avbuffer = nullptr;
		void *rgbdata = nullptr;
		GLuint ioBuf;
		
		Box* decknamebox[2];
		Box *modebox;
		int mode = 0;
		bool staged = true;
		Button *genmidi[2];
		Param *crossfade;
		Param *crossfadecomp;

		int currscene[2] = {0, 0};
		bool deck = 0;
		int scrollon = 0;
		int scrollmx;
		float scrolltime = 0.0f;
		int mouseeffect = -1;
		Layer *mouselayer = nullptr;
		int mousedeck = -1;
		Shelf *mouseshelf;
		int mouseshelfelem;
		bool insert;
		Node *mousenode = nullptr;
		Clip* mouseclip;
		LoopStationElement *mouselpstelem = nullptr;
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
		Param *wipex[2];
		Param *wipey[2];
		bool addlay = false;
		Param* deckspeed[2][2];
		int fps[25];
		int fpscount = 0;
		int rate;

		float time = 0.0f;
		float oldtime = 0.0f;
		float cbduration = 0.0f;

		bool domix;
		int waitmixtex = 0;

		std::unordered_map<int, std::unordered_map<int, std::unordered_map<std::string, registered_midi>>> midi_registrations;
		
		std::vector<GLuint> fbotexes;

		std::vector<std::unordered_set<Layer*>*> clonesets;
		std::unordered_map<int, Layer*> firstlayers;  //first decompressed layer per cloneset
};