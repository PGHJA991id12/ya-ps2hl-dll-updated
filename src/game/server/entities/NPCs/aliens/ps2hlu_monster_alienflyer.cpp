#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "soundent.h"
#include "effects.h"
#include "military/osprey.h"

class CAlienFlyer : public COsprey
{
public:
	void Spawn() override;
	void Precache() override;
	int  Classify() { return CLASS_NONE; };
	int  BloodColor() override { return BLOOD_COLOR_YELLOW; }
	void Killed(entvars_t *pevAttacker, int iGib) override;
	bool KeyValue(KeyValueData *pkvd) override;

	void UpdateGoal(void);
	void EXPORT FlyThink(void);
	void Flight(void);
	void EXPORT FindAllThink(void);
	void EXPORT HoverThink(void);
	void EXPORT CrashTouch(CBaseEntity *pOther);
	void EXPORT DyingThink(void);
	void EXPORT CommandUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void EXPORT AttackThink(void);

	//int  TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType );
	void TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType) override;

	CBaseEntity *m_pGoalEnt;
	Vector m_vel1;
	Vector m_vel2;
	Vector m_pos1;
	Vector m_pos2;
	Vector m_ang1;
	Vector m_ang2;
	float m_startTime;
	float m_dTime;

	Vector m_velocity;

	float m_flIdealtilt;
	float m_flRotortilt;

	int m_iSpriteTexture;

	int m_iPitch;

	int m_iExplode;
	int	m_iBodyGibs;

	int m_iDoLeftSmokePuff = 0;
	int m_iDoRightSmokePuff =0;

	string_t death_target = 0;
};

LINK_ENTITY_TO_CLASS(monster_alienflyer, CAlienFlyer);


void CAlienFlyer::Spawn(void)
{
	Precache();
	// motor
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/flyer.mdl");
	UTIL_SetSize(pev, Vector(-400, -400, -100), Vector(400, 400, 32));
	UTIL_SetOrigin(pev, pev->origin);

	// Set FL_FLY so the model is interpolated
	pev->flags |= FL_MONSTER | FL_FLY;
	pev->takedamage = DAMAGE_YES;
	pev->health = gSkillData.flyerHealth;
	m_bloodColor = BLOOD_COLOR_YELLOW;

	m_flFieldOfView = 0; // 180 degrees

	pev->sequence = 0;
	ResetSequenceInfo();
	pev->frame = RANDOM_LONG(0, 0xFF);

	//InitBoneControllers();

	SetThink(&CAlienFlyer::FindAllThink);
	SetUse(&CAlienFlyer::CommandUse);




	if ((pev->spawnflags & SF_WAITFORTRIGGER) == 0)
	{
		pev->nextthink = gpGlobals->time + 1.0;
	}
	else
	{
		pev->effects |= EF_NODRAW;
		pev->solid = SOLID_NOT;
	}

	m_pos2 = pev->origin;
	m_ang2 = pev->angles;
	m_vel2 = pev->velocity;
}


void CAlienFlyer::Precache(void)
{
	UTIL_PrecacheOther("monster_alien_grunt");

	PRECACHE_MODEL("models/flyer.mdl");
	PRECACHE_MODEL("models/HVR.mdl");

	//PRECACHE_SOUND("apache/ap_rotor4.wav");
	PRECACHE_SOUND("weapons/mortarhit.wav");

	m_iSpriteTexture = PRECACHE_MODEL("sprites/xenobeam.spr");
	PRECACHE_SOUND("debris/beamstart1.wav");
	m_iExplode = PRECACHE_MODEL("sprites/fexplo.spr");
	//m_iTailGibs = PRECACHE_MODEL("models/osprey_tailgibs.mdl");
	m_iBodyGibs = PRECACHE_MODEL("models/flyer_gibs.mdl");
	//m_iEngineGibs = PRECACHE_MODEL("models/osprey_enginegibs.mdl");
}

bool CAlienFlyer::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "death_target"))
	{
		death_target = ALLOC_STRING(pkvd->szValue);
		return true;
	}

	return CBaseMonster::KeyValue(pkvd);
}

void CAlienFlyer::CommandUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	pev->solid = SOLID_BBOX;
	pev->effects &= ~EF_NODRAW;
	pev->nextthink = gpGlobals->time + 0.1;
}

void CAlienFlyer::Killed(entvars_t *pevAttacker, int iGib)
{
	pev->movetype = MOVETYPE_TOSS;
	pev->gravity = 0.3;
	pev->velocity = m_velocity;
	pev->avelocity = Vector(RANDOM_FLOAT(-20, 20), 0, RANDOM_FLOAT(-50, 50));
	//STOP_SOUND(ENT(pev), CHAN_STATIC, "apache/ap_rotor4.wav");

	UTIL_SetSize(pev, Vector(-32, -32, -64), Vector(32, 32, 0));
	SetThink(&CAlienFlyer::DyingThink);
	SetTouch(&CAlienFlyer::CrashTouch);
	pev->nextthink = gpGlobals->time + 0.1;
	pev->health = 0;
	pev->takedamage = DAMAGE_NO;

	m_startTime = gpGlobals->time + 4.0;
}

void CAlienFlyer::TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	//UTIL_Sparks(ptr->vecEndPos);
	AddMultiDamage(pevAttacker, this, flDamage, bitsDamageType);
}

void CAlienFlyer::Flight()
{
	float t = (gpGlobals->time - m_startTime);

	// Only update if delta time is non-zero. It's zero if we're not moving at all (usually because we have no target).
	if (m_dTime != 0)
	{
		float scale = 1.0 / m_dTime;

		float f = UTIL_SplineFraction(t * scale, 1.0);

		Vector pos = (m_pos1 + m_vel1 * t) * (1.0 - f) + (m_pos2 - m_vel2 * (m_dTime - t)) * f;
		Vector ang = (m_ang1) * (1.0 - f) + (m_ang2)*f;
		m_velocity = m_vel1 * (1.0 - f) + m_vel2 * f;

		UTIL_SetOrigin(pev, pos);
		pev->angles = ang;
	}

	UTIL_MakeAimVectors(pev->angles);
	float flSpeed = DotProduct(gpGlobals->v_forward, m_velocity);

	// float flSpeed = DotProduct( gpGlobals->v_forward, pev->velocity );

	float m_flIdealtilt = (160 - flSpeed) / 10.0;

	// ALERT( at_console, "%f %f\n", flSpeed, flIdealtilt );
	if (m_flRotortilt < m_flIdealtilt)
	{
		m_flRotortilt += 0.5;
		if (m_flRotortilt > 0)
			m_flRotortilt = 0;
	}
	if (m_flRotortilt > m_flIdealtilt)
	{
		m_flRotortilt -= 0.5;
		if (m_flRotortilt < -90)
			m_flRotortilt = -90;
	}
	//SetBoneController(0, m_flRotortilt);

	/*
	if (m_iSoundState == 0)
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, "apache/ap_rotor4.wav", 1.0, 0.15, 0, 110);
		// EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, "apache/ap_whine1.wav", 0.5, 0.2, 0, 110 );

		m_iSoundState = SND_CHANGE_PITCH; // hack for going through level transitions
	}
	else
	{
		CBaseEntity *pPlayer = NULL;

		pPlayer = UTIL_FindEntityByClassname(NULL, "player");
		// UNDONE: this needs to send different sounds to every player for multiplayer.	
		if (pPlayer)
		{
			float pitch = DotProduct(m_velocity - pPlayer->pev->velocity, (pPlayer->pev->origin - pev->origin).Normalize());

			pitch = (int)(100 + pitch / 75.0);

			if (pitch > 250)
				pitch = 250;
			if (pitch < 50)
				pitch = 50;

			if (pitch == 100)
				pitch = 101;

			if (pitch != m_iPitch)
			{
				m_iPitch = pitch;
				EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, "apache/ap_rotor4.wav", 1.0, 0.15, SND_CHANGE_PITCH | SND_CHANGE_VOL, pitch);
				// ALERT( at_console, "%.0f\n", pitch );
			}
		}
		// EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, "apache/ap_whine1.wav", flVol, 0.2, SND_CHANGE_PITCH | SND_CHANGE_VOL, pitch);
	}
	*/
}

void CAlienFlyer::FindAllThink(void)
{
	SetThink(&CAlienFlyer::FlyThink);
	pev->nextthink = gpGlobals->time + 0.1;
	m_startTime = gpGlobals->time;
}

void CAlienFlyer::FlyThink(void)
{
	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1;

	if (m_pGoalEnt == NULL && !FStringNull(pev->target))// this monster has a target
	{
		m_pGoalEnt = CBaseEntity::Instance(FIND_ENTITY_BY_TARGETNAME(NULL, STRING(pev->target)));
		UpdateGoal();
	}

	if (gpGlobals->time > m_startTime + m_dTime)
	{
		if (m_pGoalEnt != nullptr)
		{
			if (m_pGoalEnt->pev->speed == 0)
			{
				// ALERT(at_console, "Alien flyer has stopped!\n");
				// pev->speed = 0; // doesnt solve the visual bug
				SetThink(&CAlienFlyer::AttackThink);
				pev->nextthink = gpGlobals->time + 0.1;
			}
			// removed the flyer speed check it was causing the previous check to fail
			FireTargets(STRING(m_pGoalEnt->pev->message), this, this, USE_TOGGLE, 0);
			m_pGoalEnt = CBaseEntity::Instance(FIND_ENTITY_BY_TARGETNAME(NULL, STRING(m_pGoalEnt->pev->target)));

			UpdateGoal();
		}
	}

	Flight();
}

void CAlienFlyer::UpdateGoal()
{
	if (m_pGoalEnt)
	{
		m_pos1 = m_pos2;
		m_ang1 = m_ang2;
		m_vel1 = m_vel2;
		m_pos2 = m_pGoalEnt->pev->origin;
		m_ang2 = m_pGoalEnt->pev->angles;
		UTIL_MakeAimVectors(Vector(0, m_ang2.y, 0));
		m_vel2 = gpGlobals->v_forward * m_pGoalEnt->pev->speed;

		m_startTime = m_startTime + m_dTime;
		m_dTime = 2.0 * (m_pos1 - m_pos2).Length() / (m_vel1.Length() + m_pGoalEnt->pev->speed);

		if (m_ang1.y - m_ang2.y < -180)
		{
			m_ang1.y += 360;
		}
		else if (m_ang1.y - m_ang2.y > 180)
		{
			m_ang1.y -= 360;
		}

		if (m_pGoalEnt->pev->speed < 400)
			m_flIdealtilt = 0;
		else
			m_flIdealtilt = -90;
	}
	else
	{
		ALERT(at_console, "osprey missing target");
	}
}

void CAlienFlyer::AttackThink(void)
{
	UTIL_MakeAimVectors(pev->angles);
	CBaseEntity* emitter = CBaseEntity::Instance(FIND_ENTITY_BY_CLASSNAME(NULL, "item_focusemitter"));
		if (emitter != NULL)
			{
				EMIT_SOUND(ENT(pev), CHAN_STATIC, "debris/beamstart1.wav", 1.0, 0.15);

				CBeam* pBeam = CBeam::BeamCreate("sprites/xenobeam.spr", 255);
				pBeam->PointEntInit(pev->origin + Vector(0, 0, 15), emitter->entindex());
				// commented out to mathch the ps2 version
				// pBeam->SetFlags(0x80); // shade end
				pBeam->SetScrollRate(40);
				pBeam->SetNoise(3);
				pBeam->SetColor(254, 244, 233);
				pBeam->SetBrightness(150);
				pBeam->SetThink(&CBaseEntity::SUB_Remove);
				pBeam->pev->nextthink = gpGlobals->time + 1.0f;

				CBeam* pBeam2 = CBeam::BeamCreate("sprites/laserbeam.spr", 30);
				pBeam2->PointEntInit(pev->origin + Vector(0, 0, 15), emitter->entindex());
				// pBeam2->SetFlags(0x80); // shade end
				pBeam2->SetScrollRate(35);
				pBeam2->SetNoise(100);
				pBeam2->SetColor(255, 255, 255);
				pBeam2->SetBrightness(125);
				pBeam2->SetThink(&CBaseEntity::SUB_Remove);
				pBeam2->pev->nextthink = gpGlobals->time + 1.0f;

				// TODO: Add beam cylinder

				// Skill level in PS2 Decay is 2
				emitter->TakeDamage(pev, pev, (gSkillData.flyerDmg / 40.0f), DMG_ENERGYBEAM);
				ALERT(at_console, "focus emitter health: %f \n", (int)emitter->pev->health);
			}
			else
			ALERT(at_console, "Focus emitter not found!");

	SetThink(&CAlienFlyer::HoverThink);
	pev->nextthink = gpGlobals->time + 0.1;
}

void CAlienFlyer::HoverThink(void)
{
	m_startTime = gpGlobals->time;
	SetThink(&CAlienFlyer::FlyThink);

	pev->nextthink = gpGlobals->time + 0.1;
	UTIL_MakeAimVectors(pev->angles);
}

void CAlienFlyer::CrashTouch(CBaseEntity *pOther)
{
	// only crash if we hit something solid
	if (pOther->pev->solid == SOLID_BSP)
	{
		SetTouch(NULL);
		m_startTime = gpGlobals->time;
		pev->nextthink = gpGlobals->time;
		m_velocity = pev->velocity;
	}
}


void CAlienFlyer::DyingThink(void)
{
	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1;

	pev->avelocity = pev->avelocity * 1.02;

	FireTargets(STRING(death_target), this, this, USE_TOGGLE, 0);

	// still falling?
	if (m_startTime > gpGlobals->time)
	{
		UTIL_MakeAimVectors(pev->angles);

		Vector vecSpot = pev->origin + pev->velocity * 0.2;

		// random explosions
		MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, vecSpot);
		WRITE_BYTE(TE_EXPLOSION);		// This just makes a dynamic light now
		WRITE_COORD(vecSpot.x + RANDOM_FLOAT(-150, 150));
		WRITE_COORD(vecSpot.y + RANDOM_FLOAT(-150, 150));
		WRITE_COORD(vecSpot.z + RANDOM_FLOAT(-150, -50));
		WRITE_SHORT(g_sModelIndexFireball);
		WRITE_BYTE(RANDOM_LONG(0, 29) + 30); // scale * 10
		WRITE_BYTE(12); // framerate
		WRITE_BYTE(TE_EXPLFLAG_NONE);
		MESSAGE_END();

		// lots of smoke
		MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, vecSpot);
		WRITE_BYTE(TE_SMOKE);
		WRITE_COORD(vecSpot.x + RANDOM_FLOAT(-150, 150));
		WRITE_COORD(vecSpot.y + RANDOM_FLOAT(-150, 150));
		WRITE_COORD(vecSpot.z + RANDOM_FLOAT(-150, -50));
		WRITE_SHORT(g_sModelIndexSmoke);
		WRITE_BYTE(100); // scale * 10
		WRITE_BYTE(10); // framerate
		MESSAGE_END();

		/*
		vecSpot = pev->origin + (pev->mins + pev->maxs) * 0.5;
		MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, vecSpot);
		WRITE_BYTE(TE_BREAKMODEL);

		// position
		WRITE_COORD(vecSpot.x);
		WRITE_COORD(vecSpot.y);
		WRITE_COORD(vecSpot.z);

		// size
		WRITE_COORD(800);
		WRITE_COORD(800);
		WRITE_COORD(132);

		// velocity
		WRITE_COORD(pev->velocity.x);
		WRITE_COORD(pev->velocity.y);
		WRITE_COORD(pev->velocity.z);

		// randomization
		WRITE_BYTE(50);

		// Model
		WRITE_SHORT(m_iTailGibs);	//model id#

		// # of shards
		WRITE_BYTE(8);	// let client decide

		// duration
		WRITE_BYTE(200);// 10.0 seconds

		// flags

		WRITE_BYTE(BREAK_METAL);
		MESSAGE_END();*/



		// don't stop it we touch a entity
		pev->flags &= ~FL_ONGROUND;
		pev->nextthink = gpGlobals->time + 0.2;
		return;
	}
	else
	{
		Vector vecSpot = pev->origin + (pev->mins + pev->maxs) * 0.5;

		/*
		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_EXPLOSION);		// This just makes a dynamic light now
			WRITE_COORD( vecSpot.x );
			WRITE_COORD( vecSpot.y );
			WRITE_COORD( vecSpot.z + 512 );
			WRITE_SHORT( m_iExplode );
			WRITE_BYTE( 250 ); // scale * 10
			WRITE_BYTE( 10  ); // framerate
		MESSAGE_END();
		*/

		// gibs
		MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, vecSpot);
		WRITE_BYTE(TE_SPRITE);
		WRITE_COORD(vecSpot.x);
		WRITE_COORD(vecSpot.y);
		WRITE_COORD(vecSpot.z + 512);
		WRITE_SHORT(m_iExplode);
		WRITE_BYTE(250); // scale * 10
		WRITE_BYTE(255); // brightness
		MESSAGE_END();

		/*
		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( vecSpot.x );
			WRITE_COORD( vecSpot.y );
			WRITE_COORD( vecSpot.z + 300 );
			WRITE_SHORT( g_sModelIndexSmoke );
			WRITE_BYTE( 250 ); // scale * 10
			WRITE_BYTE( 6  ); // framerate
		MESSAGE_END();
		*/

		// blast circle
		MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
		WRITE_BYTE(TE_BEAMCYLINDER);
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z);
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z + 2000); // reach damage radius over .2 seconds
		WRITE_SHORT(m_iSpriteTexture);
		WRITE_BYTE(0); // startframe
		WRITE_BYTE(0); // framerate
		WRITE_BYTE(4); // life
		WRITE_BYTE(32);  // width
		WRITE_BYTE(0);   // noise
		WRITE_BYTE(255);   // r, g, b
		WRITE_BYTE(255);   // r, g, b
		WRITE_BYTE(192);   // r, g, b
		WRITE_BYTE(128); // brightness
		WRITE_BYTE(0);		// speed
		MESSAGE_END();

		EMIT_SOUND(ENT(pev), CHAN_STATIC, "weapons/mortarhit.wav", 1.0, 0.3);

		RadiusDamage(pev->origin, pev, pev, 300, CLASS_NONE, DMG_BLAST);

		// gibs
		vecSpot = pev->origin + (pev->mins + pev->maxs) * 0.5;
		MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, vecSpot);
		WRITE_BYTE(TE_BREAKMODEL);

		// position
		WRITE_COORD(vecSpot.x);
		WRITE_COORD(vecSpot.y);
		WRITE_COORD(vecSpot.z + 64);

		// size
		WRITE_COORD(800);
		WRITE_COORD(800);
		WRITE_COORD(128);

		// velocity
		WRITE_COORD(m_velocity.x);
		WRITE_COORD(m_velocity.y);
		WRITE_COORD(fabs(m_velocity.z) * 0.25);

		// randomization
		WRITE_BYTE(40);

		// Model
		WRITE_SHORT(m_iBodyGibs);	//model id#

		// # of shards
		WRITE_BYTE(128);

		// duration
		WRITE_BYTE(200);// 10.0 seconds

		// flags

		WRITE_BYTE(BREAK_FLESH);
		MESSAGE_END();

		UTIL_Remove(this);
	}
}