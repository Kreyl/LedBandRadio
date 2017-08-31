/*
 * IntelLedEffs.h
 *
 *  Created on: 31 рту. 2017 у.
 *      Author: Kreyl
 */

#pragma once

#include <inttypes.h>
#include "color.h"
#include "ws2812b.h"

#if 1 // =============================== Chunk =================================
class LedChunk_t {
private:
    int Head, Tail;
    uint8_t GetNext(int *PCurrent);
    int GetPrevN(int Current, int N);
public:
    int Start, End, NLeds;
    Color_t Color;
    LedChunk_t(int AStart, int AEnd) {
        Start = AStart;
        End = AEnd;
        NLeds = 1;
        Head = AStart;
        Tail = AEnd;
        Color = clBlack;
    }
    uint32_t ProcessAndGetDelay();
    void StartOver();
};
#endif

#if 1 // ============================== Effects ================================
enum EffState_t {effEnd, effInProgress};

class EffBase_t {
public:
    virtual EffState_t Process() = 0;
};

class EffAllTogetherNow_t : public EffBase_t {
public:
    void SetupAndStart(Color_t Color);
    EffState_t Process() { return effEnd; } // Dummy, never used
};

class EffAllTogetherSmoothly_t : public EffBase_t {
protected:
    uint32_t ISmoothValue;
public:
    void SetupAndStart(Color_t Color, uint32_t ASmoothValue);
    EffState_t Process();
};

class EffFadeOneByOne_t : public EffAllTogetherSmoothly_t {
private:
    uint8_t IDs[LED_CNT];
    Color_t IClrLo, IClrHi;
public:
    void SetupAndStart(int32_t ThrLo, int32_t ThrHi);
    void SetupIDs();
    EffFadeOneByOne_t(uint32_t ASmoothValue, Color_t AClrLo, Color_t AClrHi) :
        IClrLo(AClrLo), IClrHi(AClrHi) { ISmoothValue = ASmoothValue; }
};

extern EffAllTogetherNow_t EffAllTogetherNow;
extern EffAllTogetherSmoothly_t EffAllTogetherSmoothly;
extern EffFadeOneByOne_t EffFadeOneByOne;

void LedEffectsInit();

#endif

