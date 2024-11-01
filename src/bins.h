typedef enum
{
	BET_DELETE = 0,
	BET_RENAME = 1,
	BET_OPENFILES = 2,
	BET_LOADMIX = 3,
	BET_LOADDECKA = 4,
	BET_LOADDECKB = 5,
	BET_INSDECKA = 6,
	BET_INSDECKB = 7,
	BET_INSMIX = 8,
	BET_LOADSHELFA = 9,
	BET_LOADSHELFB = 10,
	BET_HAPELEM = 11,
	BET_HAPBIN = 12,
    BET_QUIT = 13,
    BET_SAVPROJ = 14,
    BET_MOVSEL = 15,
    BET_DELSEL = 16,
    BET_HAPSEL = 17,
} BINELMENU_OPTION;

class Bin;
class BinElement;

class BinsMain {
	public:
		std::vector<Bin*> bins;
        Bin *currbin;
        std::vector<std::tuple<Bin*, std::string>> undobins;
        int undopos = 0;
		std::vector<Boxx*> elemboxes;
		BinElement *currbinel = nullptr;
		BinElement *prevbinel = nullptr;
		std::vector<BINELMENU_OPTION> binelmenuoptions;
        int previ;
        int prevj;
        int firsti;
        int firstj;
        Boxx* loadbinbox;
        Boxx* savebinbox;
		Boxx* newbinbox;
		Boxx* renamingbox;
		GLuint binelpreviewtex;
		bool binpreview = false;
		std::string newpath;
		std::string binpath;
		std::vector<std::string> addpaths;
		std::string tempjpegpath;
		GLuint movingtex = -1;
		std::vector<GLuint> inputtexes;
		std::vector<ELEM_TYPE> inputtypes;
		std::vector<std::string> inputjpegpaths;
		std::vector<int> inputwidths;
		std::vector<int> inputheights;
		std::vector<GLuint> inserttexes[2];
		std::vector<ELEM_TYPE> inserttypes[2];
		std::vector<std::string> insertpaths[2];
		std::vector<std::string> insertjpegpaths[2];
		std::string previewimage = "";
		Shelf* insertshelf = nullptr;
		BinElement* previewbinel = nullptr;
		BinElement* movingbinel = nullptr;
		BinElement *backupbinel = nullptr;
		BinElement* menubinel = nullptr;
		BinElement* menuactbinel = nullptr;
        std::unordered_set<std::string> removeset[2];
		std::vector<BinElement*> delbinels;
		std::vector<BinElement*> movebinels;
		Bin* dragbin = nullptr;
		int dragbinpos = -1;
		bool dragbinsense = false;
		bool indragbox = false;
		Boxx* dragbox;
		int binsscroll = 0;
		Boxx* binsscrolldown;
        Boxx* binsscrollup;

        Boxx* floatbox;
        bool floating = false;
        bool floatingsync = false;
        bool floatset = false;
        bool inbin = false;
        bool inbinwin = false;
        SDL_Window *win;
        SDL_GLContext glc;
        int screen;
        float globw;
        float globh;
        std::mutex syncmutex;
        std::condition_variable sync;
        bool syncnow = false;
        std::mutex syncendmutex;
        std::condition_variable syncend;
        bool syncendnow = false;

        Bin *menubin = nullptr;
		bool openfilesbin = false;
		bool importbins = false;
		int binscount;
		bool movingstruct = false;
		std::vector<BinElement*> prevelems;
		int mouseshelfnum = -1;
		int oldmouseshelfnum = -1;
		bool selboxing = false;
		int selboxx;
		int selboxy;
		Boxx *hapmodebox;
		BinElement *renamingelem = nullptr;
        std::vector<std::string> newbinelpaths;
        std::vector<char*> messages;
        std::vector<char*> rawmessages;
        std::vector<std::string> messagesocknames;
        std::vector<int> messagelengths;


        void handle(bool draw);
		int read_binslist();
		void save_binslist();
		void make_currbin(int pos);
		Bin *new_bin(std::string name);
		void open_bin(std::string path, Bin *bin, bool newbin = false);
		void save_bin(std::string path);
		void import_bins();
		void open_files_bin();
		void open_handlefile(std::string path, GLuint tex = -1);
		std::tuple<std::string, std::string> hap_binel(BinElement *binel, BinElement* bdm);
		void hap_deck(BinElement * bd);
		void hap_mix(BinElement * bm);
		void hap_encode(std::string srcpath, BinElement* binel, BinElement* bdm);
        void undo_redo(char offset);
        BinsMain();

	private:
		void do_save_bin(std::string path);
};
		
class Bin {
	public:
		std::string name = "";
		std::string path = "";
		std::vector<BinElement*> elements;
        std::vector<int> open_positions;
        std::vector<std::string> bujpegpaths;
		int encthreads = 0;
		int pos;
		bool shared = false;
        bool saved = false;
		std::vector<std::string> sendtonames;
		Bin(int pos);
		~Bin();
		
		Boxx *box;
};

class BinElement {
	public:
		Bin *bin;
		ELEM_TYPE type = ELEM_FILE;
		ELEM_TYPE oldtype = ELEM_FILE;
		std::string name = "";
		std::string oldname = "";
		std::string path = "";
        std::string oldpath = "";
        std::string relpath = "";
        std::string absjpath = "";
        std::string reljpath = "";
		std::string jpegpath = "";
		std::string oldjpegpath = "";
        bool jpegsaved = false;
        bool autosavejpegsaved = false;
        long long filesize = 0;
		GLuint tex;
		GLuint oldtex;
		bool full = false;
		bool oldfull = false;
		bool select = false;
		bool oldselect = false;
		bool boxselect = false;
        bool temp = false;
		bool encwaiting = false;
		bool encoding = false;
		int encthreads = 0;
		float encodeprogress = 0.0f;
		int allhaps = 0;
		Layer *otflay = nullptr;
		void erase(bool deletetex = true);
        void remove_elem(bool quit);
        BinElement();
		~BinElement();
};