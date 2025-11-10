#if defined(WIN32) && !defined(__linux__)
#define WINDOWS
#elif defined(__linux__) && !defined(WIN32)
#define POSIX
#endif

// Network buffer size constants
#define NETWORK_BUFFER_SIZE 148489
#define NETWORK_RECV_SIZE 148488  // Leave space for null terminator

#include "boost/bind.hpp"
#include "boost/asio.hpp"
#include "boost/thread/thread.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"

#include <condition_variable>
#include <chrono>
#include <string>
#include "GL/gl.h"
#include "BeatDetektor.h"
#include "fftw3.h"

#ifdef WINDOWS
#include <windows.h>
#endif
#include "FFGLHost.h"
#include "NDIManager.h"

#ifdef WINDOWS
#include "direnthwin/dirent.h"
#include "processthreadsapi.h"
#endif
#include <RtMidi.h>
#include <istream>
#include <lo/lo.h>
#include <lo/lo_cpp.h>
#ifdef POSIX
#include "dirent.h"
#include <cstring>
#endif

// my own headers
#include "box.h"
#include "effect.h"
#include "node.h"
#include "layer.h"
#include "window.h"
#include "loopstation.h"
#include "bins.h"
#include "retarget.h"
#include "UniformCache.h"

class PrefCat;
class Menu;
class LoopStation;
class Retarget;
class BinsMain;
class Bin;
class BinElement;
class ShelfElement;
class ISFLoader;
class ISFShaderInstance;

#ifdef POSIX
typedef int SOCKET;
#endif

typedef enum
{
	TM_NONE = 0,
	TM_PLAY = 1,
	TM_BACKW = 2,
	TM_BOUNCE = 3,
	TM_FRFORW = 4,
    TM_FRBACKW = 5,
    TM_STOP = 6,
    TM_LOOP = 7,
	TM_SPEED = 8,
	TM_OPACITY = 9,
	TM_FREEZE = 10,
    TM_SCRATCH1 = 11,
    TM_SCRATCH2 = 12,
    TM_SPEEDZERO = 13,
    TM_CROSS = 14,
} TM_LEARN;

typedef enum
{
	EDIT_NONE = 0,
	EDIT_CANCEL = 1,
	EDIT_BINNAME = 2,
    EDIT_BINELEMNAME = 3,
    EDIT_SHELFELEMNAME = 4,
	EDIT_STRING = 5,
	EDIT_NUMBER = 6,
    EDIT_FLOATPARAM = 7,
    EDIT_TEXTPARAM = 8,
} EDIT_TYPE;

typedef enum
{
	GUI_LINE = 0,
	GUI_TRIANGLE = 1,
	GUI_BOX = 2,
} GUI_ELEM_TYPE;

typedef enum
{
	PREF_ONOFF = 0,
	PREF_NUMBER = 1,
    PREF_STRING = 2,
    PREF_PATH = 3,
    PREF_PATHS = 4,
    PREF_MENU = 5,
} PREF_TYPE;

struct gui_line {
	float linec[4];
	float x1;
	float y1;
	float x2;
	float y2;
};

struct gui_triangle {
	float linec[4];
	float areac[4];
	float x1;
	float y1;
	float xsize;
	float ysize;
	ORIENTATION orient;
	TRIANGLE_TYPE type;
};

struct gui_box {
	float linec[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	float areac[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	float x;
	float y;
	float wi;
	float he;
	int circle;
	GLuint tex;
	bool text = false;
    bool vertical = false;
    bool inverted = false;
};


class GUI_Element {
public:
	GUI_ELEM_TYPE type;
	gui_line* line = nullptr;
	gui_triangle* triangle = nullptr;
	gui_box* box = nullptr;
};

struct BatchInfo {
    int numquads;
    int indexOffset;          // in elements, not bytes
    int indexOffsetBytes;     // precomputed byte offset for glDrawElements
    int vertexOffset;         // starting vertex index for this batch
    int coordsSize;           // numquads * 4 * 3 * sizeof(float)
    int texCoordsSize;        // numquads * 4 * 2 * sizeof(float)
    int colorsSize;           // numquads * 4
    int texIndicesSize;       // numquads
    int batchArrayIndex;      // index in the original arrays
};

class OptimizedRenderer {
private:
    float* combinedCoords;
    float* combinedTexCoords;
    unsigned char* combinedColors;
    unsigned char* combinedTexIndices;
    unsigned short* sequentialIndices;  // Add this
    GLsizei* counts;
    const GLvoid** indices;
    BatchInfo* batches;

    size_t maxTotalCoordsSize;
    size_t maxTotalTexCoordsSize;
    size_t maxTotalColorsSize;
    size_t maxTotalTexIndicesSize;
    int maxBatches;

public:
    OptimizedRenderer(int maxQuads, int maxBatchCount) {
        maxTotalCoordsSize = maxQuads * 4 * 3 * sizeof(float);
        maxTotalTexCoordsSize = maxQuads * 4 * 2 * sizeof(float);
        maxTotalColorsSize = maxQuads * 4;
        maxTotalTexIndicesSize = maxQuads;
        maxBatches = maxBatchCount;

        combinedCoords = (float*)malloc(maxTotalCoordsSize);
        combinedTexCoords = (float*)malloc(maxTotalTexCoordsSize);
        combinedColors = (unsigned char*)malloc(maxTotalColorsSize);
        combinedTexIndices = (unsigned char*)malloc(maxTotalTexIndicesSize);
        sequentialIndices = (unsigned short*)malloc(maxQuads * 6 * sizeof(unsigned short));  // Add this
        counts = (GLsizei*)malloc(maxBatches * sizeof(GLsizei));
        indices = (const GLvoid**)malloc(maxBatches * sizeof(const GLvoid*));
        batches = (BatchInfo*)malloc(maxBatches * sizeof(BatchInfo));
    }

    ~OptimizedRenderer() {
        free(combinedCoords);
        free(combinedTexCoords);
        free(combinedColors);
        free(combinedTexIndices);
        free(sequentialIndices);  // Add this
        free(counts);
        free(indices);
        free(batches);
    }

    void render();
    void text_render();
};


class Shelf {
public:
	bool side;
	std::string basepath = "";
	std::vector<ShelfElement*> elements;
	std::vector<Button*> buttons;
	int prevnum = -1;
	int newnum = -1;
    int elemcount = 0;
	bool ret;
    void handle();
	void erase();
	void save(std::string path, bool undo = false, bool startsolo = true);
	bool open(std::string path, bool undo = false);
	void open_files_shelf();
	bool insert_deck(std::string path, bool deck, int pos);
	bool insert_mix(std::string path, int pos);
    void open_jpegpaths_shelf();
	Shelf(bool side);
};

class ShelfElement {
public:
	std::string path;
	std::string jpegpath;
    std::string name;
    std::string oldname;
	ELEM_TYPE type;
	GLuint tex = -1;
	GLuint oldtex = -1;
	Button* button;
    long long filesize = 0;
	Boxx* sbox;
	Boxx* pbox;
	Boxx* cbox;
	int launchtype = 0;
    bool needframeset = false;
    std::vector<Layer*> clayers;
    std::vector<float> cframes;
    std::vector<Layer*> nblayers;
    std::vector<Layer*> mixlrs[2];
    int scrollpos[2] = {0, 0};
    bool done = false;
	float crossfade = 0.5f;
    void set_nbclayers(Layer *lay);
    void kill_clayers();
	ShelfElement(bool side, int pos, Button *but);
	~ShelfElement();
};

class Project {
	public:
		std::string path;
		std::string name;
		std::string binsdir;
		std::string recdir;
        std::string shelfdir;
        std::string elementsdir;
        std::string autosavedir;
        std::string bupp;
        std::string bupn;
        std::string bubd;
        std::string busd;
        std::string burd;
        std::string buad;
        std::string bued;
        std::unordered_set<std::string> pathsinbins;
        float ow[2] = {640.0f, 1920.0f};
        float oh[2] = {360.0f, 1080.0f};
		void newp(std::string path);
		bool open(std::string path, bool autosave, bool newp = false, bool undo = false);
        void autosave();
		void save(std::string path, bool autosave = false, bool undo = false, bool nocheck = false);
        void save_as();
        void wait_for_copyover(bool undo);
private:
        bool copyingover = false;
        std::mutex copyovermutex;
        std::condition_variable copyovervar;
        bool copiedover = false;
        void copy_over(std::string path, std::string path2, std::string oldprdir);
        void delete_dirs(std::string path);
        void copy_dirs(std::string path, bool rem = true);
        void create_dirs(std::string path);
        bool create_dirs_autosave(std::string path);
};

class Preferences {
	public:
		std::vector<PrefCat*> items;
		int curritem = 1;
		void load();
		void save();
        void init_midi_devices();
		Preferences();
};

class PrefItem {
	public:
		std::string name;
		PREF_TYPE type;
		void *dest;
		bool onoff = false;
		int value;
        std::string str;
        std::string path;
        Menu *menu = nullptr;
		bool renaming = false;
		bool choosing = false;
		bool onfile = true;
        std::string audevice;
		Boxx *namebox;
		Boxx *valuebox;
        Boxx *iconbox;
        Boxx *rembox;

		bool connected = true;
		RtMidiIn *midiin = nullptr;

		PrefItem(PrefCat *cat, int pos, std::string name, PREF_TYPE type, void *dest);
};

class PrefCat {
	public:
		std::vector<PrefItem*> items;
		std::string name;
		Boxx *box;
};

class PIDev: public PrefCat {
	public:
		std::vector<std::string> onnames;
		void populate();
		PIDev();
};

class PIInvisible: public PrefCat {
public:
    PIInvisible();
};

class PIProj: public PrefCat {
public:
    PIProj();
};

class PIDirs: public PrefCat {
public:
    PIDirs();
};

class PIInt: public PrefCat {
	public:
		PIInt();
};
		
class PIVid: public PrefCat {
	public:
		int oldvalue;
		PIVid();
};
		
class PIProg: public PrefCat {
	public:
		PIProg();
};
		
class MidiElement {
    public:
        int midi0 = -1;
        int midi1 = -1;
        std::string midiport = "";
        void register_midi();
        void unregister_midi();
};

class LayMidi {
	public:
        MidiElement *play;
        MidiElement *backw;
        MidiElement *pausestop;
        MidiElement *bounce;
        MidiElement *frforw;
        MidiElement *frbackw;
        MidiElement *stop;
        MidiElement *loop;
        MidiElement *scratch1;
        MidiElement *scratch2;
        MidiElement *scratchtouch;
        MidiElement *speed;
        MidiElement *speedzero;
        MidiElement *opacity;
        MidiElement *setcue;
        MidiElement *tocue;
        MidiElement *crossfade;
        bool scrinvert = false;

        LayMidi();
        ~LayMidi();
};

class GUIString {
	public:
		std::string text;
		std::vector<float> textwvec;
		std::vector<float> texthvec;
		std::vector<std::vector<float>> textwvecvec;
		float sx;
		std::vector<float> sxvec;
		std::vector<GLuint> texturevec;

		~GUIString();
};

class OutputEntry {
	public:
		int screen;
		EWindow *win;
};

class Globals {
	public:
		float w;
		float h;
};



extern void rec_frames();

class PreciseTimerController {
private:
    std::shared_ptr<boost::asio::steady_timer> timer_;
    std::atomic<bool> running_;
    std::chrono::steady_clock::time_point start_time_;
    std::chrono::milliseconds interval_;
    int execution_count_;

public:
    PreciseTimerController(boost::asio::io_context& io, std::chrono::milliseconds interval)
            : timer_(std::make_shared<boost::asio::steady_timer>(io)),
              running_(false),
              interval_(interval),
              execution_count_(0) {}

    void start() {
        if (running_.exchange(true)) {
            return; // Already running
        }

        start_time_ = std::chrono::steady_clock::now();
        execution_count_ = 0;
        schedule_next();
    }

    void stop() {
        running_ = false;
        timer_->cancel();
    }

private:
    void schedule_next() {
        if (!running_.load()) return;

        // Calculate the absolute time for the next execution
        auto next_time = start_time_ + (interval_ * (execution_count_ + 1));

        timer_->expires_at(next_time);
        timer_->async_wait([this](const boost::system::error_code& error) {
            if (!error && running_.load()) {
                execution_count_++;

                rec_frames();

                // Schedule next execution at precise interval regardless of how long this took
                schedule_next();
            }
        });
    }
};


class Program {
	public:
        int jav = 0;
		Project *project;
		NodesMain *nodesmain;
		GLuint ShaderProgram;
		GLuint EffectShaderPrograms[43];  // One program per effect (fxid 0-42)
		UniformCache* uniformCache;
        OptimizedRenderer *renderer;
		GLuint fbovao;
		GLuint fbotex[4];
		GLuint frbuf[4];
		GLuint bvao;
        GLuint boxvao;
        GLuint binvao;
        GLuint quboxvao;
        GLuint prboxvao;
		GLuint tmboxvao;
        GLuint splboxvao;
		GLuint bvbuf;
        GLuint boxvbuf;
        GLuint binvbuf;
        GLuint quboxvbuf;
        GLuint prboxvbuf;
		GLuint tmboxvbuf;
        GLuint splboxvbuf;
		GLuint btbuf;
        GLuint boxtbuf;
        GLuint bintbuf;
        GLuint quboxtbuf;
        GLuint prboxtbuf;
		GLuint tmboxtbuf;
        GLuint splboxtbuf;
		GLuint texvao;
		GLuint rtvbo;
		GLuint rttbo;
        GLuint bgtex;
        GLuint splashfbo;
        GLuint splashtex;
        GLuint loktex;
		std::vector<OutputEntry*> outputentries;
		Boxx *scrollboxes[2];
		Layer *loadlay;
		Layer *prelay = nullptr;
        std::vector<Layer*> dellays;
        std::vector<Effect*> deleffects;
        SDL_Window *splashwindow;
        SDL_Window *mainwindow;
		std::vector<EWindow*> mixwindows;
		std::vector<Menu*> menulist;
		std::vector<Menu*> actmenulist;
		Menu *effectmenu = nullptr;
		Menu *mixmodemenu = nullptr;
        Menu *parammenu1 = nullptr;
        Menu* parammenu2 = nullptr;
        Menu *parammenu1b = nullptr;
        Menu* parammenu2b = nullptr;
		Menu* parammenu3 = nullptr;
        Menu* parammenu4 = nullptr;
        Menu* parammenu5 = nullptr;
        Menu* parammenu6 = nullptr;
		Menu* speedmenu = nullptr;
		Menu *loopmenu = nullptr;
		Menu *laymenu1 = nullptr;
		Menu *laymenu2 = nullptr;
        Menu* newlaymenu = nullptr;
        Menu* sourcemenu = nullptr;
        Menu* ndisourcemenu = nullptr;
		Menu* clipmenu = nullptr;
		Menu *aspectmenu = nullptr;
        Menu *monitormenu = nullptr;
        Menu *loopbackmenu = nullptr;
        Menu *mixtargetmenu = nullptr;
        Menu *bintargetmenu = nullptr;
		Menu *fullscreenmenu = nullptr;
		Menu *mixenginemenu = nullptr;
        Menu *livemenu = nullptr;
        Menu *auinmenu = nullptr;
		Menu *wipemenu = nullptr;
		Menu *dirmenu = nullptr;
		Menu *dir1menu = nullptr;
		Menu *dir2menu = nullptr;
		Menu *dir3menu = nullptr;
		Menu *dir4menu = nullptr;
		Menu *binelmenu = nullptr;
		Menu *binmenu = nullptr;
		Menu* bin2menu = nullptr;
		Menu* binselmenu = nullptr;
		Menu *mainmenu = nullptr;
		Menu* shelfmenu = nullptr;
		Menu* filemenu = nullptr;
        Menu* filenewmenu = nullptr;
        Menu* fileopenmenu = nullptr;
        Menu* filesavemenu = nullptr;
        Menu* laylistmenu1 = nullptr;
        Menu* laylistmenu2 = nullptr;
        Menu* laylistmenu3 = nullptr;
        Menu* laylistmenu4 = nullptr;
        Menu* editmenu = nullptr;
        Menu* lpstmenu = nullptr;
        Menu* beatmenu = nullptr;
        Menu* sendmenu = nullptr;
        Menu* optionmenu = nullptr;
        bool menuactivation = false;
        bool binmenuactivation = false;
		bool menuchosen = false;
        std::vector<int> prevmenuchoices;
		std::vector<int> menuresults;
        bool ineffmenu = false;
        bool inmonitors = false;
        bool intoparea = false;
        bool intopmenu = false;
        bool exitedtop = false;
		int fullscreen = -1;
		bool test = false;
		float z = 0.0f;
		int mx;
		int my;
		int oldmx;
		int oldmy;
        int shelfmx;
        int shelfmy;
        int binmx;
        int binmy;
		int iemy;
        float ow[2] = {640.0f, 1920.0f};
        float oh[2] = {360.0f, 1080.0f};
		float sow = 1920.0f;
		float soh= 1080.0f;
		float oldow = 1920.0f;
		float oldoh = 1080.0f;
		float layw = 0.319f;
		float layh = 0.319f;
		float numw = 0.041f;
		float numh = 0.041f;
		float monh = 0.3f;
		float cwx;
		float cwy;
		float globw;
		float globh;
		bool transforming = false;
		bool leftmousedown = false;
		bool middlemousedown = false;
		bool rightmousedown = false;
		bool leftmouse = false;
		bool orderleftmouse = false;
        bool orderleftmousedown = false;
        bool orderrightmouse = false;
        bool lmover = false;
        bool fsmouse = false;
        bool binlmover = false;
        bool doubleleftmouse = false;
        bool doublemiddlemouse = false;
		bool middlemouse = false;
		bool rightmouse = false;
		float mousewheel = false;
		bool del = false;
        bool ctrl = false;
        bool shift = false;
		bool menuondisplay = false;
		bool orderondisplay = false;
		std::vector<GLuint> ordertexes;
		bool blocking = false;
		bool eXit = false;
        std::string appimagedir;
		std::string temppath;
		std::string docpath;
        std::string fontpath;
        std::string contentpath;
        std::string ffgldir;
        std::string isfdir;
		std::string path;
		std::vector<std::string> paths;
		int counting = 0;
		std::string pathto;
		std::string lastSuccessfulDrive = "";  // Cache for test_driveletters optimization
        Button *toscreenA;
        Button *toscreenB;
        Button *toscreenM;
        Button *toscene[2][2][3];
        Button *backtopreA;
        Button *backtopreB;
        Button *backtopreM;
		Button *modusbut;
        bool prevmodus = true;
        bool bupm = true;
		BlendNode *bnodeend[2];
		Boxx *outputmonitor;
		Boxx *mainmonitor;
		Boxx *deckmonitor[2];
		Boxx *cwbox;
		bool cwon = false;
		int cwmouse = false;
        bool colorpicking = false;
		Button *effcat[2];
		int efflines = 7;
		Boxx *effscrollupA;
		Boxx *effscrolldownA;
		Boxx *effscrollupB;
		Boxx *effscrolldownB;
		Boxx* addeffectbox;
		Boxx* inserteffectbox;
        Boxx* orderscrolldown;
        Boxx* orderscrollup;
        Boxx* defaultsearchscrolldown;
        Boxx* defaultsearchscrollup;
        Boxx* searchscrolldown;
        Boxx* searchscrollup;
		bool startloop = false;
        bool firsttime = true;
        bool newproject = false;
        bool newproject2 = false;
		std::vector<std::string> recentprojectpaths;
		bool wiping = false;
		float texth;
		float buth;
        float bdcoords[32][65536];
        float bdtexcoords[32][65536];
        unsigned char bdcolors[32][8192];
        unsigned char bdtexes[32][2048];
        float textbdcoords[32][65536];
        float textbdtexcoords[32][65536];
        unsigned char textbdcolors[32][8192];
        unsigned char textbdtexes[32][2048];
		std::vector<float> bdwi;
		std::vector<float> bdhe;
        float* bdvptr[32];
        float* bdtcptr[32];
        unsigned char* bdcptr[32];
        unsigned char* bdtptr[32];
        GLuint* bdtnptr[32];
        float* textbdvptr[32];
        float* textbdtcptr[32];
        unsigned char* textbdcptr[32];
        unsigned char* textbdtptr[32];
        GLuint* textbdtnptr[32];
		GLuint bdvao;
		GLuint bdvbo;
		GLuint bdtcbo;
		GLuint bdibo;
		int boxcount;
		GLint maxtexes = 16;
        int countingtexes[32];
        GLuint boxtexes[32][1024];
        int textcountingtexes[32];
        GLuint textboxtexes[32][1024];
		int boxoffset[32];
        int currbatch = 0;
        int textcurrbatch = 0;
		short indices[6144];
		float boxz = 0.0f;
		bool directmode = false;
		bool frontbatch = false;
        std::vector<GUI_Element*> guielems;
        std::vector<GUI_Element*> binguielems;
		Button* onscenebutton;
		float onscenemilli;
        bool onscenedeck;
		Boxx* delbox;
		Boxx* addbox;
        bool repeatdefault = true;
        bool autoplay = true;
        std::chrono::high_resolution_clock::time_point now;
        GLsync syncobj = nullptr;
        AVFormatContext* audio = nullptr;
        AVCodecContext *audio_dec_ctx = nullptr;
        BeatDetektor *beatdet = nullptr;
        std::chrono::high_resolution_clock::time_point austarttime;
        size_t autime = 0;
        int aubpmcounter;
        float topquality = 0.0f;
        float qtime = 0.0f;
        bool inbetween = false;


        GLuint boxcoltbo;
		GLuint boxtextbo;
		GLuint boxbrdtbo;
		GLuint bdcoltex;
		GLuint bdtextex;
		GLuint bdbrdtex;

		lo::ServerThread *st;
		std::unordered_map<std::string, int> wipesmap;
        std::vector<int> abeffects;
        std::vector<int> absources;

        SDL_Window* requesterwindow = nullptr;
        SDL_Window* dummywindow = nullptr;

        SDL_Window* config_midipresetswindow = nullptr;
		bool drawnonce = false;
		bool midipresets = false;
		int midipresetsset = 0;
		int configcatmidi = 0;
        bool scratch2phase = 0;
        Boxx* tmcat[3];
        Boxx* tmset[4];
        Boxx* tmscratch1;
        Boxx* tmscratch2;
        Boxx *tmfreeze;
        Boxx *tmscrinvert;
		Boxx *tmplay;
		Boxx *tmbackw;
		Boxx *tmbounce;
		Boxx *tmfrforw;
        Boxx *tmfrbackw;
        Boxx *tmstop;
        Boxx *tmloop;
		Boxx *tmspeed;
		Boxx *tmspeedzero;
        Boxx *tmopacity;
        Boxx *tmcross;
		TM_LEARN tmlearn = TM_NONE;
		TM_LEARN tmchoice = TM_NONE;
		int waitmidi = 0;
		std::vector<std::string> openports;
		std::vector<PrefItem*> pmon;
		clock_t stt;
		std::vector<unsigned char> savedmessage;
		PrefItem* savedmidiitem;
		bool queueing = false;
		int filecount = 0;
        bool openerr = false;

		SDL_Window *prefwindow = nullptr;
        bool prefon = false;
        bool prefoff = true;
        bool filereqon = false;
		Preferences *prefs;
		bool needsclick = false;
		bool insmall = false;
		bool showtooltips = true;
		bool longtooltips = true;
		Boxx *tooltipbox = nullptr;
		float tooltipmilli = 0.0f;
        bool ttreserved = false;
        bool boxhit = false;
        bool autosave = true;
        bool undoon = true;
        bool stashvideos = false;
		float asminutes = 1;
		int astimestamp = 0;
		float qualfr = 1;
        bool keepeffpref = false;
        bool swappingscene = false;
        bool autosaving = false;
        int concatting = 0;
        std::mutex concatlock;
        std::condition_variable concatvar;
        std::mutex ordermutex;
        std::condition_variable ordercv;
        bool goconcat = false;
        int numconcatted = 0;
        int concatlimit = 0;
        int concatsuffix = 0;
        bool startsolo = false;
        bool saveas = false;
        bool inautosave = false;
        bool openautosave = false;
        std::vector<std::string> oldbins;
        bool err = false;

        std::unordered_map <std::string, GUIString*> guitextmap;
        std::unordered_map <std::string, GUIString*> prguitextmap;
        std::unordered_map <std::string, GUIString*> tmguitextmap;
        std::unordered_map <std::string, GUIString*> flguitextmap;
        std::unordered_map <std::string, std::string> devvideomap;
        std::vector<std::wstring> livedevices;
        std::vector<std::string> devices;
        std::vector<std::string> auindevices;
        std::vector<std::string> adevices;
        std::string audevice;
        bool audioinit = false;
        SDL_AudioDeviceID audeviceid;
        float ausamplerate;
        float* aubuffer;
        int aubuffersize;
        int ausamples;
        std::vector<float> auoutfloat;
        int auoutsize;
        double* auin;
        fftw_complex* auout;
        fftw_plan auplan;
        bool auinitialized = false;
        std::vector<std::string> busylist;
		std::vector<Layer*> busylayers;
		std::vector<Layer*> mimiclayers;
		
		bool binsscreen = false;
		BinElement *dragbinel = nullptr;
		Clip *dragclip = nullptr;
        bool draggedclip = false;
        Layer *draglay = nullptr;
        int draglaypos = -1;
        int draglaydeck = -1;
		std::string dragpath;
		int dragpos = -1;
		bool drag = false;
        bool draggingrec = false;
		bool inwormgate = false;
		Button* wormgate1;
		Button* wormgate2;
		DIR *opendir;
        bool submenuscreated = false;
        bool gotaudioinputs = false;

		EDIT_TYPE renaming = EDIT_NONE;
        bool renamingseat = false;
        bool renamingip = false;
        bool enteringserverip = false;
        Boxx* renamingbox;
		std::string choosedir = "";
		std::string inputtext;
		int cursorpos0;
		int cursorpos1 = -1;
		int cursorpos2 = -1;
		int cursortemp1 = -1;
		int cursortemp2 = -1;
		bool cursorreset = true;
		float cursorpixels = -1;
		float startpos = 0;
		int startcursor = 0;
		int endcursor = 0;
        bool copytocomp = false;

		int numcores = 0;
		int maxthreads;
		int encthreads = 0;
		bool threadmode = true;
		std::mutex hapmutex;
		std::condition_variable hap;
		bool hapnow = false;

        std::mutex orderglcmutex;
        std::condition_variable orderglccond;
        bool orderglcswitch = false;
        std::vector<std::string> getvideotexpaths;
        std::vector<Layer*> getvideotexlayers;
        std::vector<Layer*> errlays;
        bool gettinglayertex = false;
        Layer *gettinglayertexlay = nullptr;
        std::vector<Layer*> gettinglayertexlayers;
        std::string result;
        int32_t resnum = -1;

        std::string projdir;
		std::string binsdir;
        std::string currprojdir;
		std::string currbinsdir;
		std::string currshelfdir;
		std::string currrecdir;
		std::string currshelfdirdir;
        std::string currfilesdir;
        std::string currelemsdir;
        std::string ffglfiledir;
		std::string homedir;
		std::string datadir;
		std::string fontdir = "/usr/share/fonts";
		bool openshelfdir = false;
		std::string shelfpath;
		int shelfdircount;
		bool openfilesshelf = false;
		int shelffileselem;
		int shelffilescount;
		int shelfdragnum = -1;
		ShelfElement *shelfdragelem = nullptr;
        bool catchup = false;
        ShelfElement* midishelfelem = nullptr;
        ShelfElement* midiconfigshelfelem = nullptr;
		Shelf *shelves[2];
		int inshelf = -1;
		int inclips = -1;
        bool clipsaving = false;
		bool openclipfiles = false;
		int clipfilescount = 0;
		Clip* clipfilesclip = nullptr;
		Layer* clipfileslay = nullptr;
		bool openfileslayers = false;
		bool openfilesqueue = false;
		int pathscount = 0;
        Layer* fileslay = nullptr;
        bool addedlay = false;
        Layer* prevlayer = nullptr;
        int laypos = -1;
		int multistage = 0;
		Effect* drageff = nullptr;
		int drageffpos = -1;
		bool drageffsense = false;
		std::string dragstr = "";
		int dragpathpos = -1;
		bool dragpathsense = false;
		std::vector<Boxx*> pathboxes;
        std::vector<GLuint> pathtexes;
        std::vector<GLuint> vidtexes;
        std::vector<std::string> pathtstrs;
        int pathscroll = 0;
        int onoffscroll = 0;
        bool prefonoff = false;
		bool indragbox = false;
		Boxx* dragbox;
		bool dragmiddle = false;
		bool dragout[2] = { true, true };
        std::string quitting;
        std::string infostr;
        bool infoanswer = false;
        bool shutdown = false;
        bool projnamechanged = false;
        std::string projname;
		bool saveproject = false;
        bool adaptivelprow = false;
        bool steplprow = false;
        bool waitonetime = false;
        float ordertime = 0.0f;
        bool sameeight = false;
        bool check = false;
        bool lpstmenuon = false;
        std::vector<char*> dropfiles;
        bool stringcomputing = false;

        ShelfElement *lpstelem = nullptr;

    #ifdef WINDOWS
        SOCKET sock;
		std::vector<SOCKET> connsockets;
    #endif
    #ifdef POSIX
        int sock;
		std::vector<int> connsockets;
    #endif
		std::string sockname;
		std::vector<std::string> connsocknames;
        int last_recv_bytes = 0;  // Track bytes received in last bl_recv call
        bool server = false;
        int connected = 0;
        bool connfailed = false;
        int connfailedmilli = 0;
        std::string seatname = "SEAT";
        std::string oldseatname = "SEAT";
        std::string serverip = "0.0.0.0";
        bool serveripchanged = false;
        std::string localip;
        std::string publicip;  // Public/internet IP for remote connections
        std::string manualserverip;  // Manually entered server IP for clients
        std::string broadcastip;
        struct sockaddr_in serv_addr_server;
        struct sockaddr_in serv_addr_client;
        class UPnPPortMapper* upnpMapper;  // UPnP port forwarding manager
        
        struct DiscoveredSeat {
            std::string ip;
            std::string name;
            std::chrono::steady_clock::time_point lastSeen;
        };
        std::vector<DiscoveredSeat> discoveredSeats;
        std::mutex discoveryMutex;
        bool discoveryRunning = false;
        bool discoveryInitialized = false;
        int discoverySocket = -1;
        bool autoConnectAttempted = false;
        bool autoServerAttempted = false;
        std::chrono::steady_clock::time_point discoveryStartTime;
        std::thread *clientthread;
#ifdef WINDOWS
        std::unordered_map<std::string, SOCKET> connmap;
#endif
#ifdef POSIX
        std::unordered_map<std::string, int> connmap;
        #endif
        std::mutex clientmutex;
        std::condition_variable startclient;

        // Subscription management: tracks which clients want updates for which bins
        // Key: (bin_owner_seatname, bin_name), Value: set of subscriber seatnames
        std::map<std::pair<std::string, std::string>, std::unordered_set<std::string>> subscriptionMap;
        std::mutex subscriptionMutex;

#ifdef POSIX
        std::unordered_map<std::string, GLuint> v4l2lbtexmap;
        std::unordered_map<std::string, size_t> v4l2lbnewmap;
        std::unordered_map<std::string, int> v4l2lboutputmap;
#endif

        std::vector<std::string> *prefsearchdirs;
        int choosing = -1;
        int pathrenaming = -1;

        // UNDO
        bool recundo = false;
        bool undoing = false;
        int undopos = 0;
        int undopbpos = 0;
        bool inbox = false;
        void *currundoelem = nullptr;
        char undowaiting = 0;
        std::unordered_set<Param*> undoparams;
        std::unordered_set<Button*> undobuttons;
        std::vector<std::vector<std::tuple<std::tuple<Param*, Button*, int, int, int, int, int, std::string>, std::variant<float, std::string>>>> undomapvec;
        std::vector<std::string> undopaths;
        std::unordered_map<Shelf*, bool> shelfjpegpaths;
        bool openjpegpathsshelf = false;
        bool adaptparaming = false;
        int transferclonesetnr = -1;
        Param *beatthres;
        float minbpm = 90;
        ShelfElement *renamingshelfelem = nullptr;

        Boxx *boxbig;
        Boxx *boxbefore;
        Boxx *boxafter;
        Boxx *boxlayer;

        std::multimap<std::tuple<int, int, GLint>, GLuint> texpool;
        std::multimap<int, std::pair<GLuint, GLubyte*>> pbopool;
        std::unordered_set<GLuint> fbopool;
        std::multimap<std::tuple<int, int>, SDL_Window*> winpool;
        std::unordered_map<GLuint, GLint> texintfmap;

        boost::asio::io_context *io;

        std::vector<std::string> ffgleffectnames;
        std::vector<std::string> ffglsourcenames;
        std::vector<std::string> ffglmixernames;
        std::vector<std::shared_ptr<FFGLPlugin>> ffglplugins;
        std::vector<std::shared_ptr<FFGLPlugin>> ffgleffectplugins;
        std::vector<std::shared_ptr<FFGLPlugin>> ffglsourceplugins;
        std::vector<std::shared_ptr<FFGLPlugin>> ffglmixerplugins;
        FFGLHost *ffglhost = nullptr;
        std::unordered_set<std::string> missingplugs;
        GLuint redgradienttex;
        GLuint redgradientfbo;
        GLuint greengradienttex;
        GLuint greengradientfbo;
        GLuint bluegradienttex;
        GLuint bluegradientfbo;
        GLuint huegradienttex;
        GLuint huegradientfbo;
        GLuint satgradienttex;
        GLuint satgradientfbo;
        GLuint brightgradienttex;
        GLuint brightgradientfbo;
        GLuint alphagradienttex;
        GLuint alphagradientfbo;

        ISFLoader isfloader;
        std::vector<std::string> isfeffectnames;
        std::vector<std::string> isfsourcenames;
        std::vector<std::string> isfmixernames;
        std::vector<std::vector<ISFShaderInstance*>> isfinstances;

        NDIManager& ndimanager;
        std::vector<std::string> ndisourcenames;
        int ndilaycount = 0;


    void remove_ec(std::string filepath, std::error_code& ec);
        void remove(std::string filepath);
		int quit_requester();
        void show_info();
		GLuint set_shader();
		int load_shader(char* filename, char** ShaderSource, unsigned long len);
		void set_ow3oh3();
		void handle_changed_owoh();
        bool handle_button(Button *but, bool circlein = false);
        bool handle_button(Button *but, bool circlein, bool automation, bool copycomp = false);
		int handle_menu(Menu* menu);
		int handle_menu(Menu* menu, float xshift, float yshift);
		void handle_fullscreen();
		void make_menu(std::string name, Menu *&menu, std::vector<std::string> &entries);
		void get_outname(const char *title, std::string filters, std::string defaultdir);
		void get_inname(const char *title, std::string filters, std::string defaultdir);
		void get_multinname(const char* title, std::string filters, std::string defaultdir);
		void get_dir(const char *title , std::string defaultdir);
        void register_v4l2lbdevices(std::vector<std::string>& entries, GLuint tex);
        void v4l2_start_device(std::string device, GLuint tex);
        size_t set_v4l2format(int output, GLuint tex);
#ifdef WINDOWS
		void win_dialog(const char* title, LPCSTR filters, std::string defaultdir, bool open, bool multi);
#endif
		float xscrtovtx(float scrcoord);
		float yscrtovtx(float scrcoord);
		float xvtxtoscr(float vtxcoord);
		float yvtxtoscr(float vtxcoord);
		void add_main_oscmethods();
        GLuint get_tex(Layer *lay);
		bool order_paths(bool dodeckmix);
		void handle_wormgate(bool gate);
        int handle_scrollboxes(Boxx &upperbox, Boxx &lowerbox, int numlines, int scrollpos, int scrlines);
        int handle_scrollboxes(Boxx &upperbox, Boxx &lowerbox, int numlines, int scrollpos, int scrlines, int mx, int
            my);
        void shelf_triggering(ShelfElement *elem, int deck = -1, Layer *lay = nullptr);
		int config_midipresets_handle();
		bool config_midipresets_init();
		void preferences();
		bool preferences_handle();
		void layerstack_scrollbar_handle();
		void preview_modus_buttons();
		void pick_color(Layer* lay, Boxx* cbox);
		void tooltips_handle(int win);
		void define_menus();
		void handle_mixenginemenu();
		void handle_effectmenu();
        void handle_parammenu1();
        void handle_parammenu2();
        void handle_parammenu1b();
        void handle_parammenu2b();
		void handle_parammenu3();
        void handle_parammenu4();
        void handle_parammenu5();
        void handle_parammenu6();
		void handle_loopmenu();
        void handle_monitormenu();
        void make_mixtargetmenu();
        void handle_bintargetmenu();
		void handle_wipemenu();
		void handle_laymenu1();
		void handle_newlaymenu();
		void handle_clipmenu();
		void handle_mainmenu();
		void handle_shelfmenu();
		void handle_filemenu();
        void handle_editmenu();
        void handle_lpstmenu();
        void handle_beatmenu();
        void handle_optionmenu();
        void write_recentprojectlist();
        void socket_server(struct sockaddr_in serv_addr, int opt);
        void socket_client(struct sockaddr_in serv_addr, int opt);
        void socket_server_receive(SOCKET sock);
        void start_discovery();
        void stop_discovery();
        void discovery_broadcast();
        void discovery_listen();
        void fetch_public_ip();
        void stream_to_v4l2loopbacks();
        void postponed_to_front(std::string title);
        void postponed_to_front_win(std::string title, SDL_Window *win = nullptr);
        void concat_files(std::string ofpath, std::string path, std::vector<std::vector<std::string>> filepaths, int count, bool startsolo);
        std::string deconcat_files(std::string path);
        void delete_text(std::string str);
        void register_undo(Param*, Button*);
        void undo_redo_parbut(char offset, bool again = false, bool swap = false);
        void undo_redo_save();
        void add_to_texpool(GLuint tex);
        void add_to_pbopool(GLuint pbo, GLubyte* mapptr);
        void add_to_fbopool(GLuint fbo);
        void add_to_winpool(SDL_Window *win, int sw, int sh);
        GLuint grab_from_texpool(int w, int h, GLint compressed);
        std::pair<GLuint, GLubyte*> grab_from_pbopool(int bsize);
        GLuint grab_from_fbopool();
        SDL_Window* grab_from_winpool(int w, int h);
        std::tuple<Param*, int, int, int, int, int> newparam(int offset, bool swap);
        std::tuple<Button*, int, int, int, int> newbutton(int offset, bool swap);
        std::string get_typestring(std::string path);
        void process_audio();
        void init_audio(const char* device);
        char* bl_recv(int sock, char *buf, size_t sz, int flags);
        int bl_send(int sock, const char *buf, size_t sz, int flags);
        Program();

	private:
#ifdef WINDOWS
        LPCSTR mime_to_wildcard(std::string filters);
#endif
#ifdef POSIX
        char const* mime_to_tinyfds(std::string filters);
#endif
        bool do_order_paths();
        void create_auinmenu();
};



extern Globals *glob;
extern Program *mainprogram;
extern Mixer *mainmix;
extern BinsMain *binsmain;
extern LoopStation *loopstation;
extern LoopStation *lp;
extern LoopStation *lpc;
extern Retarget *retarget;
extern Menu *effectmenu;
extern float smw, smh;
extern SDL_GLContext glc;
extern SDL_GLContext orderglc;
extern LayMidi* laymidiA;
extern LayMidi* laymidiB;
extern LayMidi* laymidiC;
extern LayMidi* laymidiD;
extern std::vector<Boxx*> allboxes;
extern bool collectingboxes;
extern std::mutex dellayslock;

extern float yellow[];
extern float white[];
extern float halfwhite[];
extern float black[];
extern float orange[];
extern float purple[];
extern float yellow[];
extern float lightblue[];
extern float darkblue[];
extern float red[];
extern float lightgrey[];
extern float grey[];
extern float pink[];
extern float green[];
extern float darkgreygreen[];
extern float darkgreen1[];
extern float darkgreen2[];
extern float blue[];
extern float alphawhite[];
extern float alphablue[];
extern float alphagreen[];
extern float darkred1[];
extern float darkred2[];
extern float darkred3[];
extern float darkgrey[];

extern "C" int kdialogPresent();

#ifdef POSIX
extern void Sleep(int milliseconds);
extern void strcat_s(char* dest, const char* input);
#endif

extern bool safegetline(std::istream& is, std::string &t);
extern void midi_callback(double deltatime, std::vector< unsigned char >* message, void* userData);

extern bool display_mix();

extern bool get_imagetex(Layer *lay, std::string path);
extern bool get_videotex(Layer *lay, std::string path);
extern bool get_layertex(Layer *lay, std::string path);
extern bool get_deckmixtex(Layer *lay, std::string path);
extern int encode_frame(AVFormatContext *fmtctx, AVFormatContext *srcctx, AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *pkt, FILE *outfile, int framenr);

extern std::vector<Layer*>& choose_layers(bool j);
extern void make_layboxes();
extern void handle_scenes(Scene* scene);
extern void switch_to_scene(int i, Scene* from_scene, Scene* to_scene);

extern void new_file(int decks, bool alive, bool add);
extern void draw_box(float* linec, float* areac, float x, float y, float wi, float he, float dx, float dy, float
scale, float opacity, int circle, GLuint tex, float fw, float fh, bool text, bool vertical, bool inverted);
extern void draw_box(float* linec, float* areac, float x, float y, float wi, float he, float dx, float dy, float scale, float opacity, int circle, GLuint tex, float fw, float fh, bool text);
extern void draw_box(float* linec, float* areac, float x, float y, float wi, float he, GLuint tex, bool text, bool
vertical, bool inverted);
extern void draw_box(float* linec, float* areac, float x, float y, float wi, float he, GLuint tex);
extern void draw_box(float *color, float x, float y, float radius, int circle);
extern void draw_box(float *color, float x, float y, float radius, int circle, float fw, float fh);
extern void draw_box(Boxx *box, GLuint tex);
extern void draw_box(float *linec, float *areac, Boxx *box, GLuint tex);
extern void draw_box(float *linec, float *areac, std::unique_ptr <Boxx> const &box, GLuint tex);
extern void draw_box(Boxx *box, float opacity, GLuint tex);
extern void draw_box(Boxx *box, float dx, float dy, float scale, GLuint tex);
extern void draw_direct(float* linec, float* areac, float x, float y, float wi, float he, float dx, float dy, float scale, float opacity, int circle, GLuint tex, float smw, float smh, bool vertical, bool inverted);

extern void register_triangle_draw(float* linec, float* areac, float x1, float y1, float xsize, float ysize, ORIENTATION orient, TRIANGLE_TYPE type);
extern void register_triangle_draw(float* linec, float* areac, float x1, float y1, float xsize, float ysize, ORIENTATION orient, TRIANGLE_TYPE type, bool directdraw);
extern void draw_triangle(gui_triangle* triangle);

extern void register_line_draw(float* linec, float x1, float y1, float x2, float y2);
extern void register_line_draw(float* linec, float x1, float y1, float x2, float y2, bool directdraw);
extern void draw_line(gui_line* line);

extern std::vector<float> render_text(const char* text, float* textc, float x, float y, float sx, float sy, const char* save = "true");
extern std::vector<float> render_text(const char* text, float* textc, float x, float y, float sx, float sy, int smflag);
extern std::vector<float> render_text(const char* text, float* textc, float x, float y, float sx, float sy, int smflag, bool vertical);
extern std::vector<float> render_text(const char* text, float *textc, float x, float y, float sx, float sy, int smflag, bool display, bool vertical);
extern std::vector<float> render_text(const std::string& text, float* textc, float x, float y, float sx, float sy);
extern std::vector<float> render_text(const std::string& text, float* textc, float x, float y, float sx, float sy, int smflag);
extern std::vector<float> render_text(const std::string& text, float* textc, float x, float y, float sx, float sy, int smflag, bool vertical);
extern std::vector<float> render_text(const std::string& text, const char* ctext, float *textc, float x, float y, float sx, float sy, int smflag, bool display, bool vertical, const char* save = "true");
extern float textwvec_total(std::vector<float> textwvec);

extern float xscrtovtx(float scrcoord);
extern float yscrtovtx(float scrcoord);

extern float pdistance(float x, float y, float x1, float y1, float x2, float y2);
extern void enddrag();

extern void open_files_bin();
extern void save_bin(std::string path);
extern void save_thumb(std::string path, GLuint tex);
extern void open_thumb(std::string path, GLuint tex);
extern void new_state();
extern GLuint copy_tex(GLuint tex);
extern GLuint copy_tex(GLuint tex, bool yflip);
extern GLuint copy_tex(GLuint tex, int tw, int th);
extern GLuint copy_tex(GLuint tex, int tw, int th, bool yflip, int sx = 0, int sy = 0);
extern void blacken(GLuint tex);
extern GLuint set_texes(GLuint tex, GLuint *fbo, float ow, float oh);

void open_genmidis(std::string path);
void save_genmidis(std::string path);

void screenshot();

int find_stream_index(int *stream_idx, AVFormatContext *video, enum AVMediaType type);

void set_live_base(Layer *lay, std::string livename);

extern void set_queueing(bool onoff);

extern Effect* new_effect(Layer *lay, EFFECT_TYPE type, int ffglnr, int isfnr);

extern bool exists(std::string name);
extern std::string dirname(std::string pathname);
extern std::string basename(std::string pathname);
extern std::string remove_extension(std::string filename);
extern bool rename(std::string source, std::string destination);
extern bool copy_file(const std::string& source_path, const std::string& destination_path);
extern bool copy_dir(const std::string& source_path, const std::string& destination_path, bool recursive = true, bool overwrite = true);
extern bool check_permission(std::string directory);
extern std::string remove_version(std::string filename);
extern std::string pathtoplatform(std::string path);
extern std::string pathtoposix(std::string path);
extern std::vector<std::string> getListOfDrives();
extern std::string test_driveletters(std::string path);
extern bool isimage(std::string path);
extern bool isvideo(std::string path);
extern bool isdeckfile(std::string path);
extern bool ismixfile(std::string path);
extern bool islayerfile(std::string path);

extern void drag_into_layerstack(std::vector<Layer*>& layers, bool deck);

extern void do_text_input(float x, float y, float sx, float sy, int mx, int my, float width, int smflag, PrefItem* item);
extern void do_text_input(float x, float y, float sx, float sy, int mx, int my, float width, int smflag, PrefItem* item, bool directdraw);
extern void end_input();

extern void onestepfrom(bool stage, Node *node, Node *prevnode, GLuint prevfbotex, GLuint prevfbo);

extern int osc_param(const char *path, const char *types, lo_arg **argv, int argc, lo_message m, void *data);

extern void LockBuffer(GLsync& syncObj);
extern void WaitBuffer(GLsync& syncObj);

extern void make_searchbox(bool val);

extern std::string find_unused_filename(std::string basename, std::string path, std::string extension);

extern std::string exec(const char* cmd);
