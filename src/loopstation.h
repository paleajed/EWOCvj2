#include <iostream>
#include <chrono>
#include <ctime>


class LoopStationElement;

class LoopStation {
	public:
		std::vector<LoopStationElement*> elems;
		int numelems = 256;
		std::vector<LoopStationElement*> readelems;
		std::vector<int> readelemnrs;
		std::vector<Param*> allparams;
		std::vector<Button*> allbuttons;
		std::unordered_map<Param*, Param*> parmap;
		std::unordered_map<Button*, Button*> butmap;
		std::unordered_map<Param*, LoopStationElement*> parelemmap;
		std::unordered_map<Button*, LoopStationElement*> butelemmap;
		std::vector<float> colvals = {0.8f, 0.3f, 0.3f
									, 0.3f, 0.4f, 0.3f
									, 0.3f, 0.3f, 0.7f
									, 0.7f, 0.7f, 0.3f
									, 0.7f, 0.3f, 0.7f
									, 0.3f, 0.7f, 0.7f
									, 0.7f, 0.4f, 0.3f
									, 0.4f, 0.4f, 0.3f
									, 0.7f, 0.3f, 0.4f
									, 0.4f, 0.3f, 0.7f
									, 0.3f, 0.7f, 0.4f
									, 0.3f, 0.4f, 0.7f
									, 0.4f, 0.3f, 0.3f
									, 0.3f, 0.4f, 0.3f
									, 0.3f, 0.3f, 0.4f
									, 0.3f, 0.2f, 0.5f
									, 0.3f, 0.4f, 0.4f};
        Boxx *upscrbox;
        Boxx *downscrbox;
        Boxx *confupscrbox;
        Boxx *confdownscrbox;
        int scrpos = 0;
        int confscrpos = 0;
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
        int comparepos = -1;
		std::unordered_set<Param*> params;
		std::unordered_set<Button*> buttons;
        std::unordered_set<Layer*> layers;
        std::vector<int> effcatposns;  // for shelf triggering nblayers
        std::vector<int> effposns;  // for shelf triggering nblayers
        std::vector<int> parposns;  // for shelf triggering nblayers
        std::vector<int> buteffcatposns;  // for shelf triggering nblayers
        std::vector<int> buteffposns;  // for shelf triggering nblayers
        std::vector<int> butposns;  // for shelf triggering nblayers
        std::vector<int> compareelems;  // for shelf triggering nblayers
        LoopStation* lpst;
		Button *recbut;
		Button *loopbut;
		Button *playbut;
		Boxx *colbox;
		Boxx *box;
		std::chrono::high_resolution_clock::time_point starttime;
        float interimtime = 0;
        float totaltime = 0;
		float speedadaptedtime = 0;
		Param *speed;
		std::vector<std::tuple<long long, Param*, Button*, float>> eventlist;
		int eventpos = 0;
		bool atend = false;
		bool didsomething = false;	
		void init();
		void handle();
		void erase_elem();
        void add_param_automationentry(Param* par);
        void add_param_automationentry(Param* par, long long mc);
		void add_button_automationentry(Button* but);
		void set_values();
		LoopStationElement();
		~LoopStationElement();
		
	private:
		void visualize();
		void mouse_handle();
};