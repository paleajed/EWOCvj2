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

class Box {
	public:
		float lcolor[4] = {1.0, 1.0, 1.0, 1.0};
		float acolor[4] = {0.0, 0.0, 0.0, 0.0};
		void set_vtxcoords(BOX_COORDS *vtxcoords);
		BOX_COORDS *scrcoords;
		BOX_COORDS *vtxcoords;
		GLuint tex = -1;
		void upscrtovtx();
		void upvtxtoscr();
		bool in();
		bool in(bool menu);
		Box();
		~Box();
};