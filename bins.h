class Bin;
class BinElement;
class BinDeck;
class BinMix;

class BinsMain {
	public:
		std::vector<Bin*> bins;
		Bin *currbin;
		BinElement *currbinel = nullptr;
		BinElement *prevbinel = nullptr;
		int previ;
		int prevj;
		Box *newbinbox;
		GLuint binelpreviewtex;
		bool binpreview = false;
		std::vector<Layer*> templayers;
		std::string newpath;
		std::string binpath;
		std::vector<std::string> newpaths;
		GLuint movingtex = -1;
		GLuint dragtex = -1;
		std::vector<GLuint> inputtexes;
		std::vector<int> inputwidths;
		std::vector<int> inputheights;
		std::vector<GLuint> dragtexes[2];
		std::vector<GLuint> inserttexes[2];
		std::vector<ELEM_TYPE> inserttypes[2];
		std::vector<std::string> insertpaths[2];
		std::vector<std::string> insertjpegpaths[2];
		BinElement *movingbinel = nullptr;
		BinElement *backupbinel = nullptr;
		BinElement *inputbinel = nullptr;
		BinElement *menubinel = nullptr;
		BinDeck *dragdeck = nullptr;
		BinMix *dragmix = nullptr;
		Bin *menubin = nullptr;
		bool openbindir = false;
		bool openbinfile = false;
		int inserting = -1;
		bool movingstruct = false;
		BinDeck *movingdeck;
		BinMix *movingmix;		
		
		handle();
		int read_binslist();
		save_binslist();
		make_currbin(int pos);
		Bin *new_bin(const std::string &name);
		open_bin(const std::string &path, Bin *bin);
		save_bin(const std::string &path);
		open_binfiles();
		open_bindir();
		open_handlefile(const std::string &path);
		get_texes(int deck);
		std::tuple<std::string, std::string> hap_binel(BinElement *binel, BinDeck *bd, BinMix *bm);
		hap_deck(BinDeck * bd);
		hap_mix(BinMix * bm);
		hap_encode(const std::string &srcpath, BinElement *binel, BinDeck *bd, BinMix *bm);
};
		
class Bin {
	public:
		std::string name;
		std::vector<BinElement*> elements;
		std::vector<BinDeck*> decks;
		std::vector<BinMix*> mixes;
		int encthreads;
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
};

class BinDeck {
	public:
		std::string path;
		int i, j;
		int height;
		int encthreads = 0;
		Box *box;
		BinDeck();
		~BinDeck();
};

class BinMix {
	public:
		std::string path;
		int j;
		int height;
		int encthreads = 0;
		Box *box;
		BinMix();
		~BinMix();
};

