#ifndef USE_GLES
#include "GL/glew.h"
#include "GL/gl.h"
#define FREEGLUT_STATIC
#define _LIB
#define FREEGLUT_LIB_PRAGMAS 0
#include "GL/freeglut.h"
#endif

#include <string>
#include <vector>
#include <mutex>
#include <chrono>
#include <cctype>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <filesystem>
#include <unordered_map>
#include <unordered_set>

// my own header
#include "program.h"

// OSC control of EWOCvj2.
//
// liblo receives on its own thread and only queues the raw messages (osc_handler).  All interpretation of
// addresses and all state changes happen on the main thread in osc_process(), called once per frame next to the
// MIDI queue.  Resolving layers/effects there avoids racing with layer/effect creation and deletion.
//
// Address space (modus = preview | output, deck = deckA | deckB, N = 1-based position):
//   /mix/crossfade_all                    f 0..1       both crossfaders
//   /mix/<modus>/crossfade                  f 0..1
//   /mix/<modus>/wipe/type                  s name | i nr
//   /mix/<modus>/wipe/dir                   i
//   /mix/<modus>/wipe/<param>               f 0..1       parameter of the ISF wipe, when one is selected (or wipe/param/J)
//   /mix/<modus>/wipe/xpos, ypos            f 0..1
//   /mix/<modus>/<deck>/maskup              trigger      go up one level in the mask hierarchy of the deck
//   /mix/<modus>/<deck>/scroll           i            first layer shown of the layer stack of the deck (from 1)
//   /mix/<modus>/<deck>/genmidi          i 0..4 | s   deck general MIDI preset (off, A, B, C, D) for all layers of the deck
//   /mix/<modus>/<deck>/speed               f            deck speed factor as shown in the program (0 to ~11.1)
//   /mix/output/record                    i 0/1 | s start/stop | none = toggle   (HAP output recording)
//   /mix/<modus>/<deck>/save, open         s path       save the deck to a deck file / open a deck file into the deck
//   /mix/<modus>/<deck>/new                trigger      empty the deck
//   /mix/<modus>/save, open                s path       save the mix to a mix file / open a mix file
//   /mix/<modus>/new                       trigger      empty the whole mix
//   /mix/<modus>/exchange                 s op, s from, s to    reorder layers: op "swap" or "insert", layers as "A2", "B1";
//                                                       insert puts the first layer before the second ("B4" = at the end)
//   /mix/<modus>/current                    s layer, ... set the current layers of a modus, as "A1", "B2" (deck, position); the
//                                                       last one becomes the current layer
//   /mix/display/<deckA|deckB|mix|output>   i            show a monitor on another display (number from 1), again to close
//   /mix/aistyle/<deckA|deckB|mix|output>  s name       AI style over a monitor ("none" switches it off)
//        effect/addstyle, effect/K/replacestyle  s name    add / replace by an AI style (effect/add and replace also find style names)
//   /mix/fullscreen/<deckA|deckB|mix|output>  i 0/1    show a monitor full screen (output: preview modus only)
//        fullscreen                       i 0/1        (layer) show the layer full screen
//   /mix/ndioutput/<deckA|deckB|mix|output> i 0/1    NDI output of a monitor of the modus on screen (output: preview modus only)
//   /ndi/refresh                          trigger      look for NDI sources; the names go back as /ndi/sources, s...
//   /webcam/refresh                       trigger      look for webcams; the names go back as /webcam/sources, s...
//   /mix/modus                        i 1/0 | s    preview modus (1, "preview") or live modus (0, "live"); none = toggle
//   /mix/scene[/deckA|/deckB]           i 1..4       switch scene of a deck, or of both decks (like shift+click)
//   /mix/scene/<deck>/send/up             i 1..4       copy the preview modus of a deck to that scene
//   /mix/scene/<deck>/send/down           i 1..4       copy that scene back to the preview modus of the deck
//   /mix/send/up[/a|/b|/mix]          trigger      copy preview modus to output modus
//   /mix/send/down[/a|/b|/mix]            trigger      copy output modus back to preview modus
//   /mix/<modus>/<deck>/layer/N/
//        opacity, volume                  f 0..1
//        speed, scale                     f            raw values
//        shiftx, shifty                   f            raw values
//        save                             s path       save the layer to a layer file
//        add, addbefore                   trigger      the "+" buttons: add a new layer behind this one, or in front of it
//                                                      (addbefore: first layer of a stack only)
//        duplicate, clone               trigger      duplicate the layer, or make a clone linked to it
//        recordreplace                    i 0/1        record the layer to a HAP file and replace it by the recording; 0 stops
//        hapencode                        trigger    encode the video of the layer to HAP in the background
//        display                          i            show the layer on another display (number from 1), again to close
//        aspect                           s name | i  aspect ratio of the layer: output (0), original_inside (1), original_outside (2)
//        ndi                              s name       connect the layer to an NDI source (by name)
//        webcam                           s name | i   connect the layer to a webcam (by name, or 1-based number in /webcam/sources)
//        ndioutput                        i 0/1        send the layer out as an NDI stream
//        mute, solo                       i 0/1        the M and S buttons of the layer (solo mutes the other layers of the stack)
//        key/tolerance, key/feather       f 0..1       key settings for the COLORKEY/CHROMAKEY/LUMAKEY mix modes
//        key/direction, key/invert        i 0/1        the D and I buttons of the key settings
//        key/color                        f f f | s    key colour: red green blue (0..1) or "#RRGGBB"
//        lockspeed, lockzoompan         i 0/1        keep the speed / the scale and shift when another clip is loaded
//        keepeffects, keepmask          i 0/1        the E and K buttons: keep the effects / masks when another clip is loaded
//        select                           trigger      make this the only current layer
//        maskedit                         trigger      enter mask edit mode for the masks of the layer
//        masked                           i 0/1        whether the mask influences the layer
//        mask/M/...                       any layer control of mask M of the layer, also of masks of masks
//        maskscroll                       i            first shown layer of the mask stack of the layer (from 1)
//        effect/K/maskedit, masked, mask/M/...         the same for the masks of an effect
//        effect/K/maskscroll              i            the same for the masks of an effect
//        loopbox/start, loopbox/end     f 0..1       loop start and end markers (L and P) as a position in the video
//        loopbox/setstart, setend         trigger      set the loop start / end to the current frame
//        loopbox/copyduration             trigger      copy the duration of the loop
//        loopbox/pasteduration            trigger      give the loop the copied duration by changing the speed
//        loopbox/pastelength              trigger      give the loop the copied duration by changing its length
//        loopbeats                        f           beatmatch the loop of the layer to f beats (0.5, 1, 2, 4, ...), 0 is off
//        position                         f 0..1      scrub to a position within the video (as dragging in the loopbox)
//        play, reverse, bounce, loop      i 0/1        set; no argument toggles
//        stop, pause, frame/forward, frame/backward     triggers
//        queue/show                       i 0/1        unfold/fold the clip queue of the layer
//        queue/next                       trigger      play the next clip of the queue
//        queue/scroll                     i            first shown clip of the queue (from 1)
//        queue/position                   f 0..1       scrub in the loop bar of the queue (0 = loop start, 1 = loop end)
//        queue/beats                      f            beat switching of the queue every f beats, 0 is off
//        queue/N/load                     s path       load a file into place N of the queue (the last place adds one)
//        queue/N/delete                   trigger      delete clip N from the queue
//        source                           s name     make the layer a source plugin layer (FFGL/ISF generator)
//        source/<param>                   f 0..1       parameter of the source plugin of the layer (or source/param/J)
//                                         (colour parameters, here and for effects, mixer and wipe parameters, take s "#RRGGBB";
//                                          the alpha of an ISF colour is <param>_alpha; feedback of a colour is the hex string)
//        mixer/<param>                    f 0..1       parameter of the FFGL/ISF mixer plugin used as mix mode or wipe (or mixer/param/J)
//        upscale                          i 0/1      Lanczos3 + RCAS upscaling of the layer on/off
//        sharpness                        f 0..1       RCAS sharpness of the upscaling
//        mixmode                          s name | i  mix mode of the layer (MIX, ALPHA OVER, ... or an FFGL/ISF mixer)
//        mixfactor                        f 0..1       the mix factor slider of the layer
//        wipe/type                        s name | i   layer wipe (CROSSFADE is the MIX mode)
//        wipe/dir                         i            layer wipe direction
//        wipe/xpos, ypos                  f 0..1       layer wipe position
//        genmidi                          i 0..4 | s   general MIDI preset of the layer (off, A, B, C, D)
//        load                             s path       open one video, image, layer, deck or mix file
//        effect/K/<param>                 f 0..1       K = 1-based place in the effect chain, <param> = lowercased
//                                                      name with non-alphanumerics as '_', or param/J (1-based)
//        effect/add                       s name [, i place]   add an effect by its name in the effect menu (default: at the end)
//        effect/K/replace                 s name       replace the effect in place K by another one
//        effect/K/delete                  trigger      delete the effect in place K
//        fxchain                          i 0/1 | s    effect stack category: layer effects (0, "layer") or stream effects (1, "stream")
//        effect/K/onoff                   i 0/1
//        effect/K/drywet                  f 0..1
//        effect2/K/...                    same, for the second effect chain of the layer (after the mix)
//        effects that are in a chain are always given by their place K (the same effect can be in a chain several
//        times); effects are given by name only when they are chosen (add, replace)
//   /shelf/<A|B>/bank                     i 1..4       select the shelf bank
//   /shelf/load                           s A|B, i bank 1..4, i element 1..16, s path   load a file into a shelf element
//   /shelf/<A|B>/load                     i bank 1..4, i element 1..16, s path          the same, shelf in the address
//   /shelf/new, /shelf/<A|B>/new          s shelf (A|B), i bank 1..4          empty the 16 elements of a bank
//   /shelf/open, /shelf/<A|B>/open        s shelf, i bank, s path              open a shelf file into a bank
//   /shelf/save, /shelf/<A|B>/save        s shelf, i bank, s path              save a bank to a shelf file
//   /shelf/insertinbin, /shelf/<A|B>/insertinbin  s shelf, i bank 1..4, i block 1..9   put a bank in a block of the current bin
//   /shelf/insertdeck, /shelf/<A|B>/insertdeck same arguments, s "A"|"B" instead of the path: put deck A or B in an element
//   /shelf/insertmix, /shelf/<A|B>/insertmix     same arguments without the path: put the whole mix in an element
//   /shelf/delete, /shelf/<A|B>/delete  same arguments without the path                erase a shelf element
//   /shelf/rename, /shelf/<A|B>/rename    same arguments, s name instead of the path     rename a shelf element
//   /shelf/<A|B>/trigger                i 1..16      trigger a shelf element of the current bank into the mix
//   /shelf/<A|B>/launchtype               i 0..2 | s, [i bank 1..4]   set launch type of all elements of a bank (default: current)
//   /shelf/<A|B>/element/N/launchtype     i 0..2 | s, [i bank 1..4]   set launch type of one element (restart/continue/catchup)
//   /loopstation/current                  i 1..256     select the current loopstation line
//   /loopstation/line/N/rec, loop, play   i 0/1        record / loop-play / one-off play button of a line
//   /loopstation/line/N/speed             f            line speed factor (0..4)
//   /loopstation/line/N/position          f 0..1       scrub inside the recording of a line
//   /loopstation/line/N/clear             trigger      clear the line
//   /loopstation/line/N/copyduration      trigger      copy the loop duration of the line
//   /loopstation/line/N/pastespeed        trigger      give the line the copied duration by changing its speed
//   /loopstation/line/N/pastelength       trigger      give the line the copied duration by changing its loop length
//   /loopstation/line/N/beats             f            beatmatch the line to f beats (0.5, 1, 2, 4, ...), 0 is off
//   /loopstation/line/N/copycurve         trigger      copy the curve of the line
//   /loopstation/line/N/pastecurve        trigger      paste the copied curve on the line
//   /loopstation/scroll                   i            first line shown in the loopstation list (from 1)
//   /beat/threshold                       f 0..1       beat detection threshold (feedback also has /beat/bpm, read only)
//   /bin/select                           i 1..      make a bin the current bin (number in the bins list)
//   /bin/new, /bin/delete                 trigger      new bin / delete the current bin
//   /bin/rename                           s name       rename the current bin
//   /bin/open, /bin/save                  s path       open a bin file / save the current bin to a bin file
//   /bin/loadshelf                        s shelf, i bank 1..4, i block 1..9   block of the current bin into a shelf bank
//   /bin/hapmode                          i 0/1 | s    HAP encoding mode of the bins room: live (0, one thread) or max (1)
//   /bin/hapencode                        trigger      HAP encode all elements of the current bin
//   /bin/selection/delete, hapencode, clear  trigger   delete / HAP encode / deselect the selected elements
//   /bin/element/R/C/delete, hapencode    trigger      empty an element / HAP encode an element (again: stop)
//   /bin/element/R/C/select               i 0/1        select an element
//   /bin/element/R/C/rename               s name      rename the element at row R, column C (1..12) of the current bin
//   /bin/element/R/C/launchtype           i 0..2 | s   launch type of that element
//   /bin/element/R/C/loaddeck             s "A"|"B"    open that deck element into deck A or B
//   /bin/element/R/C/loadmix              trigger      open that mix element
//   /list/<what>                          trigger      send back /list/<what> with the names: effects, aistyles, mixmodes, wipes,
//                                                      sources, displays, bins, ndi (feedback also has names and counts)
//   /osc/target  s host, i port           where state feedback goes (default: first sender, or first to log in, port 9001)
//   /osc/feedback i 0/1                   state feedback on/off
//   /osc/sync                             resend the complete state
//   /osc/auth    s password               log in, when the "OSC Password" preference is set (answer: /osc/auth ok|denied)
// Triggers fire on no argument or a non-zero one, so push buttons that send 1 on press and 0 on release work.
// State feedback sends the same addresses back, with the values above, whenever something changes.

// The ports and the other settings come from the preferences ("OSC" tab), see osc_apply_prefs().

struct OscArg {
	char t = 0;
	double n = 0.0;
	std::string s;
};

struct OscMsg {
	std::string path;
	std::string host;	// numeric address of the sender
	std::vector<OscArg> args;
};

static lo_server_thread osc_st = nullptr;
static int osc_port = 0;						// the port the server listens on while it runs
static std::atomic<bool> osc_localhost{false};	// only messages from this computer are accepted (read by the receiving thread)
static std::string osc_feedbackport = "9001";	// the port feedback is sent to
static std::mutex osc_qmutex;
static std::vector<OscMsg> osc_queue;

// Password: when the preferences have one, a computer first has to send it with /osc/auth; all other messages from
// computers that haven't done so are dropped.  The check is done on the main thread, in osc_process().
static bool osc_oldhw = false;							// "Old hardware": slower, less detailed feedback (main thread)
static bool osc_safemode = false;						// ignore messages that touch files or erase content (main thread)
static std::string osc_password;					// "" is no password
static std::unordered_set<std::string> osc_authed;		// hosts that have sent the password
struct OscAuthTry {
	int failures = 0;
	std::chrono::steady_clock::time_point lockeduntil;
};
static std::unordered_map<std::string, OscAuthTry> osc_authtries;

// main thread only from here on
static lo_address osc_target = nullptr;
static std::string osc_targethost;		// the host of osc_target
static bool osc_target_auto = true;		// the target was taken from the sender of a message, not set with /osc/target
static bool osc_feedback_on = true;
static bool osc_resync = false;	// target changed or /osc/sync: next scan must send everything
static std::unordered_map<std::string, float> osc_lastsent;
static std::unordered_map<std::string, std::string> osc_laststr;	// the same for the names that are sent as strings
static std::chrono::steady_clock::time_point osc_lastscan;

static void osc_error(int num, const char *msg, const char *where) {
	printf("OSC error %d in %s: %s\n", num, where ? where : "?", msg ? msg : "");
}

static bool is_local_host(const std::string &host) {
	// the host of this computer, as liblo reports the numeric address of the sender
	return host == "127.0.0.1" || host == "::1" || host == "localhost" || host.rfind("127.", 0) == 0 ||
		   host.rfind("::ffff:127.", 0) == 0;
}

static int osc_handler(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *data) {
	if (osc_localhost) {
		// "Localhost only": the port is open on the network, but only this computer's messages are used
		lo_address from = lo_message_get_source(msg);
		const char *fh = from ? lo_address_get_hostname(from) : nullptr;
		if (!fh || !is_local_host(fh)) return 0;
	}
	OscMsg m;
	m.path = path;
	for (int i = 0; i < argc; i++) {
		OscArg a;
		a.t = types[i];
		switch (types[i]) {
			case 'f': a.n = argv[i]->f; break;
			case 'd': a.n = argv[i]->d; break;
			case 'i': a.n = argv[i]->i; break;
			case 'h': a.n = (double)argv[i]->h; break;
			case 'T': a.n = 1.0; break;
			case 'F': a.n = 0.0; break;
			case 's':
			case 'S': a.s = &argv[i]->s; break;
			default: break;
		}
		m.args.push_back(a);
	}
	lo_address src = lo_message_get_source(msg);
	if (src) {
		const char *h = lo_address_get_hostname(src);
		if (h) m.host = h;
	}
	std::lock_guard<std::mutex> lock(osc_qmutex);
	if (osc_queue.size() >= 8192) osc_queue.erase(osc_queue.begin(), osc_queue.begin() + 4096);
	osc_queue.push_back(std::move(m));
	return 0;
}

void osc_start(int port) {
	if (osc_st) return;
	osc_st = lo_server_thread_new(std::to_string(port).c_str(), osc_error);
	if (!osc_st) {
		printf("OSC: could not listen on port %d\n", port);
		return;
	}
	lo_server_thread_add_method(osc_st, nullptr, nullptr, osc_handler, nullptr);
	lo_server_thread_start(osc_st);
	osc_port = port;
	printf("OSC: listening on port %d%s\n", port, osc_localhost ? " (messages from this computer only)" : "");
}

void osc_stop() {
	// Shut OSC down at the end of the program (or when it is turned off): the receiving thread is stopped and waited for
	// before it is freed, so no handler is running afterwards; whatever was still queued is dropped.  Safe to call more
	// than once, and osc_process() does nothing after it.
	if (osc_st) {
		lo_server_thread_stop(osc_st);
		lo_server_thread_free(osc_st);
		osc_st = nullptr;
	}
	{
		std::lock_guard<std::mutex> lock(osc_qmutex);
		osc_queue.clear();
	}
	osc_authed.clear();
	osc_authtries.clear();
	if (osc_target) {
		lo_address_free(osc_target);
		osc_target = nullptr;
	}
	osc_targethost.clear();
	osc_target_auto = true;
	osc_port = 0;
	osc_lastsent.clear();
	osc_laststr.clear();
	osc_resync = false;
}

// ---------------------------------------------------------------------------------------------------------
// argument helpers

static bool arg_num(const OscMsg &m, size_t k, double &v) {
	if (k >= m.args.size()) return false;
	char t = m.args[k].t;
	if (t == 's' || t == 'S') {
		const std::string &s = m.args[k].s;
		if (s == "on" || s == "start" || s == "true") { v = 1.0; return true; }
		if (s == "off" || s == "stop" || s == "false") { v = 0.0; return true; }
		return false;
	}
	if (t == 'f' || t == 'd' || t == 'i' || t == 'h' || t == 'T' || t == 'F') {
		v = m.args[k].n;
		return true;
	}
	return false;
}

static bool arg_str(const OscMsg &m, size_t k, std::string &s) {
	if (k >= m.args.size() || (m.args[k].t != 's' && m.args[k].t != 'S')) return false;
	s = m.args[k].s;
	return true;
}

static bool is_trigger(const OscMsg &m) {
	double v;
	if (!m.args.size()) return true;
	return arg_num(m, 0, v) && v != 0.0;
}

static std::string sanitize(const std::string &name) {
	std::string r;
	for (char c: name) r += std::isalnum((unsigned char)c) ? (char)std::tolower((unsigned char)c) : '_';
	return r;
}

static std::vector<std::string> split_path(const std::string &path) {
	std::vector<std::string> t;
	size_t pos = 0;
	while (pos < path.size()) {
		size_t e = path.find('/', pos);
		if (e == std::string::npos) e = path.size();
		if (e > pos) t.push_back(path.substr(pos, e - pos));
		pos = e + 1;
	}
	return t;
}

static bool parse_int(const std::string &s, int &v) {
	if (s.empty() || s.size() > 6) return false;
	for (char c: s) if (!std::isdigit((unsigned char)c)) return false;
	v = std::stoi(s);
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// applying values

static LoopStation *osc_lpst(int set) {
	return set == 0 ? lp : lpc;
}

static void param_changed(Param *par, int set) {
	// same bookkeeping as the MIDI handlers: let loopstation playback yield, record into armed loopstation lines
	par->midistarttime = std::chrono::system_clock::now();
	par->midistarted = true;
	LoopStation *ls = osc_lpst(set);
	if (ls) {
		for (auto elem: ls->elements) {
			if (elem->recbut->value) elem->add_param_automationentry(par);
		}
	}
	if (!par->shadervar.empty()) mainprogram->uniformCache->setFloat(par->shadervar.c_str(), par->value);
}

static void button_changed(Button *but, int set) {
	but->midistarttime = std::chrono::system_clock::now();
	LoopStation *ls = osc_lpst(set);
	if (ls) {
		for (auto elem: ls->elements) {
			if (elem->recbut->value) elem->add_button_automationentry(but);
		}
	}
}

static void set_param_normalized(Param *par, double v, int set) {
	v = std::clamp(v, 0.0, 1.0);
	par->value = par->range[0] + (float)v * (par->range[1] - par->range[0]);
	param_changed(par, set);
}

// Colour parameters (ISF colour inputs, FFGL red/green/blue groups) keep their colour in colvalue of the first parameter of
// the group; the green and blue parameters of an FFGL group (colslave) are hidden.  Over OSC a colour is a hex string.
static bool is_color_param(const Param *par) {
	return par->type == ISFLoader::PARAM_COLOR;
}

static std::string param_osc_name(const Param *par) {
	// the name used in addresses; the alpha that goes with an ISF colour has the name of the colour, so it gets a suffix
	std::string n = sanitize(par->name);
	if (par->type == ISFLoader::PARAM_ALPHA) n += "_alpha";
	return n;
}

static bool parse_hex_color(const std::string &s, float rgb[3]) {
	std::string hex = s;
	if (!hex.empty() && hex[0] == '#') hex.erase(0, 1);
	if (hex.size() != 6 || hex.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos) return false;
	for (int k = 0; k < 3; k++) rgb[k] = std::stoi(hex.substr(k * 2, 2), nullptr, 16) / 255.0f;
	return true;
}

static std::string color_hex(const Param *par) {
	char buf[8];
	int c[3];
	for (int k = 0; k < 3; k++) c[k] = (int)std::lround(std::clamp(par->colvalue[k], 0.0f, 1.0f) * 255.0f);
	snprintf(buf, sizeof(buf), "#%02X%02X%02X", c[0], c[1], c[2]);
	return buf;
}

static void set_param_msg(Param *par, const OscMsg &m, int set) {
	// set a parameter from the first argument of a message: a number 0..1, or "#RRGGBB" for a colour parameter
	if (par->colslave) return;
	double v;
	if (is_color_param(par)) {
		std::string hex;
		float rgb[3];
		if (arg_str(m, 0, hex) && parse_hex_color(hex, rgb)) {
			for (int k = 0; k < 3; k++) par->colvalue[k] = rgb[k];
		}
		return;
	}
	if (arg_num(m, 0, v)) set_param_normalized(par, v, set);
}

static float raw_value(Param *par) {
	// playback speed params (powertwo) are stored as the square root of the speed the user sees
	return par->powertwo ? par->value * par->value : par->value;
}

static void set_param_raw(Param *par, double v, int set) {
	// v is the value as displayed; for powertwo params that is the square of the stored value
	if (par->powertwo) v = std::sqrt(std::max(v, 0.0));
	float lo = std::min(par->range[0], par->range[1]);
	float hi = std::max(par->range[0], par->range[1]);
	par->value = std::clamp((float)v, lo, hi);
	param_changed(par, set);
}

static bool resolve_wipe(const OscMsg &m, int &nr, int &isfnr) {
	// wipe given by name or number: nr = the wipe number (-1 is CROSSFADE), or isfnr = an ISF mixer used as wipe.
	// Names match regardless of case and punctuation: "Push/Pull", "push_pull" and "PUSH PULL" are the same.
	std::string name;
	double v;
	nr = -1;
	isfnr = -1;
	if (arg_str(m, 0, name)) {
		std::string key = sanitize(name);
		for (auto &w: mainprogram->wipesmap) {
			if (sanitize(w.first) == key) { nr = w.second; return true; }
		}
		// the ISF wipes by their name in the wipe menu (equal names are numbered: "CLASSIC 2")
		for (size_t i = 0; i < mainprogram->wipeisfnames.size() && i < mainprogram->isfwipemixernrs.size(); i++) {
			if (sanitize(mainprogram->wipeisfnames[i]) == key) { isfnr = mainprogram->isfwipemixernrs[i]; return true; }
		}
		return false;
	}
	if (arg_num(m, 0, v) && (int)v >= -1 && (int)v < mainprogram->mixwipebase - 1) {
		nr = (int)v;
		return true;
	}
	return false;
}

static void set_layer_mixmode(Layer *lay, const OscMsg &m) {
	// mix mode of the layer by name (the mixmode menu: built-in modes, then FFGL mixers, then ISF mixers) or by
	// BLEND_TYPE number for the built-in ones (MIX = 1 ... MASK = 23)
	BlendNode *bn = lay->blendnode;
	if (!bn) return;
	std::string name;
	double v;
	// the names of the mixmode menu, as shown there (equal names of plugin mixers are numbered: "ADD", "ADD 2")
	const std::vector<std::string> &names = mainprogram->mixmodenames;
	const int numbuiltin = 23;
	if (arg_str(m, 0, name)) {
		std::string key = sanitize(name);
		for (size_t k = 0; k < names.size(); k++) {
			if (sanitize(names[k]) != key) continue;
			if ((int)k < numbuiltin) {
				bn->blendtype = (BLEND_TYPE)(k + 1);
				bn->ffglmixernr = -1;
				bn->isfmixernr = -1;
			}
			else if (k - numbuiltin < mainprogram->ffglmixernames.size()) {
				bn->set_ffglmixer((int)(k - numbuiltin));
			}
			else {
				size_t isfpos = k - numbuiltin - mainprogram->ffglmixernames.size();
				if (isfpos < mainprogram->isfmixmodemixernrs.size()) bn->set_isfmixer(mainprogram->isfmixmodemixernrs[isfpos]);
			}
			return;
		}
	}
	else if (arg_num(m, 0, v) && (int)v >= 1 && (int)v <= numbuiltin) {
		bn->blendtype = (BLEND_TYPE)(int)v;
		bn->ffglmixernr = -1;
		bn->isfmixernr = -1;
	}
}

static void set_layer_wipe(Layer *lay, const OscMsg &m) {
	// the layer wipe, as chosen in the wipe submenu of the layer mix button: CROSSFADE is the plain MIX mode
	BlendNode *bn = lay->blendnode;
	if (!bn) return;
	int nr, isfnr;
	if (!resolve_wipe(m, nr, isfnr)) return;
	if (isfnr != -1) {
		bn->set_isfmixer(isfnr);
	}
	else if (nr == -1) {
		bn->blendtype = MIXING;
		bn->ffglmixernr = -1;
		bn->isfmixernr = -1;
	}
	else {
		bn->blendtype = WIPE;
		bn->wipetype = nr;
		bn->ffglmixernr = -1;
		bn->isfmixernr = -1;
	}
}

extern void set_genmidi_recursive(Layer *lay, bool deck);	// mixer.cpp

static bool parse_midipreset(const OscMsg &m, int &preset) {
	// general MIDI preset: 0/"off", 1/"A", 2/"B", 3/"C", 4/"D"
	std::string s;
	double v;
	if (arg_str(m, 0, s)) {
		s = sanitize(s);
		if (s == "off") preset = 0;
		else if (s == "a") preset = 1;
		else if (s == "b") preset = 2;
		else if (s == "c") preset = 3;
		else if (s == "d") preset = 4;
		else return false;
		return true;
	}
	if (arg_num(m, 0, v) && v >= 0.0 && v <= 4.0) {
		preset = (int)v;
		return true;
	}
	return false;
}

static int *stack_scrollpos(int set, int deck) {
	// the scroll position of the layer stack of a deck: the first shown layer, from 0.  The program keeps the one of the
	// modus on screen with the scene of the deck, and swaps it with mainmix->swapscrollpos when the modus is switched.
	if (set == !mainprogram->prevmodus) return &mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos;
	return &mainmix->swapscrollpos[deck];
}

static Layer *find_layer(int set, int deck, int n) {
	std::vector<Layer*> &lv = mainmix->layers[set * 2 + deck];
	if (n < 1 || n > (int)lv.size()) return nullptr;
	return lv[n - 1];
}

static void set_play_exclusive(Layer *lay, Button *on, int set, const OscMsg &m) {
	// play/reverse/bounce exclude each other.  An argument sets, no argument toggles.
	double v;
	bool val = arg_num(m, 0, v) ? v != 0.0 : !on->value;
	lay->playbut->value = false;
	lay->revbut->value = false;
	lay->bouncebut->value = false;
	on->value = val ? 1 : 0;
	button_changed(on, set);
}

static void layer_pause(Layer *lay, int set) {
	// pause/resume with the same codes as the space bar and the MIDI genplay control
	Button *but = nullptr;
	if (lay->playbut->value) { lay->playkind = 0; lay->playbut->value = false; but = lay->playbut; }
	else if (lay->revbut->value) { lay->playkind = 1; lay->revbut->value = false; but = lay->revbut; }
	else if (lay->bouncebut->value == 1) { lay->playkind = 2; lay->bouncebut->value = 0; but = lay->bouncebut; }
	else if (lay->bouncebut->value == 2) { lay->playkind = 3; lay->bouncebut->value = 0; but = lay->bouncebut; }
	else if (lay->playkind == 1) { lay->revbut->value = true; but = lay->revbut; }
	else if (lay->playkind == 2) { lay->bouncebut->value = 1; but = lay->bouncebut; }
	else if (lay->playkind == 3) { lay->bouncebut->value = 2; but = lay->bouncebut; }
	else { lay->playbut->value = true; but = lay->playbut; }
	button_changed(but, set);
}

static void layer_load(Layer *lay, int set, const std::string &path) {
	// Loading is done by Layer::open_files_layers(), the code that opens files chosen in the file requester or dropped
	// on a layer (videos, images, layer, deck and mix files).  It works on mainprogram->paths over several frames,
	// so it is started here with that list holding the single path.  A load that arrives while the paths list or
	// the file opening state is in use by something else is ignored.
	if (mainprogram->openfileslayers || mainprogram->openfilesqueue || mainprogram->openfilesshelf ||
		mainprogram->openclipfiles || mainprogram->multistage != 0 || !mainprogram->paths.empty()) {
		printf("OSC: busy opening files, load ignored: %s\n", path.c_str());
		return;
	}
	// open_files_layers() works on the layers of the modus on screen
	if (set != !mainprogram->prevmodus) {
		printf("OSC: can not load into the %s set while the %s set is on screen: %s\n", set ? "output" : "preview",
			   set ? "preview" : "output", path.c_str());
		return;
	}
	std::error_code ec;
	if (path.size() < 5 || !std::filesystem::exists(path, ec)) {
		printf("OSC: file not found: %s\n", path.c_str());
		return;
	}
	mainprogram->paths.push_back(path);
	mainprogram->pathscount = 0;
	mainprogram->loadlay = lay;
	mainprogram->fileslay = lay;
	mainmix->addlay = false;
	mainmix->addbefore = false;
	mainprogram->openfileslayers = true;
}

static bool parse_layer_spec(const std::string &s, int &deck, int &pos) {
	// a layer as deck letter and position (from 1): "A1", "B3"
	if (s.size() < 2) return false;
	char dl = (char)std::toupper((unsigned char)s[0]);
	if (dl != 'A' && dl != 'B') return false;
	deck = dl == 'B';
	return parse_int(s.substr(1), pos);
}

static bool on_screen_ok(int set);

static void exchange_layers(int set, const OscMsg &m) {
	// One reordering of the layers of the layer stacks, with Layer::exchange() (what dragging a layer does):
	//   "swap"   "A2" "B1": the layers swap places
	//   "insert" "A2" "B1": layer A2 is taken out of its stack and put in front of layer B1; a place one past the last
	//                       layer of a stack ("B4" for three layers) puts it at the end
	// The layers can be in different decks.
	std::string op, from, to;
	if (!arg_str(m, 0, op) || !arg_str(m, 1, from) || !arg_str(m, 2, to)) return;
	op = sanitize(op);
	bool swap = op == "swap";
	if (!swap && op != "insert") return;
	int sdeck, spos, ddeck, dpos;
	if (!parse_layer_spec(from, sdeck, spos) || !parse_layer_spec(to, ddeck, dpos)) return;
	if (!on_screen_ok(set)) return;
	if (mainprogram->swappingscene || mainprogram->openfileslayers) {
		printf("OSC: can not reorder layers while scenes or files are being loaded\n");
		return;
	}
	Layer *lay = find_layer(set, sdeck, spos);
	if (!lay) return;
	std::vector<Layer*> &dlayers = mainmix->layers[set * 2 + ddeck];
	int size = (int)dlayers.size();
	if (swap) {
		if (dpos < 1 || dpos > size) return;
		lay->exchange(*lay->layers, dlayers, ddeck, dpos - 1, 0);
	}
	else {
		if (dpos < 1 || dpos > size + 1) return;
		if (dpos == size + 1) lay->exchange(*lay->layers, dlayers, ddeck, size - 1, 2);
		else lay->exchange(*lay->layers, dlayers, ddeck, dpos - 1, 1);
	}
}

static void set_current_layers(int set, const std::vector<Layer*> &lays) {
	// replace the current layers of a modus (mainmix->currlays) and make the last one the current layer
	// (mainmix->currlay), as selecting layers with the mouse does
	if (lays.empty()) return;
	mainmix->currlays[set] = lays;
	mainmix->currlay[set] = lays.back();
	mainprogram->effcat[lays.back()->deck]->value = lays.back()->effcat;
}

static void layer_menu_entry(Layer *lay, int option, int choice) {
	// Carry out an entry of the layer menu on a layer, with the code of the menu itself (Program::handle_laymenu1): the
	// menu works on mainmix->mouselayer, and takes the choice in a submenu from mainprogram->menuresults.
	Layer *bumouselayer = mainmix->mouselayer;
	std::vector<int> bumenuresults = mainprogram->menuresults;
	int bumonitorvalue = mainprogram->monitormenu->value;
	mainmix->mouselayer = lay;
	mainprogram->menuresults.clear();
	if (choice >= 0) mainprogram->menuresults.push_back(choice);
	mainprogram->handle_laymenu1(option);
	mainprogram->menuresults = bumenuresults;
	mainprogram->monitormenu->value = bumonitorvalue;
	// the entry can have replaced the layer; only put back what we set ourselves.  HAP encoding of a clone moves
	// mouselayer to the first layer of its clones on purpose, that is not meant to stay.
	if (mainmix->mouselayer == lay || option == HAP_ENCODE) mainmix->mouselayer = bumouselayer;
}

// Saving and opening layer, deck and mix files goes through the pathto system, the way the file dialogs hand over their
// result: Program::pathto says what to do ("SAVEDECK", "OPENDECK", "SAVEMIX", "OPENMIX", "SAVELAYFILE") and
// Program::path is the file; the main loop picks it up at the start of its next round, uses mainmix->mousedeck /
// mainmix->mouselayer as the deck / layer, and clears both.  Operations wait here until that is free.
struct OscFileOp {
	std::string pathto;
	std::string path;
	int set, deck, pos;		// the deck, and for layers the position in it (from 0); pos is -1 for decks and mixes
	int side = -1, bank = -1;	// for shelves: which one ("SAVESHELF", "OPENSHELF"), the modus is -1 for those
};
static std::vector<OscFileOp> osc_pendfileops;

static void queue_file_op(const char *pathto, const std::string &path, int set, int deck, int pos = -1, int side = -1, int bank = -1) {
	if (osc_pendfileops.size() < 16) osc_pendfileops.push_back({pathto, path, set, deck, pos, side, bank});
}

static void fileop_apply() {
	if (osc_pendfileops.empty()) return;
	{
		std::lock_guard<std::mutex> lock(mainprogram->pathmutex);
		if (!mainprogram->pathto.empty() || !mainprogram->path.empty() || !mainprogram->paths.empty()) return;
	}
	if (mainprogram->swappingscene || mainprogram->openfileslayers || mainprogram->openfilesqueue ||
		mainprogram->openfilesshelf || mainprogram->openclipfiles || binsmain->importbins || binsmain->openfilesbin) return;
	OscFileOp op = osc_pendfileops.front();
	osc_pendfileops.erase(osc_pendfileops.begin());

	// the save and open functions work on the modus that is on screen (shelves don't depend on the modus)
	if (op.set >= 0 && op.set != !mainprogram->prevmodus) {
		printf("OSC: %s only works on the set that is on screen: %s\n", op.pathto.c_str(), op.path.c_str());
		return;
	}
	bool save = op.pathto.rfind("SAVE", 0) == 0;
	std::error_code ec;
	if (save) {
		std::filesystem::path dir = std::filesystem::path(op.path).parent_path();
		if (!dir.empty() && !std::filesystem::is_directory(dir, ec)) {
			printf("OSC: directory not found: %s\n", dir.string().c_str());
			return;
		}
	}
	else {
		if (!std::filesystem::exists(op.path, ec)) {
			printf("OSC: file not found: %s\n", op.path.c_str());
			return;
		}
		if ((op.pathto == "OPENDECK" && !isdeckfile(op.path)) || (op.pathto == "OPENMIX" && !ismixfile(op.path))) {
			printf("OSC: not a %s file: %s\n", op.pathto == "OPENDECK" ? "deck" : "mix", op.path.c_str());
			return;
		}
	}
	if (op.pathto == "SAVELAYFILE") {
		Layer *lay = find_layer(op.set, op.deck, op.pos + 1);
		if (!lay) return;
		mainmix->mouselayer = lay;
	}
	else if (op.side >= 0) {
		// the shelf the handler works on
		mainmix->mouseshelf = mainprogram->shelves[op.side][op.bank];
	}
	else if (op.deck >= 0) {
		mainmix->mousedeck = op.deck;
	}
	std::lock_guard<std::mutex> lock(mainprogram->pathmutex);
	if (op.pathto == "OPENBIN") mainprogram->paths.push_back(op.path);		// the bin import takes a list of files
	else mainprogram->path = op.path;
	mainprogram->pathto = op.pathto;
}

static void new_deck_or_mix(int deck) {
	// the New deck / New mix entries of the layer menu (deck 2 is the whole mix): empties the deck or the mix
	if (deck < 2) mainmix->mousedeck = deck;
	mainprogram->handle_laymenu1(deck < 2 ? NEW_DECK : NEW_MIX);
}

static void handle_key(Layer *lay, int set, const std::vector<std::string> &t, size_t i, const OscMsg &m) {
	// The key settings of a layer: they apply when its mix mode is COLORKEY, CHROMAKEY or LUMAKEY.  t[i] is the setting.
	const std::string &c = t[i];
	double v;
	if (c == "tolerance" && arg_num(m, 0, v)) {
		set_param_normalized(lay->chtol, v, set);
	}
	else if (c == "feather" && arg_num(m, 0, v)) {
		set_param_normalized(lay->chfeather, v, set);
	}
	else if (c == "direction") {
		lay->chdir->value = arg_num(m, 0, v) ? v != 0.0 : !lay->chdir->value;
		button_changed(lay->chdir, set);
	}
	else if (c == "invert") {
		lay->chinv->value = arg_num(m, 0, v) ? v != 0.0 : !lay->chinv->value;
		button_changed(lay->chinv, set);
	}
	else if (c == "color") {
		// the key colour, as three numbers 0 to 1 (red, green, blue) or a hex string "#RRGGBB"
		float rgb[3];
		std::string hex;
		double r, g, b;
		if (arg_str(m, 0, hex)) {
			if (!hex.empty() && hex[0] == '#') hex.erase(0, 1);
			if (hex.size() != 6 || hex.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos) return;
			for (int k = 0; k < 3; k++) rgb[k] = std::stoi(hex.substr(k * 2, 2), nullptr, 16) / 255.0f;
		}
		else if (arg_num(m, 0, r) && arg_num(m, 1, g) && arg_num(m, 2, b)) {
			rgb[0] = (float)std::clamp(r, 0.0, 1.0);
			rgb[1] = (float)std::clamp(g, 0.0, 1.0);
			rgb[2] = (float)std::clamp(b, 0.0, 1.0);
		}
		else return;
		// as the colour picker does it: a chroma key only has the hue of the colour, a luma key only its brightness
		float mx = std::max({rgb[0], rgb[1], rgb[2]});
		float mn = std::min({rgb[0], rgb[1], rgb[2]});
		if (lay->blendnode->blendtype == CHROMAKEY) {
			float d = mx - mn;
			float h = 0.0f;		// hue in 0..6
			if (d > 0.0f) {
				if (mx == rgb[0]) h = std::fmod((rgb[1] - rgb[2]) / d, 6.0f);
				else if (mx == rgb[1]) h = (rgb[2] - rgb[0]) / d + 2.0f;
				else h = (rgb[0] - rgb[1]) / d + 4.0f;
				if (h < 0.0f) h += 6.0f;
			}
			float x = 1.0f - std::fabs(std::fmod(h, 2.0f) - 1.0f);
			switch ((int)h) {
				case 0: rgb[0] = 1.0f; rgb[1] = x; rgb[2] = 0.0f; break;
				case 1: rgb[0] = x; rgb[1] = 1.0f; rgb[2] = 0.0f; break;
				case 2: rgb[0] = 0.0f; rgb[1] = 1.0f; rgb[2] = x; break;
				case 3: rgb[0] = 0.0f; rgb[1] = x; rgb[2] = 1.0f; break;
				case 4: rgb[0] = x; rgb[1] = 0.0f; rgb[2] = 1.0f; break;
				default: rgb[0] = 1.0f; rgb[1] = 0.0f; rgb[2] = x; break;
			}
		}
		else if (lay->blendnode->blendtype == LUMAKEY) {
			rgb[0] = rgb[1] = rgb[2] = mx;
		}
		for (int k = 0; k < 3; k++) {
			lay->rgb[k] = rgb[k];
			lay->colorbox->acolor[k] = rgb[k];
		}
		lay->colorbox->acolor[3] = 1.0f;
		lay->blendnode->chred = rgb[0];
		lay->blendnode->chgreen = rgb[1];
		lay->blendnode->chblue = rgb[2];
	}
}

static void set_layer_aspect(Layer *lay, const OscMsg &m) {
	// the aspect ratio entry of the layer menu: 0 "Same as output", 1 "Original inside", 2 "Original outside"
	std::string name;
	double v;
	int ratio;
	if (arg_str(m, 0, name)) {
		std::string key = sanitize(name);
		if (key == "same_as_output" || key == "output") ratio = RATIO_OUTPUT;
		else if (key == "original_inside" || key == "inside") ratio = RATIO_ORIGINAL_INSIDE;
		else if (key == "original_outside" || key == "outside") ratio = RATIO_ORIGINAL_OUTSIDE;
		else return;
	}
	else if (arg_num(m, 0, v) && v >= 0.0 && v <= 2.0) {
		ratio = (int)v;
	}
	else return;
	lay->aspectratio = (RATIO_TYPE)ratio;
	if (lay->type == ELEM_IMAGE) {
		if (lay->loadedImage) {
			lay->set_aspectratio(lay->loadedImage->getFrameWidth((int)lay->frame),
								 lay->loadedImage->getFrameHeight((int)lay->frame));
		}
		std::lock_guard<std::mutex> lock(lay->decresult_mutex);
		lay->decresult->newdata = true;
	}
	else if (lay->type == ELEM_NDI) {
		if (lay->iw > 0) {
			lay->set_aspectratio(lay->iw, lay->ih);
		}
		else if (lay->ndisource && lay->ndiintex.getWidth() > 0) {
			lay->set_aspectratio(lay->ndiintex.getWidth(), lay->ndiintex.getHeight());
		}
		else if (lay->ndiparentlay && lay->ndiparentlay->ndiintex.getWidth() > 0) {
			lay->set_aspectratio(lay->ndiparentlay->ndiintex.getWidth(), lay->ndiparentlay->ndiintex.getHeight());
		}
	}
	else if (lay->video_dec_ctx) {
		lay->set_aspectratio(lay->video_dec_ctx->width, lay->video_dec_ctx->height);
	}
}

static void handle_loopbox(Layer *lay, int set, const std::vector<std::string> &t, size_t i, const OscMsg &m) {
	// The loop of the layer (the part of the video between the L and P markers of the loopbox) and the entries of the
	// loop menu.  t[i] is the control.
	const std::string &c = t[i];
	double v;
	if ((c == "start" || c == "end") && arg_num(m, 0, v)) {
		// move a marker to a position in the video (0 to 1), as dragging it with ctrl does: the markers can't pass
		// each other and the playhead stays inside the loop
		if (lay->numf < 2) return;
		float pos = (float)(std::clamp(v, 0.0, 1.0) * (lay->numf - 1));
		if (c == "start") {
			lay->startframe->value = pos;
			if (lay->startframe->value > lay->frame) lay->frame = lay->startframe->value;
			if (lay->startframe->value > lay->endframe->value) {
				lay->startframe->value = lay->endframe->value;
				lay->frame = lay->endframe->value;
			}
			param_changed(lay->startframe, set);
		}
		else {
			lay->endframe->value = pos;
			if (lay->endframe->value < lay->frame) lay->frame = lay->endframe->value;
			if (lay->endframe->value < lay->startframe->value) {
				lay->endframe->value = lay->startframe->value;
				lay->frame = lay->endframe->value;
			}
			param_changed(lay->endframe, set);
		}
		lay->set_clones();
		return;
	}
	// the entries of the loop menu: the menu code itself is run on the layer
	int k = -1;
	if (c == "setstart") k = 0;
	else if (c == "setend") k = 1;
	else if (c == "copyduration") k = 2;
	else if (c == "pasteduration") k = 3;
	else if (c == "pastelength") k = 4;
	if (k < 0 || !is_trigger(m)) return;
	if (set != !mainprogram->prevmodus) {
		printf("OSC: loop menu actions only work on the set that is on screen\n");
		return;
	}
	Layer *bumouselayer = mainmix->mouselayer;
	mainmix->mouselayer = lay;
	mainprogram->handle_loopmenu(k);
	mainmix->mouselayer = bumouselayer;
}

static bool files_busy() {
	// the paths list and the file opening state are shared by all the file opening functions
	return mainprogram->openfileslayers || mainprogram->openfilesqueue || mainprogram->openfilesshelf ||
		   mainprogram->openclipfiles || mainprogram->multistage != 0 || !mainprogram->paths.empty();
}

static void handle_queue(Layer *lay, int set, const std::vector<std::string> &t, size_t i, const OscMsg &m) {
	// The clip queue of a layer: lay->clips, whose last clip is the empty one that new clips are added before.
	// t[i] is the control: show, next, scroll, beats, or a clip number followed by load or delete.
	if (!lay->clips) return;
	std::vector<Clip*> &clips = *lay->clips;
	const std::string &c = t[i];
	double v;
	if (c == "show") {
		// fold/unfold the queue, as the queue button of the layer
		bool want = arg_num(m, 0, v) ? v != 0.0 : !lay->queueing;
		lay->queueing = want;
		if (want) {
			mainprogram->queueing = true;
		}
		else {
			bool found = false;
			for (std::vector<Layer*> &lvec: mainmix->layers) {
				for (Layer *l: lvec) if (l->queueing) found = true;
			}
			if (!found) mainprogram->queueing = false;
		}
	}
	else if (c == "next" && is_trigger(m)) {
		// play the next clip of the queue, as when the video ends
		if (clips.size() > 1 && !lay->isclone) lay->clip_display_next(0, lay->comp);
	}
	else if (c == "scroll" && arg_num(m, 0, v)) {
		// the first clip of the queue that is shown, from 1
		int maxscroll = std::max(0, (int)clips.size() - 4);
		lay->queuescroll = std::clamp((int)v - 1, 0, maxscroll);
	}
	else if (c == "position" && arg_num(m, 0, v)) {
		// scrub in the loop bar of the queue, as dragging in it does: 0 is the loop start, 1 the loop end
		if (lay->numf > 0) {
			lay->scrubframe = lay->startframe->value + (float)std::clamp(v, 0.0, 1.0) * (lay->endframe->value - lay->startframe->value);
			lay->frame = lay->scrubframe;
			lay->set_clones();
		}
	}
	else if (c == "beats" && arg_num(m, 0, v)) {
		// beat switching of the queue every v beats (0.5, 1, 2, 4, ...); 0 turns it off
		if (v <= 0.0) {
			lay->beats = 0;
			lay->beatdetbut->value = false;
		}
		else {
			lay->beats = (float)v;
			lay->beatdet_group = -1;
			lay->beatdetbut->value = true;
		}
	}
	else if (i + 1 < t.size()) {
		int n;
		if (!parse_int(c, n) || n < 1 || n > (int)clips.size()) return;
		if (t[i + 1] == "delete" && is_trigger(m)) {
			// the last clip is the empty one at the end of the queue and can't be deleted
			if (n < (int)clips.size()) {
				Clip *clip = clips[n - 1];
				clips.erase(clips.begin() + (n - 1));
				if (mainprogram->clipfilesclip == clip) mainprogram->clipfilesclip = clips[std::min(n - 1, (int)clips.size() - 1)];
				if (mainmix->mouseclip == clip) mainmix->mouseclip = clips.back();
				delete clip;
				lay->queuescroll = std::clamp(lay->queuescroll, 0, std::max(0, (int)clips.size() - 4));
			}
		}
		else if (t[i + 1] == "load") {
			// load a file into the queue place, as the "Open file(s)" entry of the clip menu does.  The last place is
			// the empty one: the file is added to the end of the queue.
			std::string path;
			if (!arg_str(m, 0, path)) return;
			if (files_busy()) {
				printf("OSC: busy opening files, queue load ignored: %s\n", path.c_str());
				return;
			}
			std::error_code ec;
			if (path.size() < 5 || !std::filesystem::exists(path, ec)) {
				printf("OSC: file not found: %s\n", path.c_str());
				return;
			}
			mainprogram->paths.push_back(path);
			mainprogram->clipfilescount = 0;
			mainprogram->clipfilesclip = clips[n - 1];
			mainprogram->clipfileslay = lay;
			lay->cliploading = true;
			mainprogram->openclipfiles = true;
		}
	}
}

static void set_layer_source(Layer *lay, const OscMsg &m) {
	// make the layer a source plugin layer (FFGL or ISF generator), by the name in the source plugin menu, with the
	// numbered name for equal names, as in "noise_2".  As the SOURCE_PLUGIN entry of the layer menu.
	std::string name;
	if (!arg_str(m, 0, name)) return;
	std::string key = sanitize(name);
	for (size_t i = 0; i < mainprogram->ffglsourcedisplay.size(); i++) {
		if (sanitize(mainprogram->ffglsourcedisplay[i]) == key) {
			lay->set_ffglsource((int)i);
			goto done;
		}
	}
	for (size_t i = 0; i < mainprogram->isfsourcedisplay.size(); i++) {
		if (sanitize(mainprogram->isfsourcedisplay[i]) == key) {
			lay->set_isfsource((int)i);
			goto done;
		}
	}
	return;
done:
	if (lay->ndisource != nullptr) {
		lay->ndisource->releaseReference();
		lay->ndisource = nullptr;
	}
}

static void set_named_param(const std::vector<Param*> &pars, const std::vector<std::string> &t, size_t i, const OscMsg &m, int set) {
	// t[i] is the parameter name (lowercased, non-alphanumerics as '_') or "param" followed by its number (from 1)
	if (i >= t.size()) return;
	if (t[i] == "param" && i + 1 < t.size()) {
		int j;
		if (parse_int(t[i + 1], j) && j >= 1 && j <= (int)pars.size() && pars[j - 1]) set_param_msg(pars[j - 1], m, set);
		return;
	}
	for (Param *par: pars) {
		if (par && !par->colslave && param_osc_name(par) == t[i]) {
			set_param_msg(par, m, set);
			return;
		}
	}
}

static std::vector<Param*> mixer_params(BlendNode *bn) {
	// the parameters of the FFGL or ISF mixer plugin of a layer, as shown on the layer.  The first parameter of an ISF
	// wipe mixer (PROGRESS) is driven by the mix factor and not listed.
	std::vector<Param*> pars;
	if (!bn) return pars;
	if (bn->ffglmixernr != -1) {
		pars = bn->ffglparams;
	}
	else if (bn->isfmixernr != -1) {
		pars = bn->isfparams;
		if (mainprogram->is_isfwipemixer(bn->isfmixernr) && !pars.empty()) pars.erase(pars.begin());
	}
	return pars;
}

static std::vector<Param*> mainwipe_params(int set) {
	// the parameters of the ISF wipe used as main mix wipe (shared by the preview and output wipe), without PROGRESS
	std::vector<Param*> pars;
	if (mainmix->mixwipeisf[set] == -1) return pars;
	pars = mainmix->isfparams;
	if (!pars.empty()) pars.erase(pars.begin());
	return pars;
}

static std::vector<Param*> *source_params(Layer *lay) {
	if (lay->ffglsourcenr != -1) return &lay->ffglparams;
	if (lay->isfsourcenr != -1) return &lay->isfparams;
	return nullptr;
}

static void handle_layer(Layer *lay, int set, const std::vector<std::string> &t, size_t i, const OscMsg &m);

static bool on_screen_ok(int set) {
	// mask editing, like the effect stack functions, works on the modus that is on screen
	if (set != !mainprogram->prevmodus) {
		printf("OSC: this only works on the set that is on screen\n");
		return false;
	}
	return true;
}

static void add_layer_button(Layer *lay, int set, bool before) {
	// The "+" buttons of a layer: add a new layer behind this one, or (the extra "+" left of the first layer) in front of
	// it.  The code is that of the buttons in Layer::display(), and so is the scrolling to the new layer.  A new mask
	// layer is a SOLID COLOR source layer, and only exists while its deck is in mask edit mode.
	if (!on_screen_ok(set)) return;
	if (before && lay->pos != 0) {
		printf("OSC: addbefore only works for the first layer of a stack\n");
		return;
	}
	int pos = before ? lay->pos : lay->pos + 1;
	Layer *nlay = nullptr;
	int *scrollpos = nullptr;
	if (lay->ismask) {
		std::vector<Layer*> *masks = nullptr;
		if (mainmix->editedmaskeff[lay->comp][lay->deck]) masks = &mainmix->editedmaskeff[lay->comp][lay->deck]->masks;
		else if (lay->parentlayer) masks = &lay->parentlayer->masks;
		if (!masks) return;
		std::string sourcename = "SOLID COLOR";
		int isfnr = std::find(mainprogram->isfsourcenames.begin(), mainprogram->isfsourcenames.end(), sourcename) - mainprogram->isfsourcenames.begin();
		if (isfnr == (int)mainprogram->isfsourcenames.size()) {
			printf("OSC: the SOLID COLOR source plugin is needed to make a mask\n");
			return;
		}
		nlay = mainmix->add_layer(*masks, pos);
		mainmix->reconnect_all(*masks);
		nlay->set_isfsource(isfnr);
		nlay->isfparams[0]->colvalue[1] = 1.0f;
		nlay->isfparams[0]->colvalue[2] = 1.0f;
		nlay->deck = lay->deck;
		nlay->genmidibut->value = 0;
		nlay->ismask = true;
		nlay->parentlayer = lay->parentlayer;
		scrollpos = lay->parenteffect ? &lay->parenteffect->maskscrollpos : &lay->parentlayer->maskscrollpos;
	}
	else {
		nlay = mainmix->add_layer(*lay->layers, pos);
		scrollpos = &mainmix->scenes[lay->deck][mainmix->currscene[lay->deck]]->scrollpos;
	}
	if (!before && scrollpos && nlay->pos > *scrollpos + 2) *scrollpos = nlay->pos - 2;	// layer at end: scroll
}

static Layer *new_mask_layer(Layer *parent, Effect *parenteff, std::vector<Layer*> &masks, int deck) {
	// the first mask of a layer or effect: a layer with the SOLID COLOR source plugin, as the edit mask buttons make it
	std::string sourcename = "SOLID COLOR";
	int isfnr = std::find(mainprogram->isfsourcenames.begin(), mainprogram->isfsourcenames.end(), sourcename) - mainprogram->isfsourcenames.begin();
	if (isfnr == (int)mainprogram->isfsourcenames.size()) {
		printf("OSC: the SOLID COLOR source plugin is needed to make a mask\n");
		return nullptr;
	}
	Layer *masklay = mainmix->add_layer(masks, masks.size());
	masklay->set_isfsource(isfnr);
	masklay->isfparams[0]->colvalue[1] = 1.0f;
	masklay->isfparams[0]->colvalue[2] = 1.0f;
	masklay->deck = deck;
	masklay->genmidibut->value = 0;
	masklay->ismask = true;
	masklay->parentlayer = parent;
	if (parenteff) masklay->parenteffect = parenteff;
	return masklay;
}

static void enter_layer_maskedit(Layer *lay) {
	// the EM button of a layer: go into mask edit mode for the masks of the layer, making the first mask when it has none
	if (lay->masks.empty()) {
		Layer *masklay = new_mask_layer(lay, nullptr, lay->masks, lay->deck);
		if (!masklay) return;
		mainmix->currlay[lay->comp] = masklay;
		mainmix->currlays[lay->comp] = {masklay};
	}
	else {
		mainmix->currlay[lay->comp] = lay->masks[0];
		mainmix->currlays[lay->comp] = {lay->masks[0]};
	}
	mainmix->editedmask[lay->comp][lay->deck] = lay;
	mainmix->editedmaskeff[lay->comp][lay->deck] = nullptr;
	lay->laymasked->value = 1;
	lay->laymasked->oldvalue = 1;
	lay->masked = true;
}

static void enter_effect_maskedit(Layer *lay, Effect *eff) {
	// the E button of an effect: the same for the masks of an effect.  The button isn't there when the clip queue of
	// the layer would change the effects.
	if (lay->clips && lay->clips->size() > 1 && lay->keepeffbut->value == 0) return;
	if (eff->masks.empty()) {
		Layer *masklay = new_mask_layer(lay, eff, eff->masks, lay->deck);
		if (!masklay) return;
		mainmix->currlay[lay->comp] = masklay;
		mainmix->currlays[lay->comp] = {masklay};
	}
	else {
		mainmix->currlay[lay->comp] = eff->masks[0];
		mainmix->currlays[lay->comp] = {eff->masks[0]};
	}
	mainmix->editedmask[lay->comp][lay->deck] = lay;
	mainmix->editedmaskeff[lay->comp][lay->deck] = eff;
	eff->masked = true;
	eff->maskbutton->value = true;
	eff->maskbutton->oldvalue = true;
}

static void mask_up(int set, int deck) {
	// go up a level in the mask hierarchy, as the arrow under "Mask edit" does.  Out of the masks of a normal layer
	// (or of an effect of it) means leaving mask edit mode; out of the masks of a mask goes to the masks it sits in.
	Layer *cur = mainmix->editedmask[set][deck];
	if (!cur) return;
	mainmix->currlay[set] = cur->parentlayer;
	mainmix->currlays[set] = {mainmix->currlay[set]};
	if (cur == cur->parentlayer) {
		mainmix->editedmask[set][deck] = nullptr;
		mainmix->editedmaskeff[set][deck] = nullptr;
	}
	else {
		mainmix->editedmaskeff[set][deck] = cur->parenteffect;
		mainmix->editedmask[set][deck] = cur->parentlayer;
	}
}

extern void create_ndi_submenu();	// program.cpp

static void ndi_refresh(Layer *forlay) {
	// look which NDI sources there are now (Program::ndisourcenames).  The outputs of this program itself are left out
	// for the layer given, as the menu does for the layer it is opened for.
	Layer *bumouselayer = mainmix->mouselayer;
	mainmix->mouselayer = forlay;
	create_ndi_submenu();
	mainmix->mouselayer = bumouselayer;
}

static void ndi_send_sources() {
	// tell the controller which NDI sources there are: /ndi/sources with the names as string arguments
	if (!osc_target) return;
	lo_message msg = lo_message_new();
	for (const std::string &name: mainprogram->ndisourcenames) lo_message_add_string(msg, name.c_str());
	lo_send_message(osc_target, "/ndi/sources", msg);
	lo_message_free(msg);
}

static void set_layer_ndi(Layer *lay, const OscMsg &m) {
	// connect the layer to an NDI source by name (as shown in the NDI source menu): set_ndi() takes the number of the
	// source in Program::ndisourcenames from the menu results
	std::string name;
	if (!arg_str(m, 0, name)) return;
	std::string key = sanitize(name);
	int idx = -1;
	for (int pass = 0; pass < 2 && idx == -1; pass++) {
		if (pass == 1) ndi_refresh(lay);	// not known (yet): look again
		for (size_t i = 0; i < mainprogram->ndisourcenames.size(); i++) {
			if (mainprogram->ndisourcenames[i] == name || sanitize(mainprogram->ndisourcenames[i]) == key) {
				idx = (int)i;
				break;
			}
		}
	}
	if (idx == -1) {
		printf("OSC: NDI source not found: %s\n", name.c_str());
		return;
	}
	std::vector<int> bumenuresults = mainprogram->menuresults;
	mainprogram->menuresults = {idx};
	set_ndi(lay);
	mainprogram->menuresults = bumenuresults;
}

extern void get_cameras();	// program.cpp

static std::vector<std::string> webcam_names() {
	// the live devices as listed in the "Connect live to:" menu, after a fresh look
	get_cameras();
	std::vector<std::string> names;
	for (const std::wstring &w: mainprogram->livedevices) names.push_back(std::string(w.begin(), w.end()));
	return names;
}

static void webcam_send_sources() {
	// tell the controller which webcams there are: /webcam/sources with the names as string arguments
	if (!osc_target) return;
	lo_message msg = lo_message_new();
	for (const std::string &name: webcam_names()) lo_message_add_string(msg, name.c_str());
	lo_send_message(osc_target, "/webcam/sources", msg);
	lo_message_free(msg);
}

static void set_layer_webcam(Layer *lay, const OscMsg &m) {
	// connect the layer to a webcam by name or by 1-based number in the list, as the "Connect live to:" layer menu does
	std::vector<std::string> names = webcam_names();
	std::string name;
	double v;
	int idx = -1;
	if (arg_str(m, 0, name)) {
		std::string key = sanitize(name);
		for (size_t i = 0; i < names.size(); i++) {
			if (names[i] == name || sanitize(names[i]) == key) {
				idx = (int)i;
				break;
			}
		}
	}
	else if (arg_num(m, 0, v) && v >= 1.0 && v <= (double)names.size()) idx = (int)v - 1;
	if (idx == -1) {
		printf("OSC: webcam not found\n");
		return;
	}
#ifdef WINDOWS
	std::string livename = "video=" + names[idx];
#elif defined(LINUX) || defined(MACOS)
	std::string livename = mainprogram->devvideomap[names[idx]];
#else
	std::string livename;
#endif
	if (livename.empty()) return;
	lay->set_live_base(livename);
}

static void set_layer_ndioutput(Layer *lay, const OscMsg &m) {
	// send the layer out as an NDI stream, as the NDI output entry of the layer menu: it switches the output on and off
	double v;
	bool want = arg_num(m, 0, v) ? v != 0.0 : lay->ndioutput == nullptr;
	if (want == (lay->ndioutput != nullptr)) return;
	if (want) {
		int count = 1;
		std::string name;
		while (1) {
			if (!mainprogram->takennumbers.contains(count)) {
				mainprogram->takennumbers.emplace(count);
				name = "EWOCvj2 - Layer " + std::to_string(count);
				break;
			}
			count++;
		}
		lay->ndioutput = mainprogram->ndimanager.createOutput(name, mainprogram->ow[!mainprogram->prevmodus],
															  mainprogram->oh[!mainprogram->prevmodus], 30.0f);
		lay->ndioutput->number = count;
		lay->ndiaspected = false;
		lay->ndioutput->startStream();
	}
	else {
		lay->ndioutput->stopStream();
		lay->ndioutput = nullptr;
	}
}

// Full screen display: Program::fullscreen is -1 (off), the number of the monitor shown (0 deck A, 1 deck B, 2 the mix
// monitor in preview modus, 3 the output mix monitor / the mix monitor in live modus) or 5 for a layer, which is
// Program::fullscreenlay.  There is one full screen display, a new one replaces the one that is on.  The program stops it
// with a click or Escape.
static int monitor_fullscreen_code(const std::string &which) {
	if (which == "deckA") return 0;
	if (which == "deckB") return 1;
	if (which == "mix") return mainprogram->prevmodus ? 2 : 3;
	if (which == "output") return mainprogram->prevmodus ? 3 : -1;	// the output monitor is only shown in preview modus
	return -1;
}

static void set_fullscreen(int code, Layer *lay, const OscMsg &m) {
	bool on = mainprogram->fullscreen == code && mainprogram->fullscreenlay == lay;
	double v;
	bool want = arg_num(m, 0, v) ? v != 0.0 : !on;
	if (want == on) return;
	if (want) {
		mainprogram->fullscreen = code;
		mainprogram->fullscreenlay = lay;
	}
	else {
		mainprogram->fullscreen = -1;
		mainprogram->fullscreenlay = nullptr;
	}
}

static MixNode *monitor_node(const std::string &which) {
	// the mix node of one of the monitors: deckA, deckB, mix (the mix monitor of the modus that is on screen) or output (the
	// output mix monitor, which is shown in preview modus); nullptr when there is no such monitor
	if (which == "deckA") return mainprogram->nodesmain->mixnodes[!mainprogram->prevmodus][0];
	if (which == "deckB") return mainprogram->nodesmain->mixnodes[!mainprogram->prevmodus][1];
	if (which == "mix") return mainprogram->nodesmain->mixnodes[!mainprogram->prevmodus][2];
	if (which == "output" && mainprogram->prevmodus) return mainprogram->nodesmain->mixnodes[1][2];
	return nullptr;
}

static void set_monitor_aistyle(const std::string &which, const OscMsg &m) {
	// The "AI STYLE" entry of the monitor menu: run an AI style over the picture of a monitor (and so over what is sent
	// from it, to outputs for instance).  "none" or "off" switches it off.
	MixNode *mnode = monitor_node(which);
	std::string name;
	if (!mnode || !arg_str(m, 0, name)) return;
	std::string key = sanitize(name);
	if (key == "none" || key == "off") {
		Program::set_mixnode_aistyle(mnode, -1);
		return;
	}
	for (size_t i = 0; i < mainprogram->aistylenames.size(); i++) {
		if (sanitize(mainprogram->aistylenames[i]) == key) {
			if (mnode->aistylnr == (int)i && mnode->aieffect) return;	// that style is on already
			Program::set_mixnode_aistyle(mnode, (int)i);
			return;
		}
	}
}

static void set_monitor_display(const std::string &which, const OscMsg &m) {
	// The "Show on display" entry of the monitor menu: the picture of a monitor fullscreen on another display, number
	// from 1 in the list of the other displays; the same again closes it.  The menu code is run for the monitor, it
	// takes the monitor from monitormenu->value (as the monitor menu sets it when it is opened) and the display from
	// menuresults.
	double v;
	if (!arg_num(m, 0, v) || v < 1.0) return;
	MixNode *mnode = monitor_node(which);
	if (!mnode) return;
	int value = which == "deckA" ? 0 : which == "deckB" ? 1 : which == "mix" ? (mainprogram->prevmodus ? 2 : 3) : 3;
	int bumonitorvalue = mainprogram->monitormenu->value;
	Node *bumousenode = mainmix->mousenode;
	std::vector<int> bumenuresults = mainprogram->menuresults;
	mainprogram->monitormenu->value = value;
	mainmix->mousenode = mnode;
	mainprogram->menuresults.clear();
	mainprogram->menuresults.push_back((int)v - 1);
	mainprogram->handle_monitormenu(3);
	mainprogram->menuresults = bumenuresults;
	mainmix->mousenode = bumousenode;
	mainprogram->monitormenu->value = bumonitorvalue;
}

static void set_monitor_ndioutput(const std::string &which, const OscMsg &m) {
	// NDI output of one of the monitors, as the NDI entry of the monitor menu: deckA, deckB, mix (the mix monitor of the
	// modus that is on screen) or output (the output mix monitor, which is shown in preview modus)
	int i;
	if (which == "deckA") i = 0;
	else if (which == "deckB") i = 1;
	else if (which == "mix") i = 2;
	else if (which == "output") i = 3;
	else return;
	if (i == 3 && !mainprogram->prevmodus) return;
	MixNode *mnode = (i == 3) ? mainprogram->nodesmain->mixnodes[1][2] : mainprogram->nodesmain->mixnodes[!mainprogram->prevmodus][i];
	double v;
	bool want = arg_num(m, 0, v) ? v != 0.0 : mnode->ndioutput == nullptr;
	if (want == (mnode->ndioutput != nullptr)) return;
	if (want) {
		// the menu value of the monitor decides the name and the size
		int value = i;
		if (!mainprogram->prevmodus && i == 2) value = 3;
		std::string name;
		if (value == 3) {
			name = "EWOCvj2 - Main Output (Preview)";
		}
		else if (value == 2) {
			name = mainprogram->prevmodus ? "EWOCvj2 - Preview Output" : "EWOCvj2 - Main Output";
		}
		else {
			std::string deckstr = value == 0 ? "A" : "B";
			name = (mainprogram->prevmodus ? "EWOCvj2 - Preview Monitor " : "EWOCvj2 - Main Monitor ") + deckstr;
		}
		int w = value == 3 ? mainprogram->ow[1] : mainprogram->ow[!mainprogram->prevmodus];
		int h = value == 3 ? mainprogram->oh[1] : mainprogram->oh[!mainprogram->prevmodus];
		mnode->ndioutput = mainprogram->ndimanager.createOutput(name, w, h, 30.0f);
		mnode->ndioutput->startStream();
	}
	else {
		mnode->ndioutput->stopStream();
		mnode->ndioutput = nullptr;
	}
}

static bool resolve_effect(const OscMsg &m, size_t k, EFFECT_TYPE &type, int &ffglnr, int &isfnr, int &aistylnr, bool onlystyle = false) {
	// the effect given by its name in the effect menu (lower case, non-alphanumerics as '_', equal plugin names are
	// numbered there: "emboss_2") or the name of an AI style; with onlystyle only the AI styles are looked at
	std::string name;
	if (!arg_str(m, k, name)) return false;
	std::string key = sanitize(name);
	ffglnr = isfnr = aistylnr = -1;
	for (int code: mainprogram->abeffects) {
		if (onlystyle) break;
		const std::string *shown = nullptr;
		if (code >= 2000) {
			if (code - 2000 < (int)mainprogram->isfeffectdisplay.size()) shown = &mainprogram->isfeffectdisplay[code - 2000];
		}
		else if (code >= 1000) {
			if (code - 1000 < (int)mainprogram->ffgleffectdisplay.size()) shown = &mainprogram->ffgleffectdisplay[code - 1000];
		}
		else if (code >= 0 && code < (int)mainprogram->builtineffectdisplay.size()) {
			shown = &mainprogram->builtineffectdisplay[code];
		}
		if (shown && sanitize(*shown) == key) {
			type = (EFFECT_TYPE)code;
			if (code >= 2000) isfnr = code - 2000;
			else if (code >= 1000) ffglnr = code - 1000;
			return true;
		}
	}
	for (size_t i = 0; i < mainprogram->aistylenames.size(); i++) {
		if (sanitize(mainprogram->aistylenames[i]) == key) {
			type = (EFFECT_TYPE)(3000 + i);
			aistylnr = (int)i;
			return true;
		}
	}
	return false;
}

struct EffectChainGuard {
	// Layer::add_effect(), replace_effect() and delete_effect() work on the effect chain that the deck of the layer
	// is showing (mainprogram->effcat), so show the wanted chain while they run
	int deck;
	int saved;
	EffectChainGuard(Layer *lay, int cat) {
		deck = lay->deck;
		saved = mainprogram->effcat[deck]->value;
		mainprogram->effcat[deck]->value = cat;
	}
	~EffectChainGuard() {
		mainprogram->effcat[deck]->value = saved;
	}
};

static bool effect_stack_ok(Layer *lay, int set, int cat) {
	// the effect stack functions use the modus that is on screen, and the first layer of a deck has no second chain
	if (set != !mainprogram->prevmodus) {
		printf("OSC: effects can only be changed on the set that is on screen\n");
		return false;
	}
	return !(cat == 1 && lay->pos == 0);
}

static void handle_effect(Layer *lay, int set, int cat, const std::vector<std::string> &t, size_t i, const OscMsg &m) {
	// t[i] is "add" (the effect is chosen by name, as in the effect menu), or the place K of an effect in the chain (from
	// 1; the same effect can be in a chain several times, so existing effects are always given by their place).  The
	// control follows: t[ci...]
	if (i >= t.size()) return;
	std::vector<Effect*> &effs = lay->effects[cat];
	double v;
	if (t[i] == "add" || t[i] == "addstyle") {
		// effect/add  s name [, i place]: insert an effect at a place (from 1) in the chain, default at the end;
		// effect/addstyle does the same for an AI style (the "AI STYLE" entry of the effect menu)
		EFFECT_TYPE type;
		int ffglnr, isfnr, aistylnr;
		if (!effect_stack_ok(lay, set, cat) || !resolve_effect(m, 0, type, ffglnr, isfnr, aistylnr, t[i] == "addstyle")) return;
		int pos = (int)effs.size();
		if (arg_num(m, 1, v)) pos = std::clamp((int)v - 1, 0, pos);
		EffectChainGuard guard(lay, cat);
		lay->add_effect(type, pos, cat, ffglnr, isfnr, aistylnr);
		return;
	}
	int k;
	if (i + 1 >= t.size() || !parse_int(t[i], k) || k < 1 || k > (int)effs.size()) return;
	Effect *eff = effs[k - 1];
	size_t ci = i + 1;
	if (t[ci] == "maskedit") {
		// enter mask edit mode for the masks of the effect
		if (is_trigger(m) && on_screen_ok(set)) enter_effect_maskedit(lay, eff);
		return;
	}
	if (t[ci] == "mask" && ci + 2 < t.size()) {
		// the controls of a mask layer of the effect: mask/M/<layer control>, M from 1
		int n;
		if (parse_int(t[ci + 1], n) && n >= 1 && n <= (int)eff->masks.size()) handle_layer(eff->masks[n - 1], set, t, ci + 2, m);
		return;
	}
	if (t[ci] == "delete") {
		if (is_trigger(m) && effect_stack_ok(lay, set, cat)) {
			EffectChainGuard guard(lay, cat);
			lay->delete_effect(k - 1);
		}
		return;
	}
	if (t[ci] == "replace" || t[ci] == "replacestyle") {
		// effect/K/replace  s name: put another effect in this place; replacestyle for an AI style
		EFFECT_TYPE type;
		int ffglnr, isfnr, aistylnr;
		if (!effect_stack_ok(lay, set, cat) || !resolve_effect(m, 0, type, ffglnr, isfnr, aistylnr, t[ci] == "replacestyle")) return;
		EffectChainGuard guard(lay, cat);
		lay->replace_effect(type, k - 1, ffglnr, isfnr, aistylnr);
		return;
	}
	if (!arg_num(m, 0, v)) {
		// a hex string sets a colour parameter
		if (m.args.size() && (m.args[0].t == 's' || m.args[0].t == 'S')) set_named_param(eff->params, t, ci, m, set);
		return;
	}
	const std::string &ctl = t[ci];
	if (ctl == "masked") {
		// whether the mask influences the effect (the M button of the effect)
		eff->masked = v != 0.0;
		eff->maskbutton->value = eff->masked;
		eff->maskbutton->oldvalue = eff->masked;
		return;
	}
	if (ctl == "maskscroll") {
		// the scroll position of the mask stack of the effect: the first shown mask (from 1)
		eff->maskscrollpos = std::clamp((int)v - 1, 0, std::max(0, (int)eff->masks.size() - 1));
		return;
	}
	if (ctl == "onoff") {
		eff->onoffbutton->value = v != 0.0 ? 1 : 0;
		button_changed(eff->onoffbutton, set);
	}
	else if (ctl == "drywet") {
		set_param_normalized(eff->drywet, v, set);
	}
	else if (ctl == "param" && ci + 1 < t.size()) {
		int j;
		if (parse_int(t[ci + 1], j) && j >= 1 && j <= (int)eff->params.size()) set_param_msg(eff->params[j - 1], m, set);
	}
	else {
		for (Param *par: eff->params) {
			if (!par->colslave && param_osc_name(par) == ctl) {
				set_param_msg(par, m, set);
				break;
			}
		}
	}
}

static void handle_layer(Layer *lay, int set, const std::vector<std::string> &t, size_t i, const OscMsg &m) {
	// t[i...] is the control within the layer
	if (i >= t.size()) return;
	const std::string &c = t[i];
	double v;
	if (c == "opacity" && arg_num(m, 0, v)) set_param_normalized(lay->opacity, v, set);
	else if (c == "volume" && arg_num(m, 0, v)) set_param_normalized(lay->volume, v, set);
	else if (c == "speed" && arg_num(m, 0, v)) set_param_raw(lay->speed, v, set);
	else if (c == "scale" && arg_num(m, 0, v)) set_param_raw(lay->scale, v, set);
	else if (c == "shiftx" && arg_num(m, 0, v)) set_param_raw(lay->shiftx, v, set);
	else if (c == "shifty" && arg_num(m, 0, v)) set_param_raw(lay->shifty, v, set);
	else if (c == "position" && arg_num(m, 0, v)) {
		// scrubbing in the loopbox, as dragging in it does (Layer::handle_loopbox): the playhead jumps there, clones follow,
		// the decoder is told to seek and the scrub is recorded into loopstation lines that are armed for recording
		if (lay->numf > 0) {
			lay->scrubframe = (float)(std::clamp(v, 0.0, 1.0) * (lay->numf - 1));
			lay->frame = lay->scrubframe;
			lay->set_clones();
			lay->scritch->value = lay->frame;
			param_changed(lay->scritch, set);
			lay->scritched = true;
			lay->scritchpause = false;
		}
	}
	else if (c == "maskedit") {
		// enter mask edit mode for the masks of the layer
		if (is_trigger(m) && on_screen_ok(set)) enter_layer_maskedit(lay);
	}
	else if (c == "masked" && arg_num(m, 0, v)) {
		// whether the mask influences the layer (the M button of the layer)
		lay->masked = v != 0.0;
		lay->laymasked->value = lay->masked;
		lay->laymasked->oldvalue = lay->masked;
	}
	else if (c == "mask" && i + 2 < t.size()) {
		// the controls of a mask layer of the layer: mask/M/<layer control>, M from 1
		int n;
		if (parse_int(t[i + 1], n) && n >= 1 && n <= (int)lay->masks.size()) handle_layer(lay->masks[n - 1], set, t, i + 2, m);
	}
	else if (c == "maskscroll" && arg_num(m, 0, v)) {
		// the scroll position of the mask stack of the layer: the first shown mask (from 1)
		lay->maskscrollpos = std::clamp((int)v - 1, 0, std::max(0, (int)lay->masks.size() - 1));
	}
	else if ((c == "add" || c == "addbefore") && is_trigger(m)) {
		// the "+" buttons of the layer: a new layer behind this one, or in front of it (first layer of a stack only)
		add_layer_button(lay, set, c == "addbefore");
	}
	else if ((c == "duplicate" || c == "clone") && is_trigger(m)) {
		// "Duplicate layer": a new layer right after this one, with the same content and settings.  "Clone layer":
		// a new layer that is linked to this one (shares its playing).
		if (!on_screen_ok(set)) return;
		layer_menu_entry(lay, c == "duplicate" ? DUP_LAYER : CLONE_LAYER, -1);
	}
	else if (c == "recordreplace") {
		// "Record and replace": record the layer, with its effects and settings, to a HAP video file; the layer is
		// replaced by the recording when that is done.  The recording stops by itself at the end of the video, or
		// when stopped here.  1 or no argument while idle starts, 0 or no argument while recording stops.
		bool recording = (mainmix->reclay == lay && mainmix->recording[0]);
		bool want = arg_num(m, 0, v) ? v != 0.0 : !recording;
		if (want && !recording) {
			if (!on_screen_ok(set)) return;
			if (mainmix->reclay || mainmix->recording[0] || lay->ffglsourcenr != -1 || lay->isfsourcenr != -1 ||
				!lay->clips || lay->clips->size() != 1) {
				printf("OSC: this layer can not be recorded now (busy recording, a source plugin, or a clip queue)\n");
				return;
			}
			layer_menu_entry(lay, REC_REP, -1);
		}
		else if (!want && recording) {
			mainmix->recording[0] = false;
		}
	}
	else if (c == "hapencode" && is_trigger(m)) {
		// "HAP encode on-the-fly": encode the video of the layer to HAP in the background, after which the layer
		// switches to the HAP version.  The menu only offers it for videos that aren't HAP yet.
		if (lay->vidformat == AV_CODEC_ID_HAP || lay->filename == "" || lay->type == ELEM_IMAGE ||
			lay->type == ELEM_LIVE || lay->type == ELEM_NDI || lay->ffglsourcenr != -1 || lay->isfsourcenr != -1) {
			printf("OSC: this layer has no video that can be HAP encoded\n");
		}
		else if (lay->hapbinel) {
			printf("OSC: this layer is already being HAP encoded\n");
		}
		else {
			layer_menu_entry(lay, HAP_ENCODE, -1);
		}
	}
	else if (c == "display" && arg_num(m, 0, v)) {
		// "Show on display": the layer fullscreen on another display, number from 1 in the list of the other displays
		// of the menu; the same number again closes it
		if (v >= 1.0) layer_menu_entry(lay, SHOW_DISPLAY, (int)v - 1);
	}
	else if (c == "save") {
		// save the layer to a layer file
		std::string path;
		if (!arg_str(m, 0, path) || path == "") return;
		if (lay->ismask) printf("OSC: masks can not be saved as layer files\n");
		else queue_file_op("SAVELAYFILE", path, set, lay->deck, lay->pos);
	}
	else if (c == "aspect") set_layer_aspect(lay, m);
	else if (c == "key" && i + 1 < t.size()) handle_key(lay, set, t, i + 1, m);
	else if (c == "fullscreen") set_fullscreen(5, lay, m);
	else if (c == "ndi") set_layer_ndi(lay, m);
	else if (c == "webcam") set_layer_webcam(lay, m);
	else if (c == "ndioutput") set_layer_ndioutput(lay, m);
	else if (c == "mute") {
		// the M button of the layer: the layer is not shown while it is on.  0/1, without argument it switches
		bool on = arg_num(m, 0, v) ? v != 0.0 : !lay->mutebut->value;
		lay->mutebut->value = lay->mutebut->oldvalue = on;
		button_changed(lay->mutebut, set);
	}
	else if (c == "solo") {
		// the S button of the layer: only this layer is shown (all the others of its stack are muted) while it is on
		bool on = arg_num(m, 0, v) ? v != 0.0 : !lay->solobut->value;
		lay->solobut->value = lay->solobut->oldvalue = on;
		if (lay->layers) {
			std::vector<Layer*> &lvec = *lay->layers;
			if (on) lay->mutebut->value = lay->mutebut->oldvalue = false;
			for (int k = 0; k < (int)lvec.size(); k++) {
				if (lvec[k] == lay) continue;
				lvec[k]->mutebut->value = lvec[k]->mutebut->oldvalue = on;
				if (on) lvec[k]->solobut->value = lvec[k]->solobut->oldvalue = false;
			}
		}
		button_changed(lay->solobut, set);
	}
	else if (c == "lockspeed") {
		// the "lock" entry of the speed menu: the speed of the layer stays when another clip is loaded in it
		lay->lockspeed = arg_num(m, 0, v) ? v != 0.0 : !lay->lockspeed;
	}
	else if (c == "lockzoompan") {
		// the "lock zoom and pan" entry of the zoom/pan menu: the scale and the shift of the layer stay when another
		// clip is loaded in it
		lay->lockzoompan = arg_num(m, 0, v) ? v != 0.0 : !lay->lockzoompan;
	}
	else if (c == "keepeffects") {
		// the E button of the layer: the effects stay on the layer when another clip is loaded into it
		lay->keepeffbut->value = arg_num(m, 0, v) ? v != 0.0 : !lay->keepeffbut->value;
	}
	else if (c == "keepmask") {
		// the K button of the layer: the masks stay on the layer when another clip is loaded into it
		lay->keepmaskbut->value = arg_num(m, 0, v) ? v != 0.0 : !lay->keepmaskbut->value;
	}
	else if (c == "select" && is_trigger(m)) set_current_layers(set, {lay});
	else if (c == "fxchain") {
		// which of the two effect chains (categories) of the layer is shown, the effect chain switch of the layer
		// stack: 0 "Layer effects", 1 "Stream effects", given as number or name.  The first layer of a deck only has
		// the first chain.
		std::string cname;
		if (arg_str(m, 0, cname)) {
			cname = sanitize(cname);
			if (cname == "layer" || cname == "layer_effects") v = 0.0;
			else if (cname == "stream" || cname == "stream_effects") v = 1.0;
			else return;
		}
		else if (!arg_num(m, 0, v)) return;
		int chain = (v != 0.0 && lay->pos != 0) ? 1 : 0;
		lay->effcat = chain;
		if (mainmix->currlay[set] == lay) mainprogram->effcat[lay->deck]->value = chain;
	}
	else if (c == "loopbox" && i + 1 < t.size()) handle_loopbox(lay, set, t, i + 1, m);
	else if (c == "loopbeats" && arg_num(m, 0, v)) {
		// beatmatching: the video loop is fitted to v beats (0.5, 1, 2, 4, ...) by adapting the speed; 0 turns it off
		// and puts back the speed from before.  As the beatmatching entry of the layer menu.
		if (v <= 0.0) {
			if (lay->loopbeats != 0) lay->speed->value = lay->buspeed;
			lay->loopbeats = 0;
		}
		else {
			if (lay->loopbeats == 0) lay->buspeed = lay->speed->value;
			lay->loopbeats = (float)v;
		}
		// force a new baseline so the new interval takes effect at once
		lay->loopbeat_group = -1;
		param_changed(lay->speed, set);
	}
	else if (c == "play") set_play_exclusive(lay, lay->playbut, set, m);
	else if (c == "reverse") set_play_exclusive(lay, lay->revbut, set, m);
	else if (c == "bounce") {
		set_play_exclusive(lay, lay->bouncebut, set, m);
	}
	else if (c == "loop") {
		lay->lpbut->value = arg_num(m, 0, v) ? (v != 0.0) : !lay->lpbut->value;
		button_changed(lay->lpbut, set);
	}
	else if (c == "stop" && is_trigger(m)) {
		lay->playbut->value = false;
		lay->revbut->value = false;
		lay->bouncebut->value = false;
		button_changed(lay->stopbut, set);
	}
	else if (c == "pause" && is_trigger(m)) layer_pause(lay, set);
	else if (c == "frame" && i + 1 < t.size() && is_trigger(m) && lay->numf > 0) {
		float f = lay->frame;
		if (t[i + 1] == "forward") { f += 1; if (f >= lay->numf) f = 0; lay->frame = f; }
		else if (t[i + 1] == "backward") { f -= 1; if (f < 0) f = (float)(lay->numf - 1); lay->frame = f; }
	}
	else if (c == "load") {
		std::string path;
		if (arg_str(m, 0, path)) layer_load(lay, set, path);
	}
	else if (c == "queue" && i + 1 < t.size()) handle_queue(lay, set, t, i + 1, m);
	else if (c == "source") {
		if (i + 1 == t.size()) {
			set_layer_source(lay, m);
		}
		else if (m.args.size()) {
			// parameters of the source plugin: by name, or param/J
			std::vector<Param*> *pars = source_params(lay);
			if (!pars) return;
			if (t[i + 1] == "param" && i + 2 < t.size()) {
				int j;
				if (parse_int(t[i + 2], j) && j >= 1 && j <= (int)pars->size() && (*pars)[j - 1]) set_param_msg((*pars)[j - 1], m, set);
			}
			else {
				for (Param *par: *pars) {
					if (par && !par->colslave && param_osc_name(par) == t[i + 1]) {
						set_param_msg(par, m, set);
						break;
					}
				}
			}
		}
	}
	else if (c == "mixer" && i + 1 < t.size() && m.args.size()) {
		// parameters of the FFGL/ISF mixer plugin that is the mix mode (or wipe) of the layer
		set_named_param(mixer_params(lay->blendnode), t, i + 1, m, set);
	}
	else if (c == "upscale") {
		// Lanczos3 + RCAS upscaling on/off, no argument toggles
		lay->upscale->value = arg_num(m, 0, v) ? (v != 0.0 ? 1.0f : 0.0f) : (lay->upscale->value != 0.0f ? 0.0f : 1.0f);
		param_changed(lay->upscale, set);
	}
	else if (c == "sharpness" && arg_num(m, 0, v)) set_param_normalized(lay->rcassharpness, v, set);
	else if (c == "mixmode") set_layer_mixmode(lay, m);
	else if (c == "mixfactor" && arg_num(m, 0, v) && lay->blendnode && lay->blendnode->mixfac) {
		set_param_normalized(lay->blendnode->mixfac, v, set);
	}
	else if (c == "wipe" && i + 1 < t.size() && lay->blendnode) {
		BlendNode *bn = lay->blendnode;
		if (t[i + 1] == "type") set_layer_wipe(lay, m);
		else if ((t[i + 1] == "dir" || t[i + 1] == "direction") && arg_num(m, 0, v)) bn->wipedir = std::max((int)v, 0);
		else if (t[i + 1] == "xpos" && arg_num(m, 0, v) && bn->wipex) set_param_normalized(bn->wipex, v, set);
		else if (t[i + 1] == "ypos" && arg_num(m, 0, v) && bn->wipey) set_param_normalized(bn->wipey, v, set);
	}
	else if (c == "genmidi") {
		int preset;
		if (parse_midipreset(m, preset)) {
			// set directly, as the popup menu of the layer's preset button does (and its clones)
			lay->genmidibut->value = preset;
			lay->genmidibut->oldvalue = preset;
			lay->set_clones();
		}
	}
	else if (c == "effect") handle_effect(lay, set, 0, t, i + 1, m);
	else if (c == "effect2") handle_effect(lay, set, 1, t, i + 1, m);
}

// LIVE/PREVIEW modus: 1 = preview modus, 0 = live modus, -1 = nothing requested.  The modus button is handled by
// Program::preview_modus_buttons() (mix room only), which switches prevmodus and does the bookkeeping when the button
// value changes, so the request is carried out by flipping the button there.
static int osc_pendmodus = -1;

static void modus_apply() {
	if (osc_pendmodus < 0) return;
	bool mixroom = !mainprogram->binsroom && !mainmix->retargeting && !mainprogram->styleroom &&
				   !mainprogram->genroom && !mainprogram->segmentationroom;
	if (!mixroom) return;
	Button *but = mainprogram->modusbut;
	if (but->value != but->oldvalue) return;	// a switch is being handled
	if ((int)mainprogram->prevmodus == osc_pendmodus) {
		osc_pendmodus = -1;
		return;
	}
	but->value = !but->value;
}

// scene switches are held until no other scene swap is busy, the last request for a deck wins
static int osc_pendscene[2] = {-1, -1};
static int osc_pendboth = -1;

// SEND to scene buttons: [deck][0] = preview modus to scene, [deck][1] = scene back to the preview modus.  Holds the
// 0-based scene number.
static int osc_pendsend[2][2] = {{-1, -1}, {-1, -1}};

static void scenes_apply() {
	// The SEND buttons are handled by Program::preview_modus_buttons() when it draws them, so a send is requested
	// by pressing the button: it only exists for the scenes a deck is not on, and only works in the mix room.
	bool mixroom = !mainprogram->binsroom && !mainmix->retargeting && !mainprogram->styleroom &&
				   !mainprogram->genroom && !mainprogram->segmentationroom;
	if (mixroom && !mainprogram->swappingscene) {
		bool pressed = false;
		for (int d = 0; d < 2; d++) {
			for (int dir = 0; dir < 2; dir++) {
				int n = osc_pendsend[d][dir];
				if (n < 0) continue;
				osc_pendsend[d][dir] = -1;
				int i = 0;
				for (int j = 0; j < 4; j++) {
					if (j == mainmix->currscene[d]) continue;
					if (j == n) {
						mainprogram->toscene[d][dir][i]->value = 1;
						pressed = true;
						break;
					}
					i++;
				}
			}
		}
		// let the program handle the buttons before switching scenes, which changes which scene is which button
		if (pressed) return;
	}
	if (mainprogram->swappingscene) return;
	if (osc_pendboth >= 0) {
		// same as shift+click on a scene box: both decks go to the scene, deck A's crossfade is adopted
		int i = osc_pendboth;
		osc_pendboth = -1;
		osc_pendscene[0] = osc_pendscene[1] = -1;
		mainprogram->swappingscene = true;
		mainmix->scenes[1][i]->switch_to(true);
		mainmix->currscene[1] = i;
		if (mainmix->currscene[0] == mainmix->currscene[1]) {
			mainmix->scenes[0][mainmix->currscene[0]]->crossfade = mainmix->crossfadecomp->value;
			mainmix->scenes[1][mainmix->currscene[1]]->crossfade = mainmix->crossfadecomp->value;
		}
		Scene *si = mainmix->scenes[0][i];
		mainmix->crossfadecomp->value = si->crossfade;
		si->switch_to(true);
		mainmix->currscene[0] = i;
		mainmix->setscene = -1;
		si->loaded = false;
		return;
	}
	for (int d = 0; d < 2; d++) {
		int i = osc_pendscene[d];
		if (i < 0) continue;
		osc_pendscene[d] = -1;
		if (i == mainmix->currscene[d]) continue;
		Scene *si = mainmix->scenes[d][i];
		mainprogram->swappingscene = true;
		si->switch_to(true);
		mainmix->currscene[d] = i;
		mainmix->setscene = -1;
		si->loaded = false;
		return;		// the other deck follows once this swap has finished
	}
}

static void handle_mix(const std::vector<std::string> &t, const OscMsg &m) {
	// t[0] == "mix"
	if (t.size() < 2) return;
	double v;
	if (t[1] == "crossfade_all" && arg_num(m, 0, v)) {
		v = std::clamp(v, 0.0, 1.0);
		mainmix->crossfade->value = (float)v;
		mainmix->crossfadecomp->value = (float)v;
		param_changed(mainmix->crossfade, 0);
		param_changed(mainmix->crossfadecomp, 1);
		return;
	}
	if (t[1] == "send" && t.size() >= 3 && is_trigger(m)) {
		bool up = t[2] == "up";
		if (!up && t[2] != "down") return;
		bool a = true, b = true;
		if (t.size() > 3) {
			if (t[3] == "a") b = false;
			else if (t[3] == "b") a = false;
			else if (t[3] != "mix") return;
		}
		mainmix->copy_to_comp(a, b, up);
		return;
	}
	if (t[1] == "display" && t.size() == 3) {
		set_monitor_display(t[2], m);
		return;
	}
	if (t[1] == "aistyle" && t.size() == 3) {
		set_monitor_aistyle(t[2], m);
		return;
	}
	if (t[1] == "fullscreen" && t.size() == 3) {
		int code = monitor_fullscreen_code(t[2]);
		if (code >= 0) set_fullscreen(code, nullptr, m);
		return;
	}
	if (t[1] == "ndioutput" && t.size() == 3) {
		set_monitor_ndioutput(t[2], m);
		return;
	}
	if (t[1] == "modus" && t.size() == 2) {
		std::string s;
		int want;
		if (arg_str(m, 0, s) && (sanitize(s) == "preview" || sanitize(s) == "live")) want = sanitize(s) == "preview";
		else if (m.args.size() && arg_num(m, 0, v)) want = v != 0.0;
		else if (!m.args.size()) want = !(osc_pendmodus >= 0 ? osc_pendmodus : (int)mainprogram->prevmodus);
		else return;
		osc_pendmodus = want;
		return;
	}
	if (t[1] == "scene" && arg_num(m, 0, v)) {
		// scene numbers count from 1
		int i = (int)v - 1;
		if (i < 0 || i >= (int)mainmix->scenes[0].size()) return;
		if (t.size() == 2) osc_pendboth = i;
		else if (t[2] == "deckA" || t[2] == "deckB") {
			int d = t[2] == "deckB";
			if (t.size() == 3) osc_pendscene[d] = i;
			else if (t.size() == 5 && t[3] == "send" && (t[4] == "up" || t[4] == "down")) {
				osc_pendsend[d][t[4] == "down"] = i;
			}
		}
		return;
	}
	if (t[1] == "output" && t.size() == 3 && t[2] == "record") {
		bool start;
		if (m.args.size() && arg_num(m, 0, v)) start = v != 0.0;
		else start = !mainmix->recording[1];
		if (start && !mainmix->recording[1]) {
			mainmix->reccodec = "hap";
			mainmix->reckind = 1;
			mainmix->start_recording();
			if (mainmix->recbutQ) mainmix->recbutQ->value = 1;
		}
		else if (!start && mainmix->recording[1]) {
			mainmix->recording[1] = false;
			if (mainmix->recbutQ) mainmix->recbutQ->value = 0;
		}
		return;
	}

	int set;
	if (t[1] == "preview") set = 0;
	else if (t[1] == "output") set = 1;
	else return;
	if (t.size() < 3) return;

	if (t[2] == "crossfade" && arg_num(m, 0, v)) {
		set_param_normalized(set ? mainmix->crossfadecomp : mainmix->crossfade, v, set);
	}
	else if (t[2] == "wipe" && t.size() >= 4) {
		if (t[3] == "type") {
			int nr = -1;
			int isfnr = -1;
			bool found = resolve_wipe(m, nr, isfnr);
			if (found && isfnr != -1) {
				// ISF PROGRESS mixer as wipe, as chosen in the wipe menu
				mainmix->wipe[set] = -1;
				mainmix->set_mixwipeisf(set, isfnr);
				BlendNode dummybnode;
				dummybnode.set_isfmixer(isfnr);
				mainmix->isfparams = dummybnode.isfparams;
			}
			else if (found && nr >= -1 && nr < mainprogram->mixwipebase - 1) {
				mainmix->set_mixwipeisf(set, -1);
				mainmix->wipe[set] = nr;
			}
		}
		else if ((t[3] == "dir" || t[3] == "direction") && arg_num(m, 0, v)) mainmix->wipedir[set] = (int)v;
		else if (t[3] == "xpos" && arg_num(m, 0, v)) set_param_normalized(mainmix->wipex[set], v, set);
		else if (t[3] == "ypos" && arg_num(m, 0, v)) set_param_normalized(mainmix->wipey[set], v, set);
		else if (m.args.size()) {
			// parameters of the ISF wipe, when one is selected
			set_named_param(mainwipe_params(set), t, 3, m, set);
		}
	}
	else if (t[2] == "exchange") {
		exchange_layers(set, m);
	}
	else if (t.size() == 3 && (t[2] == "save" || t[2] == "open")) {
		// /mix/<modus>/save|open  s path: save the whole mix to a mix file, or open a mix file
		std::string path;
		if (arg_str(m, 0, path) && path != "") queue_file_op(t[2] == "save" ? "SAVEMIX" : "OPENMIX", path, set, -1);
	}
	else if (t.size() == 3 && t[2] == "new" && is_trigger(m)) {
		// an empty mix
		if (on_screen_ok(set)) new_deck_or_mix(2);
	}
	else if (t[2] == "current") {
		// /mix/<modus>/current  s layer, ... : the current layers, each given as deck letter and position ("A1", "B3");
		// the last one is the current layer
		std::vector<Layer*> lays;
		for (size_t k = 0; k < m.args.size(); k++) {
			std::string s;
			int n;
			if (!arg_str(m, k, s) || s.size() < 2) continue;
			char dl = (char)std::toupper((unsigned char)s[0]);
			if ((dl != 'A' && dl != 'B') || !parse_int(s.substr(1), n)) continue;
			Layer *lay = find_layer(set, dl == 'B', n);
			if (lay && std::find(lays.begin(), lays.end(), lay) == lays.end()) lays.push_back(lay);
		}
		set_current_layers(set, lays);
	}
	else if (t[2] == "deckA" || t[2] == "deckB") {
		int deck = t[2] == "deckB";
		if (t.size() == 4 && t[3] == "speed" && arg_num(m, 0, v)) {
			set_param_raw(mainmix->deckspeed[set][deck], v, set);
		}
		else if (t.size() == 4 && (t[3] == "save" || t[3] == "open")) {
			// /mix/<modus>/<deck>/save|open  s path: save the deck to a deck file, or open a deck file into the deck
			std::string path;
			if (arg_str(m, 0, path) && path != "") queue_file_op(t[3] == "save" ? "SAVEDECK" : "OPENDECK", path, set, deck);
		}
		else if (t.size() == 4 && t[3] == "new" && is_trigger(m)) {
			// an empty deck
			if (on_screen_ok(set)) new_deck_or_mix(deck);
		}
		else if (t.size() == 4 && t[3] == "maskup" && is_trigger(m)) {
			// one level up in the mask hierarchy of the deck; out of the first level leaves mask edit mode
			mask_up(set, deck);
		}
		else if (t.size() == 4 && t[3] == "scroll" && arg_num(m, 0, v)) {
			// the first layer that is shown of the layer stack of the deck (from 1), within the layers that exist
			int maxscroll = std::max(0, (int)mainmix->layers[set * 2 + deck].size() - 2);
			*stack_scrollpos(set, deck) = std::clamp((int)v - 1, 0, maxscroll);
		}
		else if (t.size() == 4 && t[3] == "genmidi") {
			// the deck's general MIDI preset, applied to all layers of this deck in this set (masks included), set
			// directly as the popup menu of the deck's preset button does
			int preset;
			if (!parse_midipreset(m, preset)) return;
			Button *but = mainmix->genmidi[deck];
			but->value = preset;
			but->oldvalue = preset;
			for (Layer *lay: mainmix->layers[set * 2 + deck]) {
				lay->genmidibut->value = preset;
				lay->genmidibut->oldvalue = preset;
				set_genmidi_recursive(lay, deck);
			}
		}
		else if (t.size() >= 6 && t[3] == "layer") {
			int n;
			if (!parse_int(t[4], n)) return;
			Layer *lay = find_layer(set, deck, n);
			if (lay) handle_layer(lay, set, t, 5, m);
		}
	}
}

static void osc_set_target(const std::string &host, const std::string &port, bool automatic = false) {
	if (osc_target) {
		lo_address_free(osc_target);
		osc_target = nullptr;
	}
	osc_targethost = host;
	osc_target_auto = automatic;
	if (host != "") osc_target = lo_address_new(host.c_str(), port.c_str());
	osc_lastsent.clear();	// the new target needs the complete state
	osc_laststr.clear();
	osc_resync = true;
}

// shelf triggers wait here until the previous one has been taken over by Program::shelf_triggering().  Each entry is
// {side, 0-based element number in the current bank}.
static std::vector<std::pair<int, int>> osc_pendshelf;

static void shelf_apply() {
	if (osc_pendshelf.empty() || mainprogram->midishelfelem) return;
	auto p = osc_pendshelf.front();
	osc_pendshelf.erase(osc_pendshelf.begin());
	ShelfElement *elem = mainprogram->shelves[p.first][mainmix->currbank[p.first]]->elements[p.second];
	if (elem->path == "") return;
	// the same as a MIDI shelf button: the_loop takes midishelfelem over right after this
	mainprogram->midishelfelem = elem;
	Button *but = mainprogram->shelves[p.first][0]->buttons[p.second];
	if (loopstation) {
		for (auto lelem: loopstation->elements) {
			if (lelem->recbut->value) lelem->add_button_automationentry(but);
		}
	}
}

static void handle_loopstation(const std::vector<std::string> &t, const OscMsg &m) {
	// t[0] == "loopstation".  Works on the loopstation on screen (preview in preview modus, else output): any line, in
	// view or not (oscforce makes LoopStationElement::handle() process a line that is scrolled out of view).  The buttons
	// are set the way a MIDI-mapped button sets them, after which LoopStationElement::mouse_handle() sees the change and
	// does the rest.
	LoopStation *ls = loopstation;
	if (!ls || t.size() < 2) return;
	double v;
	int n = ls->elements.size();
	if (t[1] == "scroll" && arg_num(m, 0, v)) {
		// the first line of the list on screen (the list shows 8 lines), from 1
		ls->scrpos = std::clamp((int)v - 1, 0, std::max(0, n - 8));
	}
	else if (t[1] == "current" && arg_num(m, 0, v)) {
		int i = (int)v - 1;
		if (i < 0 || i >= n) return;
		ls->currelem = ls->elements[i];
	}
	else if (t[1] == "line" && t.size() == 4) {
		int i;
		if (!parse_int(t[2], i) || i < 1 || i > n) return;
		LoopStationElement *elem = ls->elements[i - 1];
		const std::string &c = t[3];
		Button *but = nullptr;
		if (c == "rec") but = elem->recbut;
		else if (c == "loop") but = elem->loopbut;
		else if (c == "play") but = elem->playbut;
		if (but) {
			but->value = arg_num(m, 0, v) ? (v != 0.0) : !but->value;
			elem->oscforce = true;	// also handled when the line is scrolled out of view
		}
		else if (c == "speed" && arg_num(m, 0, v)) {
			elem->speed->value = std::clamp((float)v, elem->speed->range[0], elem->speed->range[1]);
		}
		else if (c == "clear" && is_trigger(m)) {
			elem->erase_elem();
		}
		else if (c == "copyduration" && is_trigger(m)) {
			// the duration of the loop, for pasting on other lines
			if (elem->speed->value > 0.0f) mainmix->cbduration = elem->totaltime / elem->speed->value;
		}
		else if (c == "pastespeed" && is_trigger(m)) {
			// give the line the copied duration by changing its speed
			if (mainmix->cbduration > 0.0f && elem->totaltime > 0.0f && elem->speed->value > 0.0f) {
				float buspeed = elem->speed->value;
				elem->speed->value = elem->totaltime / mainmix->cbduration;
				elem->speedadaptedtime *= buspeed / elem->speed->value;
			}
		}
		else if (c == "pastelength" && is_trigger(m)) {
			// give the line the copied duration by changing the length of the loop
			if (mainmix->cbduration > 0.0f) elem->totaltime = mainmix->cbduration * elem->speed->value;
		}
		else if (c == "beats" && arg_num(m, 0, v)) {
			// beatmatch the line: its loop lasts v beats (0.5, 1, 2, 4, ...), 0 turns it off and puts back the speed
			if (v <= 0.0) {
				if (elem->beats != 0) elem->speed->value = elem->buspeed;
				elem->beats = 0;
			}
			else {
				if (elem->beats == 0) elem->buspeed = elem->speed->value;
				elem->beats = (float)v;
			}
		}
		else if (c == "copycurve" && is_trigger(m)) {
			if (elem->curve && lpst_has_targets(elem)) {
				lpcurveclip = *elem->curve;
				lpcurveclipvalid = true;
			}
		}
		else if (c == "pastecurve" && is_trigger(m)) {
			if (lpcurveclipvalid && lpst_has_targets(elem)) lpst_paste_curve(elem);
		}
		else if (c == "position" && arg_num(m, 0, v)) {
			// scrub inside the recording, like dragging in the framecounter box of the row
			if (elem->eventlist.empty() || elem->totaltime <= 0.0f) return;
			elem->oscforce = true;
			elem->scritch->range[0] = 0.0f;
			elem->scritch->range[1] = elem->totaltime;
			elem->scritch->value = (float)std::clamp(v, 0.0, 1.0) * elem->totaltime;
		}
	}
}

static bool parse_launchtype(const OscMsg &m, int &lt) {
	std::string s;
	double v;
	if (arg_str(m, 0, s)) {
		s = sanitize(s);
		if (s == "restart") lt = 0;
		else if (s == "continue") lt = 1;
		else if (s == "catchup" || s == "catch_up") lt = 2;
		else return false;
		return true;
	}
	if (arg_num(m, 0, v) && v >= 0.0 && v <= 2.0) {
		lt = (int)v;
		return true;
	}
	return false;
}

static void shelf_load(int side, int bank, int elem, const std::string &path) {
	// Shelf::open_files_shelf() does the loading (video, image, layer, deck and mix files), like for files dropped on a
	// shelf element: it works on mainprogram->paths over several frames, on the shelf in mainmix->tempmouseshelf (which
	// takes priority over mainmix->mouseshelf, so mouse actions on a shelf meanwhile can't redirect the load).
	if (bank < 0 || bank > 3 || elem < 0 || elem > 15) return;
	if (mainprogram->openfilesshelf || mainprogram->openfileslayers || mainprogram->openfilesqueue ||
		mainprogram->openclipfiles || mainprogram->multistage != 0 || !mainprogram->paths.empty()) {
		printf("OSC: busy opening files, shelf load ignored: %s\n", path.c_str());
		return;
	}
	std::error_code ec;
	if (path.size() < 5 || !std::filesystem::exists(path, ec)) {
		printf("OSC: file not found: %s\n", path.c_str());
		return;
	}
	mainprogram->paths.push_back(path);
	mainmix->tempmouseshelf = mainprogram->shelves[side][bank];
	mainprogram->shelffilescount = 0;
	mainprogram->shelffileselem = elem;
	mainprogram->openfilesshelf = true;
}

static void shelf_delete(int side, int bank, int pos) {
	// the "Erase element" entry of the shelf menu
	ShelfElement *elem = mainprogram->shelves[side][bank]->elements[pos];
	elem->name = "";
	elem->path = "";
	elem->type = ELEM_FILE;
	blacken(elem->tex);
	blacken(elem->oldtex);
	elem->button->unregister_midi();
	elem->button->midi[0] = -1;
	elem->button->midi[1] = -1;
	elem->button->midiport = "";
	elem->kill_clayers();
}

static void shelf_insert(int side, int bank, int pos, int deck) {
	// The "Insert deck A", "Insert deck B" and "Insert full mix" entries of the shelf menu (deck 0, 1 or -1): the deck or
	// the mix as it is now, of the modus that is on screen, is saved to a temporary file and put in the shelf element.  The
	// menu code works on mainmix->mouseshelf / mouseshelfelem (and sets mousedeck), which are put back afterwards.
	Shelf *bumouseshelf = mainmix->mouseshelf;
	int bumouseshelfelem = mainmix->mouseshelfelem;
	int bumousedeck = mainmix->mousedeck;
	mainmix->mouseshelf = mainprogram->shelves[side][bank];
	mainmix->mouseshelfelem = pos;
	mainprogram->handle_shelfmenu(deck >= 0 ? 4 + deck : 6);
	mainmix->mouseshelf = bumouseshelf;
	mainmix->mouseshelfelem = bumouseshelfelem;
	mainmix->mousedeck = bumousedeck;
}

static void shelf_element_command(const std::string &cmd, int side, const OscMsg &m) {
	// load, delete and rename of one shelf element take the same parameters: the shelf (A or B, only as first argument
	// when side is -1, otherwise it is in the address), the bank (1..4), the element (1..16) and, for load and
	// rename, a string (the path, the new name)
	size_t k = 0;
	if (side < 0) {
		std::string deck;
		if (!arg_str(m, k++, deck) || (deck != "A" && deck != "B")) return;
		side = deck == "B";
	}
	double bank, elem;
	if (!arg_num(m, k, bank) || !arg_num(m, k + 1, elem)) return;
	int b = (int)bank - 1;
	int e = (int)elem - 1;
	if (b < 0 || b > 3 || e < 0 || e > 15) return;
	std::string str;
	if (cmd == "delete") {
		shelf_delete(side, b, e);
	}
	else if (cmd == "insertmix") {
		shelf_insert(side, b, e, -1);
	}
	else if (arg_str(m, k + 2, str)) {
		if (cmd == "load") {
			shelf_load(side, b, e, str);
		}
		else if (cmd == "insertdeck") {
			// the deck to insert: "A" or "B"
			if (str == "A" || str == "B") shelf_insert(side, b, e, str == "B");
		}
		else if (cmd == "rename") {
			ShelfElement *el = mainprogram->shelves[side][b]->elements[e];
			if (el->path == "") return;		// an empty element has no name to show
			el->oldname = el->name;
			el->name = str;
		}
	}
}

static void shelf_whole_command(const std::string &cmd, int side, const OscMsg &m) {
	// new, open and save of a whole shelf (one bank of 16 elements): the shelf (A or B, only as first argument when side
	// is -1, otherwise it is in the address), the bank (1..4) and, for open and save, the path of the shelf file
	size_t k = 0;
	if (side < 0) {
		std::string deck;
		if (!arg_str(m, k++, deck) || (deck != "A" && deck != "B")) return;
		side = deck == "B";
	}
	double bank;
	if (!arg_num(m, k, bank)) return;
	int b = (int)bank - 1;
	if (b < 0 || b > 3) return;
	if (cmd == "insertinbin") {
		// the "Insert in bin" entry of the shelf menu without the choosing in the bins room: the whole bank goes in a block
		// of the current bin, block 1 to 9, left to right and top to bottom
		double block;
		if (!arg_num(m, k + 1, block) || (int)block < 1 || (int)block > 9) return;
		if (!binsmain->insert_shelf_in_block(mainprogram->shelves[side][b], (int)block - 1)) {
			printf("OSC: can not insert the shelf in the current bin\n");
		}
	}
	else if (cmd == "new") {
		// the "New shelf" entry of the shelf menu, which works on mainmix->mouseshelf
		Shelf *bumouseshelf = mainmix->mouseshelf;
		mainmix->mouseshelf = mainprogram->shelves[side][b];
		mainprogram->handle_shelfmenu(1);
		mainmix->mouseshelf = bumouseshelf;
	}
	else {
		// "Open shelf" and "Save shelf": through the pathto system, like the file requester of the menu does
		std::string path;
		if (arg_str(m, k + 1, path) && path != "") queue_file_op(cmd == "save" ? "SAVESHELF" : "OPENSHELF", path, -1, -1, -1, side, b);
	}
}

static void handle_shelf(const std::vector<std::string> &t, const OscMsg &m) {
	// t[0] == "shelf", t[1] = A | B
	if (t.size() == 2 && (t[1] == "new" || t[1] == "open" || t[1] == "save" || t[1] == "insertinbin")) {
		// /shelf/<new|open|save>  s shelf (A | B), i bank (1..4), s path (open, save)
		shelf_whole_command(t[1], -1, m);
		return;
	}
	if (t.size() == 2 && (t[1] == "load" || t[1] == "delete" || t[1] == "rename" || t[1] == "insertdeck" || t[1] == "insertmix")) {
		// /shelf/<load|delete|rename>  s deck (A | B), i bank (1..4), i element (1..16), s path / name
		shelf_element_command(t[1], -1, m);
		return;
	}
	if (t.size() < 3) return;
	int side;
	if (t[1] == "A") side = 0;
	else if (t[1] == "B") side = 1;
	else return;
	double v;
	if (t[2] == "bank" && arg_num(m, 0, v)) {
		int b = (int)v - 1;
		if (b >= 0 && b < 4) mainmix->currbank[side] = b;
	}
	else if (t[2] == "new" || t[2] == "open" || t[2] == "save" || t[2] == "insertinbin") {
		// /shelf/<A|B>/<new|open|save>  i bank (1..4), s path (open, save)
		shelf_whole_command(t[2], side, m);
	}
	else if (t[2] == "load" || t[2] == "delete" || t[2] == "rename" || t[2] == "insertdeck" || t[2] == "insertmix") {
		// /shelf/<A|B>/<load|delete|rename>  i bank (1..4), i element (1..16), s path / name
		shelf_element_command(t[2], side, m);
	}
	else if (t[2] == "trigger" && arg_num(m, 0, v)) {
		int i = (int)v - 1;
		if (i >= 0 && i < 16 && osc_pendshelf.size() < 64) osc_pendshelf.push_back({side, i});
	}
	else if (t[2] == "launchtype") {
		// all elements of a bank: the second argument is the bank (1..4), default the current bank
		int lt, b = mainmix->currbank[side];
		if (!parse_launchtype(m, lt)) return;
		if (arg_num(m, 1, v)) {
			b = (int)v - 1;
			if (b < 0 || b > 3) return;
		}
		for (ShelfElement *elem: mainprogram->shelves[side][b]->elements) elem->launchtype = lt;
	}
	else if (t[2] == "element" && t.size() == 5 && t[4] == "launchtype") {
		int i, lt, b = mainmix->currbank[side];
		if (!parse_int(t[3], i) || i < 1 || i > 16 || !parse_launchtype(m, lt)) return;
		if (arg_num(m, 1, v)) {
			b = (int)v - 1;
			if (b < 0 || b > 3) return;
		}
		mainprogram->shelves[side][b]->elements[i - 1]->launchtype = lt;
	}
}

static BinElement *bin_element(const std::vector<std::string> &t, size_t i) {
	// the element of the current bin at row t[i] and column t[i+1] (both from 1, the bin is 12 x 12 elements)
	int r, c;
	if (i + 1 >= t.size() || !parse_int(t[i], r) || !parse_int(t[i + 1], c) || r < 1 || r > 12 || c < 1 || c > 12) return nullptr;
	if (!binsmain->currbin || binsmain->currbin->elements.size() < 144) return nullptr;
	return binsmain->currbin->elements[(r - 1) * 12 + (c - 1)];
}

static void bin_hap_encode(BinElement *binel, bool bulk) {
	// start the HAP encoding of a bin element (a file or layer file, a deck or a mix), as the bin menu entries do
	if (binel->path == "" || binel->vidupscaling) return;
	if (binel->name != "" && (binel->type == ELEM_FILE || binel->type == ELEM_LAYER)) binsmain->hap_binel(binel, nullptr, -1, bulk);
	else if (binel->type == ELEM_DECK) binsmain->hap_deck(binel);
	else if (binel->type == ELEM_MIX) binsmain->hap_mix(binel);
}

static void handle_bin(const std::vector<std::string> &t, const OscMsg &m) {
	// t[0] == "bin".  The bins of the project, as in the bins room.
	if (t.size() < 2) return;
	std::lock_guard<std::recursive_mutex> binslock(binsmain->binsmutex);
	double v;
	const std::string &c = t[1];
	if (c == "hapmode" && t.size() == 2) {
		// the HAP encoding mode switch of the bins room: "live" (0, one thread) or "max" (1, all cores and one more);
		// without argument it switches
		std::string mode;
		bool max;
		if (arg_str(m, 0, mode) && (sanitize(mode) == "live" || sanitize(mode) == "max")) max = sanitize(mode) == "max";
		else if (m.args.size() && arg_num(m, 0, v)) max = v != 0.0;
		else if (!m.args.size()) max = !mainprogram->threadmode;
		else return;
		mainprogram->threadmode = max;
		mainprogram->maxthreads = mainprogram->numcores * (max ? 1 : 0) + 1;	// as the bins room sets it when drawing
		return;
	}
	if (c == "hapencode" && t.size() == 2 && is_trigger(m) && binsmain->currbin) {
		// "HAP encode bin": all the elements of the current bin that aren't encoding already
		for (BinElement *binel: binsmain->currbin->elements) {
			if (!binel->encoding) bin_hap_encode(binel, true);
		}
		return;
	}
	if (c == "selection" && t.size() == 3 && is_trigger(m) && binsmain->currbin) {
		// the selected elements of the current bin (the ones with the lighter frame in the bins room)
		for (BinElement *binel: binsmain->currbin->elements) {
			if (!binel->select) continue;
			if (t[2] == "delete") binel->erase();
			else if (t[2] == "hapencode") bin_hap_encode(binel, true);
			else if (t[2] == "clear") binel->select = binel->oldselect = false;
		}
		return;
	}
	if (c == "select" && t.size() == 2 && arg_num(m, 0, v)) {
		// make bin number v (from 1, in the order of the bins list) the current bin
		int n = (int)v - 1;
		if (n >= 0 && n < (int)binsmain->bins.size()) binsmain->make_currbin(n);
	}
	else if (c == "new" && t.size() == 2 && is_trigger(m)) {
		// a new bin, with a name that isn't used yet, as the New bin entry of the bin menu
		if (binsmain->dragbin) return;
		std::string name = "new bin";
		int count = 0;
		for (Bin *bin: binsmain->bins) {
			if (name == bin->name) {
				count++;
				name = remove_version(name) + "_" + std::to_string(count);
			}
		}
		binsmain->new_bin(name);
		if (binsmain->bins.size() > 20) binsmain->binsscroll++;
	}
	else if (c == "delete" && t.size() == 2 && is_trigger(m)) {
		// delete the current bin; the last bin can't be deleted
		if (binsmain->dragbin || binsmain->bins.size() < 2 || !binsmain->currbin) return;
		Bin *cur = binsmain->currbin;
		if (cur->shared) binsmain->sharedbinnamesmap.erase(cur->name);
		mainprogram->remove(cur->path);
		int delpos = cur->pos;
		binsmain->bins.erase(std::find(binsmain->bins.begin(), binsmain->bins.end(), cur));
		delete cur;
		binsmain->make_currbin(delpos == 0 ? 0 : delpos - 1);
		for (int i = 0; i < (int)binsmain->bins.size(); i++) {
			if (binsmain->bins[i] == binsmain->currbin) binsmain->currbin->pos = i;
			binsmain->bins[i]->box->vtxcoords->y1 = (i + 1) * -0.05f;
			binsmain->bins[i]->box->upvtxtoscr();
		}
	}
	else if (c == "rename" && t.size() == 2) {
		// rename the current bin, as typing a new name in the bins list does
		std::string name;
		if (!arg_str(m, 0, name) || name == "" || !binsmain->currbin || name == binsmain->currbin->name) return;
		for (Bin *bin: binsmain->bins) {
			if (bin->name == name) {
				printf("OSC: there already is a bin called %s\n", name.c_str());
				return;
			}
		}
		binsmain->backupname = binsmain->currbin->name;
		binsmain->currbin->oldname = binsmain->currbin->name;
		binsmain->binrenamemap.erase(binsmain->currbin->name);
		binsmain->currbin->name = name;
		binsmain->binrenamemap[binsmain->currbin->name] = binsmain->backupname;
	}
	else if ((c == "open" || c == "save") && t.size() == 2) {
		// open a bin file (added to the bins) or save the current bin to a bin file, through the pathto system
		std::string path;
		if (arg_str(m, 0, path) && path != "") queue_file_op(c == "save" ? "SAVEBIN" : "OPENBIN", path, -1, -1);
	}
	else if (c == "loadshelf" && t.size() == 2) {
		// /bin/loadshelf  s shelf (A | B), i bank (1..4), i block (1..9): a block of the current bin into a shelf bank
		std::string shelf;
		double bank, block;
		if (arg_str(m, 0, shelf) && (shelf == "A" || shelf == "B") && arg_num(m, 1, bank) && arg_num(m, 2, block) &&
			(int)bank >= 1 && (int)bank <= 4 && (int)block >= 1 && (int)block <= 9) {
			if (!binsmain->load_block_in_shelf(mainprogram->shelves[shelf == "B"][(int)bank - 1], (int)block - 1)) {
				printf("OSC: can not load that block of the current bin in a shelf\n");
			}
		}
	}
	else if (c == "element" && t.size() == 5) {
		// /bin/element/R/C/<rename|launchtype|loaddeck|loadmix>: an element of the current bin
		BinElement *binel = bin_element(t, 2);
		if (!binel) return;
		const std::string &cmd = t[4];
		std::string str;
		int lt;
		if (cmd == "delete" && is_trigger(m)) {
			// empty the element, as the Delete entry of the element menu does
			binel->erase();
		}
		else if (cmd == "select") {
			// select or deselect the element, without argument toggles
			bool on = arg_num(m, 0, v) ? v != 0.0 : !binel->select;
			binel->select = binel->oldselect = on;
		}
		else if (cmd == "hapencode" && is_trigger(m)) {
			// HAP encode the element, or stop it when it is being encoded
			if (binel->encoding) binel->encoding = false;
			else bin_hap_encode(binel, false);
		}
		else if (cmd == "rename" && arg_str(m, 0, str) && binel->path != "") {
			binel->oldname = binel->name;
			binel->name = str;
		}
		else if (cmd == "launchtype" && parse_launchtype(m, lt)) {
			binel->launchtype = lt;
		}
		else if (cmd == "loaddeck" && arg_str(m, 0, str) && (str == "A" || str == "B")) {
			if (binel->type == ELEM_DECK) queue_file_op("OPENDECK", binel->path, !mainprogram->prevmodus, str == "B");
		}
		else if (cmd == "loadmix" && is_trigger(m)) {
			if (binel->type == ELEM_MIX) queue_file_op("OPENMIX", binel->path, !mainprogram->prevmodus, -1);
		}
	}
}

void osc_apply_prefs() {
	// Start, stop or restart OSC according to the "OSC" tab of the preferences: "OSC Control" (on/off), "OSC port",
	// "OSC Feedback port" and "Localhost only".  Called when the program starts and when the preferences are saved.
	// The values are read from the preference items themselves.
	bool on = true, local = false;
	int port = 9000, fport = 9001;
	std::string password;
	bool safe = false, oldhw = false;
	if (mainprogram->prefs) {
		for (PrefCat *cat: mainprogram->prefs->items) {
			if (cat->name != "OSC") continue;
			for (PrefItem *pi: cat->items) {
				if (pi->name == "OSC Control") on = pi->onoff;
				else if (pi->name == "OSC port") port = pi->value;
				else if (pi->name == "OSC Feedback port") fport = pi->value;
				else if (pi->name == "Localhost only") local = pi->onoff;
				else if (pi->name == "OSC Password") password = pi->str;
				else if (pi->name == "Safe mode") safe = pi->onoff;
				else if (pi->name == "Old hardware") oldhw = pi->onoff;
			}
		}
	}
	osc_safemode = safe;
	if (oldhw != osc_oldhw) {
		osc_oldhw = oldhw;
		osc_resync = true;	// what was left out has to be sent when it comes back
	}
	if (password != osc_password) {
		// a new password: everybody has to authenticate again, and feedback stops going to the old sender
		osc_password = password;
		osc_authed.clear();
		osc_authtries.clear();
		if (osc_target && osc_target_auto) osc_set_target("", osc_feedbackport, true);
	}
	if (port < 1 || port > 65535) port = 9000;
	if (fport < 1 || fport > 65535) fport = 9001;
	osc_localhost = local;

	std::string newfport = std::to_string(fport);
	if (newfport != osc_feedbackport) {
		osc_feedbackport = newfport;
		// feedback that goes to the sender of the messages follows the new port; /osc/target keeps its own port
		if (osc_target && osc_target_auto) osc_set_target(osc_targethost, osc_feedbackport, true);
	}
	// feedback to another computer can't go on when only this one is allowed
	if (local && osc_target && !is_local_host(osc_targethost)) osc_set_target("", osc_feedbackport, true);

	if (!on) {
		osc_stop();
		return;
	}
	if (osc_st && osc_port != port) osc_stop();	// listening on another port: start again
	if (!osc_st) osc_start(port);
}

static std::string effect_code_display(int code) {
	// the menu name of an entry of the effect menu: its code is an EFFECT_TYPE, 1000 + FFGL index or 2000 + ISF index
	if (code >= 2000) return code - 2000 < (int)mainprogram->isfeffectdisplay.size() ? mainprogram->isfeffectdisplay[code - 2000] : "";
	if (code >= 1000) return code - 1000 < (int)mainprogram->ffgleffectdisplay.size() ? mainprogram->ffgleffectdisplay[code - 1000] : "";
	return (code >= 0 && code < (int)mainprogram->builtineffectdisplay.size()) ? mainprogram->builtineffectdisplay[code] : "";
}

static void send_list(const std::string &what) {
	// /list/<what>: the names that can be used in the addresses, in the order of the menus they come from, sent back as
	// one message /list/<what> with a string for every name.  The names are those to put in the addresses (lower case,
	// non-alphanumerics as '_'), except for the displays and the bins, which have the name they have.
	if (!osc_target) return;
	std::vector<std::string> names;
	if (what == "effects") {
		for (int code: mainprogram->abeffects) names.push_back(sanitize(effect_code_display(code)));
	}
	else if (what == "aistyles") {
		for (const std::string &name: mainprogram->aistylenames) names.push_back(sanitize(name));
	}
	else if (what == "mixmodes") {
		for (const std::string &name: mainprogram->mixmodenames) names.push_back(sanitize(name));
	}
	else if (what == "wipes") {
		// by wipe number (CROSSFADE is -1), then the ISF wipes
		std::vector<std::pair<int, std::string>> wipes;
		for (auto &w: mainprogram->wipesmap) wipes.push_back({w.second, w.first});
		std::sort(wipes.begin(), wipes.end());
		for (auto &w: wipes) names.push_back(sanitize(w.second));
		for (const std::string &name: mainprogram->wipeisfnames) names.push_back(sanitize(name));
	}
	else if (what == "sources") {
		for (int code: mainprogram->absources) {
			if (code >= 2000 && code - 2000 < (int)mainprogram->isfsourcedisplay.size()) names.push_back(sanitize(mainprogram->isfsourcedisplay[code - 2000]));
			else if (code >= 1000 && code < 2000 && code - 1000 < (int)mainprogram->ffglsourcedisplay.size()) names.push_back(sanitize(mainprogram->ffglsourcedisplay[code - 1000]));
		}
	}
	else if (what == "displays") {
		// the other displays, in the order of the "Show on display" menus (number 1 is the first)
		int numd = 0;
		SDL_DisplayID *ids = SDL_GetDisplays(&numd);
		if (ids) SDL_free(ids);
		for (int i = 1; i < numd; i++) {
			const char *dname = SDL_GetDisplayName(EWOC_DisplayIndexToID(i));
			names.push_back(dname ? dname : "");
		}
	}
	else if (what == "bins") {
		for (Bin *bin: binsmain->bins) names.push_back(bin->name);
	}
	else if (what == "ndi") {
		ndi_refresh(nullptr);
		names = mainprogram->ndisourcenames;
	}
	else return;
	lo_message msg = lo_message_new();
	for (const std::string &name: names) lo_message_add_string(msg, name.c_str());
	lo_send_message(osc_target, ("/list/" + what).c_str(), msg);
	lo_message_free(msg);
}

static void handle_message(const OscMsg &m) {
	std::vector<std::string> t = split_path(m.path);
	if (t.empty()) return;
	if (t[0] == "mix") handle_mix(t, m);
	else if (t[0] == "shelf") handle_shelf(t, m);
	else if (t[0] == "bin") handle_bin(t, m);
	else if (t[0] == "list" && t.size() == 2 && is_trigger(m)) send_list(t[1]);
	else if (t[0] == "beat" && t.size() == 2 && t[1] == "threshold") {
		// the beat detection threshold (the slider of the beat detection): music quieter than this isn't taken as a beat
		double v;
		if (arg_num(m, 0, v)) set_param_normalized(mainprogram->beatthres, v, !mainprogram->prevmodus);
	}
	else if (t[0] == "ndi" && t.size() == 2 && t[1] == "refresh" && is_trigger(m)) {
		ndi_refresh(nullptr);
		ndi_send_sources();
	}
	else if (t[0] == "webcam" && t.size() == 2 && t[1] == "refresh" && is_trigger(m)) {
		webcam_send_sources();
	}
	else if (t[0] == "loopstation") handle_loopstation(t, m);
	else if (t[0] == "osc" && t.size() >= 2) {
		double v;
		if (t[1] == "target") {
			std::string host;
			if (arg_str(m, 0, host)) {
				std::string port = osc_feedbackport;
				if (arg_num(m, 1, v)) port = std::to_string((int)v);
				if (osc_localhost && !is_local_host(host)) {
					printf("OSC: feedback to %s refused: OSC is set to this computer only\n", host.c_str());
				}
				else {
					osc_set_target(host, port);
				}
			}
		}
		else if (t[1] == "feedback" && arg_num(m, 0, v)) osc_feedback_on = v != 0.0;
		else if (t[1] == "sync") { osc_lastsent.clear(); osc_laststr.clear(); osc_resync = true; }
	}
}

// ---------------------------------------------------------------------------------------------------------
// state feedback

static void feedback_value(const std::string &path, float val, bool send) {
	auto it = osc_lastsent.find(path);
	if (it != osc_lastsent.end() && it->second == val) return;
	osc_lastsent[path] = val;
	if (send && osc_target) lo_send(osc_target, path.c_str(), "f", val);
}

static void feedback_string(const std::string &path, const std::string &val, bool send) {
	// names: sent as a string when they change
	auto it = osc_laststr.find(path);
	if (it != osc_laststr.end() && it->second == val) return;
	osc_laststr[path] = val;
	if (send && osc_target) lo_send(osc_target, path.c_str(), "s", val.c_str());
}

static std::string effect_display_name(Effect *eff) {
	// the name of an effect as in the effect menu (the numbered one for equal plugin names)
	if (eff->ffglnr != -1 && eff->ffglnr < (int)mainprogram->ffgleffectdisplay.size()) return mainprogram->ffgleffectdisplay[eff->ffglnr];
	if (eff->isfnr != -1 && eff->isfnr < (int)mainprogram->isfeffectdisplay.size()) return mainprogram->isfeffectdisplay[eff->isfnr];
	if (eff->aistylnr != -1 && eff->aistylnr < (int)mainprogram->aistylenames.size()) return mainprogram->aistylenames[eff->aistylnr];
	if ((int)eff->type >= 0 && (int)eff->type < (int)mainprogram->builtineffectdisplay.size()) return mainprogram->builtineffectdisplay[eff->type];
	return "";
}

static std::string layer_display_name(Layer *lay) {
	// what the layer shows: the name of its source plugin, or the name of the file
	if (lay->ffglsourcenr != -1 && lay->ffglsourcenr < (int)mainprogram->ffglsourcedisplay.size()) return mainprogram->ffglsourcedisplay[lay->ffglsourcenr];
	if (lay->isfsourcenr != -1 && lay->isfsourcenr < (int)mainprogram->isfsourcedisplay.size()) return mainprogram->isfsourcedisplay[lay->isfsourcenr];
	return lay->filename == "" ? "" : basename(lay->filename);
}

static float normalized(Param *par) {
	float span = par->range[1] - par->range[0];
	if (span == 0.0f) return 0.0f;
	return (par->value - par->range[0]) / span;
}

static void feedback_param(const std::string &prefix, Param *par, size_t j, bool send) {
	// a parameter under prefix: by name (or param/J without one); a colour parameter is reported as "#RRGGBB"
	std::string name = param_osc_name(par);
	std::string path = prefix + (name != "" ? "/" + name : "/param/" + std::to_string(j + 1));
	if (is_color_param(par)) feedback_string(path, color_hex(par), send);
	else feedback_value(path, normalized(par), send);
}

static void feedback_effects(Layer *lay,const std::string &prefix, int cat, bool send) {
	std::vector<Effect*> &effs = lay->effects[cat];
	for (size_t k = 0; k < effs.size(); k++) {
		Effect *eff = effs[k];
		if (!eff) continue;
		std::string ep = prefix + (cat ? "/effect2/" : "/effect/") + std::to_string(k + 1);
		feedback_string(ep + "/name", effect_display_name(eff), send);
		feedback_value(ep + "/onoff", (float)eff->onoffbutton->value, send);
		if (eff->masks.size()) {
			feedback_value(ep + "/maskscroll", (float)(eff->maskscrollpos + 1), send);
			feedback_value(ep + "/masked", eff->masked ? 1.0f : 0.0f, send);
		}
		if (eff->drywet) feedback_value(ep + "/drywet", normalized(eff->drywet), send);
		for (size_t j = 0; j < eff->params.size(); j++) {
			Param *par = eff->params[j];
			if (!par || par->colslave) continue;
			feedback_param(ep, par, j, send);
		}
	}
}

static void feedback_scan(bool send) {
	// walk the state and (when send) report what changed since the previous scan
	feedback_value("/mix/preview/crossfade", mainmix->crossfade->value, send);
	feedback_value("/mix/output/crossfade", mainmix->crossfadecomp->value, send);
	feedback_value("/mix/output/record", mainmix->recording[1] ? 1.0f : 0.0f, send);
	feedback_value("/mix/modus", mainprogram->prevmodus ? 1.0f : 0.0f, send);
	{
		// AI style of the monitors: 0 for none, else the number of the style (from 1)
		static const char *aistylenamesfb[4] = {"deckA", "deckB", "mix", "output"};
		for (int i = 0; i < 4; i++) {
			MixNode *mnode = monitor_node(aistylenamesfb[i]);
			feedback_value(std::string("/mix/aistyle/") + aistylenamesfb[i], mnode ? (float)(mnode->aistylnr + 1) : 0.0f, send);
		}
	}
	{
		// full screen display of the monitors
		static const char *fsnames[4] = {"deckA", "deckB", "mix", "output"};
		for (int i = 0; i < 4; i++) {
			int code = monitor_fullscreen_code(fsnames[i]);
			feedback_value(std::string("/mix/fullscreen/") + fsnames[i], (code >= 0 && mainprogram->fullscreen == code) ? 1.0f : 0.0f, send);
		}
	}
	{
		// NDI outputs of the monitors of the modus that is on screen
		static const char *monitornames[3] = {"deckA", "deckB", "mix"};
		for (int i = 0; i < 3; i++) {
			MixNode *mnode = mainprogram->nodesmain->mixnodes[!mainprogram->prevmodus][i];
			feedback_value(std::string("/mix/ndioutput/") + monitornames[i], (mnode && mnode->ndioutput) ? 1.0f : 0.0f, send);
		}
		if (mainprogram->prevmodus) {
			MixNode *mnode = mainprogram->nodesmain->mixnodes[1][2];
			feedback_value("/mix/ndioutput/output", (mnode && mnode->ndioutput) ? 1.0f : 0.0f, send);
		}
	}
	feedback_value("/mix/scene/deckA",(float)(mainmix->currscene[0] + 1), send);
	feedback_value("/mix/scene/deckB", (float)(mainmix->currscene[1] + 1), send);
	for (int side = 0; side < 2; side++) {
		std::string shp = side ? "/shelf/B" : "/shelf/A";
		int bank = mainmix->currbank[side];
		feedback_value(shp + "/bank", (float)(bank + 1), send);
		std::vector<ShelfElement*> &elems = mainprogram->shelves[side][bank]->elements;
		for (size_t i = 0; i < elems.size() && !osc_oldhw; i++) {
			feedback_value(shp + "/element/" + std::to_string(i + 1) + "/launchtype", (float)elems[i]->launchtype, send);
			feedback_string(shp + "/element/" + std::to_string(i + 1) + "/name", elems[i]->path == "" ? "" : elems[i]->name, send);
		}
	}
	{
		// beat detection: the threshold, and the tempo found in the music (0 when none is found)
		feedback_value("/beat/threshold", normalized(mainprogram->beatthres), send);
		float bpm = 0.0f;
		if (mainprogram->beatdet && mainprogram->beatdet->winning_bpm > 0.0f) {
			// winning_bpm is the time of one beat in seconds
			bpm = std::round(60.0f / mainprogram->beatdet->winning_bpm * 10.0f) / 10.0f;
		}
		feedback_value("/beat/bpm", bpm, send);
	}
	if (binsmain && binsmain->currbin) {
		// the bins: which one is current, and how many there are
		int cur = 0;
		for (size_t i = 0; i < binsmain->bins.size(); i++) {
			if (binsmain->bins[i] == binsmain->currbin) cur = (int)i;
		}
		feedback_value("/bin/current", (float)(cur + 1), send);
		feedback_value("/bin/count", (float)binsmain->bins.size(), send);
		feedback_string("/bin/name", binsmain->currbin->name, send);
		feedback_value("/bin/hapmode", mainprogram->threadmode ? 1.0f : 0.0f, send);
	}
	if (loopstation) {
		static std::vector<std::string> linepaths;
		if (linepaths.size() < loopstation->elements.size()) {
			for (size_t i = linepaths.size(); i < loopstation->elements.size(); i++) {
				linepaths.push_back("/loopstation/line/" + std::to_string(i + 1));
			}
		}
		feedback_value("/loopstation/current", (float)(loopstation->currelem->pos + 1), send);
		feedback_value("/loopstation/scroll", (float)(loopstation->scrpos + 1), send);
		for (size_t i = 0; i < loopstation->elements.size(); i++) {
			LoopStationElement *elem = loopstation->elements[i];
			const std::string &lpath = linepaths[i];
			feedback_value(lpath + "/rec", (float)(elem->recbut->value != 0), send);
			feedback_value(lpath + "/loop", (float)(elem->loopbut->value != 0), send);
			feedback_value(lpath + "/play", (float)(elem->playbut->value != 0), send);
			feedback_value(lpath + "/speed", elem->speed->value, send);
			feedback_value(lpath + "/beats", elem->beats, send);
			float pos = 0.0f;
			if (elem->totaltime > 0.0f && !elem->eventlist.empty()) {
				pos = std::round(std::clamp(elem->speedadaptedtime / elem->totaltime, 0.0f, 1.0f) * 1000.0f) / 1000.0f;
			}
			if (!osc_oldhw) feedback_value(lpath + "/position", pos, send);
		}
	}
	static const char *setnames[2] = {"preview", "output"};
	static const char *decknames[2] = {"deckA", "deckB"};
	for (int s = 0; s < 2; s++) {
		std::string sp = std::string("/mix/") + setnames[s];
		feedback_value(sp + "/wipe/type", (float)mainmix->wipe[s], send);
		feedback_value(sp + "/wipe/dir", (float)mainmix->wipedir[s], send);
		feedback_value(sp + "/wipe/xpos", normalized(mainmix->wipex[s]), send);
		feedback_value(sp + "/wipe/ypos", normalized(mainmix->wipey[s]), send);
		{
			std::vector<Param*> wp = mainwipe_params(s);
			for (size_t j = 0; j < wp.size(); j++) {
				if (!wp[j] || wp[j]->colslave) continue;
				feedback_param(sp + "/wipe", wp[j], j, send);
			}
		}
		for (int d = 0; d < 2; d++) {
			std::string dp = sp + "/" + decknames[d];
			feedback_value(dp + "/speed", raw_value(mainmix->deckspeed[s][d]), send);
			feedback_value(dp + "/layers", (float)mainmix->layers[s * 2 + d].size(), send);
			feedback_value(dp + "/genmidi", (float)mainmix->genmidi[d]->value, send);
			feedback_value(dp + "/scroll", (float)(*stack_scrollpos(s, d) + 1), send);
			{
				// how deep in the mask hierarchy the deck is: 0 is the normal layer stack
				int depth = 0;
				for (Layer *cur = mainmix->editedmask[s][d]; cur; cur = cur->parentlayer) {
					depth++;
					if (cur == cur->parentlayer) break;
				}
				feedback_value(dp + "/maskdepth", (float)depth, send);
			}
			std::vector<Layer*> &lv = mainmix->layers[s * 2 + d];
			for (size_t n = 0; n < lv.size(); n++) {
				Layer *lay = lv[n];
				if (!lay) continue;
				std::string lpath = dp + "/layer/" + std::to_string(n + 1);
				feedback_value(lpath + "/opacity", normalized(lay->opacity), send);
				feedback_value(lpath + "/volume", normalized(lay->volume), send);
				feedback_value(lpath + "/speed", lay->speed->value, send);
				feedback_value(lpath + "/play", (float)(lay->playbut->value != 0), send);
				feedback_value(lpath + "/reverse", (float)(lay->revbut->value != 0), send);
				feedback_value(lpath + "/bounce", (float)(lay->bouncebut->value != 0), send);
				feedback_value(lpath + "/loop", (float)(lay->lpbut->value != 0), send);
				feedback_value(lpath + "/mute", (float)(lay->mutebut->value != 0), send);
				feedback_value(lpath + "/solo", (float)(lay->solobut->value != 0), send);
				feedback_value(lpath + "/key/tolerance", normalized(lay->chtol), send);
				feedback_value(lpath + "/key/feather", normalized(lay->chfeather), send);
				feedback_value(lpath + "/key/direction", lay->chdir->value ? 1.0f : 0.0f, send);
				feedback_value(lpath + "/key/invert", lay->chinv->value ? 1.0f : 0.0f, send);
				feedback_value(lpath + "/lockspeed", lay->lockspeed ? 1.0f : 0.0f, send);
				feedback_value(lpath + "/lockzoompan", lay->lockzoompan ? 1.0f : 0.0f, send);
				feedback_value(lpath + "/keepeffects", (float)(lay->keepeffbut->value != 0), send);
				feedback_value(lpath + "/keepmask", (float)(lay->keepmaskbut->value != 0), send);
				feedback_string(lpath + "/name", layer_display_name(lay), send);
				feedback_value(lpath + "/effect/count", (float)lay->effects[0].size(), send);
				feedback_value(lpath + "/effect2/count", (float)lay->effects[1].size(), send);
				feedback_value(lpath + "/mask/count", (float)lay->masks.size(), send);
				if (lay->clips && !osc_oldhw) {
					// the names of the clips in the queue (the last one is the empty place at the end)
					for (size_t q = 0; q + 1 < lay->clips->size() && q < 32; q++) {
						feedback_string(lpath + "/queue/" + std::to_string(q + 1) + "/name", basename((*lay->clips)[q]->path), send);
					}
				}
				feedback_value(lpath + "/genmidi", (float)lay->genmidibut->value, send);
				feedback_value(lpath + "/loopbeats", lay->loopbeats, send);
				feedback_value(lpath + "/fxchain", lay->effcat ? 1.0f : 0.0f, send);
				feedback_value(lpath + "/ndioutput", lay->ndioutput ? 1.0f : 0.0f, send);
				feedback_value(lpath + "/fullscreen", (mainprogram->fullscreen == 5 && mainprogram->fullscreenlay == lay) ? 1.0f : 0.0f, send);
				feedback_value(lpath + "/recordreplace", (mainmix->reclay == lay && mainmix->recording[0]) ? 1.0f : 0.0f, send);
				feedback_value(lpath + "/hapencode", lay->hapbinel ? std::max(lay->hapbinel->encodeprogress, 0.001f) : 0.0f, send);
				feedback_value(lpath + "/aspect", (float)lay->aspectratio, send);
				if (lay->masks.size()) {
					feedback_value(lpath + "/maskscroll", (float)(lay->maskscrollpos + 1), send);
					feedback_value(lpath + "/masked", lay->masked ? 1.0f : 0.0f, send);
				}
				if (lay->numf > 1 && !osc_oldhw) {
					feedback_value(lpath + "/loopbox/start", lay->startframe->value / (float)(lay->numf - 1), send);
					feedback_value(lpath + "/loopbox/end", lay->endframe->value / (float)(lay->numf - 1), send);
				}
				if (lay->clips) {
					feedback_value(lpath + "/queue/show", lay->queueing ? 1.0f : 0.0f, send);
					if (!osc_oldhw && lay->endframe->value > lay->startframe->value) {
						// the playhead in the loop bar of the queue, quantised like the layer position
						float qpos = (lay->frame - lay->startframe->value) / (lay->endframe->value - lay->startframe->value);
						feedback_value(lpath + "/queue/position", std::clamp(std::round(qpos * 1000.0f) / 1000.0f, 0.0f, 1.0f), send);
					}
					feedback_value(lpath + "/queue/size", (float)std::max(0, (int)lay->clips->size() - 1), send);
					feedback_value(lpath + "/queue/scroll", (float)(lay->queuescroll + 1), send);
					feedback_value(lpath + "/queue/beats", lay->beatdetbut->value ? lay->beats : 0.0f, send);
				}
				if (!osc_oldhw) {
					std::vector<Param*> mp = mixer_params(lay->blendnode);
					for (size_t j = 0; j < mp.size(); j++) {
						if (!mp[j] || mp[j]->colslave) continue;
						feedback_param(lpath + "/mixer", mp[j], j, send);
					}
				}
				if (std::vector<Param*> *sp = osc_oldhw ? nullptr : source_params(lay)) {
					for (size_t j = 0; j < sp->size(); j++) {
						Param *par = (*sp)[j];
						if (!par || par->colslave) continue;
						feedback_param(lpath + "/source", par, j, send);
					}
				}
				feedback_value(lpath + "/upscale",lay->upscale->value != 0.0f ? 1.0f : 0.0f, send);
				feedback_value(lpath + "/sharpness", normalized(lay->rcassharpness), send);
				if (BlendNode *bn = lay->blendnode) {
					// mixmode: the BLEND_TYPE number (WIPE = 25, FFGL mixers 1000, ISF mixers 2000)
					feedback_value(lpath + "/mixmode", (float)bn->blendtype, send);
					if (bn->mixfac) feedback_value(lpath + "/mixfactor", normalized(bn->mixfac), send);
					feedback_value(lpath + "/wipe/type", (float)(bn->blendtype == WIPE ? bn->wipetype : -1), send);
					feedback_value(lpath + "/wipe/dir", (float)bn->wipedir, send);
					if (bn->wipex) feedback_value(lpath + "/wipe/xpos", normalized(bn->wipex), send);
					if (bn->wipey) feedback_value(lpath + "/wipe/ypos", normalized(bn->wipey), send);
				}
				if (lay->numf > 1 && !osc_oldhw) {
					// quantised to avoid flooding the controller while a video plays
					float pos = std::round((lay->frame / (float)(lay->numf - 1)) * 1000.0f) / 1000.0f;
					feedback_value(lpath + "/position", std::clamp(pos, 0.0f, 1.0f), send);
				}
				if (!osc_oldhw) {
					feedback_effects(lay, lpath, 0, send);
					feedback_effects(lay, lpath, 1, send);
				}
			}
		}
	}
}

static bool osc_is_unsafe(const OscMsg &m) {
	// "Safe mode": messages that read or write files, or that erase or replace content, are ignored.  They are
	// recognized by the last part of the address.
	static const std::unordered_set<std::string> unsafe = {
		"save", "open", "new", "delete", "rename", "insertdeck", "insertmix", "insertinbin", "hapencode"
	};
	if (m.path.rfind("/loopstation/", 0) == 0 && m.path.size() >= 6 && m.path.compare(m.path.size() - 6, 6, "/clear") == 0) return true;
	size_t p = m.path.rfind('/');
	return unsafe.count(p == std::string::npos ? m.path : m.path.substr(p + 1)) > 0;
}

static void osc_try_auth(const OscMsg &m) {
	// /osc/auth <password> from a computer that isn't authenticated yet.  After 5 wrong passwords the computer is
	// ignored for a minute.  The answer, "/osc/auth ok" or "/osc/auth denied", goes to the sender at the feedback port.
	if (m.host == "") return;
	auto now = std::chrono::steady_clock::now();
	OscAuthTry &t = osc_authtries[m.host];
	if (now < t.lockeduntil) return;
	std::string given = m.args.size() && (m.args[0].t == 's' || m.args[0].t == 'S') ? m.args[0].s : "";
	// compare without stopping at the first difference
	unsigned diff = (unsigned)(given.size() ^ osc_password.size());
	for (size_t i = 0; i < given.size(); i++) diff |= (unsigned char)given[i] ^ (unsigned char)osc_password[i % osc_password.size()];
	bool ok = diff == 0;
	if (ok) {
		osc_authed.insert(m.host);
		osc_authtries.erase(m.host);
		if (!osc_target) osc_set_target(m.host, osc_feedbackport, true);
		osc_resync = true;
	} else if (++t.failures >= 5) {
		t.failures = 0;
		t.lockeduntil = now + std::chrono::seconds(60);
	}
	lo_address a = lo_address_new(m.host.c_str(), osc_feedbackport.c_str());
	if (a) {
		lo_send(a, "/osc/auth", "s", ok ? "ok" : "denied");
		lo_address_free(a);
	}
}

void osc_process() {
	if (!osc_st) return;

	std::vector<OscMsg> messages;
	{
		std::lock_guard<std::mutex> lock(osc_qmutex);
		messages.swap(osc_queue);
	}

	size_t handled = 0;
	for (const OscMsg &m: messages) {
		if (osc_password != "" && !osc_authed.count(m.host)) {
			// not authenticated: the only thing accepted is the password
			if (m.path == "/osc/auth") osc_try_auth(m);
			continue;
		}
		// feedback follows whoever talks to us, unless a target was set explicitly with /osc/target
		if (!osc_target && m.host != "") osc_set_target(m.host, osc_feedbackport, true);
		if (m.path == "/osc/auth") continue;	// already in, or no password
		if (osc_safemode && osc_is_unsafe(m)) continue;
		handle_message(m);
		handled++;
	}
	modus_apply();
	scenes_apply();
	shelf_apply();
	fileop_apply();

	bool send = osc_feedback_on && osc_target;
	if (handled && send && !osc_resync) {
		// values the controller just set are already known to it: record them without echoing back
		feedback_scan(false);
	}
	auto now = std::chrono::steady_clock::now();
	if (send && now - osc_lastscan >= std::chrono::milliseconds(osc_oldhw ? 250 : 33)) {
		osc_lastscan = now;
		osc_resync = false;
		feedback_scan(true);
	}
}
