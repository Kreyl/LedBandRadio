/*
 * ColorTable.h
 *
 *  Created on: 3 февр. 2017 г.
 *      Author: Kreyl
 */

#pragma once

#include "color.h"
//#include "SaveToFlash.h"

// ========= Constants ==========
#define SMOOTH_CONST    360 // Fade-in and Fade-out speed (lower is faster)

#if 1 // ============================ Color table ==============================
struct ClrProfile_t {
    Color_t Steady, Impact;
};

#define CLR_PROFILE_CNT     12
const ClrProfile_t ClrProfile[CLR_PROFILE_CNT] = {
                { {0,   196, 26 }, {255, 255, 255} },
                { {255, 0,   255}, {255, 255,   0} },
                { {34,  17,  255}, {0,   255, 255} },
                { {207, 36, 0  },  {255, 11,  255} },
                { {0,   17,  255}, {255, 11,  255} },
                { {105, 196, 187}, {255, 98,  9  } },
                { {255, 255, 0  }, {255, 11,  255} },
                { {0,   255, 0  }, {255, 128, 0  } },
                { {255, 27,  0  }, {255, 255, 0  } },
                { {187, 196, 2  }, {255, 0,   0  } },
                { {255, 0,   0  }, {255, 255, 27 } },
                { {63,  196, 0  }, {255, 255, 255} },
};
#endif

class Profile_t {
private:
    uint32_t ClrProfileIndx = 0;
public:
    void SelectNext() {
        ClrProfileIndx++;
        if(ClrProfileIndx >= CLR_PROFILE_CNT) ClrProfileIndx = 0;
        Printf("Profile %u\r", ClrProfileIndx);
    }
    void Load() {
        chSysLock();
//        Flash::Load(&ClrProfileIndx, 4);
        chSysUnlock();
        if(ClrProfileIndx >= CLR_PROFILE_CNT) ClrProfileIndx = 0;
        Printf("Profile loaded: %u\r", ClrProfileIndx);
    }
    void Save() {
//        if(Flash::Save(&ClrProfileIndx, 4) == OK) Uart.Printf("Saved\r");
    }
    Color_t GetClrSteady() { return ClrProfile[ClrProfileIndx].Steady; }
    Color_t GetClrImpact() { return ClrProfile[ClrProfileIndx].Impact; }
};

extern Profile_t Profile;
