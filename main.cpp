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
bool bForceNoCull = false;

/* Config */
ConfigEntry* pCfgDebugFPS;
ConfigEntry* pCfgDebugFPSText;
ConfigEntry* pCfgFPSNew;
ConfigEntry* pCfgBackfaceCulling;
ConfigEntry* pCfgVehicleBackfaceCulling;
ConfigEntry* pCfgBreakableBackfaceCulling;

const char* pYesNo[] = 
{
    "FEM_OFF",
    "FEM_ON",
};

enum eSettingToChange : unsigned char
{
    DebugFPS = 0,
    DebugFPSText,
    LimitFPS,
    BackfaceCulling
};

char szRetText[8];
char szFPSText[12];
void OnSettingChange(int oldVal, int newVal, void* data)
{
    eSettingToChange set = (eSettingToChange)(int)data; // why do i need to cast it to int first? bruh compiler moment
    switch(set)
    {
        case DebugFPS:
        {
            pCfgDebugFPS->SetBool(newVal != 0);
            *(bool*)(pGTASA + 0x98F1AD) = (newVal != 0);
            break;
        }
        case DebugFPSText:
        {
            pCfgDebugFPSText->SetBool(newVal != 0);
            szFPSText[0] = 0;
            strcpy(szFPSText, newVal != 0 ? "%.2f" : "FPS: %.2f");
            break;
        }
        case LimitFPS:
        {
            pCfgFPSNew->SetInt(newVal);
            *(char*)(pGTASA + 0x5E4978) = newVal;
            *(char*)(pGTASA + 0x5E4990) = newVal;
            break;
        }
        case BackfaceCulling:
        {
            pCfgBackfaceCulling->SetInt(newVal);
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

DECL_HOOK(void, RwRenderStateSet, int state, int val)
{
    if(state == 20 && (bForceNoCull || pCfgBackfaceCulling->GetBool()))
    {
        RwRenderStateSet(20, 1);
        return;
    }
    RwRenderStateSet(state, val);
}
DECL_HOOK(void, EntityRender, uintptr_t self)
{
    static short model_id;
    model_id = *(short*)(self + 38);
    if(model_id >= 400 && model_id < 615 && pCfgVehicleBackfaceCulling->GetBool())
    {
        bForceNoCull = true;
        //RwRenderStateSet(20, 1);
        EntityRender(self);
        bForceNoCull = false;
        //HookOf_RwRenderStateSet(20, 2);
        return;
    }
    EntityRender(self);
}

extern "C" void OnModPreLoad()
{
    logger->SetTag("GTASA More Settings");
    pGTASA = aml->GetLib("libGTASA.so");

    aml->Unprot(pGTASA + 0x98F1AD, sizeof(bool)); // Debug FPS
    aml->Unprot(pGTASA + 0x3F56A0, 16); SET_TO(szFPSText, pGTASA + 0x3F56A0); // Debug FPS Text
    aml->Unprot(pGTASA + 0x5E4978, sizeof(char)); aml->Unprot(pGTASA + 0x5E4990, sizeof(char)); // FPS

    sautils = (ISAUtils*)GetInterface("SAUtils");
    if(sautils != nullptr)
    {
        pCfgDebugFPS = cfg->Bind("DebugFPS", *(bool*)(pGTASA + 0x98F1AD), "Tweaks");
        *(bool*)(pGTASA + 0x98F1AD) = pCfgDebugFPS->GetBool();
        sautils->AddClickableItem(SetType_Game, "Debug FPS", pCfgDebugFPS->GetInt(), 0, sizeofA(pYesNo)-1, pYesNo, OnSettingChange, (void*)DebugFPS);

        pCfgDebugFPSText = cfg->Bind("DebugFPS_NoFPSText", true, "Tweaks");
        if(pCfgDebugFPSText->GetBool())
        {
            szFPSText[0] = 0;
            strcpy(szFPSText, "%.2f");
        }
        sautils->AddClickableItem(SetType_Game, "Show only FPS", pCfgDebugFPSText->GetInt(), 0, sizeofA(pYesNo)-1, pYesNo, OnSettingChange, (void*)DebugFPSText);

        pCfgFPSNew = cfg->Bind("FPSNew", 30, "Tweaks");
        *(char*)(pGTASA + 0x5E4978) = pCfgFPSNew->GetInt();
        *(char*)(pGTASA + 0x5E4990) = pCfgFPSNew->GetInt();
        sautils->AddSliderItem(SetType_Game, "FPS Limit", pCfgFPSNew->GetInt(), 20, 160, OnSettingChange, OnFPSLimitDraw, (void*)LimitFPS);

        // Backface Culling
        pCfgBackfaceCulling = cfg->Bind("DisableBackfaceCulling", false, "Tweaks");
        sautils->AddClickableItem(SetType_Display, "Disable Backface Culling", pCfgBackfaceCulling->GetBool(), 0, sizeofA(pYesNo)-1, pYesNo, OnSettingChange, (void*)BackfaceCulling);
        HOOKPLT(RwRenderStateSet, pGTASA + 0x6711B8);

        // Vehicle Backface Culling
        //pCfgVehicleBackfaceCulling = cfg->Bind("VehicleDisableBackfaceCulling", false, "Tweaks");
        //sautils->AddSettingsItem(SetType_Display, "Disable Backface Culling for Vehicle", pCfgVehicleBackfaceCulling->GetBool(), 0, sizeofA(pYesNo)-1, VehicleBackfaceCullingChanged, false, (void*)pYesNo);
        //HOOKPLT(EntityRender, pGTASA + 0x66F764);
    }
}