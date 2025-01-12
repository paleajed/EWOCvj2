#include <iostream>
#include <chrono>
#include <ctime>
#include <set>

class LoopStationElement;

class LoopStation {
	public:
		std::vector<LoopStationElement*> elements;
		int numelems = 256;
		std::vector<LoopStationElement*> readelems;
		std::vector<int> readelemnrs;
        std::unordered_map<int, int> readmap;
		std::unordered_set<Param*> allparams;
		std::unordered_set<Button*> allbuttons;
		std::unordered_map<Param*, Param*> parmap;
		std::unordered_map<Button*, Button*> butmap;
		std::unordered_map<Param*, LoopStationElement*> parelemmap;
		std::unordered_map<Button*, LoopStationElement*> butelemmap;
        std::unordered_set<LoopStationElement*> odelems;
		std::vector<float> colvals = {0.8f, 0.3f, 0.3f
									, 0.4f, 0.6f, 0.4f
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
        bool foundrec = false;
		LoopStationElement* currelem;
        std::chrono::high_resolution_clock::time_point bunow;
		LoopStationElement* add_elem();
		LoopStationElement* free_element();
		void init();
		void handle();
        void remove_entries(int copycomp);
		LoopStation();
        ~LoopStation();
		
	private:
		static void setbut(Button *but, float r, float g, float b);
};

class LoopStationElement {
	public:
		int pos = 0;
        int comparepos = -1;
		std::unordered_set<Param*> params;
		std::unordered_set<Button*> buttons;
        std::set<Layer*> layers;
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
        Param *scritch;
        int scritching = 0;
        bool midiscritch = false;
		std::chrono::high_resolution_clock::time_point starttime;
        float totaltime = 0;
        float interimtime = 0;
        float speedadaptedtime = 0;
        float buinterimtime = 0;
        float buspeedadaptedtime = 0;
		Param *speed;
		std::vector<std::tuple<long long, Param*, Button*, float>> eventlist;
		int eventpos = 0;
		bool atend = false;
		bool didsomething = false;
        char beats = 0;
        float buspeed = 1.0f;
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