/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
//=========================================================
// Generic Monster - purely for scripted sequence work.
//=========================================================
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"
#include "soundent.h"

// For holograms, make them not solid so the player can walk through them
#define SF_GENERICMONSTER_NOTSOLID 4

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

class CGenericMonster : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	void SetYawSpeed() override;
	int Classify() override;
	void HandleAnimEvent(MonsterEvent_t* pEvent) override;
	int ISoundMask() override;

	// PS2HLU
	void PlayScriptedSentence(const char* pszSentence, float duration, float volume, float attenuation, bool bConcurrent, CBaseEntity* pListener);
	void IdleHeadTurn(Vector& vecFriend);
	void EXPORT MonsterThink();

	// PS2HL
	string_t m_iszTargetTrigger;
	string_t m_iszNoTargetTrigger;

	bool TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType) {
		if (pevAttacker)
			if (!strcmp(STRING(pevAttacker->classname), "player"))
				FireTargets(STRING(m_iszTargetTrigger), this, this, USE_ON, 1);
		return CBaseMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
	}
	bool KeyValue(KeyValueData *pkvd) {
		if (FStrEq(pkvd->szKeyName, "no_target_trigger"))
		{
			//ALERT(at_console, "mon_gen: no aim - %s\n", pkvd->szValue);
			m_iszNoTargetTrigger = ALLOC_STRING(pkvd->szValue);
			return true;
		}
		else if (FStrEq(pkvd->szKeyName, "target_trigger"))
		{
			//ALERT(at_console, "mon_gen: aim - %s\n", pkvd->szValue);
			m_iszTargetTrigger = ALLOC_STRING(pkvd->szValue);
			return true;
		}

		//else if (FStrEq(pkvd->szKeyName, "health"))
		//{
		//	ALERT(at_console, "mon_gen: hp - %s\n", pkvd->szValue);
		//	pev->health = atof(pkvd->szValue);
		//	return true;
		//}
			return CBaseMonster::KeyValue(pkvd);
	}
	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;

	static TYPEDESCRIPTION m_SaveData[];

// PS2HLU
private:
	float m_talkTime;
	EHANDLE m_hTalkTarget;
	float m_flIdealYaw;
	float m_flCurrentYaw;
};
LINK_ENTITY_TO_CLASS(monster_generic, CGenericMonster);

// PS2HL - Save/restore
TYPEDESCRIPTION CGenericMonster::m_SaveData[] =
{
	DEFINE_FIELD(CGenericMonster, m_iszTargetTrigger, FIELD_STRING),
	DEFINE_FIELD(CGenericMonster, m_iszNoTargetTrigger, FIELD_STRING),
	DEFINE_FIELD(CGenericMonster, m_talkTime, FIELD_FLOAT),
	DEFINE_FIELD(CGenericMonster, m_hTalkTarget, FIELD_EHANDLE),
	DEFINE_FIELD(CGenericMonster, m_flIdealYaw, FIELD_FLOAT),
	DEFINE_FIELD(CGenericMonster, m_flCurrentYaw, FIELD_FLOAT),
};
IMPLEMENT_SAVERESTORE(CGenericMonster, CBaseMonster);

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int CGenericMonster::Classify()
{
	// PS2HLU
	// Fix human grunts shooting at a soda can on ht07signal
	// TODO: Need to figure out what the other flag does
	// Update: "trigger_hit" is the value used in the
	// Hazard Course for the autoaim section, for some reason this value
	// is defined for multiple entities in ht07signal

	// if (pev->spawnflags & 0x8000)
	if (pev->spawnflags & 131072)
		return CLASS_NONE;

	return	CLASS_PLAYER_ALLY;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CGenericMonster::SetYawSpeed()
{
	int ys;

	switch (m_Activity)
	{
	case ACT_IDLE:
	default:
		ys = 90;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CGenericMonster::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	switch (pEvent->event)
	{
	case 0:
	default:
		CBaseMonster::HandleAnimEvent(pEvent);
		break;
	}
}

//=========================================================
// ISoundMask - generic monster can't hear.
//=========================================================
int CGenericMonster::ISoundMask()
{
	return bits_SOUND_NONE;
}

//=========================================================
// Spawn
//=========================================================
void CGenericMonster::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), STRING(pev->model));

	/*
	if ( FStrEq( STRING(pev->model), "models/player.mdl" ) )
		UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);
	else
		UTIL_SetSize(pev, VEC_HULL_MIN, VEC_HULL_MAX);
*/

	if (FStrEq(STRING(pev->model), "models/player.mdl") || FStrEq(STRING(pev->model), "models/holo.mdl"))
		UTIL_SetSize(pev, VEC_HULL_MIN, VEC_HULL_MAX);
	else
		UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = 8;
	m_flFieldOfView = 0.5; // indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;

	// PS2HL
	if (pev->health == 0)
		pev->health = 8;
	//else
	//	ALERT(at_console, "mon_gen: hp - %f\n", pev->health);

	MonsterInit();

	if ((pev->spawnflags & SF_GENERICMONSTER_NOTSOLID) != 0)
	{
		pev->solid = SOLID_NOT;
		pev->takedamage = DAMAGE_NO;
	}

	// PS2HLU
	if (pev->spawnflags & 8)
		m_afCapability = bits_CAP_TURN_HEAD;

	m_flIdealYaw = m_flCurrentYaw = 0;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CGenericMonster::Precache()
{
	PRECACHE_MODEL((char*)STRING(pev->model));
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

/**
This code is from FWGS's Blue-Shift recreation, specifically from this commit:
https://github.com/FWGS/hlsdk-portable/commit/1e529268980f59c337f0f149fd57b0ecbba4db3e
**/

void CGenericMonster::PlayScriptedSentence(const char* pszSentence, float duration, float volume, float attenuation, bool bConcurrent, CBaseEntity* pListener)
{
	m_talkTime = gpGlobals->time + duration;
	PlaySentence(pszSentence, duration, volume, attenuation);

	m_hTalkTarget = pListener;
}

void CGenericMonster::IdleHeadTurn(Vector& vecFriend)
{
	// turn head in desired direction only if ent has a turnable head
	if (m_afCapability & bits_CAP_TURN_HEAD)
	{
		float yaw = VecToYaw(vecFriend - pev->origin) - pev->angles.y;

		if (yaw > 180)
			yaw -= 360;
		if (yaw < -180)
			yaw += 360;

		m_flIdealYaw = yaw;
	}
}

void CGenericMonster::MonsterThink()
{
	if (m_afCapability & bits_CAP_TURN_HEAD)
	{
		if (m_hTalkTarget != 0)
		{
			if (gpGlobals->time > m_talkTime)
			{
				m_flIdealYaw = 0;
				m_hTalkTarget = 0;
			}
			else
			{
				IdleHeadTurn(m_hTalkTarget->pev->origin);
			}
		}

		if (m_flCurrentYaw != m_flIdealYaw)
		{
			if (m_flCurrentYaw <= m_flIdealYaw)
			{
				m_flCurrentYaw += V_min(m_flIdealYaw - m_flCurrentYaw, 20.0f);
			}
			else
			{
				m_flCurrentYaw -= V_min(m_flCurrentYaw - m_flIdealYaw, 20.0f);
			}
			SetBoneController(0, m_flCurrentYaw);
		}
	}

	CBaseMonster::MonsterThink();
}