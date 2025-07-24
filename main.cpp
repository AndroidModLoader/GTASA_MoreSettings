#include <string>
#include <mod/amlmod.h>
#include <mod/logger.h>

#include <filesystem>
namespace fs = std::filesystem;

#define sizeofA(__aVar)  ((int)(sizeof(__aVar)/sizeof(__aVar[0])))
#ifdef AML32
    #include "GTASA_STRUCTS.h"
#else
    #include "AArch64_ModHelper/ARMv8_ASMHelper.h"
    #include "GTASA_STRUCTS_210.h"
#endif

MYMOD(net.rusjj.gtasa.moresettings, GTA:SA More Settings, 1.6, RusJJ)
NEEDGAME(com.rockstargames.gtasa)
BEGIN_DEPLIST()
    ADD_DEPENDENCY_VER(net.rusjj.aml, 1.2)
    ADD_DEPENDENCY_VER(net.rusjj.gtasa.utils, 1.4)
END_DEPLIST()

/* SA Utils */
#include "isautils.h"
ISAUtils* sautils = nullptr;

/* Saves */
uintptr_t pGTASA = 0;
void* hGTASA = NULL;
bool bSAMPMode = false; // Some settings are not safe for SAMP
float *m_fMouseAccelHorzntl;
CWidget** m_pWidgets;
int (*LIB_KeyboardState)(int);
bool (*TouchInterface_IsReleased)(int, CVector2D*, int);

/* Config */
int debugLvl = 0;
int debugTopleft = 0;
int fpsLimit = 30;
int freezeTime = 0;
int sensi = 18;
int radarMenuMode = 0;

const char* pYesNo[] = 
{
    "FEM_OFF",
    "FEM_ON",
};
const char* pFPSToggler[] = 
{
    "FEM_OFF",
    "Show as 'FPS: 60.00'",
    "Show as 'FPS: 60'",
    "Show as '60.00'",
    "Show as '60'",
};
const char* pRadarMenuToggler[] = 
{
    "FEM_ON",
    "FEM_OFF",
};

enum eSettingToChange : unsigned char
{
    DebugFPS = 0,
    DebugFPSTopLeft,
    LimitFPS,
    Sensitivity,
    FreezeTime,
    RadarMenuBehaviour,
    
    SettingsMax
};

/* Patches */
#ifdef AML64
uintptr_t TopLeftValue_BackTo;
__attribute__((optnone)) __attribute__((naked)) void TopLeftValue_Inject(void)
{
    asm("LDR S3, [X9,#0x148]\nLDP S1, S2, [X8,#8]");
    asm("FMOV S4, #4.0");
    asm volatile("MOV X8, %0\n" :: "r"(TopLeftValue_BackTo));
    asm("BR X8");
}
#endif

char szRetText[8];
void OnSettingChange(int oldVal, int newVal, void* data)
{
    eSettingToChange set = (eSettingToChange)(intptr_t)data; // why do i need to cast it to int first? bruh compiler moment
    switch(set)
    {
        case DebugFPS:
        {
            aml->MLSSetInt("MORSDBGLVL", newVal);
            *(bool*)(pGTASA + BYBIT(0x98F1AD, 0xC1DF01)) = (newVal != 0);
            switch(newVal)
            {
                case 1:
                    strcpy((char*)(pGTASA + BYBIT(0x3F56A0, 0x750347)), "FPS: %.2f");
                    break;
                case 2:
                    strcpy((char*)(pGTASA + BYBIT(0x3F56A0, 0x750347)), "FPS: %.0f");
                    break;
                case 3:
                    strcpy((char*)(pGTASA + BYBIT(0x3F56A0, 0x750347)), "%.2f");
                    break;
                case 4:
                    strcpy((char*)(pGTASA + BYBIT(0x3F56A0, 0x750347)), "%.0f");
                    break;
            }
            break;
        }
        case DebugFPSTopLeft:
        {
            aml->MLSSetInt("MORSDBGTL", newVal);
            #ifdef AML32
                *(float*)(pGTASA + 0x3F56B8) = (newVal != 0) ? 4.0f : 200.0f;
            #else
                if(newVal != 0)
                {
                    aml->Redirect(pGTASA + 0x4D7B20, (uintptr_t)TopLeftValue_Inject);
                }
                else
                {
                    aml->Write(pGTASA + 0x4D7B20, "\x0A\x13\x00\xF0\x23\x49\x41\xBD\x01\x09\x41\x2D\x44\x25\x4C\xBD", 16);
                }
            #endif
            break;
        }
        case LimitFPS:
        {
            aml->MLSSetInt("MORSFPS", newVal);
            #ifdef AML32
                *(char*)(pGTASA + 0x5E4978) = newVal;
                *(char*)(pGTASA + 0x5E4990) = newVal;
            #else
                uint32_t limitValASM = MOVBits::Create(newVal, 9, false);
                aml->Write32(pGTASA + 0x70A38C, limitValASM);
                aml->Write32(pGTASA + 0x70A458, limitValASM);
            #endif
            break;
        }
        case FreezeTime:
        {
            aml->MLSSetInt("MORSFREZ", newVal);
            switch(newVal)
            {
                case 0:
                    aml->PlaceRET(pGTASA + BYBIT(0x3E3378, 0x4C20A4));
                    break;

                default:
                    aml->Write(pGTASA + BYBIT(0x3E3378, 0x4C20A4), BYBIT("\xD0\xB5", "\xF7\x0F\x1C\xF8"), BYBIT(2, 4));
                    break;
            }
            break;
        }
        case Sensitivity:
        {
            aml->MLSSetInt("MORSSENSI", newVal);
            *(float*)(pGTASA + BYBIT(0x6A9F30, 0x885534)) = 0.001f + (float)newVal / 3000.0f;
            *m_fMouseAccelHorzntl = 0.001f + (float)newVal / 3000.0f; // for CLEO+
            break;
        }
        case RadarMenuBehaviour:
        {
            aml->MLSSetInt("MORSRDRM", newVal);
            radarMenuMode = newVal;
            break;
        }

        default: return;
    }
}
const char* OnFPSLimitDraw(int newVal, void* data)
{
    sprintf(szRetText, "%d", newVal);
    return szRetText;
}

DECL_HOOK(bool, GetEscapeJustDown, CPad* self)
{
    if(radarMenuMode == 1)
    {
        TouchInterface_IsReleased(161, NULL, 1); // does its own logic to show widget &etc
        return (LIB_KeyboardState(0) == 0);
    }
    return GetEscapeJustDown(self);
}

extern "C" void OnAllModsLoaded()
{
    logger->SetTag("GTASA More Settings");
    pGTASA = aml->GetLib("libGTASA.so");
    hGTASA = aml->GetLibHandle("libGTASA.so");

    bSAMPMode = (aml->GetLibHandle("libsamp.so") != NULL || aml->GetLibHandle("libvoice.so") != NULL ||
                 aml->GetLibHandle("libAlyn_SAMPMOBILE.so") != NULL || aml->HasMod("net.rusjj.resamp") ||
                 aml->GetLibHandle("libSAMP.so") != NULL || aml->GetLibHandle("libbass.so") != NULL);

    aml->Unprot(pGTASA + BYBIT(0x98F1AD, 0xC1DF01), sizeof(bool)); // Debug FPS
    aml->Unprot(pGTASA + BYBIT(0x3F56A0, 0x750347), 10);
    #ifdef AML32
        aml->Unprot(pGTASA + 0x5E4978, sizeof(char));
        aml->Unprot(pGTASA + 0x5E4990, sizeof(char)); // FPS
        aml->Unprot(pGTASA + 0x3F56B8, sizeof(float));
    #else
        TopLeftValue_BackTo = pGTASA + 0x4D7B30;
    #endif

    sautils = (ISAUtils*)GetInterface("SAUtils");
    if(sautils != NULL)
    {   
        SET_TO(m_fMouseAccelHorzntl,    aml->GetSym(hGTASA,"_ZN7CCamera20m_fMouseAccelHorzntlE"));
        SET_TO(m_pWidgets,              *(void**)(pGTASA + BYBIT(0x67947C, 0x850910)));
        SET_TO(LIB_KeyboardState,       aml->GetSym(hGTASA,"_Z17LIB_KeyboardState13OSKeyboardKey"));
        SET_TO(TouchInterface_IsReleased, aml->GetSym(hGTASA,"_ZN15CTouchInterface10IsReleasedENS_9WidgetIDsEP9CVector2Di"));

        //HOOKPLT(AnyWidgetsUsingAltBack, pGTASA + BYBIT(0x6702B0, 0x840868));
        HOOKPLT(GetEscapeJustDown,      pGTASA + BYBIT(0x66EAA4, 0x83E130));

        ///////

        aml->MLSGetInt("MORSDBGLVL", &debugLvl); clampint(0, 4, &debugLvl);
        if(debugLvl != 0) OnSettingChange(0, debugLvl, (void*)DebugFPS);
        sautils->AddClickableItem(SetType_Game, "Show FPS", debugLvl, 0, sizeofA(pFPSToggler)-1, pFPSToggler, OnSettingChange, (void*)DebugFPS);

        aml->MLSGetInt("MORSDBGTL", &debugTopleft); clampint(0, 1, &debugTopleft);
        if(debugTopleft) OnSettingChange(0, 1, (void*)DebugFPSTopLeft);
        sautils->AddClickableItem(SetType_Game, "FPS in TopLeft corner", debugTopleft, 0, sizeofA(pYesNo)-1, pYesNo, OnSettingChange, (void*)DebugFPSTopLeft);

        aml->MLSGetInt("MORSFPS", &fpsLimit); clampint(20, 160, &fpsLimit);
        OnSettingChange(0, fpsLimit, (void*)LimitFPS);
        sautils->AddSliderItem(SetType_Game, "FPS Limit", fpsLimit, 20, 160, OnSettingChange, OnFPSLimitDraw, (void*)LimitFPS);

        if(!bSAMPMode)
        {
            aml->MLSGetInt("MORSFREZ", &freezeTime); clampint(0, 1, &freezeTime);
            sautils->AddClickableItem(SetType_Game, "Freeze Time", freezeTime, 0, sizeofA(pYesNo)-1, pYesNo, OnSettingChange, (void*)FreezeTime);
            if(freezeTime) OnSettingChange(0, 1, (void*)FreezeTime);
        }

        aml->MLSGetInt("MORSSENSI", &sensi); clampint(0, 100, &sensi);
        aml->Unprot(pGTASA + BYBIT(0x6A9F30, 0x885534), sizeof(float));
        sautils->AddSliderItem(SetType_Controller, "Touch Sensitivity", sensi, 0, 100, OnSettingChange, NULL, (void*)Sensitivity);
        OnSettingChange(18, sensi, (void*)Sensitivity);
        
        aml->MLSGetInt("MORSRDRM", &radarMenuMode); clampint(0, 1, &radarMenuMode);
        OnSettingChange(0, radarMenuMode, (void*)RadarMenuBehaviour);
        sautils->AddClickableItem(SetType_Game, "Radar-menu Click", radarMenuMode, 0, sizeofA(pRadarMenuToggler)-1, pRadarMenuToggler, OnSettingChange, (void*)RadarMenuBehaviour);
    }
}
