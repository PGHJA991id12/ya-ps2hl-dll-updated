/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
/*

===== items.cpp ========================================================

  functions governing the selection/use of weapons for players

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
#include "skill.h"
#include "items.h"
#include "gamerules.h"
#include "UserMessages.h"

class CWorldItem : public CBaseEntity
{
public:
	bool KeyValue(KeyValueData* pkvd) override;
	void Spawn() override;
	int m_iType;
};

LINK_ENTITY_TO_CLASS(world_items, CWorldItem);

bool CWorldItem::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "type"))
	{
		m_iType = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "player_index"))
	{
		m_decayIndex = atof(pkvd->szValue);
		return true;
	}

	return CBaseEntity::KeyValue(pkvd);
}

void CWorldItem::Spawn()
{
	CBaseEntity* pEntity = NULL;

	switch (m_iType)
	{
	case 44: // ITEM_BATTERY:
		pEntity = CBaseEntity::Create("item_battery", pev->origin, pev->angles);
		break;
	case 42: // ITEM_ANTIDOTE:
		pEntity = CBaseEntity::Create("item_antidote", pev->origin, pev->angles);
		break;
	case 43: // ITEM_SECURITY:
		pEntity = CBaseEntity::Create("item_security", pev->origin, pev->angles);
		break;
	case 45: // ITEM_SUIT:
		pEntity = CBaseEntity::Create("item_suit", pev->origin, pev->angles);
		break;
	}

	if (!pEntity)
	{
		ALERT(at_console, "unable to create world_item %d\n", m_iType);
	}
	else
	{
		pEntity->pev->target = pev->target;
		pEntity->pev->targetname = pev->targetname;
		pEntity->pev->spawnflags = pev->spawnflags;
	}

	REMOVE_ENTITY(edict());
}


void CItem::Spawn()
{
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;
	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 16));
	SetTouch(&CItem::ItemTouch);

	if (DROP_TO_FLOOR(ENT(pev)) == 0)
	{
		ALERT(at_error, "Item %s fell out of level at %f,%f,%f", STRING(pev->classname), pev->origin.x, pev->origin.y, pev->origin.z);
		UTIL_Remove(this);
		return;
	}
}


bool CItem::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "player_index"))
	{
		m_decayIndex = atof(pkvd->szValue);
		return true;
	}

	return CBaseEntity::KeyValue(pkvd);
}

void CItem::ItemTouch(CBaseEntity* pOther)
{
	// if it's not a player, ignore
	if (!pOther->IsPlayer())
	{
		return;
	}

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;

	// PS2HL
	// Support for Player_Index
	if (m_decayIndex)
		if (!(pPlayer->m_decayIndex == this->m_decayIndex))
			return;

	// ok, a player is touching this item, but can he have it?
	if (!g_pGameRules->CanHaveItem(pPlayer, this))
	{
		// no? Ignore the touch.
		return;
	}

	if (MyTouch(pPlayer))
	{
		SUB_UseTargets(pOther, USE_TOGGLE, 0);
		SetTouch(NULL);

		// player grabbed the item.
		g_pGameRules->PlayerGotItem(pPlayer, this);
		if (g_pGameRules->ItemShouldRespawn(this) == GR_ITEM_RESPAWN_YES)
		{
			Respawn();
		}
		else
		{
			UTIL_Remove(this);
		}
	}
	else if (gEvilImpulse101)
	{
		UTIL_Remove(this);
	}
}

CBaseEntity* CItem::Respawn()
{
	SetTouch(NULL);
	pev->effects |= EF_NODRAW;

	UTIL_SetOrigin(pev, g_pGameRules->VecItemRespawnSpot(this)); // blip to whereever you should respawn.

	SetThink(&CItem::Materialize);
	pev->nextthink = g_pGameRules->FlItemRespawnTime(this);
	return this;
}

void CItem::Materialize()
{
	if ((pev->effects & EF_NODRAW) != 0)
	{
		// changing from invisible state to visible.
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "items/suitchargeok1.wav", 1, ATTN_NORM, 0, 150);
		pev->effects &= ~EF_NODRAW;
		pev->effects |= EF_MUZZLEFLASH;
	}

	SetTouch(&CItem::ItemTouch);
}

#define SF_SUIT_SHORTLOGON 0x0001

class CItemSuit : public CItem
{
	void Spawn() override
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_suit.mdl");
		CItem::Spawn();
	}
	void Precache() override
	{
		PRECACHE_MODEL("models/w_suit.mdl");
	}
	bool MyTouch(CBasePlayer* pPlayer) override
	{
		if (pPlayer->HasSuit())
			return false;

		if ((pev->spawnflags & SF_SUIT_SHORTLOGON) != 0)
			EMIT_SOUND_SUIT(pPlayer->edict(), "!HEV_A0"); // short version of suit logon,
		else
			EMIT_SOUND_SUIT(pPlayer->edict(), "!HEV_AAx"); // long version of suit logon

		pPlayer->SetHasSuit(true);
		return true;
	}
};

LINK_ENTITY_TO_CLASS(item_suit, CItemSuit);



class CItemBattery : public CItem
{
	void Spawn() override
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_battery.mdl");
		CItem::Spawn();
	}
	void Precache() override
	{
		PRECACHE_MODEL("models/w_battery.mdl");
		PRECACHE_SOUND("items/gunpickup2.wav");
	}
	bool MyTouch(CBasePlayer* pPlayer) override
	{
		if (pPlayer->pev->deadflag != DEAD_NO)
		{
			return false;
		}

		if ((pPlayer->pev->armorvalue < MAX_NORMAL_BATTERY) &&
			pPlayer->HasSuit())
		{
			int pct;
			char szcharge[64];

			pPlayer->pev->armorvalue += gSkillData.batteryCapacity;
			pPlayer->pev->armorvalue = V_min(pPlayer->pev->armorvalue, MAX_NORMAL_BATTERY);

			EMIT_SOUND(pPlayer->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM);

			MESSAGE_BEGIN(MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev);
			WRITE_STRING(STRING(pev->classname));
			MESSAGE_END();


			// Suit reports new power level
			// For some reason this wasn't working in release build -- round it.
			pct = (int)((float)(pPlayer->pev->armorvalue * 100.0) * (1.0 / MAX_NORMAL_BATTERY) + 0.5);
			pct = (pct / 5);
			if (pct > 0)
				pct--;

			sprintf(szcharge, "!HEV_%1dP", pct);

			//EMIT_SOUND_SUIT(ENT(pev), szcharge);
			pPlayer->SetSuitUpdate(szcharge, false, SUIT_NEXT_IN_30SEC);
			return true;
		}
		return false;
	}
};

LINK_ENTITY_TO_CLASS(item_battery, CItemBattery);


class CItemAntidote : public CItem
{
	void Spawn() override
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_antidote.mdl");
		CItem::Spawn();
	}
	void Precache() override
	{
		PRECACHE_MODEL("models/w_antidote.mdl");
	}
	bool MyTouch(CBasePlayer* pPlayer) override
	{
		pPlayer->SetSuitUpdate("!HEV_DET4", false, SUIT_NEXT_IN_1MIN);

		pPlayer->m_rgItems[ITEM_ANTIDOTE] += 1;
		return true;
	}
};

LINK_ENTITY_TO_CLASS(item_antidote, CItemAntidote);


class CItemSecurity : public CItem
{
	void Spawn() override
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_security.mdl");
		CItem::Spawn();
	}
	void Precache() override
	{
		PRECACHE_MODEL("models/w_security.mdl");
	}
	bool MyTouch(CBasePlayer* pPlayer) override
	{
		pPlayer->m_rgItems[ITEM_SECURITY] += 1;
		return true;
	}
};

LINK_ENTITY_TO_CLASS(item_security, CItemSecurity);

class CItemLongJump : public CItem
{
	void Spawn() override
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_longjump.mdl");
		CItem::Spawn();
	}
	void Precache() override
	{
		PRECACHE_MODEL("models/w_longjump.mdl");
	}
	bool MyTouch(CBasePlayer* pPlayer) override
	{
		if (pPlayer->m_fLongJump)
		{
			return false;
		}

		if (pPlayer->HasSuit())
		{
			pPlayer->m_fLongJump = true; // player now has longjump module

			g_engfuncs.pfnSetPhysicsKeyValue(pPlayer->edict(), "slj", "1");

			MESSAGE_BEGIN(MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev);
			WRITE_STRING(STRING(pev->classname));
			MESSAGE_END();

			EMIT_SOUND_SUIT(pPlayer->edict(), "!HEV_A1"); // Play the longjump sound UNDONE: Kelly? correct sound?
			return true;
		}
		return false;
	}
};

LINK_ENTITY_TO_CLASS(item_longjump, CItemLongJump);

// PS2HLU Focus emitter

// TODO:
// Make shiny thingy on top face current target with smooth transition
// Clean up this mess, and give the focus emitter its own file

// This is a complete mess... This definetly needs a rewrite soon!

typedef enum
{
FOCUSEMITTER_IDLE_CLOSED =0,
FOCUSEMITTER_DEPLOY,
FOCUSEMITTER_IDLE_OPEN,
FOCUSEMITTER_BROKEN1,
FOCUSEMITTER_BROKEN2,
FOCUSEMITTER_DEATH,
} FOCUSEMITTER_ANIM;

class CFocusEmitter : public CBaseMonster
{
public:
	bool KeyValue( KeyValueData *pkvd );
	void Spawn();
	void Precache();
	void Killed( entvars_t *pevAttacker, int iGib );
	void EXPORT DyingThink(void);
	void EXPORT EmitterThink(void);
	bool TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType );
	void EXPORT Animate();
	int Classify();
	int  BloodColor( void ) { return DONT_BLEED; }
	void ChangeSequence( int NewSequence );

	string_t deploy_target;
	string_t death_target;
	int m_Emitterbeam;
	//int m_iExplode;
	CBeam *m_pBeam;
};

void CFocusEmitter::Precache()
{
PRECACHE_MODEL( "models/focus_emitter.mdl" );
//m_Emitterbeam = PRECACHE_MODEL( "sprites/ht_laser.spr" );
PRECACHE_MODEL( "sprites/ht_laser.spr" );
//m_iExplode = PRECACHE_MODEL( "sprites/fexplo.spr" );
}

void CFocusEmitter::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/focus_emitter.mdl");

	//pev->solid = SOLID_NOT; // this one is old
	// BBox
	Vector Zero;
	Zero.x = Zero.y = Zero.z = 0;
	UTIL_SetSize(pev, Zero, Zero);
	pev->solid = SOLID_NOT;
	// used for sequence handleing
	//SetSequenceBox(); // This was a temponary solution
	SetBodygroup( 2,2 );
	SetThink(&CFocusEmitter::EmitterThink);

	ChangeSequence( FOCUSEMITTER_IDLE_CLOSED );
	pev->renderfx = kRenderFxFullbright;
	pev->takedamage = DAMAGE_NO;

	pev->nextthink = -1;
	
	// What was I thinking when I wrote this? This is a mess, will require a complete rewrite sometime soon.

}

void CFocusEmitter::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (pev->takedamage == DAMAGE_NO) {
	ChangeSequence(FOCUSEMITTER_DEPLOY);
	pev->nextthink = gpGlobals->time + 0.1;
	} else {
		return;
	}
}

void CFocusEmitter::EmitterThink (void)
{
	StudioFrameAdvance( 0 );
	SetThink(&CFocusEmitter::EmitterThink);
	pev->nextthink = gpGlobals->time + 0.1;

	switch (pev->sequence)
	{
	case FOCUSEMITTER_DEPLOY:
		//wait for opening animation
		if (m_fSequenceFinished)
		{
		FireTargets( STRING( deploy_target ), this, this, USE_TOGGLE, 0 );
		ChangeSequence( FOCUSEMITTER_IDLE_OPEN );
		pev->takedamage = DAMAGE_YES;

		Vector vecpevpos, vecpevang;
		GetAttachment( 1, vecpevpos, vecpevang );

		m_pBeam = CBeam::BeamCreate("sprites/ht_laser.spr", 40 );
		m_pBeam->PointEntInit( vecpevpos, entindex() );
		m_pBeam->SetEndAttachment( 1 );
		m_pBeam->SetColor( 216, 216, 216 );
		m_pBeam->SetBrightness( 255 );
		m_pBeam->SetScrollRate( 35 );
		}
		break;
	case FOCUSEMITTER_DEATH:
		if (m_fSequenceFinished)
			FireTargets( STRING( death_target ), this, this, USE_TOGGLE, 0 );
			break;
	default:
		// Do nothing
		break;
	}

}

void CFocusEmitter::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType )
{
	if(!(bitsDamageType & DMG_ENERGYBEAM))
	UTIL_Ricochet( ptr->vecEndPos, RANDOM_FLOAT(1.0, 2.0) ); // Make the focus emitter immune to all attacks except lasers!

	CBaseMonster::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
}

bool CFocusEmitter::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{

	/*if (bitsDamageType & DMG_LASER)
	{
		flDamage *= 2;
	}*/
		/*if (pevInflictor->owner == edict())
		return 0;*/

	if (IsAlive())
	{
		if (bitsDamageType & DMG_ENERGYBEAM)
			SetConditions(bits_COND_LIGHT_DAMAGE);
		else
			return false;
	}

		if (pev->health = 2)
		{
		ChangeSequence( FOCUSEMITTER_BROKEN1 );
		}
		else if (pev->health = 1)
		{
		ChangeSequence( FOCUSEMITTER_BROKEN2 );
		} // I have no idea how this was meant to work.

		return CBaseMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}

void CFocusEmitter::Killed( entvars_t *pevAttacker, int iGib )
{
	ChangeSequence( FOCUSEMITTER_DEATH );
	SetThink(&CFocusEmitter::DyingThink);
	pev->nextthink = gpGlobals->time + 0.1;
	pev->takedamage = DAMAGE_NO;
	if (m_pBeam)
	{
		UTIL_Remove( m_pBeam );
		m_pBeam = NULL;
	}
}

void CFocusEmitter::DyingThink(void)
{
	/*		// fireball
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_SPRITE );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y + 5 );
			WRITE_COORD( pev->origin.z);
			WRITE_SHORT( m_iExplode );
			WRITE_BYTE( 120 ); // scale * 10
			WRITE_BYTE( 255 ); // brightness
		MESSAGE_END();*/
		
		// big smoke
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y + 5 );
			WRITE_COORD( pev->origin.z );
			WRITE_SHORT( g_sModelIndexSmoke );
			WRITE_BYTE( 250 ); // scale * 10
			WRITE_BYTE( 5  ); // framerate
		MESSAGE_END();

		FireTargets( STRING( death_target ), this, this, USE_TOGGLE, 0 );
}

int CFocusEmitter::Classify()
{
	return CLASS_NONE;
}

LINK_ENTITY_TO_CLASS( item_focusemitter, CFocusEmitter )

bool CFocusEmitter::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "deploy_target"))
	{
		deploy_target = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "death_target"))
	{
		death_target = ALLOC_STRING(pkvd->szValue);
		return true;
	}

	return CBaseMonster::KeyValue( pkvd );
}

void CFocusEmitter::Animate()
{
/*		//ALERT(at_console, "%s at frame %f\n", STRING( pev->classname ), pev->frame );
	   SetThink(&CFocusEmitter::Animate);
    pev->nextthink = gpGlobals->time+0.01; // 0.01
	if ( pev->frame > 100 )
	{
		//ALERT(at_console, "sequence finished!\n");
		if (pev->sequence = FOCUSEMITTER_DEPLOY)
		{
			FireTargets( STRING( deploy_target ), this, this, USE_TOGGLE, 0 );
			pev->frame = 0;
			pev->sequence = 2;
			pev->framerate = 0;
		}
	} else {
	StudioFrameAdvance( );
	    pev->frame >255? pev->frame=0:pev->frame++;
	//&CFocusEmitter::Animate;
	}*/


	  SetThink(&CFocusEmitter::Animate);
   // pev->nextthink = gpGlobals->time+0.01; // 0.01
	if ( m_fSequenceFinished )
	{
	FireTargets( STRING( deploy_target ), this, this, USE_TOGGLE, 0 );
	} else {
	StudioFrameAdvance(0.1);
	pev->nextthink = gpGlobals->time + 0.1; // 0.01
	}
}

void CFocusEmitter::ChangeSequence(int NewSequence)
{
		//prepare sequence
		pev->sequence = NewSequence;
		pev->frame = 0;
		ResetSequenceInfo(); 
}