#include "board.h"
#include "uart.h"
#include "shell.h"
#include "MsgQ.h"
#include "led.h"
#include "radio_lvl1.h"
#include "ws2812b.h"
#include "Effects.h"
#include "Sequences.h"

#if 1 // ======================== Variables and defines ========================
// Forever
EvtMsgQ_t<EvtMsg_t, MAIN_EVT_Q_LEN> EvtQMain;
static const UartParams_t CmdUartParams(115200, CMD_UART_PARAMS);
CmdUart_t Uart{&CmdUartParams};
static void ITask();
static void OnCmd(Shell_t *PShell);

static const NeopixelParams_t NpxParams(NPX1_SPI, NPX1_GPIO, NPX1_PIN, NPX1_AF, NPX1_DMA, NPX_DMA_MODE(NPX1_DMA_CHNL));
Neopixels_t Leds{&NpxParams, BAND_CNT, BAND_SETUPS};

LedOnOff_t Led {LED_PIN};
PinOutput_t LedWsEn {NPX1_EN};
enum State_t { staOff, staFire, staWhite };
State_t State = staFire;
#endif

int main(void) {
    // ==== Init Clock system ====
    SetupVCore(vcore1V8);
    Clk.EnablePrefetch();
//    Clk.SetupFlashLatency(8);
//    Clk.SetupPLLDividers(pllMul4, pllDiv2);
//    Clk.SetupPLLSrc(pllSrcHSE);
//    Clk.SwitchToPLL();
//    Clk.SetupBusDividers(ahbDiv2, apbDiv1, apbDiv1);
//    Clk.SwitchToHSI();

    // PLL fed by HSI
    if(Clk.EnableHSI() == retvOk) {
        Clk.SetupFlashLatency(16);
        Clk.SetupPLLSrc(pllSrcHSI16);
        Clk.SetupPLLDividers(pllMul4, pllDiv3);
        Clk.SetupBusDividers(ahbDiv2, apbDiv1, apbDiv1);
        Clk.SwitchToPLL();
    }
    Clk.UpdateFreqValues();

    // === Init OS ===
    halInit();
    chSysInit();
    EvtQMain.Init();

    // ==== Init hardware ====
    Uart.Init();
    Printf("\r%S %S\r", APP_NAME, XSTRINGIFY(BUILD_TIME));
    Clk.PrintFreqs();

    Led.Init(); // Debug LED

    if(Radio.Init() != retvOk) {
        while(true) {
            chThdSleepMilliseconds(135);
            Led.On();
            chThdSleepMilliseconds(135);
            Led.Off();
        }
    }
    else Led.On();

    // LEDs
    LedWsEn.Init();
    LedWsEn.SetLo();
    Leds.Init();

    Eff::Init();
    Eff::StartFlaming();

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
//                if(Clr.DWord32 != (uint32_t)Msg.Value) {
//                    Clr.DWord32 = (uint32_t)Msg.Value;
//                    Clr.Print();
//                    PrintfEOL();
//                    EffAllTogetherSmoothly.SetupAndStart(Clr, 360);
//                }
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
    else if(PCmd->NameIs("mem")) PrintMemoryInfo();

    else if(PCmd->NameIs("1")) {

        PShell->Ack(retvOk);
    }

    else if(PCmd->NameIs("2")) {
        if(State == staFire) {
            State = staWhite;
            Eff::BeWhite();
        }
        else {
            State = staFire;
            Eff::StartFlaming();
        }
        PShell->Ack(retvOk);
    }

    else if(PCmd->NameIs("3")) {
        Eff::DoFlash();
        PShell->Ack(retvOk);
    }

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
