#include <string>


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

typedef struct {
	float x1, y1, w, h;
} BOX_COORDS;

class Layer;

class Box {
	public:
		float lcolor[4] = {1.0, 1.0, 1.0, 1.0};
		float acolor[4] = {0.0, 0.0, 0.0, 0.0};
		void set_vtxcoords(BOX_COORDS *vtxcoords);
		BOX_COORDS *scrcoords;
		BOX_COORDS *vtxcoords;
		GLuint tex = -1;

		std::string tooltiptitle = "";
		std::string tooltip = "";
		bool reserved = false;

		void upscrtovtx();
		void upvtxtoscr();
		bool in();
		bool in(int mx, int my);
		bool in(bool menu);
		bool in2();
		bool in2(int mx, int my);
		Box();
		~Box();
};

class Button {
	public:
		std::string name[2];
		Box *box;
		int value = 0;
		int oldvalue = 0;
		int toggle = 0;
		float tcol[4] = {0.0f, 0.7f, 0.0f, 1.0f};
		float ccol[4] = {1.0f, 0.0f, 0.0f, 1.0f};
		int midi[2] = {-1, -1};
		std::string midiport;
		Layer* layer = nullptr;
		bool handle(bool circlein = false);
		bool handle(bool circlein, bool automation);
		bool toggled();
		void deautomate();
		Button(bool state);
		~Button();
};

