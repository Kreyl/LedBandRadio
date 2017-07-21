/*
 * main.cpp
 *
 *  Created on: 20 февр. 2014 г.
 *      Author: g.kruglov
 */

#include "kl_lib.h"
#include "MsgQ.h"
#include "shell.h"
//#include "ColorProfile.h"
#include "SimpleSensors.h"
#include "buttons.h"
#include "board.h"
//#include "kl_adc.h"
#include "ws2812b.h"

#if 1 // ======================== Variables and defines ========================
// Forever
EvtMsgQ_t<EvtMsg_t, MAIN_EVT_Q_LEN> EvtQMain;
extern CmdUart_t Uart;
void OnCmd(Shell_t *PShell);
void ITask();

ColorHSV_t hsv(319, 100, 100);
PinOutput_t PwrPin { PWR_EN_PIN };
//TmrKL_t TmrAdc {MS2ST(450), evtIdEverySecond, tktPeriodic};
//Profile_t Profile;

//bool AdcFirstConv = true;

//void BtnHandler(BtnEvt_t BtnEvt);
#endif

int main(void) {
    // ==== Init Clock system ====
//    Clk.SetupFlashLatency(16);
//    Clk.SetupPLLDividers(1, pllMul4, plsHSIdiv2);
//    Clk.SwitchTo(csPLL);
    Clk.UpdateFreqValues();

    // === Init OS ===
    halInit();
    chSysInit();

    // ==== Init hardware ====
    EvtQMain.Init();
    Uart.Init(115200);
    Printf("\r%S %S\r", APP_NAME, BUILD_TIME);
    Clk.PrintFreqs();

    // Power pin
//    PwrPin.Init();
//    PwrPin.SetHi();

//    Effects.Init();
//    Effects.AllTogetherNow(hsv);

//    SimpleSensors::Init();
    // Adc
//    PinSetupAnalog(BAT_MEAS_PIN);
//    Adc.Init();
//    Adc.EnableVRef();
//    TmrAdc.InitAndStart();
    // Main cycle
    ITask();
}

__noreturn
void ITask() {
    while(true) {
        EvtMsg_t Msg = EvtQMain.Fetch(TIME_INFINITE);
        switch(Msg.ID) {
            case evtIdShellCmd:
                OnCmd((Shell_t*)Msg.Ptr);
                ((Shell_t*)Msg.Ptr)->SignalCmdProcessed();
                break;

            case evtIdButtons:
//                Printf("Btn %u\r", Msg.BtnEvtInfo.BtnID);
//                if(Msg.BtnEvtInfo.BtnID == 1) {
//                    if(hsv.H < 360) hsv.H++;
//                    else hsv.H = 0;
//                }
//                else if(Msg.BtnEvtInfo.BtnID == 2) {
//                    if(hsv.H > 0) hsv.H--;
//                    else hsv.H = 360;
//                }
//                Printf("HSV %u; ", hsv.H);
//                {
//                    Color_t rgb = hsv.ToRGB();
//                    rgb.Print();
//                }
//                Effects.AllTogetherNow(hsv);
                break;

            default: break;
        } // switch


//        Effects.AllTogetherNow(hsv);
//        hsv.H++;
//        if(hsv.H > 360) hsv.H = 0;
//        chThdSleepMilliseconds(90);


//        Effects.AllTogetherSmoothly(clRed, 360);
//        chThdSleepMilliseconds(2700);
//        Effects.AllTogetherSmoothly(clGreen, 360);
//        chThdSleepMilliseconds(2700);
//        Effects.AllTogetherSmoothly(clBlue, 360);
//        chThdSleepMilliseconds(2700);

//        __unused eventmask_t Evt = chEvtWaitAny(ALL_EVENTS);

//        if(Evt & EVT_BUTTONS) {
//            BtnEvtInfo_t EInfo;
//            while(BtnGetEvt(&EInfo) == OK) BtnHandler(EInfo.Type);
//        }

//        // If battery discharged, indicate it
//        if(Evt & EVT_BATTERY_LOW) {
//            Uart.Printf("Battery low\r");
//            State = stateDischarged;
//            Led.IndicateDischarged();
//        }

        // Led sequence end: switch off if time to sleep
//        if(Evt & EVT_LED_DONE) {
//            Uart.Printf("Led Done\r");
//        }

#if ADC_REQUIRED
        if(Evt & EVT_SAMPLING) Adc.StartMeasurement();
        if(Evt & EVT_ADC_DONE) {
            if(AdcFirstConv) AdcFirstConv = false;
            else {
                uint32_t VBat_adc = Adc.GetResult(ADC_BAT_CHNL);
                uint32_t VRef_adc = Adc.GetResult(ADC_VREFINT_CHNL);
                __unused int32_t Vbat_mv = (2 * Adc.Adc2mV(VBat_adc, VRef_adc));   // Resistor divider
//                Uart.Printf("VBat_adc: %u; Vref_adc: %u; VBat_mv: %u\r", VBat_adc, VRef_adc, Vbat_mv);
//                if(Vbat_mv < 3600) SignalEvt(EVT_BATTERY_LOW);
            } // if not big diff
        } // evt
#endif
    } // while true
} // App_t::ITask()

//void BtnHandler(BtnEvt_t BtnEvt) {
//    if(BtnEvt == beShortPress) Uart.Printf("Btn Short\r");
//    if(BtnEvt == beLongPress)  Uart.Printf("Btn Long\r");
//    if(BtnEvt == beRelease)    Uart.Printf("Btn Release\r");
//}

#if UART_RX_ENABLED // ================= Command processing ====================
void OnCmd(Shell_t *PShell) {
	Cmd_t *PCmd = &PShell->Cmd;
    __attribute__((unused)) int32_t dw32 = 0;  // May be unused in some configurations
//    Uart.Printf("%S\r", PCmd->Name);
    // Handle command
    if(PCmd->NameIs("Ping")) {
        PShell->Ack(retvOk);
    }

//    else if(PCmd->NameIs("HSL")) {
//        ColorHSL_t ClrHsl(0,0,0);
//        if(PCmd->GetNextUint16(&ClrHsl.H) != OK) return;
//        if(PCmd->GetNextByte(&ClrHsl.S)   != OK) return;
//        if(PCmd->GetNextByte(&ClrHsl.L)   != OK) return;
//        Color_t Clr;
//        ClrHsl.ToRGB(Clr);
////        Led.SetColor(Clr, 100);
//        PShell->Ack(OK);
//    }

//    else if(PCmd->NameIs("HSV")) {
//        ColorHSV_t ClrHsv(0,0,0);
//        if(PCmd->GetNextUint16(&ClrHsv.H) != OK) return;
//        if(PCmd->GetNextByte(&ClrHsv.S)   != OK) return;
//        if(PCmd->GetNextByte(&ClrHsv.V)   != OK) return;
//        Color_t Clr;
//        ClrHsv.ToRGB(Clr);
////        Clr.Print();
////        Uart.Printf("{%u; %u; %u}\r", R, G, B);
////        Led.SetColor(Clr, 100);
//        PShell->Ack(OK);
//    }

    else PShell->Ack(retvCmdUnknown);
}
#endif
