class StylePreparationElement;
class StylePreparationBin;
class Style;
class ReCoNetTrainer;

class StyleRoom {
    public:
        std::vector<Style*> styles;
        StylePreparationBin* prepbin;
        std::vector<Boxx*> elemboxes;
        Param* res = nullptr;
        Param* quality = nullptr;
        Param* usegpu = nullptr;
        int reswidth = 1280;
        int resheight = 720;
        Menu* stylepreparationmenu;
        StylePreparationElement* menuelem;
        ReCoNetTrainer* reconetTrainer = nullptr;

        void handle();
        StyleRoom();
};

class Style {
    public:
        StylePreparationBin* bin = nullptr;
        std::string name = "";
        std::string abspath = "";
        std::string relpath = "";
};

class StylePreparationBin {
    public:
        std::vector<StylePreparationElement*> elements;

        StylePreparationBin();
};

class StylePreparationElement {
    public:
        StylePreparationBin* bin = nullptr;
        ELEM_TYPE type = ELEM_FILE;
        std::string name = "";
        std::string abspath = "";
        std::string relpath = "";
        std::string absjpath = "";
        std::string reljpath = "";
        GLuint tex = -1;

        void erase();
        StylePreparationElement();
};