/*
 * main.cpp
 *
 *  Created on: 20 февр. 2014 г.
 *      Author: g.kruglov
 */

#include "board.h"
#include "uart.h"
#include "shell.h"
#include "MsgQ.h"
#include "led.h"
#include "radio_lvl1.h"
#include "sk6812.h"

#if 1 // ======================== Variables and defines ========================
// Forever
EvtMsgQ_t<EvtMsg_t, MAIN_EVT_Q_LEN> EvtQMain;
extern CmdUart_t Uart;
static void ITask();
static void OnCmd(Shell_t *PShell);

static void PercentToFade(int32_t Percent);

LedOnOff_t Led {LED_PIN};
PinOutput_t LedWsEn {LED_WS_EN};
Color_t Clr(0, 255, 0, 0);
#endif

int main(void) {
    // ==== Init Clock system ====
    SetupVCore(vcore1V5);
    Clk.EnablePrefetch();
    Clk.SetupFlashLatency(8);
//    Clk.SetupPLLDividers(pllMul4, pllDiv2);
//    Clk.SetupPLLSrc(pllSrcHSE);
//    Clk.SwitchToPLL();
    Clk.SetupBusDividers(ahbDiv2, apbDiv1, apbDiv1);
    Clk.SwitchToHSI();
    Clk.UpdateFreqValues();

    // === Init OS ===
    halInit();
    chSysInit();
    EvtQMain.Init();

    // ==== Init hardware ====
    Uart.Init(115200);
    Printf("\r%S %S\r", APP_NAME, BUILD_TIME);
    Clk.PrintFreqs();

    // Debug LED
    Led.Init();
    Led.On();

    if(Radio.Init() != retvOk) {
        for(int i=0; i<4; i++) {
            chThdSleepMilliseconds(135);
            Led.Off();
            chThdSleepMilliseconds(135);
            Led.On();
        }
    }

    // LEDs
    LedWsEn.Init();
    LedWsEn.SetLo();
    LedEffectsInit();

    EffFadeOneByOne.SetupIDs();
    EffAllTogetherSmoothly.SetupAndStart(clRGBWStars, 360);
//    EffAllTogetherNow.SetupAndStart((Color_t){255,0,0,0});
//    EffAllTogetherNow.SetupAndStart((Color_t){0,255,0,0});
//    EffAllTogetherNow.SetupAndStart((Color_t){0,0,255,0});
//    EffAllTogetherNow.SetupAndStart((Color_t){0,0,0,255});

    // Main cycle
    ITask();
}

__noreturn
void ITask() {
    while(true) {
        EvtMsg_t Msg = EvtQMain.Fetch(TIME_INFINITE);
//        Printf("Msg %u\r", Msg.ID);
        switch(Msg.ID) {
            case evtIdRadioCmd:
                Printf("New percent: %d\r", Msg.Value);
                break;

#if UART_RX_ENABLED
            case evtIdShellCmd:
                OnCmd((Shell_t*)Msg.Ptr);
                ((Shell_t*)Msg.Ptr)->SignalCmdProcessed();
                break;
#endif
            default: Printf("Unhandled Msg %u\r", Msg.ID); break;
        } // Switch
    } // while true
} // App_t::ITask()

void PercentToFade(int32_t Percent) {
    int32_t FThrLo=0, FThrHi=0;
    if(Percent > 100 or Percent < 0) return;
    if(Percent >= 70) {
        FThrLo = Proportion<int32_t>(70, 100, 0, 359, Percent);
        FThrHi = LED_CNT;
    }
    else {
        FThrLo = 0;
        FThrHi = Proportion<int32_t>(0, 70, 1, 359, Percent);
    }
    Printf("Prc: %d; Lo: %d; Hi: %d\r", Percent, FThrLo, FThrHi);
    EffFadeOneByOne.SetupAndStart(FThrLo, FThrHi);
}


#if UART_RX_ENABLED // ================= Command processing ====================
void OnCmd(Shell_t *PShell) {
	Cmd_t *PCmd = &PShell->Cmd;
//    Uart.Printf("%S\r", PCmd->Name);
    // Handle command
    if(PCmd->NameIs("Ping")) {
        PShell->Ack(retvOk);
    }

    else if(PCmd->NameIs("Version")) PShell->Printf("%S %S\r", APP_NAME, BUILD_TIME);

    else if(PCmd->NameIs("RGBW")) {
        Color_t FClr(0,0,0,0);
        if(PCmd->GetNext<uint8_t>(&FClr.R) != retvOk) return;
        if(PCmd->GetNext<uint8_t>(&FClr.G) != retvOk) return;
        if(PCmd->GetNext<uint8_t>(&FClr.B) != retvOk) return;
        if(PCmd->GetNext<uint8_t>(&FClr.W) != retvOk) return;
//        EffAllTogetherNow.SetupAndStart(FClr);
        EffAllTogetherSmoothly.SetupAndStart(FClr, 360);
        PShell->Ack(retvOk);
    }

    else if(PCmd->NameIs("Fade")) {
        int32_t FThrLo=0, FThrHi=0;
        if(PCmd->GetNext<int32_t>(&FThrLo) != retvOk) return;
        if(PCmd->GetNext<int32_t>(&FThrHi) != retvOk) return;
        EffFadeOneByOne.SetupAndStart(FThrLo, FThrHi);
        PShell->Ack(retvOk);
    }

    else if(PCmd->NameIs("Prc")) {
        int32_t Percent = 0;
        if(PCmd->GetNext<int32_t>(&Percent) != retvOk) return;
        PercentToFade(Percent);
        PShell->Ack(retvOk);
    }

    else PShell->Ack(retvCmdUnknown);
}
#endif
