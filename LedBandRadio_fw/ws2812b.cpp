/*
 * ws2812b.cpp
 *
 *  Created on: 05 апр. 2014 г.
 *      Author: Kreyl
 */

#include "ws2812b.h"
#include "kl_lib.h"

// Tx timings: bit cnt
#define SEQ_1               0b1110  // 0xE
#define SEQ_0               0b1000  // 0x8

#define SEQ_00              0x88
#define SEQ_01              0x8E
#define SEQ_10              0xE8
#define SEQ_11              0xEE

void NpxDmaDone(void *p, uint32_t flags) {
    ((Neopixels_t*)p)->OnDmaDone();
}

void Neopixels_t::Init(int32_t ALedCnt) {
    PinSetupAlterFunc(Params->PGpio, Params->Pin, omPushPull, pudNone, Params->Af);
    Params->ISpi.Setup(boMSB, cpolIdleLow, cphaFirstEdge, 4000000, bitn16);
    Params->ISpi.Enable();
    Params->ISpi.EnableTxDma();

    // Allocate memory
//    Printf("LedCnt: %u\r", ALedCnt);
    LedCnt = ALedCnt;
    uint32_t TotalW16Cnt = TOTAL_W16_CNT(LedCnt);
    uint32_t TotalW32Cnt = (TotalW16Cnt + 1) / 2;
//    Printf("TotalW16Cnt: %u; TotalW32Cnt: %u\r", TotalW16Cnt, TotalW32Cnt);
    IBitBuf = (uint32_t*)malloc(TotalW32Cnt * 4);
    if(!IBitBuf) Printf("LedBuf alloc fail");
    // Zero head and tail
    for(uint32_t i=0; i<(RST_W16_CNT / 4); i++) {
        IBitBuf[i] = 0;
        IBitBuf[TotalW32Cnt - i - 1] = 0;
    }

    ClrBuf.resize(LedCnt);

    // ==== DMA ====
    dmaStreamAllocate     (Params->PDma, IRQ_PRIO_LOW, NpxDmaDone, (void*)this);
    dmaStreamSetPeripheral(Params->PDma, &Params->ISpi.PSpi->DR);
    dmaStreamSetMode      (Params->PDma, Params->DmaMode);
}

//void Neopixels_t::SetAll(Color_t Clr) {
//    for(int32_t i=0; i<LedCnt; i++) ClrBuf[i] = Clr;
//}

static const uint32_t ITable[256] = {
        0x88888888, 0x888E8888, 0x88E88888, 0x88EE8888, 0x8E888888, 0x8E8E8888, 0x8EE88888, 0x8EEE8888, 0xE8888888, 0xE88E8888, 0xE8E88888, 0xE8EE8888, 0xEE888888, 0xEE8E8888, 0xEEE88888, 0xEEEE8888,
        0x8888888E, 0x888E888E, 0x88E8888E, 0x88EE888E, 0x8E88888E, 0x8E8E888E, 0x8EE8888E, 0x8EEE888E, 0xE888888E, 0xE88E888E, 0xE8E8888E, 0xE8EE888E, 0xEE88888E, 0xEE8E888E, 0xEEE8888E, 0xEEEE888E,
        0x888888E8, 0x888E88E8, 0x88E888E8, 0x88EE88E8, 0x8E8888E8, 0x8E8E88E8, 0x8EE888E8, 0x8EEE88E8, 0xE88888E8, 0xE88E88E8, 0xE8E888E8, 0xE8EE88E8, 0xEE8888E8, 0xEE8E88E8, 0xEEE888E8, 0xEEEE88E8,
        0x888888EE, 0x888E88EE, 0x88E888EE, 0x88EE88EE, 0x8E8888EE, 0x8E8E88EE, 0x8EE888EE, 0x8EEE88EE, 0xE88888EE, 0xE88E88EE, 0xE8E888EE, 0xE8EE88EE, 0xEE8888EE, 0xEE8E88EE, 0xEEE888EE, 0xEEEE88EE,
        0x88888E88, 0x888E8E88, 0x88E88E88, 0x88EE8E88, 0x8E888E88, 0x8E8E8E88, 0x8EE88E88, 0x8EEE8E88, 0xE8888E88, 0xE88E8E88, 0xE8E88E88, 0xE8EE8E88, 0xEE888E88, 0xEE8E8E88, 0xEEE88E88, 0xEEEE8E88,
        0x88888E8E, 0x888E8E8E, 0x88E88E8E, 0x88EE8E8E, 0x8E888E8E, 0x8E8E8E8E, 0x8EE88E8E, 0x8EEE8E8E, 0xE8888E8E, 0xE88E8E8E, 0xE8E88E8E, 0xE8EE8E8E, 0xEE888E8E, 0xEE8E8E8E, 0xEEE88E8E, 0xEEEE8E8E,
        0x88888EE8, 0x888E8EE8, 0x88E88EE8, 0x88EE8EE8, 0x8E888EE8, 0x8E8E8EE8, 0x8EE88EE8, 0x8EEE8EE8, 0xE8888EE8, 0xE88E8EE8, 0xE8E88EE8, 0xE8EE8EE8, 0xEE888EE8, 0xEE8E8EE8, 0xEEE88EE8, 0xEEEE8EE8,
        0x88888EEE, 0x888E8EEE, 0x88E88EEE, 0x88EE8EEE, 0x8E888EEE, 0x8E8E8EEE, 0x8EE88EEE, 0x8EEE8EEE, 0xE8888EEE, 0xE88E8EEE, 0xE8E88EEE, 0xE8EE8EEE, 0xEE888EEE, 0xEE8E8EEE, 0xEEE88EEE, 0xEEEE8EEE,
        0x8888E888, 0x888EE888, 0x88E8E888, 0x88EEE888, 0x8E88E888, 0x8E8EE888, 0x8EE8E888, 0x8EEEE888, 0xE888E888, 0xE88EE888, 0xE8E8E888, 0xE8EEE888, 0xEE88E888, 0xEE8EE888, 0xEEE8E888, 0xEEEEE888,
        0x8888E88E, 0x888EE88E, 0x88E8E88E, 0x88EEE88E, 0x8E88E88E, 0x8E8EE88E, 0x8EE8E88E, 0x8EEEE88E, 0xE888E88E, 0xE88EE88E, 0xE8E8E88E, 0xE8EEE88E, 0xEE88E88E, 0xEE8EE88E, 0xEEE8E88E, 0xEEEEE88E,
        0x8888E8E8, 0x888EE8E8, 0x88E8E8E8, 0x88EEE8E8, 0x8E88E8E8, 0x8E8EE8E8, 0x8EE8E8E8, 0x8EEEE8E8, 0xE888E8E8, 0xE88EE8E8, 0xE8E8E8E8, 0xE8EEE8E8, 0xEE88E8E8, 0xEE8EE8E8, 0xEEE8E8E8, 0xEEEEE8E8,
        0x8888E8EE, 0x888EE8EE, 0x88E8E8EE, 0x88EEE8EE, 0x8E88E8EE, 0x8E8EE8EE, 0x8EE8E8EE, 0x8EEEE8EE, 0xE888E8EE, 0xE88EE8EE, 0xE8E8E8EE, 0xE8EEE8EE, 0xEE88E8EE, 0xEE8EE8EE, 0xEEE8E8EE, 0xEEEEE8EE,
        0x8888EE88, 0x888EEE88, 0x88E8EE88, 0x88EEEE88, 0x8E88EE88, 0x8E8EEE88, 0x8EE8EE88, 0x8EEEEE88, 0xE888EE88, 0xE88EEE88, 0xE8E8EE88, 0xE8EEEE88, 0xEE88EE88, 0xEE8EEE88, 0xEEE8EE88, 0xEEEEEE88,
        0x8888EE8E, 0x888EEE8E, 0x88E8EE8E, 0x88EEEE8E, 0x8E88EE8E, 0x8E8EEE8E, 0x8EE8EE8E, 0x8EEEEE8E, 0xE888EE8E, 0xE88EEE8E, 0xE8E8EE8E, 0xE8EEEE8E, 0xEE88EE8E, 0xEE8EEE8E, 0xEEE8EE8E, 0xEEEEEE8E,
        0x8888EEE8, 0x888EEEE8, 0x88E8EEE8, 0x88EEEEE8, 0x8E88EEE8, 0x8E8EEEE8, 0x8EE8EEE8, 0x8EEEEEE8, 0xE888EEE8, 0xE88EEEE8, 0xE8E8EEE8, 0xE8EEEEE8, 0xEE88EEE8, 0xEE8EEEE8, 0xEEE8EEE8, 0xEEEEEEE8,
        0x8888EEEE, 0x888EEEEE, 0x88E8EEEE, 0x88EEEEEE, 0x8E88EEEE, 0x8E8EEEEE, 0x8EE8EEEE, 0x8EEEEEEE, 0xE888EEEE, 0xE88EEEEE, 0xE8E8EEEE, 0xE8EEEEEE, 0xEE88EEEE, 0xEE8EEEEE, 0xEEE8EEEE, 0xEEEEEEEE,
};

void Neopixels_t::SetCurrentColors() {
    TransmitDone = false;
    uint32_t *p = IBitBuf + (RST_W16_CNT / 4); // First and last words are zero to form reset
    // Fill bit buffer
//    systime_t LStart = chVTGetSystemTimeX();
    for(auto &Color : ClrBuf) {
        *p++ = ITable[Color.G];
        *p++ = ITable[Color.R];
        *p++ = ITable[Color.B];
    }
//    PrintfI("t: %u\r", chVTGetSystemTimeX() - LStart);
    // Start transmission
    dmaStreamDisable(Params->PDma);
    dmaStreamSetMemory0(Params->PDma, IBitBuf);
    dmaStreamSetTransactionSize(Params->PDma, TOTAL_W16_CNT(LedCnt));
    dmaStreamSetMode(Params->PDma, Params->DmaMode);
    dmaStreamEnable(Params->PDma);
}

void Neopixels_t::OnDmaDone() {
    TransmitDone = true;
    if(OnTransmitEnd) OnTransmitEnd();
}
