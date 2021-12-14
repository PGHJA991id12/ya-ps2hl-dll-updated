#pragma once

#include "client.h"
#include "cbase.h"
#include "player.h"
#include "items.h"
#include "effects.h"
#include "weapons.h"
#include "soundent.h"
#include "gamerules.h"
#include "animation.h"
#include "enginecallback.h"

#define MODEL_HGRUNT    1
#define MODEL_BARNEY    2
#define MODEL_SCIENTIST 3


class CDecayBot : public CBasePlayer
{
public:
	void Spawn(void);
	void BotThink(void);  // think function for the bot
	void CreateBot();
	void SwapBotWithPlayer();

	// Bots should return FALSE for this, they can't receive NET messages
	virtual BOOL IsNetClient(void) { return FALSE; }

	int BloodColor() { return BLOOD_COLOR_RED; }
	int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker,
		float flDamage, int bitsDamageType);
	int ObjectCaps() { return FCAP_IMPULSE_USE; };


private:
};
