#include <string>
#include <vector>
#ifdef POSIX
#include "GL/glx.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <Xos_fixindexmacro.h>
#endif

class EWindow {
	public:
		SDL_Window *win;
		SDL_GLContext glc;
		int w;
		int h;
		GLuint vao;
		GLuint vbuf;
		GLuint tbuf;
        int mixid;
        Layer *lay = nullptr;
		bool closethread = false;
		std::mutex syncmutex;
		std::condition_variable sync;
		bool syncnow = false;
		std::mutex syncendmutex;
		std::condition_variable syncend;
		bool syncendnow = false;
        #ifdef POSIX
		Display *dpy;
		GLXContext ctx;
        #endif

		~EWindow();
};

class Menu {
	public:
		std::string name;
		std::vector<std::string> entries;
		Boxx *box;
		int state = 0;
		int x, y;
		int menux;
		int menuy;
		float width = 0.156f;
		int value;
		int currsub = -1;

		// Split positioning for wide submenus
		int splitColumnsOnLeft = -1;  // -1 means no split, >= 0 means split mode
		float splitGapWidth = 0.0f;   // Width of gap (main menu) to skip over
		int splitScrollOffset = 0;    // Column scroll offset for oversized split submenus
		bool splitNeedsScrolling = false;  // True if submenu is too wide even when split

        ~Menu();
};