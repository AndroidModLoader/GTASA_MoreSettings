#include <string>
#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>

#include <filesystem>
namespace fs = std::filesystem;

#define sizeofA(__aVar)  ((int)(sizeof(__aVar)/sizeof(__aVar[0])))

MYMODCFG(net.rusjj.gtasa.moresettings, GTA:SA More Settings, 1.2, RusJJ)
NEEDGAME(com.rockstargames.gtasa)
BEGIN_DEPLIST()
    ADD_DEPENDENCY_VER(net.rusjj.aml, 1.0)
    ADD_DEPENDENCY_VER(net.rusjj.gtasa.utils, 1.1)
END_DEPLIST()

/* SA Utils */
#include "isautils.h"
ISAUtils* sautils = nullptr;

/* Saves */
uintptr_t pGTASA = 0;
bool bForceNoCull = false;

/* Config */
ConfigEntry* pCfgDebugFPS;
ConfigEntry* pCfgFPSNew;
ConfigEntry* pCfgBackfaceCulling;
ConfigEntry* pCfgVehicleBackfaceCulling;
ConfigEntry* pCfgBreakableBackfaceCulling;

const char* pYesNo[] = 
{
    "FEM_OFF",
    "FEM_ON",
};

void DebugFPSChanged(int oldVal, int newVal)
{
    pCfgDebugFPS->SetBool(newVal==0?false:true);
    *(bool*)(pGTASA + 0x98F1AD) = pCfgDebugFPS->GetBool();
    cfg->Save();
}
void FPSNewChanged(int oldVal, int newVal)
{
    pCfgFPSNew->SetInt(newVal);
    *(char*)(pGTASA + 0x5E4978) = newVal;
    *(char*)(pGTASA + 0x5E4990) = newVal;
    cfg->Save();
}
char szRetText[8];
const char* FPSNewDrawed(int newVal)
{
    sprintf(szRetText, "%d", newVal);
    return szRetText;
}
void BackfaceCullingChanged(int oldVal, int newVal)
{
    pCfgBackfaceCulling->SetInt(newVal);
    cfg->Save();
}
void VehicleBackfaceCullingChanged(int oldVal, int newVal)
{
    pCfgVehicleBackfaceCulling->SetInt(newVal);
    cfg->Save();
}
void BreakableBackfaceCullingChanged(int oldVal, int newVal)
{
    pCfgBreakableBackfaceCulling->SetInt(newVal);
    cfg->Save();
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

extern "C" void OnModLoad()
{
    logger->SetTag("GTASA More Settings");
    pGTASA = aml->GetLib("libGTASA.so");

    aml->Unprot(pGTASA + 0x98F1AD, sizeof(bool)); // Debug FPS
    aml->Unprot(pGTASA + 0x5E4978, sizeof(char)); aml->Unprot(pGTASA + 0x5E4990, sizeof(char)); // FPS

    sautils = (ISAUtils*)GetInterface("SAUtils");
    if(sautils != nullptr)
    {
        pCfgDebugFPS = cfg->Bind("DebugFPS", *(bool*)(pGTASA + 0x98F1AD), "Tweaks");
        *(bool*)(pGTASA + 0x98F1AD) = pCfgDebugFPS->GetBool();
        sautils->AddClickableItem(Game, "Debug FPS", pCfgDebugFPS->GetInt(), 0, sizeofA(pYesNo)-1, pYesNo, DebugFPSChanged);

        pCfgFPSNew = cfg->Bind("FPSNew", 30, "Tweaks");
        *(char*)(pGTASA + 0x5E4978) = pCfgFPSNew->GetInt();
        *(char*)(pGTASA + 0x5E4990) = pCfgFPSNew->GetInt();
        sautils->AddSliderItem(Game, "FPS Limit", pCfgFPSNew->GetInt(), 20, 120, FPSNewChanged, FPSNewDrawed);

        // Backface Culling
        pCfgBackfaceCulling = cfg->Bind("DisableBackfaceCulling", false, "Tweaks");
        sautils->AddClickableItem(Display, "Disable Backface Culling", pCfgBackfaceCulling->GetBool(), 0, sizeofA(pYesNo)-1, pYesNo, BackfaceCullingChanged);
        HOOKPLT(RwRenderStateSet, pGTASA + 0x6711B8);

        // Vehicle Backface Culling
        //pCfgVehicleBackfaceCulling = cfg->Bind("VehicleDisableBackfaceCulling", false, "Tweaks");
        //sautils->AddSettingsItem(Display, "Disable Backface Culling for Vehicle", pCfgVehicleBackfaceCulling->GetBool(), 0, sizeofA(pYesNo)-1, VehicleBackfaceCullingChanged, false, (void*)pYesNo);
        //HOOKPLT(EntityRender, pGTASA + 0x66F764);
    }
}