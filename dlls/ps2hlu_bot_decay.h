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
	void EXPORT BotThink(void);  // think function for the bot
	void Killed(entvars_t *pevAttacker, int iGib);
	void EXPORT PlayerDeathThink(void);
	void EXPORT CreateBot();
	void EXPORT SwapBotWithPlayer();
	virtual void Crouch(void);
	bool IsCrouching(void) const { return m_fIsCrouching; }
	virtual void ExecuteCommand(void);
	//virtual Vector GetAutoaimVector(float delta);

	// actual AI part of the bot
	CBasePlayer *GetEnemy();
	CBaseMonster *GetAttacker() const;
	CBaseMonster *m_attacker;
	bool IsEnemy(CBaseEntity *enemy);
	virtual void PrimaryAttack(void);
	virtual void ClearPrimaryAttack(void);

	// Bots should return FALSE for this, they can't receive NET messages
	virtual bool IsNetClient(void) { return false; }

	int BloodColor() { return BLOOD_COLOR_RED; }
	bool TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker,
		float flDamage, int bitsDamageType);
	int ObjectCaps() { return FCAP_IMPULSE_USE; };
	mutable EHANDLE m_enemy;
private:
	
	bool m_fIsCrouching = false;
	unsigned short m_buttonFlags;
	float m_forwardSpeed;

	// Think mechanism variables
	float m_flNextBotThink;
	float m_flNextFullBotThink;

	// Command interface variables
	float m_flPreviousCommandTime;

	float g_flBotCommandInterval = 1.0 / 30.0;

	// full AI only 10 times per second
	float g_flBotFullThinkInterval = 1.0 / 10.0;

	float m_verticalSpeed;
	byte ThrottledMsec(void) const;
};

inline CBasePlayer *CDecayBot::GetEnemy()
{
	return (CBasePlayer*)(CBaseEntity*)m_enemy;
}