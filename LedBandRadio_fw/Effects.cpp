/*
 * Effects.cpp
 *
 *  Created on: 5 dec 2019
 *      Author: Kreyl
 */

#include "Effects.h"
#include "ch.h"
#include "kl_lib.h"
#include "color.h"
#include "ws2812b.h"
#include "MsgQ.h"
#include "board.h"

extern Neopixels_t Leds;

#define BACK_CLR    clBlack

#if 1 // ============================= Flare ===================================
static void IFlareTmrMoveCallback(void *p);

//
class Flare_t {
private:
    virtual_timer_t ITmrMove;

public:
    Flare_t(int32_t BandIndx) : BandIndx(BandIndx) {}
    enum State_t { flstIdle, flstMoving } State = flstIdle;
    uint32_t MoveTick_ms = 54; // Move every MoveTick_ms
    int32_t Len = 4;
    int32_t CurrX = 0;
    uint32_t BandIndx = 0;
    ColorHSV_t ClrHSV = hsvRed;


    void Start() {
        chSysLock();
        chVTResetI(&ITmrMove);
        chSysUnlock();
        CurrX = 0;
        State = flstMoving;
        Len = Random::Generate(3, 8);
        int32_t h = Random::Generate(-12, 22);
        MoveTick_ms = Random::Generate(27, 108);
        if(h < 0) h += 360L;
        ClrHSV.H = h;
        chSysLock();
        chVTSetI(&ITmrMove, TIME_MS2I(MoveTick_ms), IFlareTmrMoveCallback, this);
        chSysUnlock();
    }

    void Draw() {
        for(int32_t i=0; i<Len; i++) {
            Leds.MixIntoBand(CurrX - i, BandIndx, ClrHSV);
        }
    }


    // Inner use
    void OnMoveTickI() {
        if((CurrX - Len) < Leds.BandSetup[BandIndx].Length) {
            CurrX++;
            chVTSetI(&ITmrMove, TIME_MS2I(MoveTick_ms), IFlareTmrMoveCallback, this);
        }
        else State = flstIdle;
    }

};

static void IFlareTmrMoveCallback(void *p) {
    chSysLockFromISR();
    ((Flare_t*)p)->OnMoveTickI();
    chSysUnlockFromISR();
}

static Flare_t Flares[] = { 0,1,2,3,4,5,6,7,8 };

#endif


// Thread
static THD_WORKING_AREA(waNpxThread, 512);
__noreturn
static void NpxThread(void *arg) {
    chRegSetThreadName("Npx");
    while(true) {
        chThdSleepMilliseconds(9);
        // Reset colors
        Leds.SetAll(BACK_CLR);

        // ==== Draw flares ====
        for(Flare_t &Flare : Flares) {
            if(Flare.State == Flare_t::flstMoving) Flare.Draw();
            else Flare.Start();

        }

        // Show it
        Leds.SetCurrentColors();
    }
}

namespace Eff {
void Init() {
//    for(uint32_t i=0; i<FLASH_CNT; i++) {
//        chSysLock();
//        FlashBuf[i].GenerateI(i * 1107 + 9);
//        chSysUnlock();
//    }

    chThdCreateStatic(waNpxThread, sizeof(waNpxThread), NORMALPRIO, (tfunc_t)NpxThread, nullptr);
}

void SetColor(Color_t AClr) {
//    for(uint32_t i=0; i<FLASH_CNT; i++) FlashBuf[i].Clr = AClr;
}

//void FadeIn()  { OnOffLayer.FadeIn();  }
//void FadeOut() { OnOffLayer.FadeOut(); }

} // namespace
