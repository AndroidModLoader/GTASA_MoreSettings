#include <string>
#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>

#include <filesystem>
namespace fs = std::filesystem;

#define sizeofA(__aVar)  ((int)(sizeof(__aVar)/sizeof(__aVar[0])))

MYMODCFG(net.rusjj.gtasa.moresettings, GTA:SA More Settings, 1.3, RusJJ)
NEEDGAME(com.rockstargames.gtasa)
BEGIN_DEPLIST()
    ADD_DEPENDENCY_VER(net.rusjj.aml, 1.0.2.1)
    ADD_DEPENDENCY_VER(net.rusjj.gtasa.utils, 1.4.1)
END_DEPLIST()

/* SA Utils */
#include "isautils.h"
ISAUtils* sautils = nullptr;

/* Saves */
uintptr_t pGTASA = 0;
void* hGTASA = NULL;
bool bForceNoCull = false;

/* Config */
ConfigEntry* pCfgDebugFPS;
ConfigEntry* pCfgDebugFPSTopLeft;
ConfigEntry* pCfgFPSNew;
ConfigEntry* pCfgBackfaceCulling;
ConfigEntry* pCfgVehicleBackfaceCulling;
ConfigEntry* pCfgBreakableBackfaceCulling;

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
    LimitFPS
};

char szRetText[8];
void OnSettingChange(int oldVal, int newVal, void* data)
{
    eSettingToChange set = (eSettingToChange)(int)data; // why do i need to cast it to int first? bruh compiler moment
    switch(set)
    {
        case DebugFPS:
        {
            pCfgDebugFPS->SetInt(newVal);
            *(bool*)(pGTASA + 0x98F1AD) = (newVal != 0);
            switch(newVal)
            {
                case 1:
                    strcpy((char*)(pGTASA + 0x3F56A0), "FPS: %.2f");
                    break;
                case 2:
                    strcpy((char*)(pGTASA + 0x3F56A0), "FPS: %.0f");
                    break;
                case 3:
                    strcpy((char*)(pGTASA + 0x3F56A0), "%.2f");
                    break;
                case 4:
                    strcpy((char*)(pGTASA + 0x3F56A0), "%.0f");
                    break;
            }
            break;
        }
        case DebugFPSTopLeft:
        {
            pCfgDebugFPSTopLeft->SetBool(newVal != 0);
            *(float*)(pGTASA + 0x3F56B8) = (newVal != 0) ? 4.0f : 200.0f;
            break;
        }
        case LimitFPS:
        {
            pCfgFPSNew->SetInt(newVal);
            *(char*)(pGTASA + 0x5E4978) = newVal;
            *(char*)(pGTASA + 0x5E4990) = newVal;
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

extern "C" void OnModPreLoad()
{
    logger->SetTag("GTASA More Settings");
    pGTASA = aml->GetLib("libGTASA.so");
    hGTASA = aml->GetLibHandle("libGTASA.so");

    aml->Unprot(pGTASA + 0x98F1AD, sizeof(bool)); // Debug FPS
    aml->Unprot(pGTASA + 0x3F56A0, 12);
    aml->Unprot(pGTASA + 0x3F56B8, sizeof(float));
    aml->Unprot(pGTASA + 0x5E4978, sizeof(char)); aml->Unprot(pGTASA + 0x5E4990, sizeof(char)); // FPS

    sautils = (ISAUtils*)GetInterface("SAUtils");
    if(sautils != NULL)
    {
        pCfgDebugFPS = cfg->Bind("DebugFPS", *(bool*)(pGTASA + 0x98F1AD) ? 1 : 0, "Tweaks");
        *(bool*)(pGTASA + 0x98F1AD) = (pCfgDebugFPS->GetInt() != 0);
        switch(pCfgDebugFPS->GetInt())
        {
            case 1:
                strcpy((char*)(pGTASA + 0x3F56A0), "FPS: %.2f");
                break;
            case 2:
                strcpy((char*)(pGTASA + 0x3F56A0), "FPS: %.0f");
                break;
            case 3:
                strcpy((char*)(pGTASA + 0x3F56A0), "%.2f");
                break;
            case 4:
                strcpy((char*)(pGTASA + 0x3F56A0), "%.0f");
                break;
        }
        sautils->AddClickableItem(SetType_Game, "Show FPS", pCfgDebugFPS->GetInt(), 0, sizeofA(pFPSToggler)-1, pFPSToggler, OnSettingChange, (void*)DebugFPS);

        pCfgDebugFPSTopLeft = cfg->Bind("DebugFPS_TopLeft", true, "Tweaks");
        if(pCfgDebugFPSTopLeft->GetBool()) *(float*)(pGTASA + 0x3F56B8) = 4.0f;
        sautils->AddClickableItem(SetType_Game, "FPS in TopLeft corner", pCfgDebugFPSTopLeft->GetInt(), 0, sizeofA(pYesNo)-1, pYesNo, OnSettingChange, (void*)DebugFPSTopLeft);

        pCfgFPSNew = cfg->Bind("FPSNew", 30, "Tweaks");
        *(char*)(pGTASA + 0x5E4978) = pCfgFPSNew->GetInt();
        *(char*)(pGTASA + 0x5E4990) = pCfgFPSNew->GetInt();
        sautils->AddSliderItem(SetType_Game, "FPS Limit", pCfgFPSNew->GetInt(), 20, 160, OnSettingChange, OnFPSLimitDraw, (void*)LimitFPS);
    }
}