#if defined(WIN32) && !defined(__linux__)
#define WINDOWS
#elif defined(__linux__) && !defined(WIN32)
#define POSIX
#endif

#include <boost/filesystem.hpp>

#include <string>
#include "GL/gl.h"
#ifdef WINDOWS
#include "direnthwin/dirent.h"
#endif
#include <rtmidi/RtMidi.h>
#include <istream>
#include <lo/lo.h>
#include <lo/lo_cpp.h>
#ifdef POSIX
#include "dirent.h"
#endif


class PrefCat;
class Menu;
class LoopStation;
class Retarget;
class BinsMain;
class Bin;
class BinElement;
class ShelfElement;

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
	TM_SCRATCH = 11,
	TM_SPEEDZERO = 12,
} TM_LEARN;

typedef enum
{
	EDIT_NONE = 0,
	EDIT_CANCEL = 1,
	EDIT_BINNAME = 2,
	EDIT_BINELEMNAME = 3,
	EDIT_STRING = 4,
	EDIT_NUMBER = 5,
	EDIT_PARAM = 6,
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
    PREF_PATH = 2,
    PREF_PATHS = 3,
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
	float linec[4];
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


class Shelf {
public:
	bool side;
	std::string basepath = "";
	std::vector<ShelfElement*> elements;
	std::vector<Button*> buttons;
	int prevnum = -1;
	int newnum = -1;
	bool ret;
	void handle();
	void erase();
	void save(const std::string& path);
	bool open(const std::string& path);
	void open_files_shelf();
	bool insert_deck(const std::string& path, bool deck, int pos);
	bool insert_mix(const std::string& path, int pos);
	Shelf(bool side);
};

class ShelfElement {
public:
	std::string path;
	std::string jpegpath;
	ELEM_TYPE type;
	GLuint tex = -1;
	GLuint oldtex = -1;
	Button* button;
	Box* sbox;
	Box* pbox;
	Box* cbox;
	int launchtype = 0;
	std::vector<int> cframes;
	std::vector<Layer*> nblayers;
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
        std::string autosavedir;
        std::vector<std::string> autosavelist;
        float ow = 1920.0f;
        float oh = 1080.0f;
		void newp(const std::string &path);
		void open(const std::string &path);
        void save(const std::string& path);
        void autosave();
		void do_save(const std::string& path);
		void delete_dirs();
		void create_dirs(const std::string &path);
private:
};

class Preferences {
	public:
		std::vector<PrefCat*> items;
		int curritem = 0;
		void load();
		void save();
		Preferences();
};

class PrefItem {
	public:
		std::string name;
		PREF_TYPE type;
		void *dest;
		bool onoff = false;
		int value;
		std::string path;
		bool renaming = false;
		bool choosing = false;
		Box *namebox;
		Box *valuebox;
		Box *iconbox;

		bool connected = true;
		RtMidiIn *midiin = nullptr;

		PrefItem(PrefCat *cat, int pos, std::string name, PREF_TYPE type, void *dest);
};

class PrefCat {
	public:
		std::vector<PrefItem*> items;
		std::string name;
		Box *box;
};

class PIMidi: public PrefCat {
	public:
		std::vector<std::string> onnames;
		void populate();
		PIMidi();
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
        int midi0;
        int midi1;
        std::string midiport;
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
        MidiElement *scratch;
        MidiElement *scratchtouch;
        MidiElement *speed;
        MidiElement *speedzero;
        MidiElement *opacity;
        MidiElement *setcue;
        MidiElement *tocue;

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
		GLuint texture;
		std::vector<GLuint> texturevec;
		GLuint tbo;
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
		float resfac;
};

class Program {
	public:
        int jav = 0;
		Project *project;
		NodesMain *nodesmain;
		GLuint ShaderProgram;
		GLuint ShaderProgram_tm;
		GLuint ShaderProgram_pr;
		GLuint fbovao;
		GLuint fbotex[4];
		GLuint frbuf[4];
		GLuint bvao;
		GLuint boxvao;
		GLuint prboxvao;
		GLuint tmboxvao;
		GLuint bvbuf;
		GLuint boxvbuf;
		GLuint prboxvbuf;
		GLuint tmboxvbuf;
		GLuint btbuf;
		GLuint boxtbuf;
		GLuint prboxtbuf;
		GLuint tmboxtbuf;
		GLuint texvao;
		GLuint rtvbo;
		GLuint rttbo;
		GLuint pr_texvao;
		GLuint pr_rtvbo;
		GLuint pr_rttbo;
		GLuint tm_texvao;
		GLuint tm_rtvbo;
		GLuint tm_rttbo;
        GLuint bgtex;
		std::vector<OutputEntry*> outputentries;
		std::vector<Button*> buttons;
		Box *scrollboxes[2];
		Box *prevbox;
		Layer *loadlay;
		Layer *prelay = nullptr;
		SDL_Window *mainwindow;
		std::vector<EWindow*> mixwindows;
		std::vector<Menu*> menulist;
		std::vector<Menu*> actmenulist;
		Menu *effectmenu = nullptr;
		Menu *mixmodemenu = nullptr;
		Menu *parammenu1 = nullptr;
		Menu* parammenu2 = nullptr;
		Menu* parammenu3 = nullptr;
		Menu* parammenu4 = nullptr;
		Menu* speedmenu = nullptr;
		Menu *loopmenu = nullptr;
		Menu *laymenu1 = nullptr;
		Menu *laymenu2 = nullptr;
		Menu* newlaymenu = nullptr;
		Menu* clipmenu = nullptr;
		Menu *aspectmenu = nullptr;
		Menu *mixtargetmenu = nullptr;
		Menu *fullscreenmenu = nullptr;
		Menu *mixenginemenu = nullptr;
		Menu *livemenu = nullptr;
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
		Menu* filedomenu = nullptr;
		Menu* laylistmenu1 = nullptr;
		Menu* laylistmenu2 = nullptr;
        Menu* editmenu = nullptr;
        Menu* lpstmenu = nullptr;
        Menu* sendmenu = nullptr;
		bool menuactivation = false;
		bool menuchosen = false;
		std::vector<int> menuresults;
		bool intopmenu = false;
		int fullscreen = -1;
		bool test = false;
		float z = 0.0f;
		int mx;
		int my;
		int oldmx;
		int oldmy;
        int shelfmx;
        int shelfmy;
		int iemy;
		float ow = 1920.0f;
		float oh = 1080.0f;
		float ow3, oh3;
		float oldow = 1920.0f;
		float oldoh = 1080.0f;
		float layw = 0.319f;
		float layh = 0.319f;
		float numw = 0.041f;
		float numh = 0.041f;
		float monh = 0.3f;
		float cwx;
		float cwy;
		bool transforming = false;
		bool leftmousedown = false;
		bool middlemousedown = false;
		bool rightmousedown = false;
		bool leftmouse = false;
		bool orderleftmouse = false;
		bool orderleftmousedown = false;
		bool lmover = false;
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
		bool blocking = false;
		bool eXit = false;
		std::string temppath;
		std::string docpath;
        std::string fontpath;
        std::string contentpath;
		std::string path;
		std::vector<std::string> paths;
		int counting;
		std::string pathto;
		Button *toscreen;
		Button *backtopre;
		Button *modusbut;
		bool prevmodus = true;
		BlendNode *bnodeend;
		BlendNode *bnodeendcomp;
		Box *outputmonitor;
		Box *mainmonitor;
		Box *deckmonitor[2];
		Box *cwbox;
		bool cwon = false;
		int cwmouse = false;
		Button *effcat[2];
		int efflines = 7;
		Box *effscrollupA;
		Box *effscrolldownA;
		Box *effscrollupB;
		Box *effscrolldownB;
		Box* addeffectbox;
		Box* inserteffectbox;
        Box* orderscrolldown;
        Box* orderscrollup;
        Box* defaultsearchscrolldown;
        Box* defaultsearchscrollup;
        Box* searchscrolldown;
        Box* searchscrollup;
		bool startloop = false;
		bool newproject = false;
		std::vector<std::string> recentprojectpaths;
		bool wiping = false;
		float texth;
		float bdcoords[32][65536];
		float bdtexcoords[32][65536];
		char bdcolors[32][4096];
		char bdtexes[32][2048];
		std::vector<float> bdwi;
		std::vector<float> bdhe;
		float* bdvptr[32];
		float* bdtcptr[32];
		char* bdcptr[32];
		char* bdtptr[32];
		GLuint* bdtnptr[32];
		GLuint bdvao;
		GLuint bdvbo;
		GLuint bdtcbo;
		GLuint bdibo;
		int boxcount;
		GLint maxtexes = 16;
		int countingtexes[32];
		GLuint boxtexes[32][1024];
		int boxoffset[32];
		int currbatch = 0;
		short indices[6144];
		float boxz = 0.0f;
		bool directmode = false;
		bool frontbatch = false;
		std::vector<GUI_Element*> guielems;
		Button* onscenebutton;
		float onscenemilli;
		Box* delbox;
		Box* addbox;
		bool repeatdefault = true;

		GLuint boxcoltbo;
		GLuint boxtextbo;
		GLuint boxbrdtbo;
		GLuint bdcoltex;
		GLuint bdtextex;
		GLuint bdbrdtex;

		lo::ServerThread *st;
		std::unordered_map<std::string, int> wipesmap;
		std::unordered_map<EFFECT_TYPE, std::string> effectsmap;
		std::vector<EFFECT_TYPE> abeffects;
		
		SDL_Window* dummywindow = nullptr;
		SDL_Window* config_midipresetswindow = nullptr;
		bool drawnonce = false;
		bool midipresets = false;
		int midipresetsset = 1;
		Box* tmset;
		Box* tmscratch;
		Box *tmfreeze;
		Box *tmplay;
		Box *tmbackw;
		Box *tmbounce;
		Box *tmfrforw;
        Box *tmfrbackw;
        Box *tmstop;
        Box *tmloop;
		Box *tmspeed;
		Box *tmspeedzero;
		Box *tmopacity;
		TM_LEARN tmlearn = TM_NONE;
		TM_LEARN tmchoice = TM_NONE;
		int waitmidi = 0;
		std::vector<int> openports;
		std::vector<PrefItem*> pmon;
		clock_t stt;
		std::vector<unsigned char> savedmessage;
		PrefItem* savedmidiitem;
		bool queueing = false;
		int filecount;
        bool openerr = false;

		SDL_Window *prefwindow = nullptr;
		bool prefon = false;
		Preferences *prefs;
		bool needsclick = false;
		bool insmall;
		bool showtooltips = true;
		bool longtooltips = true;
		Box *tooltipbox = nullptr;
		float tooltipmilli = 0.0f;
        bool ttreserved = false;
        bool boxhit = false;
		bool autosave;
		float asminutes = 1;
		int astimestamp = 0;
		int qualfr = 3;

		std::unordered_map <std::string, GUIString*> guitextmap;
		std::unordered_map <std::string, GUIString*> prguitextmap;
		std::unordered_map <std::string, GUIString*> tmguitextmap;
		std::vector<std::wstring> livedevices;
		std::vector<std::string> devices;
		std::vector<std::string> busylist;
		std::vector<Layer*> busylayers;
		std::vector<Layer*> mimiclayers;
		
		bool binsscreen = false;
		BinElement *dragbinel = nullptr;
		Clip *dragclip = nullptr;
		Layer *draglay = nullptr;
		std::string dragpath;
		int dragpos; 
		bool drag = false;
		bool dragmousedown = false;
		bool inwormgate = false;
		Button* wormgate1;
		Button* wormgate2;
		DIR *opendir;
		bool gotcameras = false;

		EDIT_TYPE renaming = EDIT_NONE;
		std::string choosedir = "";
		std::string inputtext;
		std::string backupname;
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

		int numcores = 0;
		int maxthreads;
		int encthreads = 0;
		bool threadmode = true;
		std::mutex hapmutex;
		std::condition_variable hap;
		bool hapnow = false;

		std::string projdir;
		std::string binsdir;
		std::string currprojdir;
		std::string currbinsdir;
		std::string currshelfdir;
		std::string currrecdir;
		std::string currshelfdirdir;
		std::string currshelffilesdir;
		std::string currclipfilesdir;
		std::string currbinfilesdir;
		std::string currfilesdir;
		std::string currstatedir;
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
		ShelfElement* midishelfelem = nullptr;
		Shelf *shelves[2];
		int inshelf = -1;
		int inclips = -1;
		bool openclipfiles = false;
		int clipfilescount = 0;
		Clip* clipfilesclip = nullptr;
		Layer* clipfileslay = nullptr;
		bool openfileslayers = false;
		bool openfilesqueue = false;
		int filescount = 0;
		Layer* fileslay = nullptr;
		int multistage = 0;
		Effect* drageff = nullptr;
		int drageffpos = -1;
		bool drageffsense = false;
		std::string dragstr = "";
		int dragpathpos = -1;
		bool dragpathsense = false;
		std::vector<Box*> pathboxes;
		std::vector<GLuint> pathtexes;
		int pathscroll = 0;
		bool indragbox = false;
		Box* dragbox;
		bool dragmiddle = false;
		bool dragout[2] = { true, true };
		std::string quitting;
		Layer* draginscrollbarlay = nullptr;
		bool projnamechanged = false;
		bool saveproject = false;

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
        bool server = false;
        bool connected = false;
        std::string seatname = "";
        std::string serverip = "";
        bool serveripchanged = false;
        char localip[80];
        struct sockaddr_in serv_addr;
#ifdef WINDOWS
        std::unordered_map<std::string, SOCKET> connmap;
#endif
#ifdef POSIX
        std::unordered_map<std::string, int> connmap;
        #endif
        std::mutex clientmutex;
        std::condition_variable startclient;

        std::vector<std::string> v4l2lbdevices;

		int quit_requester();
		GLuint set_shader();
		int load_shader(char* filename, char** ShaderSource, unsigned long len);
		void set_ow3oh3();
		void handle_changed_owoh();
		int handle_menu(Menu* menu);
		int handle_menu(Menu* menu, float xshift, float yshift);
		void handle_fullscreen();
		void make_menu(const std::string &name, Menu *&menu, std::vector<std::string> &entries);
		void get_outname(const char *title, std::string filters, std::string defaultdir);
		void get_inname(const char *title, std::string filters, std::string defaultdir);
		void get_multinname(const char* title, std::string filters, std::string defaultdir);
		void get_dir(const char *title , std::string defaultdir);
#ifdef WINDOWS
		void win_dialog(const char* title, LPCSTR filters, std::string defaultdir, bool open, bool multi);
#endif
		float xscrtovtx(float scrcoord);
		float yscrtovtx(float scrcoord);
		float xvtxtoscr(float vtxcoord);
		float yvtxtoscr(float vtxcoord);
		void preview_init();
		void add_main_oscmethods();
		bool order_paths(bool dodeckmix);
		void handle_wormgate(bool gate);
        int handle_scrollboxes(Box *upperbox, Box *lowerbox, int numlines, int scrollpos, int scrlines);
        int handle_scrollboxes(Box* upperbox, Box* lowerbox, int numlines, int scrollpos, int scrlines, int mx, int
            my);
		void shelf_miditriggering();
		int config_midipresets_handle();
		bool config_midipresets_init();
		void preferences();
		bool preferences_handle();
		void layerstack_scrollbar_handle();
		void preview_modus_buttons();
		void pick_color(Layer* lay, Box* cbox);
		void tooltips_handle(int win);
		void define_menus();
		void handle_mixenginemenu();
		void handle_effectmenu();
		void handle_parammenu1();
		void handle_parammenu2();
		void handle_parammenu3();
		void handle_parammenu4();
		void handle_speedmenu();
		void handle_loopmenu();
		void handle_mixtargetmenu();
		void handle_wipemenu();
		void handle_laymenu1();
		void handle_newlaymenu();
		void handle_clipmenu();
		void handle_mainmenu();
		void handle_shelfmenu();
		void handle_filemenu();
        void handle_editmenu();
        void handle_lpstmenu();
        void write_recentprojectlist();
        void socket_server(struct sockaddr_in serv_addr, int opt);
        void socket_client(struct sockaddr_in serv_addr, int opt);
        void socket_server_recieve(SOCKET sock);
        void stream_to_v4l2loopbacks();
        Program();
		
	private:
#ifdef WINDOWS
        LPCSTR mime_to_wildcard(std::string filters);
#endif
#ifdef POSIX
        char const* mime_to_tinyfds(std::string filters);
#endif
        bool do_order_paths();
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
extern SDL_GLContext glc;
extern SDL_GLContext glc;
extern SDL_GLContext glc_th;
extern LayMidi* laymidiA;
extern LayMidi* laymidiB;
extern LayMidi* laymidiC;
extern LayMidi* laymidiD;

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

extern std::istream& safegetline(std::istream& is, std::string& t);
extern void mycallback(double deltatime, std::vector< unsigned char >* message, void* userData);

extern GLuint get_imagetex(const std::string& path);
extern GLuint get_videotex(const std::string& path);
extern GLuint get_layertex(const std::string& path);
extern GLuint get_deckmixtex(const std::string& path);
extern int encode_frame(AVFormatContext *fmtctx, AVFormatContext *srcctx, AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *pkt, FILE *outfile, int framenr);

extern std::vector<Layer*>& choose_layers(bool j);
extern void make_layboxes();

extern void new_file(int decks, bool alive);
extern void draw_box(float* linec, float* areac, float x, float y, float wi, float he, float dx, float dy, float
scale, float opacity, int circle, GLuint tex, float fw, float fh, bool text, bool vertical, bool inverted);
extern void draw_box(float* linec, float* areac, float x, float y, float wi, float he, float dx, float dy, float scale, float opacity, int circle, GLuint tex, float fw, float fh, bool text);
extern void draw_box(float* linec, float* areac, float x, float y, float wi, float he, GLuint tex, bool text, bool
vertical, bool inverted);
extern void draw_box(float* linec, float* areac, float x, float y, float wi, float he, GLuint tex);
extern void draw_box(float *color, float x, float y, float radius, int circle);
extern void draw_box(float *color, float x, float y, float radius, int circle, float fw, float fh);
extern void draw_box(Box *box, GLuint tex);
extern void draw_box(float *linec, float *areac, Box *box, GLuint tex);
extern void draw_box(float *linec, float *areac, std::unique_ptr <Box> const &box, GLuint tex);
extern void draw_box(Box *box, float opacity, GLuint tex);
extern void draw_box(Box *box, float dx, float dy, float scale, GLuint tex);

extern void register_triangle_draw(float* linec, float* areac, float x1, float y1, float xsize, float ysize, ORIENTATION orient, TRIANGLE_TYPE type);
extern void register_triangle_draw(float* linec, float* areac, float x1, float y1, float xsize, float ysize, ORIENTATION orient, TRIANGLE_TYPE type, bool directdraw);
extern void draw_triangle(gui_triangle* triangle);

extern void register_line_draw(float* linec, float x1, float y1, float x2, float y2);
extern void register_line_draw(float* linec, float x1, float y1, float x2, float y2, bool directdraw);
extern void draw_line(gui_line* line);

extern std::vector<float> render_text(std::string text, float *textc, float x, float y, float sx, float sy);
extern std::vector<float> render_text(std::string text, float* textc, float x, float y, float sx, float sy, int smflag);
extern std::vector<float> render_text(std::string text, float* textc, float x, float y, float sx, float sy, int smflag, bool vertical);
extern std::vector<float> render_text(std::string text, float* textc, float x, float y, float sx, float sy, int smflag, bool display, bool vertical);
extern float textwvec_total(std::vector<float> textwvec);

extern float xscrtovtx(float scrcoord);
extern float yscrtovtx(float scrcoord);

extern float pdistance(float x, float y, float x1, float y1, float x2, float y2);
extern void enddrag();

extern void open_files_bin();
extern void save_bin(const std::string &path);
extern void save_thumb(std::string path, GLuint tex);
extern void open_thumb(std::string path, GLuint tex);
extern void new_state();
void concat_files(std::ostream &ofile, const std::string &path, std::vector<std::vector<std::string>> &jpegpaths);
bool check_version(const std::string &path);
std::string deconcat_files(const std::string &path);
extern GLuint copy_tex(GLuint tex);
extern GLuint copy_tex(GLuint tex, bool yflip);
extern GLuint copy_tex(GLuint tex, int tw, int th);
extern GLuint copy_tex(GLuint tex, int tw, int th, bool yflip);
extern void blacken(GLuint tex);

extern GLuint set_texes(GLuint tex, GLuint *fbo, float ow, float oh);

void open_genmidis(std::string path);
void save_genmidis(std::string path);

void screenshot();

int open_codec_context(int *stream_idx, AVFormatContext *video, enum AVMediaType type);

void set_live_base(Layer *lay, std::string livename);

extern void set_queueing(bool onoff);

extern Effect* new_effect(EFFECT_TYPE type);

extern bool exists(const std::string &name);
extern std::string dirname(std::string pathname);
extern std::string basename(std::string pathname);
extern std::string remove_extension(std::string filename);
extern std::string chop_off(std::string filename);
extern std::string remove_version(std::string filename);
extern std::string pathtoplatform(std::string path);
extern bool isimage(const std::string &path);
extern bool isvideo(const std::string &path);

extern void drag_into_layerstack(std::vector<Layer*>& layers, bool deck);

extern void do_text_input(float x, float y, float sx, float sy, int mx, int my, float width, int smflag, PrefItem* item);
extern void do_text_input(float x, float y, float sx, float sy, int mx, int my, float width, int smflag, PrefItem* item, bool directdraw);
extern void end_input();

extern void onestepfrom(bool stage, Node *node, Node *prevnode, GLuint prevfbotex, GLuint prevfbo);

extern int osc_param(const char *path, const char *types, lo_arg **argv, int argc, lo_message m, void *data);

extern void LockBuffer(GLsync& syncObj);
extern void WaitBuffer(GLsync& syncObj);

extern void make_searchbox();

extern std::string find_unused_filename(std::string basename, std::string path, std::string extension);

extern std::string exec(const char* cmd);

extern void set_nonblock(int socket);