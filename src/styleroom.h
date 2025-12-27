typedef enum
{
    SET_DELETE = 0,
    SET_OPENFILES = 1,
    SET_LOADMIX = 2,
    SET_INSDECKA = 3,
    SET_INSDECKB = 4,
    SET_INSMIX = 5,
    SET_QUIT = 6,
    SET_SAVPROJ = 7,
	SET_UPSCALEIMAGE = 8,
	SET_OPENSTYLE = 9,
    SET_SAVESTYLE = 10,
    SET_RENAMESTYLE = 10,
    SET_DELETESTYLE = 10,
} ELEMMENU_OPTION;

class StylePreparationElement;
class StylePreparationBin;
class Style;
class ReCoNetTrainer;

class StyleRoom {
    public:
        std::vector<Style*> styles;
        Style* currstyle = nullptr;
        Style* menustyle = nullptr;
        std::string backupname;
        StylePreparationBin* prepbin;
        std::vector<Boxx*> elemboxes;
        std::string currstylename = "Style_0";
		Boxx* stylenamesbox;
		int stylesscroll = 0;
        Boxx* stylesscrolldown;
        Boxx* stylesscrollup;
        Param* quality = nullptr;
		Param* influence = nullptr;
		Param* abstraction = nullptr;
		Param* res = nullptr;
		Param* coherence = nullptr;
		Param* usegpu = nullptr;
        bool advanced = false;
        Param* mode = nullptr;
		Param* layrelu1 = nullptr;
		Param* layrelu2 = nullptr;
		Param* layrelu3 = nullptr;
		Param* layrelu4 = nullptr;
		Param* layrelu5 = nullptr;
		Param* contentweight = nullptr;
		Param* styleweight = nullptr;
		Param* temporalweight = nullptr;
		Param* trainres = nullptr;
		Param* trainiter = nullptr;
		Param* batchsize = nullptr;
		int reswidth = 1280;
        int resheight = 720;
        Menu* styleroommenu;
        StylePreparationElement* menuelem;
		std::vector<ELEMMENU_OPTION> elemmenuoptions;
        bool openfilesstyle = false;
        ReCoNetTrainer* reconetTrainer = nullptr;

        void handle();
        void open_files_bin();
        void updatelists();
        StyleRoom();
};

class Style {
    public:
        StylePreparationBin* bin = nullptr;
        std::string name = "";
        std::string oldname = "";
        std::string abspath = "";
        std::string relpath = "";
        Boxx* box = nullptr;
};

class StylePreparationBin {
    public:
        std::vector<StylePreparationElement*> elements;

		void open(std::string path);
		void save();
        StylePreparationBin();
};

class StylePreparationElement {
    public:
        StylePreparationBin* bin = nullptr;
        ELEM_TYPE type = ELEM_FILE;
        int pos = -1;
        std::string name = "";
        std::string abspath = "";
        std::string relpath = "";
        GLuint tex = -1;
		int filesize = 0;

        // Async upscaling support
        std::unique_ptr<BinElement> upscaleBinel;

        void erase();
        void check_upscale_complete();
        StylePreparationElement();
        ~StylePreparationElement();
};