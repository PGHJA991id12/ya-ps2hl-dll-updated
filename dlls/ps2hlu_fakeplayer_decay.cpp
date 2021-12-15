// PS2HLU
// Bot code based on Botmans code

#include "extdll.h"
#include "util.h"
#include "ps2hlu_bot_decay.h"
#include "ps2hl_dbg.h"

inline edict_t *CREATE_FAKE_CLIENT(const char *netname)
{
	return (*g_engfuncs.pfnCreateFakeClient)(netname);
}

void BotCreate()
{
	CDecayBot myDecayBot;
	myDecayBot.CreateBot();
}

void DoBotSwap()
{
	CDecayBot myDecayBot;
	myDecayBot.SwapBotWithPlayer();
}

void CDecayBot::CreateBot(void)
{
		edict_t *BotEnt = CREATE_FAKE_CLIENT("Bot");

	if (FNullEnt(BotEnt)) {
		UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "Max. Players reached.  Can't create AI Player!\n");

		if (IS_DEDICATED_SERVER())
			printf("Max. Players reached.  Can't create AI Player!\n");
		return;
	}
	else
	{
		char ptr[128];  // allocate space for message from ClientConnect
		CDecayBot *BotClass;
		char *infobuffer;
		int clientIndex;

		BotClass = GetClassPtr((CDecayBot *)VARS(BotEnt));
		infobuffer = g_engfuncs.pfnGetInfoKeyBuffer(BotClass->edict());
		clientIndex = BotClass->entindex();

		g_engfuncs.pfnSetClientKeyValue(clientIndex, infobuffer, "model", "ginacol");

		ClientConnect(BotClass->edict(), "Bot", "127.0.0.1", ptr);
		DispatchSpawn(BotClass->edict());

		return;
	}
}

//START BOT
CBasePlayer *CBasePlayerByIndex(int playerIndex)
{
	CBasePlayer *pPlayer = NULL;
	entvars_t *pev;

	if (playerIndex > 0 && playerIndex <= gpGlobals->maxClients)
	{
		edict_t *pPlayerEdict = INDEXENT(playerIndex);
		if (pPlayerEdict && !pPlayerEdict->free &&
			(pPlayerEdict->v.flags & FL_FAKECLIENT || pPlayerEdict->v.flags & FL_CLIENT)) //fake
		{
			pev = &pPlayerEdict->v;
			pPlayer = GetClassPtr((CBasePlayer *)pev);
		}
	}

	return pPlayer;
}
//END BOT


void CDecayBot::SwapBotWithPlayer()
{

	CBaseEntity *pPlayer = CBaseEntity::Instance(g_engfuncs.pfnPEntityOfEntIndex(1));
	Vector BotOrigin;
	Vector PlayerOrigin;
	Vector BotAngles;
	Vector PlayerAngles;
	Vector BotMins;
	Vector PlayerMins;
	Vector BotMaxs;
	Vector PlayerMaxs;
	Vector BotVAngles;
	Vector PlayerVAngles;
	int PlayerSkin=0;
	int BotSkin=0;


	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBasePlayer *pPlayer2 = CBasePlayerByIndex(i);

		if (!pPlayer2)  // if invalid then continue with next index...
			continue;

		// check if this is a FAKECLIENT (i.e. is it a bot?)
		if (FBitSet(pPlayer2->pev->flags, FL_FAKECLIENT))
		{
			// Set Data
			BotOrigin = pPlayer2->pev->origin;
			PlayerOrigin = pPlayer->pev->origin;
			BotAngles = pPlayer2->pev->angles;
			PlayerAngles = pPlayer->pev->angles;
			PlayerMins = pPlayer->pev->mins;
			BotMins = pPlayer2->pev->mins;
			PlayerMaxs = pPlayer->pev->maxs;
			BotMaxs = pPlayer2->pev->maxs;
			BotVAngles = pPlayer2->pev->v_angle;
			PlayerVAngles = pPlayer->pev->v_angle;
			PlayerSkin = pPlayer->pev->skin;
			BotSkin = pPlayer2->pev->skin;

			// Do the actual switch
			UTIL_SetOrigin(pPlayer->pev, BotOrigin);
			UTIL_SetOrigin(pPlayer2->pev, PlayerOrigin);
			pPlayer->pev->angles = BotAngles;
			pPlayer2->pev->angles = PlayerAngles;
			pPlayer->pev->v_angle = BotVAngles;
			pPlayer2->pev->v_angle = PlayerVAngles;
			UTIL_SetSize(pPlayer->pev, BotMins, BotMaxs);
			UTIL_SetSize(pPlayer2->pev, PlayerMins, PlayerMaxs);
			//pPlayer2->pev->absmax = pPlayer->pev->absmax;
			//pPlayer2->pev->absmin = pPlayer->pev->absmax;
			pPlayer->pev->skin = BotSkin;
			pPlayer2->pev->skin = PlayerSkin;

			// This is important!!!
			// Make sure bot & player face in the right direction
			// BUGBUG: The engine corrects angles or something
			// Its always weird
			pPlayer2->pev->fixangle = 1;
			pPlayer->pev->fixangle = 1;

			// TODO: Send over health, armor, ammo, and weapons
		}
	}
}

void CDecayBot::Spawn()
{

	CBasePlayer::Spawn();

	pev->flags = FL_CLIENT | FL_FAKECLIENT;

	pev->sequence = LookupActivity(ACT_IDLE);
	SetAnimation(PLAYER_IDLE);
	pev->view_ofs = g_vecZero;

	DROP_TO_FLOOR(ENT(pev));

	m_pLastItem = NULL;
	m_fInitHUD = TRUE;
	m_iClientHideHUD = -1;     // force this to be recalculated
	m_fWeapon = FALSE;
	m_pClientActiveItem = NULL;
	m_iClientBattery = -1;

	SetThink(&CDecayBot::BotThink);
	pev->nextthink = gpGlobals->time + .1;
}

int CDecayBot::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
{
	return CBasePlayer::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}

void CDecayBot::BotThink(void)
{
	if (!IsInWorld())
	{
		SetTouch(NULL);
		UTIL_Remove(this);
		return;
	}

	if (!IsAlive())
	{
		// if bot is dead, don't take up any space in world (set size to 0)
		UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));
		pev->solid = SOLID_NOT;

		return;
	}

	StudioFrameAdvance();
	ItemPostFrame();

	// DEBUG
	// This code visualizes the bounding box
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBasePlayer *pPlayer2 = CBasePlayerByIndex(i);

		if (!pPlayer2)  // if invalid then continue with next index...
			continue;

		// check if this is a FAKECLIENT (i.e. is it a bot?)
		if (FBitSet(pPlayer2->pev->flags, FL_FAKECLIENT))
			DBG_RenderBBox(pPlayer2->pev->origin, pPlayer2->pev->mins, pPlayer2->pev->maxs, 10, 10, 10);
	}
}