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

#define BACK_CLR        (Color_t(0, 0, 0))
// On-off layer
#define SMOOTH_VAR      180

int32_t LedCnt = 17; // XXX

// Do not touch
#define BRT_MAX     255L

//static void SetColorRing(int32_t Indx, Color_t Clr) {
//    if(Indx < 0) return;
//    int32_t NStart = 0;
//    for(int32_t n=0; n < Leds.BandCnt; n++) { // Iterate bands
//        int32_t Length = Leds.BandSetup[n].Length; // to make things shorter
//        if(Indx < Length) {
//            uint32_t i;
//            if(Leds.BandSetup[n].Dir == dirForward) i = NStart + Indx;
//            else i = NStart + Length - 1 - Indx;
//            Leds.ClrBuf[i] = Clr;
//        }
//        NStart += Length; // Calculate start indx of next band
//    }
//}

//void MixToBuf(Color_t Clr, int32_t Brt, int32_t Indx) {
//    Printf("%u\r", Brt);
//    SetColorRing(Indx, Color_t(Clr, BACK_CLR, Brt));
//}

static void MixInto(uint32_t Indx, ColorHSV_t ClrHSV) {
    Printf("%u ", Indx);
    Color_t Clr = ClrHSV.ToRGB();
    Clr.Brt = 100;
    Leds.ClrBuf[Indx].MixWith(Clr);
}

#if 0 // ======= Flash =======
#define FLASH_CLR       (Color_t(0, 255, 9))
#define FLASH_CNT       3

void FlashTmrCallback(void *p);

class Flash_t {
private:
    int32_t IndxStart, Len;
    uint32_t DelayUpd_ms;  // Delay between updates
    virtual_timer_t ITmr;
    void StartTimerI(uint32_t ms) {
        chVTSetI(&ITmr, TIME_MS2I(ms), FlashTmrCallback, this);
    }
    systime_t strt;
public:
    int32_t EndIndx = FLASH_END_INDX; // which Indx to touch to consider flash ends
    Color_t Clr = FLASH_CLR;
    void Apply() {
        for(int32_t i=0; i<Len; i++) {
            MixToBuf(Clr, ((BRT_MAX * (Len - i)) / Len), IndxStart - i);
        }
    }

    void GenerateI(uint32_t DelayBefore_ms) {
        PrintfI("%u Dur: %u\r", this, TIME_I2MS(chVTTimeElapsedSinceX(strt)));
        strt = chVTGetSystemTimeX();
        DelayUpd_ms = 90; // Random::Generate(36, 63);
        IndxStart = -1;
//        Len = 11;
        Len = Random::Generate(9, 12);
//        Len = Random::Generate(14, 18);
        // Start delay before
        StartTimerI(DelayBefore_ms);
    }

    void OnTmrI() {
        IndxStart++; // Time to move
        // Check if path completed
        if((IndxStart - Len) > EndIndx) GenerateI(18);
        else StartTimerI(DelayUpd_ms);
    }
};

void FlashTmrCallback(void *p) {
    chSysLockFromISR();
    ((Flash_t*)p)->OnTmrI();
    chSysUnlockFromISR();
}

Flash_t FlashBuf[FLASH_CNT];
#endif

#if 1 // ======= OnOff Layer =======
void OnOffTmrCallback(void *p);

class OnOffLayer_t {
private:
    int32_t Brt = 0;
    enum State_t {stIdle, stFadingOut, stFadingIn} State;
    virtual_timer_t ITmr;
    void StartTimerI(uint32_t ms) {
        chVTSetI(&ITmr, TIME_MS2I(ms), OnOffTmrCallback, nullptr);
    }
public:
    void Apply() {
        if(State == stIdle) return; // No movement here
        for(int32_t i=0; i<Leds.LedCntTotal; i++) {
            ColorHSV_t ClrH;
            ClrH.FromRGB(Leds.ClrBuf[i]);
            ClrH.V = (ClrH.V * Brt) / BRT_MAX;
            Leds.ClrBuf[i].FromHSV(ClrH.H, ClrH.S, ClrH.V);
        }
    }

    void FadeIn() {
        State = stFadingIn;
        chSysLock();
        StartTimerI(ClrCalcDelay(Brt, SMOOTH_VAR));
        chSysUnlock();
    }

    void FadeOut() {
        State = stFadingOut;
        chSysLock();
        StartTimerI(ClrCalcDelay(Brt, SMOOTH_VAR));
        chSysUnlock();
    }

    void OnTmrI() {
        switch(State) {
            case stFadingIn:
                if(Brt == BRT_MAX) {
                    State = stIdle;
                    EvtQMain.SendNowOrExitI(EvtMsg_t(evtIdFadeInDone));
                }
                else {
                    Brt++;
                    StartTimerI(ClrCalcDelay(Brt, SMOOTH_VAR));
                }
                break;

            case stFadingOut:
                if(Brt == 0) {
                    State = stIdle;
                    EvtQMain.SendNowOrExitI(EvtMsg_t(evtIdFadeOutDone));
                }
                else {
                    Brt--;
                    StartTimerI(ClrCalcDelay(Brt, SMOOTH_VAR));
                }
                break;

            default: break;
        }
    }
} OnOffLayer;

void OnOffTmrCallback(void *p) {
    chSysLockFromISR();
    OnOffLayer.OnTmrI();
    chSysUnlockFromISR();
}
#endif

#if 1 // ========== Flare ==========
// Consts
#define MAX_TAIL_LEN            9
#define FLARE_FACTOR            8
#define FLARE_LEN_MAX           (FLARE_FACTOR * 20)
#define FLARE_K2                4096
#define FADE_CHANGE_PERIOD_ms   36

static int32_t FadeTable[] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,50,52,54,56,58,60,62,64,66,68,70,72,74,76,78,80,82,84,86,88,90,92,94,97,100};
static const int32_t kTable[MAX_TAIL_LEN+1] = { 0, 2500, 3300, 3640, 3780, 3860, 3920, 3960, 3990, 4011 };

class Flare_t {
private:
    int8_t IBrt[FLARE_LEN_MAX];
    virtual_timer_t ITmrMove, ITmrFade;
    int32_t TotalLen;
    void ConstructBrt(int32_t Len, int32_t LenTail, int32_t k1);
    int32_t HyperX;
    void StartFadeTmr(uint32_t Duration_ms);
    int32_t FadeIndx, FadeBrt; // FadeBrt: [0; 100]
    int32_t MaxDuration_ms;
public:
    enum State_t { flstNone, flstFadeIn, flstSteady, flstFadeout } State = flstNone;
    uint32_t MoveTick_ms;
    int32_t x0, CurrX, LenTail;
    ColorHSV_t Clr{120, 100, 100};
    bool NeedToStartNext;
    void OnMoveTickI();
    void OnFadeTickI();
    void Draw();
    void StartRandom(uint32_t ax0);
    void SteadyColor(uint32_t ax0, int32_t ClrH);
    void Stop();
};

static void IFlareTmrMoveCallback(void *p) {
    chSysLockFromISR();
    ((Flare_t*)p)->OnMoveTickI();
    chSysUnlockFromISR();
}

static void IFlareTmrFadeCallback(void *p) {
    chSysLockFromISR();
    ((Flare_t*)p)->OnFadeTickI();
    chSysUnlockFromISR();
}

void Flare_t::StartFadeTmr(uint32_t Duration_ms) {
    chVTSetI(&ITmrFade, TIME_MS2I(Duration_ms), IFlareTmrFadeCallback, this);
}

void Flare_t::StartRandom(uint32_t ax0) {
    chSysLock();
    chVTResetI(&ITmrMove);
    chVTResetI(&ITmrFade);
    chSysUnlock();
    // Choose params
    LenTail = 3;//Random::Generate(2, 4);
    MoveTick_ms = Random::Generate(45, 63);
    x0 = ax0;
    Clr.H = Random::Generate(120, 330);
    Printf("%d\r", Clr.H);
    ConstructBrt(1, LenTail, kTable[LenTail]);
    MaxDuration_ms = Random::Generate(2007, 3600);
    // Start
    NeedToStartNext = false;
    HyperX = 0;
    FadeIndx = 0;
    FadeBrt = 0;
    State = flstFadeIn;
    chSysLock();
    chVTSetI(&ITmrMove, TIME_MS2I(MoveTick_ms), IFlareTmrMoveCallback, this);
    StartFadeTmr(FADE_CHANGE_PERIOD_ms);
    chSysUnlock();
}

void Flare_t::Stop() {
    chSysLock();
    chVTResetI(&ITmrMove);
    chVTResetI(&ITmrFade);
    chSysUnlock();
}

void Flare_t::SteadyColor(uint32_t ax0, int32_t ClrH) {
    LenTail = 3;
    x0 = ax0;
    Clr.H = ClrH;
    ConstructBrt(1, LenTail, kTable[LenTail]);
    HyperX = 0;
    FadeBrt = 100;
}

void Flare_t::ConstructBrt(int32_t Len, int32_t LenTail, int32_t k1) {
    LenTail *= FLARE_FACTOR;
    Len *= FLARE_FACTOR;
    TotalLen = LenTail + Len + LenTail;
    // Draw Front Tail
    int32_t xi = LenTail-1, V = Clr.V;
    for(int32_t i=0; i<LenTail; i++) {
        V  = (V * k1) / FLARE_K2;
        IBrt[xi--] = V;
    }
    // Draw body
    xi = LenTail;
    for(int32_t i=0; i<Len; i++) IBrt[xi++] = Clr.V;
    // Draw Back Tail
    V = Clr.V;
    for(int32_t i=0; i<LenTail; i++) {
        V  = (V * k1) / FLARE_K2;
        IBrt[xi++] = V;
    }
//    for(int i=0; i<TotalLen; i++) Printf("%d,", IBrt[i]);
//    PrintfEOL();
}

void Flare_t::OnMoveTickI() {
    if(State == flstNone) return;
    HyperX++;
    chVTSetI(&ITmrMove, TIME_MS2I(MoveTick_ms), IFlareTmrMoveCallback, this);
}

void Flare_t::OnFadeTickI() {
//    PrintfI("ft st=%d i=%d\r", State, FadeIndx);
    switch(State) {
        case flstNone: break;
        case flstFadeIn:
            FadeBrt = FadeTable[FadeIndx];
            FadeIndx++;
            if(FadeIndx >= (int32_t)countof(FadeTable)) {
                State = flstSteady;
                StartFadeTmr(MaxDuration_ms);
            }
            else StartFadeTmr(FADE_CHANGE_PERIOD_ms);
            break;
        case flstSteady:
            NeedToStartNext = true; // Steady ended
            State = flstFadeout;
            FadeIndx = countof(FadeTable) - 1;
            StartFadeTmr(FADE_CHANGE_PERIOD_ms);
            break;
        case flstFadeout:
            FadeBrt = FadeTable[FadeIndx];
            FadeIndx--;
            if(FadeIndx >= 0) StartFadeTmr(FADE_CHANGE_PERIOD_ms);
            else State = flstNone;
            break;
    } // switch
}

void Flare_t::Draw() {
    int32_t HyperLedCnt = LedCnt * FLARE_FACTOR;
    if(HyperX >= HyperLedCnt) HyperX -= HyperLedCnt;
    ColorHSV_t IClr = Clr;
    int32_t xi = HyperX / FLARE_FACTOR;
    int32_t indx = HyperX % FLARE_FACTOR;
    int RealX = 0, PrevX = 32000;
    while(indx < TotalLen) {
        IClr.V = (IBrt[indx] * FadeBrt) / 100;
        RealX = x0 + xi;
//        if(RealX > PrevX) break;
//        Printf("%d %d;  ", RealX, PrevX);
        MixInto(RealX, IClr);
        PrevX = RealX;
        xi--;
        if(xi < 0) xi += LedCnt;
        indx += FLARE_FACTOR;
    } // while
    PrintfEOL();
    CurrX = RealX;
}

#define FLARE_CNT               2
Flare_t Flares[FLARE_CNT];
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
        for(uint32_t i=0; i<FLARE_CNT; i++) {
            if(Flares[i].State != Flare_t::flstNone) Flares[i].Draw();
            if(Flares[i].NeedToStartNext) {
                Flares[i].NeedToStartNext = false;
                // Construct next flare
                uint32_t j = i+1;
                if(j >= FLARE_CNT) j=0;
                if(j != i) {
//                    int32_t x0 = Flares[i].CurrX + LedCnt / FLARE_CNT + Flares[i].LenTail + 4;
//                    if(x0 >= LedCnt) x0 -= LedCnt;
//                    Flares[j].StartRandom(x0);
                    Flares[j].StartRandom(0);
                }
            }
        }


//        // Process OnOff
//        OnOffLayer.Apply();
//        // Show it
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
    Flares[0].StartRandom(0);
    chThdCreateStatic(waNpxThread, sizeof(waNpxThread), NORMALPRIO, (tfunc_t)NpxThread, nullptr);
}

void SetColor(Color_t AClr) {
//    for(uint32_t i=0; i<FLASH_CNT; i++) FlashBuf[i].Clr = AClr;
}

void FadeIn()  { OnOffLayer.FadeIn();  }
void FadeOut() { OnOffLayer.FadeOut(); }

} // namespace
