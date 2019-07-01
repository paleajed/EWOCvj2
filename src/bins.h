class Bin;
class BinElement;
class BinDeck;
class BinMix;

class BinsMain {
	public:
		std::vector<Bin*> bins;
		Bin *currbin;
		std::vector<Box*> elemboxes;
		BinElement *currbinel = nullptr;
		BinElement *prevbinel = nullptr;
		int previ;
		int prevj;
		Box *newbinbox;
		GLuint binelpreviewtex;
		bool binpreview = false;
		std::string newpath;
		std::string binpath;
		std::vector<std::string> newpaths;
		std::string tempjpegpath;
		GLuint movingtex = -1;
		GLuint dragtex = -1;
		std::vector<GLuint> inputtexes;
		std::vector<ELEM_TYPE> inputtypes;
		std::vector<int> inputwidths;
		std::vector<int> inputheights;
		std::vector<GLuint> dragtexes[2];
		std::vector<GLuint> inserttexes[2];
		std::vector<ELEM_TYPE> inserttypes[2];
		std::vector<std::string> insertpaths[2];
		std::vector<std::string> insertjpegpaths[2];
		BinElement *movingbinel = nullptr;
		BinElement *backupbinel = nullptr;
		BinElement *menubinel = nullptr;
		BinDeck *dragdeck = nullptr;
		BinMix *dragmix = nullptr;
		Bin *menubin = nullptr;
		bool openbindir = false;
		bool openbinfile = false;
		int inserting = -1;
		bool movingstruct = false;
		BinDeck *movingdeck = nullptr;
		BinMix *movingmix = nullptr;	
		std::vector<BinElement*> prevelems;
		Box *hapmodebox;
		
		void handle(bool draw);
		int read_binslist();
		void save_binslist();
		void make_currbin(int pos);
		Bin *new_bin(const std::string &name);
		void open_bin(const std::string &path, Bin *bin);
		void save_bin(const std::string &path);
		void open_binfiles();
		void open_bindir();
		void open_handlefile(const std::string &path);
		void get_texes(int deck);
		std::tuple<std::string, std::string> hap_binel(BinElement *binel, BinDeck *bd, BinMix *bm);
		void hap_deck(BinDeck * bd);
		void hap_mix(BinMix * bm);
		void hap_encode(const std::string srcpath, BinElement *binel, BinDeck *bd, BinMix *bm);
		BinsMain();
};
		
class Bin {
	public:
		std::string name;
		std::vector<BinElement*> elements;
		std::vector<BinDeck*> decks;
		std::vector<BinMix*> mixes;
		int encthreads = 0;
		int pos;
		Bin(int pos);
		~Bin();
		
		Box *box;
};

class BinElement {
	public:
		Bin *bin;
		ELEM_TYPE type = ELEM_FILE;
		ELEM_TYPE oldtype = ELEM_FILE;
		std::string path = "";
		std::string oldpath = "";
		std::string jpegpath = "";
		std::string oldjpegpath = "";
		int vidw;
		int vidh;
		GLuint tex;
		GLuint oldtex;
		bool full = false;
		bool encwaiting = false;
		bool encoding = false;
		float encodeprogress;
		BinElement();
		~BinElement();
		BinElement* next();
};

class BinDeck {
	public:
		std::string path;
		std::string jpegpath;
		int i, j;
		int height;
		int encthreads = 0;
		GLuint tex;
		Box *box;
		BinDeck();
		~BinDeck();
};

class BinMix {
	public:
		std::string path;
		std::string jpegpath;
		int j;
		int height;
		int encthreads = 0;
		GLuint tex;
		Box *box;
		BinMix();
		~BinMix();
};

