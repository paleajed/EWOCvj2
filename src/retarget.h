//
// Created by gert on 11/17/20.
//

#ifndef EWOCVJ2_RETARGET_H
#define EWOCVJ2_RETARGET_H

#endif //EWOCVJ2_RETARGET_H+


#ifdef USE_GLES
#include <GLES3/gl3.h>
#else
#include "GL/gl.h"
#endif

#include <map>
#include <string>



class Layer;
class Clip;
class ShelfElement;
class BinElement;
class StylePreparationElement;

class Retarget {
    public:
        Layer *lay = nullptr;
        Clip *clip = nullptr;
        ShelfElement *shelem = nullptr;
        BinElement *binel = nullptr;
        StylePreparationElement *stylelem = nullptr;
        GLuint tex;
        int filesize = 0;

        bool searchall = false;
        bool notfound = false;
        std::string solution;

        // remembers old (missing) path -> newly retargeted path for this
        // retargeting pass, so the same missing file found once (e.g. on a
        // clip) is applied automatically wherever else it turns up (e.g. on
        // a bin element), without asking the user again
        std::map<std::string, std::string> pathmemory;

        std::vector<std::string> searchdirs;
        std::vector<std::string> localsearchdirs;
        std::vector<std::string> globalsearchdirs;
        std::vector<Boxx*> searchboxes;
        std::vector<Button*> searchglobalbuttons;
        std::vector<Boxx*> searchclearboxes;

        Boxx* iconbox;
        Boxx* valuebox;
        Boxx* searchbox;
        Boxx* skipbox;
        Boxx* skipallbox;

        Retarget();

};