#include <string>
#include <vector>

class EWindow {
	public:
		SDL_Window *win;
		SDL_GLContext glc;
		GLuint vao;
		GLuint vbuf;
		GLuint tbuf;
		int mixid;
		bool closethread = false;
		std::mutex syncmutex;
		std::condition_variable sync;
		bool syncnow = false;
};

class Menu {
	public:
		std::string name;
		std::vector<std::string> entries;
		Box *box;
		int state = 0;
		int x, y;
		int value;
		int currsub = -1;
};