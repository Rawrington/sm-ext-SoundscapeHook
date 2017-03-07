#include "extension.h"

CSoundscapeHook g_Interface;
SMEXT_LINK(&g_Interface);

class CBasePlayer;
class CEnvSoundscape;

struct ss_update_t
{
	CBasePlayer *pPlayer;
	CEnvSoundscape	*pCurrentSoundscape;
	Vector		playerPosition;
	float		currentDistance;
	int			traceCount;
	bool		bInRange;
};

IForward *g_pSoundscapeUpdateForPlayer = NULL;
IGameConfig *g_pGameConf = NULL;
CDetour *DUpdateForPlayer;

DETOUR_DECL_MEMBER1(UpdateForPlayer, void, ss_update_t&, update)
{
	if (g_pSoundscapeUpdateForPlayer->GetFunctionCount()) {
		cell_t soundscape = gamehelpers->EntityToBCompatRef((CBaseEntity*)this);
		cell_t client = gamehelpers->EntityToBCompatRef((CBaseEntity*)(update.pPlayer));
		cell_t pl_res = Pl_Continue;
		
		g_pSoundscapeUpdateForPlayer->PushCell(soundscape);
		g_pSoundscapeUpdateForPlayer->PushCell(client);
		g_pSoundscapeUpdateForPlayer->Execute(&pl_res);
		
		if (pl_res != Pl_Continue) {
			update.pCurrentSoundscape = nullptr;
			update.currentDistance = 0;
			update.bInRange = false;
			return;
		}
	}
	
	DETOUR_MEMBER_CALL(UpdateForPlayer)(update);
}

bool CSoundscapeHook::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	g_pSoundscapeUpdateForPlayer = forwards->CreateForward("SoundscapeUpdateForPlayer", ET_Hook, 2, NULL, Param_Cell, Param_Cell);
	char conf_error[255] = "";
	if(!gameconfs->LoadGameConfigFile("soundscapehook", &g_pGameConf, conf_error, sizeof(conf_error)))
	{
		if(conf_error[0])
			snprintf(error, maxlength, "Could not read from soundscapehook.txt: %s", conf_error);
		
		return false;
	}
	CDetourManager::Init(g_pSM->GetScriptingEngine(), g_pGameConf);
	DUpdateForPlayer = DETOUR_CREATE_MEMBER(UpdateForPlayer, "CEnvSoundscape::UpdateForPlayer");
	DUpdateForPlayer->EnableDetour();
	return true;
}

void CSoundscapeHook::SDK_OnUnload()
{
	forwards->ReleaseForward(g_pSoundscapeUpdateForPlayer);
	gameconfs->CloseGameConfigFile(g_pGameConf);
	DUpdateForPlayer->Destroy();
}