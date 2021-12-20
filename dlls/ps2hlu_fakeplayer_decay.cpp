// PS2HLU
// Bot code based on Botmans code

#include "extdll.h"
#include "util.h"
#include "ps2hlu_bot_decay.h"
#include "ps2hl_dbg.h"

extern int gmsgHealth;
extern int gmsgCurWeapon;
extern int gmsgSetFOV;
extern DLL_GLOBAL ULONG g_ulModelIndexPlayer;
// Set in combat.cpp.  Used to pass the damage inflictor for death messages.
extern entvars_t *g_pevLastInflictor;

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
		edict_t *BotEnt = CREATE_FAKE_CLIENT("Colette");

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

		ClientConnect(BotClass->edict(), "Colette", "127.0.0.1", ptr);
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
	int PlayerBody = 0;
	int BotBody = 0;


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
			PlayerBody = pPlayer->pev->body;
			BotBody = pPlayer2->pev->body;

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
			pPlayer->pev->body = BotBody;
			pPlayer2->pev->body = PlayerBody;

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

	g_ulModelIndexPlayer = pev->modelindex;

	SetThink(&CDecayBot::BotThink);
	pev->nextthink = gpGlobals->time + .1;
}

int CDecayBot::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
{
	return CBasePlayer::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}

void CDecayBot::PlayerDeathThink(void)
{
	float flForward;

	pev->nextthink = gpGlobals->time + 0.1;

	if (FBitSet(pev->flags, FL_ONGROUND))
	{
		flForward = pev->velocity.Length() - 20;
		if (flForward <= 0)
			pev->velocity = g_vecZero;
		else
			pev->velocity = flForward * pev->velocity.Normalize();
	}

	if (HasWeapons())
	{
		/*
		we drop the guns here because weapons that have an area effect and can kill their user
		will sometimes crash coming back from CBasePlayer::Killed() if they kill their owner because the
		player class sometimes is freed. It's safer to manipulate the weapons once we know
		we aren't calling into any of their code anymore through the player pointer.
		*/
		PackDeadPlayerItems();
	}

	if (pev->modelindex && (!m_fSequenceFinished) && (pev->deadflag == DEAD_DYING))
	{
		StudioFrameAdvance();

		m_iRespawnFrames++;
		if (m_iRespawnFrames < 60)  // animations should be no longer than this
			return;
	}

	if (pev->deadflag == DEAD_DYING)
	{
		pev->deadflag = DEAD_DEAD;
		DROP_TO_FLOOR(ENT(pev));  // put the body on the ground
	}

	StopAnimation();

	pev->effects |= EF_NOINTERP;
	pev->framerate = 0.0;

	if (pev->deadflag == DEAD_DEAD)
	{
		if (g_pGameRules->FPlayerCanRespawn(this))
		{
			m_fDeadTime = gpGlobals->time;
			pev->deadflag = DEAD_RESPAWNABLE;
		}

		return;
	}

	// check if time to respawn...
	if (gpGlobals->time > (m_fDeadTime + 5.0))
	{
		pev->button = 0;
		m_iRespawnFrames = 0;

		//ALERT( at_console, "Respawn\n" );

		respawn(pev, !(m_afPhysicsFlags & PFLAG_OBSERVER));
		pev->nextthink = -1;
	}
}

void CDecayBot::Killed(entvars_t *pevAttacker, int iGib)
{
	CSound *pSound;

	g_pGameRules->PlayerKilled(this, pevAttacker, g_pevLastInflictor);

	if (m_pTank != NULL)
	{
		m_pTank->Use(this, this, USE_OFF, 0);
		m_pTank = NULL;
	}

	// this client isn't going to be thinking for a while, so reset the sound
	// until they respawn
	pSound = CSoundEnt::SoundPointerForIndex(CSoundEnt::ClientSoundIndex(edict()));
	{
		if (pSound)
		{
			pSound->Reset();
		}
	}

	SetAnimation(PLAYER_DIE);

	pev->modelindex = g_ulModelIndexPlayer;    // don't use eyes

#if !defined(DUCKFIX)
	pev->view_ofs = Vector(0, 0, -8);
#endif
	pev->deadflag = DEAD_DYING;
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_TOSS;
	//   pev->movetype      = MOVETYPE_NONE;  // should we use this instead???

	ClearBits(pev->flags, FL_ONGROUND);

	if (pev->velocity.z < 10)
		pev->velocity.z += RANDOM_FLOAT(0, 300);

	// clear out the suit message cache so we don't keep chattering
	SetSuitUpdate(NULL, FALSE, 0);

	// send "health" update message to zero
	m_iClientHealth = 0;

	MESSAGE_BEGIN(MSG_ONE, gmsgHealth, NULL, pev);
	WRITE_BYTE(m_iClientHealth);
	MESSAGE_END();

	// Tell Ammo Hud that the player is dead
	MESSAGE_BEGIN(MSG_ONE, gmsgCurWeapon, NULL, pev);
	WRITE_BYTE(0);
	WRITE_BYTE(0xFF);
	WRITE_BYTE(0xFF);
	MESSAGE_END();

	// reset FOV
	m_iFOV = m_iClientFOV = 0;

	MESSAGE_BEGIN(MSG_ONE, gmsgSetFOV, NULL, pev);
	WRITE_BYTE(0);
	MESSAGE_END();

	if ((pev->health < -40 && iGib != GIB_NEVER) || iGib == GIB_ALWAYS)
	{
		GibMonster();   // This clears pev->model
	}

	DeathSound();

	pev->angles.x = 0;  // don't let the player tilt
	pev->angles.z = 0;

	// PS2HLU
	// End mission target fire
	if (m_decayIndex == 1)
		FireTargets("decay_player1_dead", this, this, USE_TOGGLE, 0);
	if (m_decayIndex == 2)
		FireTargets("decay_player2_dead", this, this, USE_TOGGLE, 0);

	SetThink(&CDecayBot::PlayerDeathThink);
	pev->nextthink = gpGlobals->time + 0.1;
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
	/*
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
	*/
}