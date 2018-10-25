#include <vector>

class Box;
class Layer;
class Effect;
class Param;

typedef enum
{
	VIDEO = 0,
	MIX = 1,
	EFFECT = 2,
	BLEND = 3,
	MIDI = 4,
} NODE_TYPE;

typedef enum
{
	C_NONE = 0,
	C_OUT = 1,
	C_IN = 2,
	C_IN2 = 3,
} CONN_TYPE;

typedef enum
{
	MIXING = 1,
	MULTIPLY = 2, 
	SCREEN = 3, 
	OVERLAY = 4, 
	HARD_LIGHT = 5, 
	SOFT_LIGHT = 6, 
	DIVIDE = 7, 
	ADD = 8, 
	SUBTRACT = 9, 
	DIFF = 10, 
	DODGE = 11, 
	COLOR_BURN = 12, 
	LINEAR_BURN = 13, 
	VIVID_LIGHT = 14, 
	LINEAR_LIGHT = 15, 
	DARKEN_ONLY = 16, 
	LIGHTEN_ONLY = 17,
	WIPE = 18,
	CHROMAKEY = 19,
	DISPLACEMENT = 20,
	CROSSFADING = 21,
	
} BLEND_TYPE;

class NodePage;

class Node {
	public:
		NODE_TYPE type;
		int pos;
		NodePage *page;
		Node *in = nullptr;
		std::vector<Node*> out;
		Box *box;
		Box *vidbox;
		Node *align = NULL;
		int alignpos;
		int aligned = 0;
		bool calc = false;
		bool walked = false;
		int monitor = -1;
		CONN_TYPE dragging = C_NONE;
		bool nocutting = false;
		void draw_connection(Node *node, CONN_TYPE ct);
		Node();
		Node(const Node &node);
		~Node();
};

class VideoNode;
class MixNode;
class EffectNode;
class BlendNode;

class NodePage {
	public:
		int pos;
		Node *movingnode = nullptr;
		std::vector<Node*> nodescomp;
		std::vector<Node*> nodes;
		void connect_in2(Node *node1, BlendNode *node2);
		void connect_in2(Node *node1, EffectNode *node2);
		void connect_nodes(Node *node1, Node *node2);
		void connect_nodes(Node *node1, Node *node2, BlendNode *bnode);
		VideoNode *add_videonode(int comp);
		MixNode *add_mixnode(int screen, bool comp);
		EffectNode *add_effectnode(Effect *effect, VideoNode *node, int pos, bool comp);
		BlendNode *add_blendnode(BLEND_TYPE btype, bool comp);
		void delete_node(Node *node);
		void handle_nodes();
};

class NodesMain {
    public:
		int numpages = 0;
		NodePage *currpage;
        std::vector<NodePage*> pages;
        std::vector<MixNode*> mixnodescomp;
        std::vector<MixNode*> mixnodes;
        bool linked = true;
        Node *floatingnode = nullptr;
        void add_nodepages(int num);
};

class VideoNode: public Node {
	public:
		Layer *layer = nullptr;
		void upeffboxes();
		VideoNode() {}
		VideoNode(const VideoNode &node);
};

class MixNode: public Node {
	public:
		bool fullscreen = false;
		int screen = 0;
		Box *outputbox = nullptr;
		GLuint mixfbo = -1;
		GLuint mixtex = -1;
		MixNode() {}
		MixNode(const MixNode &node);
		~MixNode();
};

class EffectNode: public Node {
	public:
		Effect *effect = NULL;
		Node *in2 = NULL;
		EffectNode();
};

class BlendNode: public Node {
	public:
		Layer *firstlayer = nullptr;
		BLEND_TYPE blendtype = MIXING;
		Param *mixfac;
		int wipetype;
		int wipedir;
		float wipex;
		float wipey;
		Node *in2 = NULL;
		GLuint intex = -1;
		GLuint in2tex = -1;
		GLuint fbo = -1;
		GLuint fbotex = -1;
		bool lastblend = false;
		float chred = 0.0f;
		float chgreen = 0.0f;
		float chblue = 0.0f;
		bool onoff = true;
		BlendNode();
		~BlendNode();
};

class MidiNode: public Node {
	public:
		Param *param;
		MidiNode();
};