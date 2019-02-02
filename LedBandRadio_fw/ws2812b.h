/*
 * ws2812b.h
 *
 *  Created on: 05 апр. 2014 г.
 *      Author: Kreyl
 */

#pragma once

/*
 * ========== WS2812 control module ==========
 * Only basic command "SetCurrentColors" is implemented, all other is up to
 * higher level software.
 * SPI input frequency should be 8 MHz, which results in 4MHz bitrate =>
 * 250nS per bit. Then, LED's 0 is 1000 (250 + 750), LED's 1 is 1110 (750 + 250).
 * Originally, (400 + 850) +- 150 each.
 */


#include "ch.h"
#include "hal.h"
#include "kl_lib.h"
#include "color.h"
#include "uart.h"
#include <vector>

typedef std::vector<Color_t> ColorBuf_t;

// Do not touch
#define SEQ_LEN_BITS        4 // 4 bits of SPI to produce 1 bit of LED data
// Total zero words before and after data to produce reset: 16 * 4us = 64us (50us required)
#define RST_W16_CNT         16 // Multiply of 4 is required

// SPI16 Buffer (no tuning required)
#define DATA_BIT_CNT(Cnt)   (Cnt * 3 * 8 * SEQ_LEN_BITS) // Each led has 3 channels 8 bit each
#define DATA_W16_CNT(Cnt)   ((DATA_BIT_CNT(Cnt) + 15) / 16)
#define TOTAL_W16_CNT(Cnt)  (DATA_W16_CNT(Cnt) + RST_W16_CNT)

#define NPX_DMA_MODE(Chnl) \
                        (STM32_DMA_CR_CHSEL(Chnl) \
                        | DMA_PRIORITY_HIGH \
                        | STM32_DMA_CR_MSIZE_HWORD \
                        | STM32_DMA_CR_PSIZE_HWORD \
                        | STM32_DMA_CR_MINC     /* Memory pointer increase */ \
                        | STM32_DMA_CR_DIR_M2P)  /* Direction is memory to peripheral */ \
                        | STM32_DMA_CR_TCIE

struct NeopixelParams_t {
    // SPI
    Spi_t ISpi;
    GPIO_TypeDef *PGpio;
    uint16_t Pin;
    AlterFunc_t Af;
    // DMA
    const stm32_dma_stream_t *PDma;
    uint32_t DmaMode;
    NeopixelParams_t(SPI_TypeDef *ASpi,
            GPIO_TypeDef *APGpio, uint16_t APin, AlterFunc_t AAf,
            const stm32_dma_stream_t *APDma, uint32_t ADmaMode) :
                ISpi(ASpi), PGpio(APGpio), Pin(APin), Af(AAf),
                PDma(APDma), DmaMode(ADmaMode) {}
};


class Neopixels_t {
private:
    const NeopixelParams_t *Params;
    uint32_t *IBitBuf = nullptr;
public:
    Neopixels_t(const NeopixelParams_t *APParams) : Params(APParams) {}

    int32_t LedCnt = 0;

    void Init(int32_t ALedCnt);
//    bool AreOff() {
//        for(uint8_t i=0; i<LedCnt; i++) {
//            if(ClrBuf[i] != clBlack) return false;
//        }
//        return true;
//    }
    bool TransmitDone = false;
    ftVoidVoid OnTransmitEnd = nullptr;
    ColorBuf_t ClrBuf;
    // Methods
    void SetCurrentColors();
    void SetAll(Color_t Clr);
    void OnDmaDone();
};
