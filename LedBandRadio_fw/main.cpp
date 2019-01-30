#include "board.h"
#include "uart.h"
#include "shell.h"
#include "MsgQ.h"
#include "led.h"
#include "radio_lvl1.h"
#include "ws2812b.h"
#include "IntelLedEffs.h"
#include "Sequences.h"

#if 1 // ======================== Variables and defines ========================
// Forever
EvtMsgQ_t<EvtMsg_t, MAIN_EVT_Q_LEN> EvtQMain;
static const UartParams_t CmdUartParams(115200, CMD_UART_PARAMS);
extern CmdUart_t Uart{&CmdUartParams};
static void ITask();
static void OnCmd(Shell_t *PShell);

LedOnOff_t Led {LED_PIN};
PinOutput_t LedWsEn {LED_WS_EN};
//Color_t Clr(0, 255, 0, 0);

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
    Uart.Init();
    Printf("\r%S %S\r", APP_NAME, XSTRINGIFY(BUILD_TIME));
    Clk.PrintFreqs();

    // Debug LED
    Led.Init();

//    if(Radio.Init() != retvOk) {
//        for(int i=0; i<45; i++) {
//            chThdSleepMilliseconds(135);
//            Led.Off();
//            chThdSleepMilliseconds(135);
//            Led.On();
//        }
//    }
//    else Led.Off();

    // LEDs
//    LedWsEn.Init();
//    LedWsEn.SetLo();
//    LedEffectsInit();

//    EffAllTogetherSequence.

//    EffFadeOneByOne.SetupIDs();
//    EffAllTogetherSmoothly.SetupAndStart(clRGBWStars, 360);
//    EffAllTogetherNow.SetupAndStart((Color_t){255,0,0});
//    EffAllTogetherNow.SetupAndStart((Color_t){0,255,0});
//    EffAllTogetherNow.SetupAndStart((Color_t){0,0,255});
//    EffAllTogetherSequence.StartOrRestart(lsqStart);

//    EvtMsg_t Msg(evtIdLedEnd);
//    EffAllTogetherSequence.SetupSeqEndEvt(Msg);

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
                Printf("Btn: %d\r", Msg.Value);
                break;

            case evtIdShellCmd:
                OnCmd((Shell_t*)Msg.Ptr);
                ((Shell_t*)Msg.Ptr)->SignalCmdProcessed();
                break;

            default: Printf("Unhandled Msg %u\r", Msg.ID); break;
        } // Switch
    } // while true
} // App_t::ITask()

#if 1 // ================= Command processing ====================
void OnCmd(Shell_t *PShell) {
	Cmd_t *PCmd = &PShell->Cmd;
//    Uart.Printf("%S\r", PCmd->Name);
    // Handle command
    if(PCmd->NameIs("Ping")) {
        PShell->Ack(retvOk);
    }

    else if(PCmd->NameIs("Version")) PShell->Print("%S %S\r", APP_NAME, XSTRINGIFY(BUILD_TIME));

//    else if(PCmd->NameIs("RGB")) {
//        Color_t FClr(0,0,0);
//        if(PCmd->GetNext<uint8_t>(&FClr.R) != retvOk) return;
//        if(PCmd->GetNext<uint8_t>(&FClr.G) != retvOk) return;
//        if(PCmd->GetNext<uint8_t>(&FClr.B) != retvOk) return;
//        EffAllTogetherNow.SetupAndStart(FClr);
////        EffAllTogetherSmoothly.SetupAndStart(FClr, 360);
//        PShell->Ack(retvOk);
//    }
//
//    else if(PCmd->NameIs("Fade")) {
//        int32_t FThrLo=0, FThrHi=0;
//        if(PCmd->GetNext<int32_t>(&FThrLo) != retvOk) return;
//        if(PCmd->GetNext<int32_t>(&FThrHi) != retvOk) return;
////        EffFadeOneByOne.SetupAndStart(FThrLo, FThrHi);
//        PShell->Ack(retvOk);
//    }

    else PShell->Ack(retvCmdUnknown);
}
#endif
