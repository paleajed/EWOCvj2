//
// Created by gert on 11/17/20.
//

#ifndef EWOCVJ2_RETARGET_H
#define EWOCVJ2_RETARGET_H

#endif //EWOCVJ2_RETARGET_H+


#include "GL/gl.h"



class Layer;
class Clip;
class ShelfElement;
class BinElement;

class Retarget {
    public:
        Layer *lay = nullptr;
        Clip *clip = nullptr;
        ShelfElement *shelem = nullptr;
        BinElement *binel = nullptr;
        GLuint tex;
        int filesize = 0;

        bool searchall = false;
        bool notfound = false;
        std::string solution;

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