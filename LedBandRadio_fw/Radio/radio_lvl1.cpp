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


void RxCallback() {
    if(CC.ReadFIFO(&Radio.PktRx, &Radio.PktRx.Rssi, RPKT_LEN) == retvOk) {  // if pkt successfully received
        Radio.RMsgQ.SendNowOrExitI(RMsg_t(rmsgPktRx));
    }
}

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
        chThdSleepMilliseconds(450);
//        if(CntTxFar > 0) {
//            CC.SetChannel(RCHNL_FAR);
//            CC.SetTxPower(TX_PWR_FAR);
//            CC.SetBitrate(CCBitrate10k);
//        //    CC.SetBitrate(CCBitrate2k4);
//            while(CntTxFar--) {
////                Printf("Far: %u\r", CntTxFar);
//                CC.Recalibrate();
//                CC.Transmit(&PktTxFar, RPKT_LEN);
//            }
//            CC.EnterIdle();
//            CC.SetChannel(RCHNL_EACH_OTH);
//            CC.SetTxPower(Cfg.TxPower);
//            CC.SetBitrate(CCBitrate500k);
//        }
//        else {
//            if(Cfg.IsAriKaesu()) {
//                CC.EnterIdle();
//                chThdSleepMilliseconds(540);
//            }
//            else { // Not Ari/Kaesu
//                RMsg_t msg = RMsgQ.Fetch(TIME_INFINITE);
//                switch(msg.Cmd) {
//                    case rmsgEachOthTx:
//                        CC.EnterIdle();
//                        CCState = ccstTx;
//                        PktTx.ID = Cfg.ID;
//                        PktTx.Cycle = RadioTime.CycleN;
//                        PktTx.TimeSrcID = RadioTime.TimeSrcId;
//                        CC.Recalibrate();
//                        CC.Transmit(&PktTx, RPKT_LEN);
//                        break;
//
//                    case rmsgEachOthRx:
//                        CCState = ccstRx;
//                        CC.ReceiveAsync(RxCallback);
//                        break;
//
//                    case rmsgEachOthSleep:
//                        CCState = ccstIdle;
//                        CC.EnterIdle();
//                        break;
//
//                    case rmsgPktRx:
//                        CCState = ccstIdle;
//        //                    Printf("ID=%u; t=%d; Rssi=%d\r", PktRx.ID, PktRx.Type, PktRx.Rssi);
//                        RxTableW->AddOrReplaceExistingPkt(PktRx);
//                        break;
//
//                    case rmsgFar:
//                        CC.SetChannel(RCHNL_FAR);
//                        CC.SetBitrate(CCBitrate10k);
//                        CC.Recalibrate();
//                        if(CC.Receive(36, &PktRx, RPKT_LEN, &PktRx.Rssi) == retvOk) {
////                            Printf("Far t=%d; Rssi=%d\r", PktRx.Type, PktRx.Rssi);
//                            RxTableW->AddOrReplaceExistingPkt(PktRx);
//                        }
//                        CC.EnterIdle();
//                        CC.SetChannel(RCHNL_EACH_OTH);
//                        CC.SetBitrate(CCBitrate500k);
//                        CC.Recalibrate();
//                        break;
//                } // switch
//            } // if Not Ari/Kaesu
//        }// if nothing to tx far
    } // while true
}
#endif // task

#if 1 // ============================
uint8_t rLevel1_t::Init() {
#ifdef DBG_PINS
    PinSetupOut(DBG_GPIO1, DBG_PIN1, omPushPull);
    PinSetupOut(DBG_GPIO2, DBG_PIN2, omPushPull);
#endif

    RMsgQ.Init();
    if(CC.Init() == retvOk) {
        CC.SetPktSize(RPKT_LEN);
        CC.DoIdleAfterTx();
        CC.SetChannel(RCHNL_EACH_OTH);
//        CC.SetTxPower(Cfg.TxPower);
        CC.SetBitrate(CCBitrate500k);
//        CC.EnterPwrDown();

        // Thread
        chThdCreateStatic(warLvl1Thread, sizeof(warLvl1Thread), HIGHPRIO, (tfunc_t)rLvl1Thread, NULL);
        return retvOk;
    }
    else return retvFail;
}
#endif
