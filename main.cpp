#include <string>
#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>

#include <filesystem>
namespace fs = std::filesystem;

#define sizeofA(__aVar)  ((int)(sizeof(__aVar)/sizeof(__aVar[0])))
#ifdef AML32
    #define BYVER(__for32, __for64) (__for32)
#else
    #include "AArch64_ModHelper/ARMv8_ASMHelper.h"
    #define BYVER(__for32, __for64) (__for64)
#endif

MYMODCFG(net.rusjj.gtasa.moresettings, GTA:SA More Settings, 1.3, RusJJ)
NEEDGAME(com.rockstargames.gtasa)
BEGIN_DEPLIST()
    ADD_DEPENDENCY_VER(net.rusjj.aml, 1.0.2.1)
    ADD_DEPENDENCY_VER(net.rusjj.gtasa.utils, 1.4)
END_DEPLIST()

/* SA Utils */
#include "isautils.h"
ISAUtils* sautils = nullptr;

/* Saves */
uintptr_t pGTASA = 0;
void* hGTASA = NULL;
bool bSAMPMode = false; // Some settings are not safe for SAMP

/* Config */
ConfigEntry* pCfgDebugFPS;
ConfigEntry* pCfgDebugFPSTopLeft;
ConfigEntry* pCfgFPSNew;
ConfigEntry* pCfgFreezeTime;

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

enum eSettingToChange : unsigned char
{
    DebugFPS = 0,
    DebugFPSTopLeft,
    LimitFPS,
    FreezeTime
};

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
            pCfgDebugFPS->SetInt(newVal);
            *(bool*)(pGTASA + BYVER(0x98F1AD, 0xC1DF01)) = (newVal != 0);
            switch(newVal)
            {
                case 1:
                    strcpy((char*)(pGTASA + BYVER(0x3F56A0, 0x750347)), "FPS: %.2f");
                    break;
                case 2:
                    strcpy((char*)(pGTASA + BYVER(0x3F56A0, 0x750347)), "FPS: %.0f");
                    break;
                case 3:
                    strcpy((char*)(pGTASA + BYVER(0x3F56A0, 0x750347)), "%.2f");
                    break;
                case 4:
                    strcpy((char*)(pGTASA + BYVER(0x3F56A0, 0x750347)), "%.0f");
                    break;
            }
            break;
        }
        case DebugFPSTopLeft:
        {
            pCfgDebugFPSTopLeft->SetBool(newVal != 0);
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
            pCfgFPSNew->SetInt(newVal);
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
            pCfgFreezeTime->SetBool(newVal != 0);
            switch(newVal)
            {
                case 0:
                    aml->PlaceRET(pGTASA + BYVER(0x3E3378, 0x4C20A4));
                    break;

                default:
                    aml->Write(pGTASA + BYVER(0x3E3378, 0x4C20A4), BYVER("\xD0\xB5", "\xF7\x0F\x1C\xF8"), BYVER(2, 4));
                    break;
            }
            break;
        }
    }
    cfg->Save();
}
const char* OnFPSLimitDraw(int newVal, void* data)
{
    sprintf(szRetText, "%d", newVal);
    return szRetText;
}

extern "C" void OnModLoad()
{
    logger->SetTag("GTASA More Settings");
    pGTASA = aml->GetLib("libGTASA.so");
    hGTASA = aml->GetLibHandle("libGTASA.so");

    bSAMPMode = (aml->GetLibHandle("libsamp.so") != NULL || aml->HasMod("net.rusjj.resamp"));

    aml->Unprot(pGTASA + BYVER(0x98F1AD, 0xC1DF01), sizeof(bool)); // Debug FPS
    aml->Unprot(pGTASA + BYVER(0x3F56A0, 0x750347), 10);
    #ifdef AML32
        aml->Unprot(pGTASA + 0x5E4978, sizeof(char)); aml->Unprot(pGTASA + 0x5E4990, sizeof(char)); // FPS
        aml->Unprot(pGTASA + 0x3F56B8, sizeof(float));
    #else
        TopLeftValue_BackTo = pGTASA + 0x4D7B30;
    #endif

    sautils = (ISAUtils*)GetInterface("SAUtils");
    if(sautils != NULL)
    {
        pCfgDebugFPS = cfg->Bind("DebugFPS", *(bool*)(pGTASA + BYVER(0x98F1AD, 0xC1DF01)) ? 1 : 0, "Tweaks");
        if(pCfgDebugFPS->GetInt()) OnSettingChange(0, pCfgDebugFPS->GetInt(), (void*)DebugFPS);
        sautils->AddClickableItem(SetType_Game, "Show FPS", pCfgDebugFPS->GetInt(), 0, sizeofA(pFPSToggler)-1, pFPSToggler, OnSettingChange, (void*)DebugFPS);

        pCfgDebugFPSTopLeft = cfg->Bind("DebugFPS_TopLeft", true, "Tweaks");
        if(pCfgDebugFPSTopLeft->GetBool()) OnSettingChange(0, 1, (void*)DebugFPSTopLeft);
        sautils->AddClickableItem(SetType_Game, "FPS in TopLeft corner", pCfgDebugFPSTopLeft->GetInt(), 0, sizeofA(pYesNo)-1, pYesNo, OnSettingChange, (void*)DebugFPSTopLeft);

        pCfgFPSNew = cfg->Bind("FPSNew", 30, "Tweaks");
        pCfgFPSNew->Clamp(20, 160);
        if(pCfgFPSNew->GetInt() != 30) OnSettingChange(0, pCfgFPSNew->GetInt(), (void*)LimitFPS);
        sautils->AddSliderItem(SetType_Game, "FPS Limit", pCfgFPSNew->GetInt(), 20, 160, OnSettingChange, OnFPSLimitDraw, (void*)LimitFPS);

        if(!bSAMPMode)
        {
            pCfgFreezeTime = cfg->Bind("FreezeTime", false, "Fun");
            sautils->AddClickableItem(SetType_Game, "Freeze Time", pCfgFreezeTime->GetBool(), 0, sizeofA(pYesNo)-1, pYesNo, OnSettingChange, (void*)FreezeTime);
            if(pCfgFreezeTime->GetBool()) OnSettingChange(0, 1, (void*)FreezeTime);
        }
    }
}