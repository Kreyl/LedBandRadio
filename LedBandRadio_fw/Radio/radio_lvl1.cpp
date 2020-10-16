/*
 * radio_lvl1.cpp
 *
 *  Created on: Nov 17, 2013
 *      Author: kreyl
 */

#include "radio_lvl1.h"
#include "cc1101.h"
#include "uart.h"

#include "led.h"
#include "Sequences.h"

#include "Effects.h"

cc1101_t CC(CC_Setup0);

//#define DBG_PINS

#ifdef DBG_PINS
#define DBG_GPIO1   GPIOB
#define DBG_PIN1    10
#define DBG1_SET()  PinSetHi(DBG_GPIO1, DBG_PIN1)
#define DBG1_CLR()  PinSetLo(DBG_GPIO1, DBG_PIN1)
#define DBG_GPIO2   GPIOB
#define DBG_PIN2    9
#define DBG2_SET()  PinSetHi(DBG_GPIO2, DBG_PIN2)
#define DBG2_CLR()  PinSetLo(DBG_GPIO2, DBG_PIN2)
#else
#define DBG1_SET()
#define DBG1_CLR()
#endif

rLevel1_t Radio;

extern PinOutput_t LedWsEn;
extern LedOnOff_t Led;

enum {staOff, staFire, staWhite} State = staFire;

#if 1 // ================================ Task =================================
static THD_WORKING_AREA(warLvl1Thread, 256);
__noreturn
static void rLvl1Thread(void *arg) {
    chRegSetThreadName("rLvl1");
    Radio.ITask();
}

__noreturn
void rLevel1_t::ITask() {
    while(true) {
        CC.Recalibrate();
        if(CC.Receive(81, &PktRx, RPKT_LEN, &PktRx.Rssi) == retvOk) {
            Led.Off();
            Printf("Rx: %u; RSSI: %d\r", PktRx.RCmd, PktRx.Rssi);
            bool TxOk = true;
            switch(PktRx.RCmd) {
                case 9: // On/off
                    if(State == staOff) {
                        State = staFire;
                        LedWsEn.SetLo();
                        Eff::FadeIn();
                        Eff::StartFlaming();
                    }
                    else {
                        State = staOff;
                        Eff::FadeOut();
                    }
                    break;

                case 27: // Be White
                    if(State == staFire) {
                        State = staWhite;
                        Eff::BeWhite();
                    }
                    break;

                case 36: // BeFire
                    if(State == staWhite) {
                        State = staFire;
                        Eff::StartFlaming();
                    }
                    break;

                case 45: // Flash
                    if(State == staFire) Eff::DoFlash();
                    break;

                default:
                    TxOk = false;
                    break;
            } // switch
            if(TxOk) {
                PktTx.DW32 = 0xCA115EA1;
                CC.Transmit(&PktTx, RPKT_LEN);
            }
            Led.On();
        } // if rx ok
    } // while true
}
#endif // task

#if 1 // ============================
uint8_t rLevel1_t::Init() {
#ifdef DBG_PINS
    PinSetupOut(DBG_GPIO1, DBG_PIN1, omPushPull);
    PinSetupOut(DBG_GPIO2, DBG_PIN2, omPushPull);
#endif

    if(CC.Init() == retvOk) {
        CC.SetPktSize(RPKT_LEN);
        CC.DoIdleAfterTx();
        CC.SetChannel(0);
        CC.SetTxPower(CC_PwrPlus5dBm);
        CC.SetBitrate(CCBitrate100k);
//        CC.EnterPwrDown();

        // Thread
        chThdCreateStatic(warLvl1Thread, sizeof(warLvl1Thread), HIGHPRIO, (tfunc_t)rLvl1Thread, NULL);
        return retvOk;
    }
    else return retvFail;
}
#endif
