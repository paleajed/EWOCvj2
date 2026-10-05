#include <iostream>
#include <chrono>
#include <ctime>
#include <set>

class LoopStationElement;

// user-editable bezier curve that is sampled into a loopstation row
struct CurveKnot {
	float x = 0.0f, y = 0.0f;  // x in seconds, y in Param units
	int type = 0;              // 0 CONSTANT, 1 FLUID, 2 BROKEN
	float hinx = 0.0f, hiny = 0.0f;    // handle offsets relative to the knot (hinx <= 0)
	float houtx = 0.0f, houty = 0.0f;  // (houtx >= 0)
};

class LoopCurve {
	public:
		float totalsize = 1.0f;
		float rmin = 0.0f, rmax = 1.0f;  // Param range
		std::vector<CurveKnot> knots;    // sorted by x
		void init_default(float rangemin, float rangemax);
		void normalize();                // sort, pin end knots, clamp handles and values
		void set_type(int knotnr, int type);
		float eval(float t) const;
		void sample(std::vector<std::pair<long long, float>>& out) const;
		void set_totalsize(float ts);    // rescales knot x positions
};

// curve copied with the lpstmenu "Copy curve" entry
extern LoopCurve lpcurveclip;
extern bool lpcurveclipvalid;

class LoopStationElement;

class LoopCurveEditor {
	public:
		bool active = false;
		LoopStationElement* elem = nullptr;
		Param* target = nullptr;
		LoopCurve curve;
		int selknot = -1;
		int selhandle = 0;   // 0 none (the knot itself), 1 in-handle, 2 out-handle
		int dragging = 0;    // 0 no, 1 knot, 2 handle
		int curtype = 0;
		Param* totalsize = nullptr;
		Param* knotx = nullptr;
		Param* knoty = nullptr;
		Button* typebut[3] = {nullptr, nullptr, nullptr};
		Button* applybut = nullptr;
		Boxx* plot = nullptr;
		Boxx* area = nullptr;
		bool prevdown = false;
		bool pressedonitem = false;
		bool dragended = false;
		int skipframes = 0;
		float px0 = 0.0f, py0 = 0.0f, pw = 1.0f, ph = 1.0f;  // data region inside the plot box (vertex coords)
		float oldtotalsize = 1.0f;
		float oldknotx = 0.0f;
		float oldknoty = 0.0f;
		int oldsel = -2;
		Button* targetbut = nullptr;  // when editing a Button curve (target is then nullptr)
		// all Params / Buttons the curve drives (one or several, a line can steer all its recorded elements)
		std::vector<Param*> tpars;
		std::vector<Button*> tbuts;
		bool multi() const { return tpars.size() + tbuts.size() > 1; }
		void open(Param* par, Button* but = nullptr);
		void open_row(LoopStationElement* e);  // edit the curve of a whole line, or turn a recorded line into a curve line
		void handle();
		void apply();
		size_t lastsig = 0;
		size_t signature() const;
		Boxx* loopsw = nullptr;      // loopbut switch of the edited row
		int initialloop = 0;         // loopbut state when the editor was opened
		bool hadcurve = false;       // row already held a curve of this param when opened
		bool touched = false;        // row was (re)filled while the editor was open
		LoopCurve backup;            // curve at open time, restored on cancel
		bool hadrecording = false;   // the row held a normally recorded line (overwritten on apply, restored on cancel)
		std::vector<std::tuple<long long, Param*, Button*, float>> bu_events;
		std::unordered_set<Param*> bu_params;
		std::unordered_set<Button*> bu_buttons;
		std::set<Layer*> bu_layers;
		float bu_totaltime = 0.0f;
		float bu_speed = 1.0f;
		float bu_beats = 0.0f;
		// running state of the recorded line at open time (it keeps running while the curve is not being tested)
		int bu_loop = 0, bu_play = 0;
		std::chrono::high_resolution_clock::time_point bu_start;
		float bu_interim = 0.0f, bu_spadt = 0.0f;
		bool testing = false;        // the curve (not the recorded line) is running on the row
		void restore_recording();    // put the recorded line back, running as it was
		void cancel();
		// while a knot / handle is dragged the rest of the program must not see the mouse:
		// start.cpp hides it at the start of the frame and restores it before the events are polled
		bool hid = false;
		int rmx = 0, rmy = 0;
		bool rdown = false, rleft = false;
		LoopCurveEditor();
	private:
		void init_widgets();
		void begin(LoopStationElement* e, bool hadrec, float rmin, float rmax);  // shared start of open() / open_row()
		float tox(float t) const;
		float toy(float v) const;
		float fromx(float vx) const;
		float fromy(float vy) const;
		void set_current_type(int type);
		void draw_curve();
};
extern LoopCurveEditor* lpcurveeditor();
// Param menu helpers for the curve entries
// (a Param or a Button is passed, the other one nullptr)
// the single Param / Button a line can take a curve for: the one of its curve, or the only one a recorded line automates
extern bool lpst_curve_target(LoopStationElement* e, Param*& par, Button*& but);
extern bool lpst_has_targets(LoopStationElement* e);  // the line automates at least one Param / Button
extern void lpst_paste_curve(LoopStationElement* e);  // the copied curve drives all elements of the line
extern bool button_curvable(Button* but);  // false for the loopstation line buttons and buttons the loopstation can not automate
extern bool target_has_curve(Param* par, Button* but);
extern void target_copy_curve(Param* par, Button* but);
extern void target_paste_curve(Param* par, Button* but);

class LoopStation {
	public:
		std::vector<LoopStationElement*> elements;
		std::mutex elements_mutex;  // Protects elements vector from concurrent access by audio thread
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
		std::vector<float> colvals = {0.7f, 0.2f, 0.2f
									, 0.4f, 0.6f, 0.4f
									, 0.3f, 0.3f, 0.7f
									, 0.7f, 0.7f, 0.3f
									, 0.7f, 0.3f, 0.7f
									, 0.3f, 0.7f, 0.7f
									, 0.7f, 0.4f, 0.3f
									, 0.3f, 0.1f, 0.06f
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
        void remove_entries(int copycomp, bool deck);
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
		LoopCurve* curve = nullptr;  // curve definition when this row was filled by the curve editor
		Param* curvepar = nullptr;
		Button* curvebut = nullptr;  // button curves: on for the top half of the curve, off for the bottom half
		// fills this row with the curve for either a Param or a Button (the other one nullptr)
		void apply_curve(Param* par, Button* but, const LoopCurve& lc);
		// the curve drives all of these: Params follow the curve scaled to their own range, Buttons are on in the top half
		void apply_curve(const std::vector<Param*>& pars, const std::vector<Button*>& buts, const LoopCurve& lc);
		Param* curve_param();  // the Param this row's curve drives (re-adopted when the Param object was replaced)
		Button* curve_button();  // idem for a Button
		bool curve_is_for(Param* par, Button* but);  // the row holds a curve driving exactly this Param / Button
		bool automates(Param* par, Button* but);
		void clear_curve();
		int eventpos = 0;
		bool atend = false;
		bool didsomething = false;
        float beats = 0;
        float buspeed = 1.0f;
		void init();
		void handle();
		void erase_elem();
        void add_param_automationentry(Param* par);
        void add_param_automationentry(Param* par, long long mc);
		void add_button_automationentry(Button* but);
		void set_values();
        void get_state_from(LoopStationElement* loop);
        LoopStationElement();
		~LoopStationElement();
		
	private:
		void visualize();
		void mouse_handle();
};