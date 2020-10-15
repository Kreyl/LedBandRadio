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
#define SMOOTH_VAR  180

#if 1 // ============================= Flare ===================================
static void IFlareTmrMoveCallback(void *p);

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
        Len = Random::Generate(3, 10);
        int32_t h = Random::Generate(-12, 22);
        MoveTick_ms = Random::Generate(27, 81);
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

#if 1 // ========================== Total Fill =================================
static void ITotalFillTmrCallback(void *p);

class TotalFill_t {
private:
    int32_t Brt = 0;
    virtual_timer_t ITmr;
    void StartTimerI() { chVTSetI(&ITmr, TIME_MS2I(ClrCalcDelay(Brt, 180)), ITotalFillTmrCallback, this); }
    void StartTimerI(uint32_t Delay_ms) { chVTSetI(&ITmr, TIME_MS2I(Delay_ms), ITotalFillTmrCallback, this); }
    void StartTimer() {
        chSysLock();
        StartTimerI();
        chSysUnlock();
    }
    enum {tfstIdle, tfstFadeInAndWait, tfstWaitAndFadeOut, tfstFadeInAndStop, tfstFadeOut} State = tfstIdle;
    Color_t Clr;
public:
    void DoFlash() {
        State = tfstFadeInAndWait;
        Brt = 0;
        Clr = clRed;
        StartTimer();
    }

    void Draw() {
        if(State == tfstIdle) return;
        Clr.Brt = Brt;
        Leds.MixAllwWeight(Clr, Brt);
    }

    void OnTickI() {
        switch(State) {
            case tfstIdle: return; break;

            case tfstFadeInAndWait:
                if(Brt >= 255) {
                    State = tfstWaitAndFadeOut;
                    StartTimerI(450);
                }
                else {
                    Brt++;
                    StartTimerI();
                }
                break;

            case tfstFadeInAndStop:
                if(Brt >= 255) State = tfstIdle;
                else {
                    Brt++;
                    StartTimerI();
                }
                break;

            case tfstWaitAndFadeOut: // End of delay
                State = tfstFadeOut;
                StartTimerI();
                break;

            case tfstFadeOut:
                if(Brt <= 0) State = tfstIdle;
                else {
                    Brt--;
                    StartTimerI();
                }
                break;
        } // switch
    }
} TotalFill;

static void ITotalFillTmrCallback(void *p) {
    chSysLockFromISR();
    ((TotalFill_t*)p)->OnTickI();
    chSysUnlockFromISR();
}
#endif

// Thread
static THD_WORKING_AREA(waNpxThread, 512);
__noreturn
static void NpxThread(void *arg) {
    chRegSetThreadName("Npx");
    while(true) {
        chThdSleepMilliseconds(9);
        Leds.SetAll(Color_t{27, 27, 0}); // Reset colors
        // Draw Foot Ring
        for(int i=0; i<BAND_CNT; i++) {
            Leds.MixIntoBand(0, i, hsvRed);
        }
        // Draw flares
        for(Flare_t &Flare : Flares) {
            if(Flare.State == Flare_t::flstMoving) Flare.Draw();
            else Flare.Start();
        }

        // TotalFill
        TotalFill.Draw();

        // Show it
        Leds.SetCurrentColors();
    }
}

namespace Eff {
void Init() {
    chThdCreateStatic(waNpxThread, sizeof(waNpxThread), NORMALPRIO, (tfunc_t)NpxThread, nullptr);
}

void EnterOff() {
//    OnOffLayer.FadeOut();
}

void StartFlaming() {
    State = staFire;
}

void DoFlash() {
    TotalFill.DoFlash();
}


//void FadeIn()  { OnOffLayer.FadeIn();  }
//void FadeOut() { OnOffLayer.FadeOut(); }

} // namespace