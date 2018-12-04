#include <string>
#include "GL/gl.h"
#include <dirent.h>
#include "nfd.h"
#include "RtMidi.h"
#include <istream>
#include <lo/lo.h>
#include <lo/lo_cpp.h>



class PrefCat;
class Menu;
class LoopStation;
class BinsMain;
class Bin;
class BinElement;

typedef enum
{
	TM_NONE = 0,
	TM_PLAY = 1,
	TM_BACKW = 2,
	TM_BOUNCE = 3,
	TM_FRFORW = 4,
	TM_FRBACKW = 5,
	TM_SPEED = 6,
	TM_OPACITY = 7,
	TM_FREEZE = 8,
	TM_SCRATCH = 9,
	TM_SPEEDZERO = 10,
} TM_LEARN;

typedef enum
{
	EDIT_NONE = 0,
	EDIT_CANCEL = 1,
	EDIT_BINNAME = 2,
	EDIT_STRING = 3,
	EDIT_NUMBER = 4,
	EDIT_PARAM = 5,
} EDIT_TYPE;

typedef enum
{
	PREF_ONOFF = 0,
	PREF_NUMBER = 1,
	PREF_PATH = 2,
} PREF_TYPE;

struct mix_target_struct {
	int width;
	int height;
};


class Shelf {
	public:
		std::string basepath = "";
		std::string paths[16];
		ELEM_TYPE types[16];
		GLuint texes[16];
		Button *buttons[16];
		void handle();
		void erase();
		void save(const std::string &path);
		void open(const std::string &path);
		bool open_videofile(const std::string &path, int pos);
		bool open_layer(const std::string &path, int pos);
		void open_dir();
		bool open_image(const std::string &path, int pos);
		Shelf(bool side);
};

class Project {
	public:
		std::string path;
		std::string binsdir;
		std::string recdir;
		std::string shelfdir;
		void newp(const std::string &path);
		void open(const std::string &path);
		void save(const std::string &path);
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
		PrefItem(PrefCat *cat, int pos, std::string name, PREF_TYPE type, void *dest);
};

class PrefCat {
	public:
		std::vector<PrefItem*> items;
		std::string name;
		Box *box;
};

class PMidiItem: public PrefItem {
	public:
		bool connected = true;
		RtMidiIn *midiin;
		PMidiItem(PrefCat *cat, int pos, std::string name, PREF_TYPE type, void *dest)
		: PrefItem(cat, pos, name, type, dest) {}
};

class PIMidi: public PrefCat {
	public:
		std::vector<std::string> onnames;
		void populate();
		PIMidi();
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
		PIVid();
};
		
class PIProg: public PrefCat {
	public:
		PIProg();
};
		

class LayMidi {
	public:
		int play[2] = {-1, -1};
		std::string playstr;
		int backw[2] = {-1, -1};
		std::string backwstr;
		int pausestop[2] = {-1, -1};
		std::string pausestopstr;
		int bounce[2] = {-1, -1};
		std::string bouncestr;
		int frforw[2] = {-1, -1};
		std::string frforwstr;
		int frbackw[2] = {-1, -1};
		std::string frbackwstr;
		int scratch[2] = {-1, -1};
		std::string scratchstr;
		int scratchtouch[2] = {-1, -1};
		std::string scratchtouchstr;
		int speed[2] = {-1, -1};
		std::string speedstr;
		int speedzero[2] = {-1, -1};
		std::string speedzerostr;
		int opacity[2] = {-1, -1};
		std::string opacitystr;
		int setcue[2] = {-1, -1};
		std::string setcuestr;
		int tocue[2] = {-1, -1};
		std::string tocuestr;
};

class GUIString {
	public:
		std::string text;
		float textw;
		float texth;
		float sx;
		GLuint texture;
		GLuint vbo;
		GLuint tbo;
		GLuint vao;
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

class Program {
	public:
		Project *project;
		NodesMain *nodesmain;
		GLuint ShaderProgram;
		GLuint ShaderProgram_tm;
		GLuint ShaderProgram_pr;
		GLuint fbovao;
		GLuint globfbo;
		GLuint globfbotex;
		GLuint smglobfbo_tm;
		GLuint smglobfbotex_tm;
		GLuint smglobfbo_pr;
		GLuint smglobfbotex_pr;
		GLuint fbotex[4];
		GLuint frbuf[4];
		std::vector<OutputEntry*> outputentries;
		std::vector<Button*> buttons;
		Box *scrollboxes[2];
		Layer *loadlay;
		Layer *prelay = nullptr;
		SDL_Window *mainwindow;
		std::vector<EWindow*> mixwindows;
		std::vector<Menu*> menulist;
		std::vector<Menu*> actmenulist;
		Menu *effectmenu = nullptr;
		Menu *mixmodemenu = nullptr;
		Menu *parammenu1 = nullptr;
		Menu *parammenu2 = nullptr;
		Menu *loopmenu = nullptr;
		Menu *deckmenu = nullptr;
		Menu *laymenu = nullptr;
		Menu *loadmenu = nullptr;
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
		Menu *bin2menu = nullptr;
		Menu *genmidimenu = nullptr;
		Menu *genericmenu = nullptr;
		Menu *shelfmenu = nullptr;
		bool menuactivation;
		bool menuchosen;
		int menux;
		int menuy;
		std::vector<int> menuresults;
		int fullscreen = -1;
		bool test = false;
		int mx;
		int my;
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
		bool leftmousedown = false;
		bool middlemousedown = false;
		bool leftmouse = false;
		bool lmsave = false;
		bool doubleleftmouse = false;
		bool middlemouse = false;
		bool rightmouse = false;
		float mousewheel = false;
		bool del = false;
		bool ctrl = false;
		bool menuondisplay = false;
		bool blocking = false;
		std::string temppath;
		const char *path;
		std::vector<std::string> paths;
		int counting;
		std::string pathto;
		Button *toscreen;
		Button *backtopre;
		Button *modusbut;
		bool prevmodus = true;
		Param *deckspeed[2];
		BlendNode *bnodeend;
		BlendNode *bnodeendcomp;
		Box *outputmonitor;
		Box *mainmonitor;
		Box *deckmonitor[2];
		Box *cwbox;
		bool cwon = false;
		int cwmouse = false;
		Button *effcat[2];
		Box *effscrollupA;
		Box *effscrolldownA;
		Box *effscrollupB;
		Box *effscrolldownB;
		Box *addeffectbox;
		bool startloop = false;
		std::vector<std::string> recentprojectpaths;
		
		lo::ServerThread *st;
		std::unordered_map<std::string, int> wipesmap;
		std::unordered_map<EFFECT_TYPE, std::string> effectsmap;
		
		SDL_Window *tunemidiwindow = nullptr;
		bool drawnonce = false;
		bool tunemidi = false;
		int tunemidideck;
		Box *tmscratch;
		Box *tmfreeze;
		Box *tmplay;
		Box *tmbackw;
		Box *tmbounce;
		Box *tmfrforw;
		Box *tmfrbackw;
		Box *tmspeed;
		Box *tmspeedzero;
		Box *tmopacity;
		TM_LEARN tmlearn = TM_NONE;
		TM_LEARN tmchoice = TM_NONE;
		int waitmidi = 0;
		std::vector<int> openports;
		std::vector<PMidiItem*> pmon;
		clock_t stt;
		std::vector<unsigned char> savedmessage;
		PMidiItem* savedmidiitem;
		bool queueing;
		int filecount;
		
		SDL_Window *prefwindow = nullptr;
		bool prefon = false;
		Preferences *prefs;
		bool needsclick = false;
		bool insmall;
		bool showtooltips = true;
		Box *tooltipbox = nullptr;
		float tooltipmilli = 0.0f;
		bool autosave;
		int asminutes = 1;
		int astimestamp = 0;
		
		std::vector<GUIString*> guistrings;
		std::vector<GUIString*> prguistrings;
		std::vector<GUIString*> tmguistrings;
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
		bool inwormhole = false;
		Button *wormhole;
		DIR *opendir;
		int menuset;
		
		EDIT_TYPE renaming = EDIT_NONE;
		std::string choosedir = "";
		std::string inputtext;
		std::string backupname;
		int cursorpos;

		int numcores = 0;
		int maxthreads;
		int encthreads = 0;
		bool threadmode = true;
		std::mutex hapmutex;
		std::condition_variable hap;
		bool hapnow = false;
		
		std::string projdir;
		std::string binsdir;
		std::string recdir;
		std::string shelfdir;
		std::string autosavedir;
		bool openshelfdir = false;
		std::string shelfpath;
		int shelfdircount;
		bool shelfdrag = false;
		Shelf *shelves[2];
		
		void quit(std::string msg);
		GLuint set_shader();
		int load_shader(char* filename, char** ShaderSource, unsigned long len);
		void set_ow3oh3();
		void make_menu(const std::string &name, Menu *&menu, std::vector<std::string> &entries);
		void get_outname(const char *title, std::string filters, std::string defaultdir);
		void get_inname(const char *title, std::string filters, std::string defaultdir);
		void get_multinname(const char* title);
		void get_dir(const char *title);
		float xscrtovtx(float scrcoord);
		float yscrtovtx(float scrcoord);
		float xvtxtoscr(float vtxcoord);
		float yvtxtoscr(float vtxcoord);
		void preveff_init();
		void add_main_oscmethods();
		Program();
		
	private:
		std::string mime_to_wildcard(std::string filters);
};

extern Globals *glob;
extern Program *mainprogram;
extern Mixer *mainmix;
extern BinsMain *binsmain;
extern LoopStation *loopstation;
extern LoopStation *lp;
extern LoopStation *lpc;
extern Menu *effectmenu;
extern Menu *mixmodemenu;

extern mix_target_struct mixtarget[2];

extern int encode_frame(AVFormatContext *fmtctx, AVFormatContext *srcctx, AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *pkt, FILE *outfile, int framenr);
extern void open_video(float frame, Layer *lay, const std::string &filename, int reset);

extern std::vector<Layer*>& choose_layers(bool j);
extern void make_layboxes();

extern int handle_menu(Menu *menu, float xshift, float yshift);
extern int handle_menu(Menu *menu);
extern void new_file(int decks, bool alive);
extern void draw_box(float *linec, float *areac, float x, float y, float wi, float he, float dx, float dy, float scale, float opacity, int circle, GLuint tex, float fw, float fh);
extern void draw_box(float *linec, float *areac, float x, float y, float wi, float he, GLuint tex);
extern void draw_box(float *color, float x, float y, float radius, int circle);
extern void draw_box(float *color, float x, float y, float radius, int circle, float fw, float fh);
extern void draw_box(Box *box, GLuint tex);
extern void draw_box(float *linec, float *areac, Box *box, GLuint tex);
extern void draw_box(Box *box, float opacity, GLuint tex);
extern void draw_box(Box *box, float dx, float dy, float scale, GLuint tex);

extern void draw_triangle(float *linec, float *areac, float x1, float y1, float xsize, float ysize, ORIENTATION orient);

extern void draw_line(float *linec, float x1, float y1, float x2, float y2);

extern float render_text(std::string text, float *textc, float x, float y, float sx, float sy);
extern float render_text(std::string text, float *textc, float x, float y, float sx, float sy, int smflag, bool vertical = false);

extern float xscrtovtx(float scrcoord);
extern float yscrtovtx(float scrcoord);

extern float pdistance(float x, float y, float x1, float y1, float x2, float y2);
extern void enddrag();

extern void open_binfiles();
extern void open_bindir();
extern void open_handlefile(const std::string &path);
extern void save_bin(const std::string &path);
extern void open_bin(const std::string &path);
extern Bin *new_bin(const std::string &name);
extern int read_binslist();
extern void save_binslist();
extern void make_currbin(int pos);
extern void get_texes(int deck);
extern void save_thumb(std::string path, GLuint tex);
extern void open_thumb(std::string path, GLuint tex);
extern void new_state();
extern BinElement *find_element(int size, int k, int i, int j, bool olc);
void concat_files(std::ostream &ofile, const std::string &path, std::vector<std::vector<std::string>> &jpegpaths);
bool check_version(const std::string &path);
std::string deconcat_files(const std::string &path);
extern GLuint copy_tex(GLuint tex);
extern GLuint copy_tex(GLuint tex, bool yflip);
extern GLuint copy_tex(GLuint tex, int tw, int th);
extern GLuint copy_tex(GLuint tex, int tw, int th, bool yflip);

void open_genmidis(std::string path);
void save_genmidis(std::string path);

bool thread_vidopen(Layer *lay, AVInputFormat *ifmt, bool skip);

void screenshot();

void calctexture(Layer *lay);

int open_codec_context(int *stream_idx, AVFormatContext *video, enum AVMediaType type);

void set_live_base(Layer *lay, std::string livename);

extern float tf(float vtxcoord);
extern bool exists(const std::string &name);
extern std::string dirname(std::string pathname);
extern std::string basename(std::string pathname);
extern std::string remove_extension(std::string filename);
extern std::string remove_version(std::string filename);
extern bool isimage(const std::string &path);

extern void onestepfrom(bool stage, Node *node, Node *prevnode, GLuint prevfbotex, GLuint prevfbo);

extern int osc_param(const char *path, const char *types, lo_arg **argv, int argc, lo_message m, void *data);