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
		LoopStationElement* add_elem();
		LoopStationElement* free_element();
		handle();
		LoopStation();
		
	private:
		setbut(Button *but, float r, float g, float b);
};

class LoopStationElement {
	public:
		int pos = 0;
		std::vector<Param*> params;
		std::vector<Layer*> layers;
		Button *recbut;
		Button *loopbut;
		Button *playbut;
		Box *colbox;
		std::chrono::high_resolution_clock::time_point starttime;
		long long interimtime = 0;
		long long speedadaptedtime = 0;
		Param *speed;
		std::vector<std::tuple<long long, Param*, float>> eventlist;
		int eventpos = 0;
		bool didsomething = false;	
		init();
		handle();
		add_param();
		LoopStationElement();
		~LoopStationElement();
		
	private:
		visualize();
		mouse_handle();
		set_params();
};