#include <iostream>
#include <chrono>
#include <ctime>


class LoopStationElement;

class LoopStation {
	public:
		std::vector<LoopStationElement*> elems;
		int numelems = 8;
		std::vector<LoopStationElement*> readelems;
		std::vector<int> readelemnrs;
		std::vector<Param*> allparams;
		std::unordered_map<Param*, Param*> parmap;
		std::unordered_map<Param*, LoopStationElement*> elemmap;
		std::vector<float> colvals = {1.0f, 0.0f, 0.0f
									, 0.0f, 0.5f, 0.0f
									, 0.0f, 0.0f, 1.0f
									, 1.0f, 1.0f, 0.0f
									, 1.0f, 0.0f, 1.0f
									, 0.0f, 1.0f, 1.0f
									, 1.0f, 0.5f, 0.0f
									, 0.5f, 0.5f, 0.0f
									, 1.0f, 0.0f, 0.5f
									, 0.5f, 0.0f, 1.0f
									, 0.0f, 1.0f, 0.5f
									, 0.0f, 0.5f, 1.0f
									, 0.5f, 0.0f, 0.0f
									, 0.0f, 0.5f, 0.0f
									, 0.0f, 0.0f, 0.5f
									, 0.5f, 0.0f, 0.5f
									, 0.0f, 0.5f, 0.5f};
		LoopStationElement* currelem;
		LoopStationElement* add_elem();
		LoopStationElement* free_element();
		void init();
		void handle();
		LoopStation();
		
	private:
		void setbut(Button *but, float r, float g, float b);
};

class LoopStationElement {
	public:
		int pos = 0;
		std::unordered_set<Param*> params;
		std::unordered_set<Layer*> layers;
		Button *recbut;
		Button *loopbut;
		Button *playbut;
		Box *colbox;
		Box *box;
		std::chrono::high_resolution_clock::time_point starttime;
		long long interimtime = 0;
		long long speedadaptedtime = 0;
		Param *speed;
		std::vector<std::tuple<long long, Param*, float>> eventlist;
		int eventpos = 0;
		bool didsomething = false;	
		void init();
		void handle();
		void erase_elem();
		void add_param();
		void set_params();
		LoopStationElement();
		~LoopStationElement();
		
	private:
		void visualize();
		void mouse_handle();
};