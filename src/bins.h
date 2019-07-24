class Bin;
class BinElement;

class BinsMain {
	public:
		std::vector<Bin*> bins;
		Bin *currbin;
		std::vector<Box*> elemboxes;
		BinElement *currbinel = nullptr;
		BinElement *prevbinel = nullptr;
		int previ;
		int prevj;
		Box* newbinbox;
		Box* renamingbox;
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
		std::vector<std::string> inputjpegpaths;
		std::vector<int> inputwidths;
		std::vector<int> inputheights;
		std::vector<GLuint> inserttexes[2];
		std::vector<ELEM_TYPE> inserttypes[2];
		std::vector<std::string> insertpaths[2];
		std::vector<std::string> insertjpegpaths[2];
		std::string previewimage = "";
		BinElement* previewbinel = nullptr;
		BinElement* movingbinel = nullptr;
		BinElement *backupbinel = nullptr;
		BinElement* menubinel = nullptr;
		std::vector<BinElement*> delbinels;
		Bin* dragbin = nullptr;
		int dragbinpos = -1;
		bool dragbinsense = false;
		bool indragbox = false;
		Box* dragbox;
		int binsscroll = 0;
		Box* binsscrolldown;
		Box* binsscrollup;
		Bin *menubin = nullptr;
		bool openbinfile = false;
		bool movingstruct = false;
		std::vector<BinElement*> prevelems;
		int mouseshelfnum = -1;
		bool selboxing = false;
		int selboxx;
		int selboxy;
		Box *hapmodebox;
		BinElement *renamingelem = nullptr;
		
		void handle(bool draw);
		int read_binslist();
		void save_binslist();
		void make_currbin(int pos);
		Bin *new_bin(const std::string &name);
		void open_bin(const std::string &path, Bin *bin);
		void save_bin(const std::string &path);
		void open_binfiles();
		void open_handlefile(const std::string &path);
		std::tuple<std::string, std::string> hap_binel(BinElement *binel, BinElement* bdm);
		void hap_deck(BinElement * bd);
		void hap_mix(BinElement * bm);
		void hap_encode(const std::string srcpath, BinElement* binel, BinElement* bdm);
		BinsMain();

	private:
		void do_save_bin(const std::string& path);
};
		
class Bin {
	public:
		std::string name;
		std::vector<BinElement*> elements;
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
		std::string name = "";
		std::string oldname = "";
		std::string path = "";
		std::string oldpath = "";
		std::string jpegpath = "";
		std::string oldjpegpath = "";
		int vidw;
		int vidh;
		GLuint tex;
		GLuint oldtex;
		bool full = false;
		bool select = false;
		bool encwaiting = false;
		bool encoding = false;
		int encthreads;
		float encodeprogress;
		int allhaps = 0;
		BinElement();
		~BinElement();
		BinElement* next();
};