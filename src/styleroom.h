class StylePreparationElement;
class StylePreparationBin;
class Style;

class StyleRoom {
    public:
        std::vector<Style*> styles;
        StylePreparationBin* prepbin;
        std::vector<Boxx*> elemboxes;
        int reswidth = 1280;
        int resheight = 720;
        Menu* stylepreparationmenu;
        StylePreparationElement* menuelem;

        void handle();
        StyleRoom();
};

class Style {
    public:
        StylePreparationBin* bin;
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
        StylePreparationBin* bin;
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