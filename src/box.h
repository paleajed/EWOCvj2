#include <string>
#include <chrono>


typedef enum
{
	RIGHT = 0,
    LEFT = 1,
    UP = 2,
    DOWN = 3,
} ORIENTATION;

typedef enum
{
	OPEN = 0,
    CLOSED = 1,
} TRIANGLE_TYPE;

typedef struct BoxCoords{
    float x1 = 0.0f;
    float y1 = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
} BOX_COORDS;

class Layer;

class Boxx {
	public:
		float lcolor[4] = {0.7, 0.7, 0.7, 1.0};
		float acolor[4] = {0.0, 0.0, 0.0, 0.0};
        BOX_COORDS *scrcoords;
		BOX_COORDS *vtxcoords;
		GLuint tex = -1;

		std::string tooltiptitle = "";
		std::string tooltip = "";
		bool reserved = false;
		int smflag = 0;

		void upscrtovtx();
		void upvtxtoscr();
		bool in();
		bool in(int mx, int my);
		bool in(bool menu);
		Boxx* copy();
		Boxx();
		~Boxx();
};

class Button {
	public:
		std::string name[2];
		Boxx *box;
        int butid = 0;
		int value = 0;
		int oldvalue = 0;
		int toggle = 0;
        bool skiponetoggled = false;
		float tcol[4] = {0.3f, 0.8f, 0.4f, 1.0f};
		float ccol[4] = {1.0f, 0.0f, 0.0f, 1.0f};
		int midi[2] = {-1, -1};
		std::string midiport;
		Layer* layer = nullptr;
        std::chrono::system_clock::time_point midistarttime;

		bool toggled();
		void deautomate();
        void register_midi();
        void unregister_midi();
		Button(bool state);
		~Button();
};

