#include "GL/glew.h"
#include "GL/gl.h"
#include "GL/glut.h"

#include <algorithm>

#include "node.h"
#include "box.h"
#include "effect.h"
#include "layer.h"
#include "window.h"
#include "program.h"
#include "loopstation.h"
#include "bins.h"


float pdistance(float x, float y, float x1, float y1, float x2, float y2) {

  float A = x - x1;
  float B = y - y1;
  float C = x2 - x1;
  float D = y2 - y1;

  float dot = A * C + B * D;
  float len_sq = C * C + D * D;
  float param = -1;
  if (len_sq != 0) //in case of 0 length line
      param = dot / len_sq;

  float xx, yy;

  if (param < 0) {
    xx = x1;
    yy = y1;
  }
  else if (param > 1) {
    xx = x2;
    yy = y2;
  }
  else {
    xx = x1 + param * C;
    yy = y1 + param * D;
  }

  float dx = x - xx;
  float dy = y - yy;
  return sqrt(dx * dx + dy * dy);
}


Node::Node() {
	this->box = new Box;
	this->monbox = new Box;
    this->page = mainprogram->nodesmain->currpage;
}

MidiNode::MidiNode() {
	this->type = MIDI;
	this->box->vtxcoords->y1 = -0.7;
	this->box->vtxcoords->w = mainprogram->xscrtovtx(60);
	this->box->vtxcoords->h = mainprogram->yscrtovtx(60);
	this->box->upvtxtoscr();
}


Node::Node(const Node &node) {
	this->page = node.page;
	this->monitor = node.monitor;
}

VideoNode::VideoNode() {
	this->type = VIDEO;
    this->vidbox = new Box;
    this->vidbox->tooltiptitle = "Layer stack - layer monitor ";
    this->vidbox->tooltip = "Layer stack: bottom layer is leftmost. Monitor shows layer video image after all layer effects are processed.  Leftdrag center box to pan layer image.  Mousewheel scales layer image.  Leftdrag (not on center box) starts layer drag'n'drop: exchange or move layers to/from either A/B layer queue, drag layer files to shelf or media bins (through the center wormhole). ";
}

MixNode::MixNode(const MixNode &node) {
	this->type = MIX;
	this->mixfbo = -1;
	this->mixtex = -1;
}

EffectNode::EffectNode() {
	this->type = EFFECT;
}

BlendNode::BlendNode() {
	this->type = BLEND;
	this->intex = -1;
	this->in2tex = -1;
	this->fbo = -1;
	this->fbotex = -1;
    this->mixfac = new Param;
    this->mixfac->name = "Factor";
    this->mixfac->value = 0.5f;
    this->mixfac->range[0] = 0.0f;
    this->mixfac->range[1] = 1.0f;
    this->mixfac->shadervar = "mixfac";
    this->mixfac->sliding = true;
    this->mixfac->box->tooltiptitle = "Mix factor ";
    this->mixfac->box->tooltip = "Leftdrag sets relative mixamount of current layer and its previous layers. ";
}

Node::~Node() {
	//delete this->box;
    //delete this->vidbox;
}

BlendNode::~BlendNode() {
	glDeleteTextures(1, &this->fbotex);
	glDeleteBuffers(1, &this->fbo);
	if (this->intex != -1) glDeleteTextures(1, &this->intex);
	if (this->in2tex != -1) glDeleteTextures(1, &this->in2tex);
}
	
MixNode::~MixNode() {
	//glDeleteTextures(1, &this->mixtex);  reminder: !
	glDeleteBuffers(1, &this->mixfbo);
}
	

void Node::draw_connection(Node *node, CONN_TYPE ct) {
	float linec[] = {1.0, 1.0, 1.0, 1.0};
	if (ct == C_IN) {
		draw_line(linec,
			this->box->vtxcoords->x1, this->box->vtxcoords->y1 + this->box->vtxcoords->h / 2.0f,
			node->box->vtxcoords->x1 + node->box->vtxcoords->w, node->box->vtxcoords->y1 + node->box->vtxcoords->h / 2.0f);
	}
	else if (ct == C_IN2) {
		draw_line(linec,
			this->box->vtxcoords->x1 + this->box->vtxcoords->w / 2.0f, this->box->vtxcoords->y1,
			node->box->vtxcoords->x1 + node->box->vtxcoords->w, node->box->vtxcoords->y1 + node->box->vtxcoords->h / 2.0f);
	}
}

void VideoNode::upeffboxes() {
	for (int i = 0; i < mainprogram->nodesmain->currpage->nodes.size(); i++) {
		Node *node = mainprogram->nodesmain->currpage->nodes[i];
		if (node->align) {
			node->box->scrcoords->x1 = node->align->box->scrcoords->x1;
			node->box->scrcoords->y1 = node->align->box->scrcoords->y1 + (node->alignpos + 1) * 40;
			node->box->upscrtovtx();
		}
	}
}

void NodePage::connect_nodes(Node *node1, Node *node2) {
	if (node2->in) {
		if (std::find(node2->in->out.begin(), node2->in->out.end(), node2) != node2->in->out.end()) {
			node2->in->out.erase(std::find(node2->in->out.begin(), node2->in->out.end(), node2));
		}
	}
	node2->in = node1;
	node1->out.push_back(node2);
}

void NodePage::connect_in2(Node *node1, BlendNode *node2) {
	if (node2->in2) {
		if (std::find(node2->in2->out.begin(), node2->in2->out.end(), node2) != node2->in2->out.end()) {
			node2->in2->out.erase(std::find(node2->in2->out.begin(), node2->in2->out.end(), node2));
		}
	}
	node2->in2 = node1;
	node1->out.push_back(node2);
}

void NodePage::connect_in2(Node *node1, EffectNode *node2) {
	if (node2->in2) {
		if (std::find(node2->in2->out.begin(), node2->in2->out.end(), node2) != node2->in2->out.end()) {
			node2->in2->out.erase(std::find(node2->in2->out.begin(), node2->in2->out.end(), node2));
		}
	}
	node2->in2 = node1;
	node1->out.push_back(node2);
}

void NodePage::connect_nodes(Node *node1, Node *node2, BlendNode *bnode) {
	if (bnode->in) {
		if (bnode->in->out.size()) bnode->in->out.erase(std::find(bnode->in->out.begin(), bnode->in->out.end(), bnode));
	}
	bnode->in = node1;
	node1->out.push_back(bnode);
	if (bnode->in2) {
		if (bnode->in2->out.size()) {
			if (std::find(bnode->in2->out.begin(), bnode->in2->out.end(), bnode) != bnode->in2->out.end()) {
				bnode->in2->out.erase(std::find(bnode->in2->out.begin(), bnode->in2->out.end(), bnode));
			}
		}
	}
	bnode->in2 = node2;
	node2->out.push_back(bnode);
}

VideoNode *NodePage::add_videonode(int comp) {
	VideoNode *node = new VideoNode;
	node->type = VIDEO;
	if (comp == 1) {
		this->nodescomp.push_back(node);
		node->pos = this->nodescomp.size();
	}
	else if (comp == 0) {
		this->nodes.push_back(node);
		node->pos = this->nodes.size();
	}
	node->page = this;
	node->box->scrcoords->y1 = 800;
	node->box->scrcoords->w = 128;
	node->box->scrcoords->h = 72;
	node->box->upscrtovtx();
	
	return node;
}

MixNode *NodePage::add_mixnode(int scr, bool comp) {
	MixNode *node = new MixNode;
	node->type = MIX;
	if (comp) {
		node->pos = this->nodescomp.size();
		this->nodescomp.push_back(node);
		mainprogram->nodesmain->mixnodescomp.push_back(node);
	}
	else {
		node->pos = this->nodes.size();
		this->nodes.push_back(node);
		mainprogram->nodesmain->mixnodes.push_back(node);
	}
	node->page = this;
	node->box->scrcoords->x1 = 500 + node->pos * 192;
	node->box->scrcoords->y1 = 800;
	node->box->scrcoords->w = 128;
	node->box->scrcoords->h = 72;
	node->box->upscrtovtx();
	node->box->lcolor[0] = 1.0;
	node->box->lcolor[1] = 0.0;
	node->box->lcolor[2] = 0.0;
	node->box->lcolor[3] = 1.0;
	
	node->outputbox = new Box;
	node->outputbox->vtxcoords->x1 = -0.3;
	node->outputbox->vtxcoords->y1 = -1;
	node->outputbox->vtxcoords->w = 0.6;
	node->outputbox->vtxcoords->h = 0.6;
	node->outputbox->upvtxtoscr();
	node->outputbox->lcolor[0] = 1.0;
	node->outputbox->lcolor[1] = 0.0;
	node->outputbox->lcolor[2] = 0.0;
	node->outputbox->lcolor[3] = 1.0;
	node->outputbox->acolor[0] = 0.0;
	node->outputbox->acolor[1] = 0.0;
	node->outputbox->acolor[2] = 0.0;
	node->outputbox->acolor[3] = 1.0;
		
	return node;
}

EffectNode *NodePage::add_effectnode(Effect *effect, VideoNode *node, int pos, bool comp) {
	EffectNode *effnode = new EffectNode;
	effnode->type = EFFECT;
	effnode->effect = effect;
	effect->node = effnode;
	if (!comp) this->nodes.push_back(effnode);
	else this->nodescomp.push_back(effnode);
	effnode->box->lcolor[0] = 1.0;
	effnode->box->lcolor[1] = 1.0;
	effnode->box->lcolor[2] = 1.0;
	effnode->box->lcolor[3] = 1.0;
	effnode->box->acolor[0] = 0.5;
	effnode->box->acolor[1] = 0.8;
	effnode->box->acolor[2] = 0.3;
	effnode->box->acolor[3] = 1.0;
	effnode->box->scrcoords->w = 128;
	effnode->box->scrcoords->h = 30;
	node->upeffboxes();
	
	return effnode;
}

BlendNode *NodePage::add_blendnode(BLEND_TYPE btype, bool comp) {
	BlendNode *bnode = new BlendNode;
	bnode->type = BLEND;
	bnode->blendtype = btype;
	if (comp) this->nodescomp.push_back(bnode);
	else this->nodes.push_back(bnode);
	bnode->box->lcolor[0] = 1.0;
	bnode->box->lcolor[1] = 1.0;
	bnode->box->lcolor[2] = 1.0;
	bnode->box->lcolor[3] = 1.0;
	bnode->box->acolor[0] = 0.8;
	bnode->box->acolor[1] = 0.5;
	bnode->box->acolor[2] = 0.3;
	bnode->box->acolor[3] = 1.0;
	bnode->box->scrcoords->x1 = 500 + bnode->pos * 192;
	bnode->box->scrcoords->y1 = 900;
	bnode->box->scrcoords->w = 64;
	bnode->box->scrcoords->h = 30;
	bnode->box->upscrtovtx();
	
	return bnode;
}

void NodePage::delete_node(Node *node) {
	for(int i = 0; i < node->out.size(); i++) {
		if (node->out[i]->in == node) node->out[i]->in = nullptr;
		else if (node->out[i]->type == BLEND) {
			if (((BlendNode*)node->out[i])->in2 == node) ((BlendNode*)node->out[i])->in2 = nullptr;
		}
	}
	if (node->in) {
		for(int i = 0; i < node->in->out.size(); i++) {
			if (node->in->out[i] == node) {
				node->in->out.erase(node->in->out.begin() + i);
				break;
			}
		}
	}
	if (node->type == BLEND) {
		BlendNode *bnode = (BlendNode*)node;
		if (bnode->in2) {
			for(int i = 0; i < bnode->in2->out.size(); i++) {
				if (bnode->in2->out[i] == node) {
					bnode->in2->out.erase(bnode->in2->out.begin() + i);
					break;
				}
			}
		}
	}
	for (int i = 0; i < node->page->nodes.size(); i++) {
		if (node->page->nodes[i] == node) {
			node->page->nodes.erase(node->page->nodes.begin() + i);
			break;
		}
	}
	for (int i = 0; i < node->page->nodescomp.size(); i++) {
		if (node->page->nodescomp[i] == node) {
			node->page->nodescomp.erase(node->page->nodescomp.begin() + i);
			break;
		}
	}
	delete node;
}

void NodesMain::add_nodepages(int num) {
	for(int i = 0; i < num; i++) {
		NodePage *page = new NodePage;
		page->pos = this->pages.size();
		this->pages.push_back(page);
	}
	this->numpages += num;
}

void NodePage::handle_nodes() {
	for (int j = 0; j < mainprogram->nodesmain->currpage->nodes.size(); j++) {
		Node *node = mainprogram->nodesmain->currpage->nodes[j];
		Effect *eff;
		float white[] = {1.0, 1.0, 1.0, 1.0};
		float black[] = {0.0, 0.0, 0.0, 1.0};
		float red[] = {1.0, 0.0, 0.0, 1.0};
		float blue[] = {0.5, 0.5, 1.0, 1.0};
		if (node->monitor != -1) {
			if (mainmix->currscene[0] == node->monitor / 6) {
				draw_box(node->monbox, node->monbox->tex);
			}
		}
		
		if (node->type == VIDEO) {
			draw_box(node->box, ((VideoNode*)node)->layer->texture);
			((VideoNode*)node)->upeffboxes();
		}
		
		if (node->type == EFFECT) {
			if (((EffectNode*)node)->in2) {
				node->draw_connection(((EffectNode*)node)->in2, C_IN2);
			}
			Effect *eff = ((EffectNode*)node)->effect;
			draw_box(node->box, -1);
			std::string effstr;
			switch (eff->type) {
				case 0:
					effstr = "BLUR";
					break;
				case 1:
					effstr = "BRIGHTNESS";
					break;
				case 2:
					effstr = "COLORROT";
					break;
				case 3:
					effstr = "CONTRAST";
					break;
				case 4:
					effstr = "DOT";
					break;
				case 5:
					effstr = "GLOW";
					break;
				case 6:
					effstr = "RADIALBLUR";
					break;
				case 7:
					effstr = "SATURATION";
					break;
				case 8:
					effstr = "SCALE";
					break;
				case 9:
					effstr = "SWIRL";
					break;
				case 10:
					effstr = "OLDFILM";
					break;
				case 11:
					effstr = "RIPPLE";
					break;
				case 12:
					effstr = "FISHEYE";
					break;
				case 13:
					effstr = "TRESHOLD";
					break;
				case 14:
					effstr = "STROBE";
					break;
				case 15:
					effstr = "POSTERIZE";
					break;
				case 16:
					effstr = "PIXELATE";
					break;
				case 17:
					effstr = "CROSSHATCH";
					break;
				case 18:
					effstr = "INVERT";
					break;
				case 19:
					effstr = "ROTATE";
					break;
				case 20:
					effstr = "TURBULENCE";
					break;
				case 21:
					effstr = "ASCII";
					break;
				case 22:
					effstr = "SOLARIZE";
					break;
				case 23:
					effstr = "VARDOT";
					break;
				case 24:
					effstr = "MAZEEDGE";
					break;
			}
			render_text(effstr, white, node->box->vtxcoords->x1 + 0.01, node->box->vtxcoords->y1 + mainprogram->yscrtovtx(7), 0.0003, 0.0005);
			
			if (node->box->in()) {
				if (mainprogram->del) {
					mainprogram->nodesmain->currpage->delete_node(node);
					mainprogram->del = 0;
					break;
				}
				node->box->acolor[0] = 0.5;
				node->box->acolor[1] = 0.5;
				node->box->acolor[2] = 1.0;
				node->box->acolor[3] = 1.0;
			}
			else {
				node->box->acolor[0] = 0.5;
				node->box->acolor[1] = 0.8;
				node->box->acolor[2] = 0.3;
				node->box->acolor[3] = 1.0;
			}
			
			if (mainprogram->effectmenu->state == 1) {
				if (node->box->in()) {
					mainprogram->effectmenu->state = 3;
					std::vector<Layer*> AB;
					AB.reserve(mainmix->layersA.size() + mainmix->layersB.size() ); // preallocate memory
					AB.insert(AB.end(), mainmix->layersA.begin(), mainmix->layersA.end());
					AB.insert(AB.end(), mainmix->layersB.begin(), mainmix->layersB.end());
					bool found = false;
					for (int m = 0; m < AB.size(); m++) {
						for (int n = 0; n < AB[m]->effects[0].size(); n++) {
							if (eff == AB[m]->effects[0][n]) {
								mainmix->mouseeffect = n;
								mainmix->mouselayer = AB[m];
								found = true;
								break;
							}
						}
						if (found) break;
					}
					if (!found) mainmix->mousenode = node;
					mainmix->insert = 0;
				}
			}
		}
		
		if (node->type == MIDI) {
			draw_box(node->box, -1);
			std::string typestr;
			if (((MidiNode*)node)->param->midi[0] == 176) typestr = "Ctrl";
			else typestr = "Note";
			render_text(typestr, white, node->box->vtxcoords->x1 + 0.01, node->box->vtxcoords->y1 + mainprogram->yscrtovtx(7), 0.0003, 0.0005);
		}

		if (node->type == BLEND) {
			if (((BlendNode*)node)->in2) {
				node->draw_connection(((BlendNode*)node)->in2, C_IN2);
			}
		
			draw_box(node->box, -1);
			std::string mixstr;
			switch (((BlendNode*)node)->blendtype) {
				case 1:
					mixstr = "Mix";
					break;
				case 2:
					mixstr = "Mult";
					break;
				case 3:
					mixstr = "Scrn";
					break;
				case 4:
					mixstr = "Ovrl";
					break;
				case 5:
					mixstr = "HaLi";
					break;
				case 6:
					mixstr = "SoLi";
					break;
				case 7:
					mixstr = "Divi";
					break;
				case 8:
					mixstr = "Add";
					break;
				case 9:
					mixstr = "Sub";
					break;
				case 10:
					mixstr = "Diff";
					break;
				case 11:
					mixstr = "Dodg";
					break;
				case 12:
					mixstr = "CoBu";
					break;
				case 13:
					mixstr = "LiBu";
					break;
				case 14:
					mixstr = "ViLi";
					break;
				case 15:
					mixstr = "LiLi";
					break;
				case 16:
					mixstr = "DaOn";
					break;
				case 17:
					mixstr = "LiOn";
					break;
			}
			render_text(mixstr, white, node->box->vtxcoords->x1 + 0.01, node->box->vtxcoords->y1 + mainprogram->yscrtovtx(7), 0.0003, 0.0005);
			
			if (node->box->in()) {
				if (mainprogram->mixmodemenu->state == 1) {
					mainprogram->mixmodemenu->state = 2;
					mainmix->mousenode = node;
				}
			}
		}
		
		
		if (node->box->scrcoords->x1 - 10 < mainprogram->mx and mainprogram->mx < node->box->scrcoords->x1 + 10) {
			if (node->box->scrcoords->y1 - node->box->scrcoords->h / 2 - 10 < mainprogram->my and mainprogram->my < node->box->scrcoords->y1 - node->box->scrcoords->h / 2 + 10) {
				draw_box(blue, blue, node->box->vtxcoords->x1 - mainprogram->xscrtovtx(10), node->box->vtxcoords->y1 + node->box->vtxcoords->h / 2.0f - mainprogram->yscrtovtx(10), mainprogram->xscrtovtx(20), mainprogram->yscrtovtx(20), -1);
				if (mainprogram->leftmousedown) {
					mainprogram->leftmousedown = false;
					if (node->in) {
						node->in->dragging = C_OUT;
					}
					else {
						node->dragging = C_IN;
					}
				}
			}
		}
		if (node->type == BLEND) {
			if (node->box->scrcoords->x1 + node->box->scrcoords->w / 2 - 10 < mainprogram->mx and mainprogram->mx < node->box->scrcoords->x1 + node->box->scrcoords->w / 2 + 10) {
				if (node->box->scrcoords->y1 - 10 < mainprogram->my and mainprogram->my < node->box->scrcoords->y1 + 10) {
					draw_box(blue, blue, node->box->vtxcoords->x1 + node->box->vtxcoords->w / 2.0f - mainprogram->xscrtovtx(10), node->box->vtxcoords->y1 - mainprogram->yscrtovtx(10), mainprogram->xscrtovtx(20), mainprogram->yscrtovtx(20), -1);
					if (mainprogram->leftmousedown) {
						mainprogram->leftmousedown = false;
						node->dragging = C_IN2;
					}
				}
			}
		}
		if (node->box->scrcoords->x1 + node->box->scrcoords->w - 10 < mainprogram->mx and mainprogram->mx < node->box->scrcoords->x1 + node->box->scrcoords->w + 10) {
			if (node->box->scrcoords->y1 - node->box->scrcoords->h / 2 - 10 < mainprogram->my and mainprogram->my < node->box->scrcoords->y1 - node->box->scrcoords->h / 2 + 10) {
				draw_box(blue, blue, node->box->vtxcoords->x1 + node->box->vtxcoords->w - mainprogram->xscrtovtx(10), node->box->vtxcoords->y1 + node->box->vtxcoords->h / 2.0f - mainprogram->yscrtovtx(10), mainprogram->xscrtovtx(20), mainprogram->yscrtovtx(20), -1);
				if (mainprogram->leftmousedown) {
					mainprogram->leftmousedown = false;
					node->dragging = C_OUT;
					node->nocutting = true;
				}
			}	
		}
		if (node->dragging == C_OUT) {
			draw_line(white, node->box->vtxcoords->x1 + node->box->vtxcoords->w, node->box->vtxcoords->y1 + node->box->vtxcoords->h / 2.0f, -1 + mainprogram->xscrtovtx(mainprogram->mx), 1 - mainprogram->yscrtovtx(mainprogram->my));
			draw_box(white, white, node->box->vtxcoords->x1 + node->box->vtxcoords->w - mainprogram->xscrtovtx(10), node->box->vtxcoords->y1 + mainprogram->yscrtovtx(node->box->scrcoords->h / 2 - 10), mainprogram->xscrtovtx(20), mainprogram->yscrtovtx(20), -1);
		}
		else if (node->dragging == C_IN) {
			draw_line(white, node->box->vtxcoords->x1, node->box->vtxcoords->y1 + node->box->vtxcoords->h / 2.0f, -1 + mainprogram->xscrtovtx(mainprogram->mx), 1 - mainprogram->yscrtovtx(mainprogram->my));
			draw_box(white, white, node->box->vtxcoords->x1 - mainprogram->xscrtovtx(10), node->box->vtxcoords->y1 + mainprogram->yscrtovtx(node->box->scrcoords->h / 2 - 10), mainprogram->xscrtovtx(20), mainprogram->yscrtovtx(20), -1);
		}
		else if (node->dragging == C_IN2) {
			draw_line(white, node->box->vtxcoords->x1 + node->box->vtxcoords->w / 2.0f, node->box->vtxcoords->y1, -1 + mainprogram->xscrtovtx(mainprogram->mx), 1 - mainprogram->yscrtovtx(mainprogram->my));
			draw_box(white, white, node->box->vtxcoords->x1  + node->box->vtxcoords->w / 2.0f - mainprogram->xscrtovtx(10), node->box->vtxcoords->y1 - mainprogram->yscrtovtx(10), mainprogram->xscrtovtx(20), mainprogram->yscrtovtx(20), -1);
		}
		if (mainprogram->leftmouse) {
			if (node->dragging == C_OUT) {
				bool found = false;
				for (int i = 0; i < mainprogram->nodesmain->currpage->nodes.size(); i++) {
					Node *conn = mainprogram->nodesmain->currpage->nodes[i];
					if (conn->box->scrcoords->x1 - 10 < mainprogram->mx and mainprogram->mx < conn->box->scrcoords->x1 + 10) {
						if (conn->box->scrcoords->y1 - conn->box->scrcoords->h / 2 - 10 < mainprogram->my and mainprogram->my < conn->box->scrcoords->y1 - conn->box->scrcoords->h / 2 + 10) {
							node->out.push_back(conn);
							if (conn->in) {
								conn->in->out.erase(std::find(conn->in->out.begin(), conn->in->out.end(), conn));
							}
							conn->in = node;
							found = true;
						}
					}
					if (conn->type == BLEND) {
						BlendNode *bconn = (BlendNode*)conn;
						if (bconn->box->scrcoords->x1 + bconn->box->scrcoords->w / 2 - 10 < mainprogram->mx and mainprogram->mx < bconn->box->scrcoords->x1 + bconn->box->scrcoords->w / 2 + 10) {
							if (bconn->box->scrcoords->y1 - 10 < mainprogram->my and mainprogram->my < bconn->box->scrcoords->y1 + 10) {
								if (bconn->in2) {
									bconn->in2->out.erase(std::find(bconn->in2->out.begin(), bconn->in2->out.end(), bconn));
								}
								bconn->in2 = node;
								node->out.push_back(bconn);
								found = true;
							}
						}
					}
				}
				if (!found and !node->nocutting) {
					for (int i = 0; i < node->out.size(); i++) node->out[i]->in = nullptr;
					for (int i = 0; i < node->out.size(); i++) node->out.erase(node->out.begin() + i);
				}
				node->nocutting = false;
			}
			else if (node->dragging == C_IN) {
				for (int i = 0; i < mainprogram->nodesmain->currpage->nodes.size(); i++) {
					Node *conn = mainprogram->nodesmain->currpage->nodes[i];
					if (conn->box->scrcoords->x1 + conn->box->scrcoords->w - 10 < mainprogram->mx and mainprogram->mx < conn->box->scrcoords->x1 + conn->box->scrcoords->w + 10) {
						if (conn->box->scrcoords->y1 - conn->box->scrcoords->h / 2 - 10 < mainprogram->my and mainprogram->my < conn->box->scrcoords->y1 - conn->box->scrcoords->h / 2 + 10) {
							conn->out.push_back(node);
							node->in = conn;
						}
					}
				}
			}
			else if (node->dragging == C_IN2) {
				for (int i = 0; i < mainprogram->nodesmain->currpage->nodes.size(); i++) {
					Node *conn = mainprogram->nodesmain->currpage->nodes[i];
					if (conn->box->scrcoords->x1 + conn->box->scrcoords->w - 10 < mainprogram->mx and mainprogram->mx < conn->box->scrcoords->x1 + conn->box->scrcoords->w + 10) {
						if (conn->box->scrcoords->y1 - conn->box->scrcoords->h / 2 - 10 < mainprogram->my and mainprogram->my < conn->box->scrcoords->y1 - conn->box->scrcoords->h / 2 + 10) {
							mainprogram->nodesmain->currpage->connect_in2(conn, (BlendNode*)node);
						}
					}
				}
			}
			node->dragging = C_NONE;
		}
		
		
		if (!mainprogram->nodesmain->linked) {
			for (int i = 0; i < 6; i++) {
				if (node->monitor == i + mainmix->currscene[0] * 6) {
					draw_box(white, red, node->box->vtxcoords->x1 + mainprogram->xscrtovtx(1) + i * mainprogram->xscrtovtx(21), node->box->vtxcoords->y1 + node->box->vtxcoords->h, mainprogram->xscrtovtx(21), mainprogram->yscrtovtx(10), -1);
				}
				else if (node->box->scrcoords->x1 + 1 + i * 21 < mainprogram->mx and mainprogram->mx < node->box->scrcoords->x1 + 22 + i * 21 and node->box->scrcoords->y1 - node->box->scrcoords->h - 21 < mainprogram->my and mainprogram->my < node->box->scrcoords->y1 - node->box->scrcoords->h) {
					if (mainprogram->leftmouse) {
						for (int k = 0; k < mainprogram->nodesmain->currpage->nodes.size(); k++){
							if (mainprogram->nodesmain->currpage->nodes[k]->monitor == i + mainmix->currscene[0] * 6) {
								mainprogram->nodesmain->currpage->nodes[k]->monitor = -1;
							}
						}
						node->monitor = i + mainmix->currscene[0] * 6;
						make_layboxes();
					}
					draw_box(white, blue, node->box->vtxcoords->x1 + mainprogram->xscrtovtx(1) + i * mainprogram->xscrtovtx(21), node->box->vtxcoords->y1 + node->box->vtxcoords->h, mainprogram->xscrtovtx(21), mainprogram->yscrtovtx(10), -1);
			}
				else draw_box(white, black, node->box->vtxcoords->x1 + mainprogram->xscrtovtx(1) + i * mainprogram->xscrtovtx(21), node->box->vtxcoords->y1 + node->box->vtxcoords->h, mainprogram->xscrtovtx(21), mainprogram->yscrtovtx(10), -1);
			}
		}
				
		if (node->type == MIX) {
			draw_box(node->box, ((MixNode*)node)->mixtex);					
			if (node->box->scrcoords->x1 - 10 < mainprogram->mx and mainprogram->mx < node->box->scrcoords->x1 + 10) {
				if (node->box->scrcoords->y1 - 45 < mainprogram->my and mainprogram->my < node->box->scrcoords->y1 - 25) {
					draw_box(blue, blue, node->box->vtxcoords->x1 - mainprogram->xscrtovtx(10), node->box->vtxcoords->y1 + mainprogram->yscrtovtx(25), mainprogram->xscrtovtx(20), mainprogram->yscrtovtx(20), -1);
					if (mainprogram->leftmousedown) {
						mainprogram->leftmousedown = false;
						if (node->in) {
							node->in->dragging = C_OUT;
						}
						else {
							node->dragging = C_IN;
						}
					}
				}
			}
			if (node->dragging == C_IN) {
				draw_line(white, node->box->vtxcoords->x1, node->box->vtxcoords->y1 + mainprogram->yscrtovtx(32), -1 + mainprogram->xscrtovtx(mainprogram->mx), 1 - mainprogram->yscrtovtx(mainprogram->my));
				draw_box(white, white, node->box->vtxcoords->x1 - mainprogram->xscrtovtx(10), node->box->vtxcoords->y1 + mainprogram->yscrtovtx(25), mainprogram->xscrtovtx(20), mainprogram->yscrtovtx(20), -1);
			}
			if (mainprogram->leftmouse) {
				if (node->dragging == C_IN) {
					for (int i = 0; i < mainprogram->nodesmain->currpage->nodes.size(); i++) {
						Node *conn = mainprogram->nodesmain->currpage->nodes[i];
						if (conn->box->scrcoords->x1 + conn->box->scrcoords->w - 10 < mainprogram->mx and mainprogram->mx < conn->box->scrcoords->x1 + conn->box->scrcoords->w + 10) {
							if (conn->box->scrcoords->y1 - 45 < mainprogram->my and mainprogram->my < conn->box->scrcoords->y1 - 25) {
								conn->out.push_back(node);
								node->in = conn;
							}
						}
					}
				}
				node->dragging = C_NONE;
			}
			
			if (!mainprogram->nodesmain->linked) {
				for (int i = 0; i < 6; i++) {
					if (node->monitor == i + mainmix->currscene[0] * 6) {
						draw_box(white, red, node->box->vtxcoords->x1 + mainprogram->xscrtovtx(1) + i * mainprogram->xscrtovtx(21), node->box->vtxcoords->y1 + node->box->vtxcoords->h, mainprogram->xscrtovtx(21), mainprogram->yscrtovtx(10), -1);
					}
					else if (node->box->scrcoords->x1 + 1 + i * 21 < mainprogram->mx and mainprogram->mx < node->box->scrcoords->x1 + 22 + i * 21 and node->box->scrcoords->y1 - node->box->scrcoords->h - 21 < mainprogram->my and mainprogram->my < node->box->scrcoords->y1 - node->box->scrcoords->h) {
						if (mainprogram->leftmouse) {
							for (int k = 0; k < mainprogram->nodesmain->currpage->nodes.size(); k++){
								if (mainprogram->nodesmain->currpage->nodes[k]->monitor == i + mainmix->currscene[0] * 6) {
									mainprogram->nodesmain->currpage->nodes[k]->monitor = -1;
								}
							}
							node->monitor = i + mainmix->currscene[0] * 6;
							make_layboxes();
						}
						draw_box(white, blue, node->box->vtxcoords->x1 + mainprogram->xscrtovtx(1) + i * mainprogram->xscrtovtx(21), node->box->vtxcoords->y1 + node->box->vtxcoords->h, mainprogram->xscrtovtx(21), mainprogram->yscrtovtx(10), -1);
				}
					else draw_box(white, black, node->box->vtxcoords->x1 + mainprogram->xscrtovtx(1) + i * mainprogram->xscrtovtx(21), node->box->vtxcoords->y1 + node->box->vtxcoords->h, mainprogram->xscrtovtx(21), mainprogram->yscrtovtx(10), -1);
				}
			}
		}
		
		
		if (node->in) {
			node->draw_connection(node->in, C_IN);
		}
		
		if (node->box->in()) {
			if (mainprogram->leftmouse) {
				if (node->type == VIDEO) {
					VideoNode *vnode = (VideoNode*)node;
					for (int i = 0; i < vnode->layer->effects[0].size(); i++) {
						if (vnode->layer->effects[0][i]->node == mainprogram->nodesmain->currpage->movingnode) {
							vnode->layer->effects[0][i]->node->align = node;
							vnode->layer->effects[0][i]->node->alignpos = vnode->aligned;
							vnode->aligned += 1;
							vnode->upeffboxes();
						}
					}
				}
			}
			if (mainprogram->leftmousedown) {
				mainprogram->leftmousedown = false;
				if (!mainprogram->nodesmain->currpage->movingnode)
					mainprogram->nodesmain->currpage->movingnode = node;
					if (node->type == EFFECT) ((EffectNode*)node)->align = nullptr;
			}
		}
		if (mainprogram->nodesmain->currpage->movingnode == node) {
			node->box->scrcoords->x1 = mainprogram->mx - node->box->scrcoords->w / 2;
			node->box->scrcoords->y1 = mainprogram->my + node->box->scrcoords->h / 2;
			node->box->upscrtovtx();
			if (node->type == VIDEO) ((VideoNode*)node)->upeffboxes();
		}
		
		
		if (node->in) {
			if (pdistance(mainprogram->mx, mainprogram->my, node->box->scrcoords->x1, node->box->scrcoords->y1 - node->box->scrcoords->h / 2, 
					node->in->box->scrcoords->x1 + node->in->box->scrcoords->w, node->in->box->scrcoords->y1 - node->in->box->scrcoords->h / 2) < 6) {
				glLineWidth(3.0);
				draw_line(blue, node->box->vtxcoords->x1, node->box->vtxcoords->y1 + node->box->vtxcoords->h / 2.0f, node->in->box->vtxcoords->x1 + node->in->box->vtxcoords->w, node->in->box->vtxcoords->y1 + node->in->box->vtxcoords->h / 2.0f);
				if (mainprogram->middlemouse) {
					node->in->out.erase(std::find(node->in->out.begin(), node->in->out.end(), node));
					node->in = nullptr;
					mainprogram->middlemouse = 0;
				}
			}
			glLineWidth(1.0);
		}
		if (node->type == BLEND) {
			if (((BlendNode*)node)->in2) {
				Node *nin2 = ((BlendNode*)node)->in2;
				if (pdistance(mainprogram->mx, mainprogram->my, node->box->scrcoords->x1 + node->box->scrcoords->w / 2, node->box->scrcoords->y1, 
						nin2->box->scrcoords->x1 + nin2->box->scrcoords->w, nin2->box->scrcoords->y1 - nin2->box->scrcoords->h / 2) < 6) {
					glLineWidth(3.0);
					draw_line(blue, node->box->vtxcoords->x1 + node->box->vtxcoords->w / 2.0f, node->box->vtxcoords->y1, nin2->box->vtxcoords->x1 + nin2->box->vtxcoords->w, nin2->box->vtxcoords->y1 + nin2->box->vtxcoords->h / 2.0f);
					if (mainprogram->middlemouse) {
						nin2->out.erase(std::find(nin2->out.begin(), nin2->out.end(), node));
						((BlendNode*)node)->in2 = nullptr;
						mainprogram->middlemouse = 0;
					}
				}
				glLineWidth(1.0);
			}
		}
		
		if (node->box->in()) {
			if (mainprogram->middlemouse) {
				mainprogram->middlemouse = 0;
				if (node->type == VIDEO) {
					if (mainmix->deck == 0) mainmix->delete_layer(mainmix->layersA, ((VideoNode*)node)->layer, true);
					else mainmix->delete_layer(mainmix->layersB, ((VideoNode*)node)->layer, true);
				}
				else {
					mainprogram->nodesmain->currpage->delete_node(node);
				}
				break;
			}
		}
	}
		
	if (mainprogram->leftmouse) mainprogram->nodesmain->currpage->movingnode = nullptr;
	
	std::vector<Layer*> lvec;
	if (mainmix->deck = 0) lvec = mainmix->layersA;
	else lvec = mainmix->layersB;
	for (int i = 0; i < lvec.size(); i++) {
		Layer *lay = lvec[i];
		if (lay->node->vidbox->in()) {
			if (mainprogram->leftmousedown) {
				mainprogram->leftmousedown = 0;
				if (!mainprogram->nodesmain->floatingnode) {
					mainprogram->nodesmain->floatingnode = mainprogram->nodesmain->currpage->add_videonode(false);
				}
			}
			if (mainprogram->nodesmain->floatingnode) {
				mainprogram->nodesmain->floatingnode->box->scrcoords->x1 = mainprogram->mx - 64;
				mainprogram->nodesmain->floatingnode->box->scrcoords->y1 = mainprogram->my + 36;
				mainprogram->nodesmain->floatingnode->box->upscrtovtx();
			}
		}
		if (mainprogram->leftmouse) {
			mainprogram->nodesmain->floatingnode = nullptr;
		}
	}
}