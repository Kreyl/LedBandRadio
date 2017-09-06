/*
 * IntelLedEffs.cpp
 *
 *  Created on: 31 рту. 2017 у.
 *      Author: Kreyl
 */

#include "IntelLedEffs.h"

IntelLeds_t Leds;

#if 1 // ========================== Common Effects =============================
EffAllTogetherNow_t EffAllTogetherNow;
EffAllTogetherSmoothly_t EffAllTogetherSmoothly;
//EffFadeOneByOne_t EffFadeOneByOne(180, clRGBWStars, clRGBWBlack);
EffAllTogetherSequence_t EffAllTogetherSequence;

static EffBase_t *PCurrentEff = nullptr;
static thread_reference_t PThd = nullptr;
static Color_t DesiredClr[LED_CNT];

static THD_WORKING_AREA(waEffectsThread, 64);
__noreturn
static void EffectsThread(void *arg) {
    chRegSetThreadName("Effects");
    while(true) {
        if(PCurrentEff == nullptr) {
            chSysLock();
            chThdSuspendS(&PThd);
            chSysUnlock();
        }
        else {
            if(PCurrentEff->Process() == effEnd) {
                PCurrentEff = nullptr;
            }
        }
    }
}

uint32_t ICalcDelayN(uint32_t n, uint32_t SmoothValue) {
    return Leds.ICurrentClr[n].DelayToNextAdj(DesiredClr[n], SmoothValue);
}

void LedEffectsInit() {
    Leds.Init();
    chThdCreateStatic(waEffectsThread, sizeof(waEffectsThread), HIGHPRIO, (tfunc_t)EffectsThread, NULL);
}
#endif

#if 1 // ========================= Individual effects ==========================
void EffAllTogetherNow_t::SetupAndStart(Color_t Color) {
    PCurrentEff = nullptr;
    for(uint32_t i=0; i<LED_CNT; i++) Leds.ICurrentClr[i] = Color;
    Leds.ISetCurrentColors();
}

void EffAllTogetherSmoothly_t::SetupAndStart(Color_t Color, uint32_t ASmoothValue) {
    if(ASmoothValue == 0) EffAllTogetherNow.SetupAndStart(Color);
    else {
        chSysLock();
        ISmoothValue = ASmoothValue;
        for(int32_t i=0; i<LED_CNT; i++) DesiredClr[i] = Color;
        PCurrentEff = this;
        chThdResumeS(&PThd, MSG_OK);
        chSysUnlock();
    }
}

EffState_t EffAllTogetherSmoothly_t::Process() {
    uint32_t Delay = 0;
    for(int32_t i=0; i<LED_CNT; i++) {
        uint32_t tmp = ICalcDelayN(i, ISmoothValue);  // }
        if(tmp > Delay) Delay = tmp;                  // } Calculate Delay
        Leds.ICurrentClr[i].Adjust(DesiredClr[i]);    // Adjust current color
    } // for
    Leds.ISetCurrentColors();
    if (Delay == 0) return effEnd;  // Setup completed
    else {
        chThdSleepMilliseconds(Delay);
        return effInProgress;
    }
}

// ======================== EffAllTogetherSequence_t ===========================
void EffAllTogetherSequence_t::SetupColors() {
    for(int32_t i=0; i<LED_CNT; i++) Leds.ICurrentClr[i] = ICurrColor;
    Leds.ISetCurrentColors();
}

void EffFadeOneByOne_t::SetupIDs() {
    for(uint32_t i=0; i<LED_CNT; i++) IDs[i] = i;
}

void EffFadeOneByOne_t::SetupAndStart(int32_t ThrLo, int32_t ThrHi) {
//    Printf("ThrLo: %d; ThrHi: %d\r", ThrLo, ThrHi);
    // Setup ColorLo
    for(int32_t i=0; i < ThrLo; i++) DesiredClr[i] = IClrLo;
    // Setup ColorHi
    for(int32_t i=ThrHi; i < LED_CNT; i++) DesiredClr[i] = IClrHi;
    // Setup gradient
    if(ThrHi > ThrLo) {
        int32_t Len = ThrHi - ThrLo;
        int32_t BrtStep = (255 * 1024) / Len;   // 255 is top brightness, 1024 is scaling coef
        for(int32_t i=0; i<Len; i++) {
            int32_t Indx = ThrLo + i;
            if(Indx >=0 and Indx < LED_CNT) {
                int32_t Brt = (i * BrtStep) / 1024;
//                Printf("%d Brt: %d\r", Indx, Brt);
                DesiredClr[Indx].BeMixOf(IClrHi, IClrLo, Brt);
            }
        }
    } // if(ThrHi > ThrLo)
    // Start processing
    chSysLock();
    PCurrentEff = this;
    chThdResumeS(&PThd, MSG_OK);
    chSysUnlock();
}
#endif
