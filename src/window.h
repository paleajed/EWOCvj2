#include <string>
#include <vector>

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
		bool closethread = false;
		std::mutex syncmutex;
		std::condition_variable sync;
		bool syncnow = false;
		std::mutex syncendmutex;
		std::condition_variable syncend;
		bool syncendnow = false;
};

class Menu {
	public:
		std::string name;
		std::vector<std::string> entries;
		Box *box;
		int state = 0;
		int x, y;
		int menux;
		int menuy;
		float width = 0.156f;
		int value;
		int currsub = -1;
};