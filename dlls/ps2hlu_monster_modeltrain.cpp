// PS2HLU
// Monster modeltrain entity
// Used  in ht04dorms for the helicopters (very underutilized)
// Also in theory this could works as a spritetrain (used in Opposing Force)
// Based on CFuncTrain

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"effects.h"

// Spawnflags of CPathCorner
#define SF_CORNER_WAITFORTRIG	0x001
#define SF_CORNER_TELEPORT		0x002
#define SF_CORNER_FIREONCE		0x004

class CModelTrain : public CBaseMonster
{
public:
	void Spawn(void);
	void Precache(void);
	void SetYawSpeed(void);
	void Activate(void);
	int  Classify(void);
	void HandleAnimEvent(MonsterEvent_t *pEvent);
	bool KeyValue(KeyValueData *pkvd) {
		if (FStrEq(pkvd->szKeyName, "speed"))
		{
			m_speed = atoi(pkvd->szValue);
			return true;
		}
		else if (FStrEq(pkvd->szKeyName, "yaw_speed"))
		{
			//ALERT(at_console, "mon_gen: aim - %s\n", pkvd->szValue);
			newYawSpeed = atoi(pkvd->szValue);
			//ALERT(at_console, "monster_modeltrain current yawspeed is:%s\n", pev->yaw_speed);
			return true;
		}
		return CBaseMonster::KeyValue(pkvd);
	}
	void Blocked(CBaseEntity *pOther);
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void EXPORT Next(void);
	void EXPORT Wait(void);

	int newYawSpeed;
	int m_speed;
	entvars_t	*m_pevCurrentTarget;
	bool		m_activated;
	bool		locked=false;
	CSprite		*m_Sprite;
	CBaseEntity *prevActivator = nullptr;
private:
};

LINK_ENTITY_TO_CLASS(monster_modeltrain, CModelTrain);

void CModelTrain::Spawn()
{
	if (pev->speed == 0)
		pev->speed = 100;

	Precache();

	if (FStringNull(pev->target))
		ALERT(at_console, "Monster_ModelTrain with no target!\n");

	SET_MODEL(ENT(pev), STRING(pev->model));
	UTIL_SetSize(pev, pev->mins, pev->maxs);
	UTIL_SetOrigin(pev, pev->origin);

	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_PUSH;
	m_bloodColor = DONT_BLEED;
	pev->takedamage = DAMAGE_NO;
	pev->effects = 0;
	SetBits(pev->flags, FL_MONSTER); // Count this as a monster
	// This may or may not cause issues, im just leaving this here for now

		if (!pev->framerate)
			pev->framerate = 24.0;

		if (!pev->scale)
			pev->scale = 1.0;


	pev->sequence = 0;
	pev->frame = 0;

	// PS2HLU
	// This flag makes it so the sequence info wont be reset
	// it causes a crash if a sprite is set as a model
	if(!(pev->spawnflags & 262144))
	ResetSequenceInfo();

	pev->effects = 0;
	pev->health = 9999;
	//m_flFieldOfView = 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_activated = false;
	locked = false;

	SetUse(&CModelTrain::Use);
}

void CModelTrain::Precache()
{
	PRECACHE_MODEL((char *)STRING(pev->model));
}

int	CModelTrain::Classify(void)
{
	return	CLASS_NONE;
}

void CModelTrain::SetYawSpeed(void)
{
	pev->yaw_speed = newYawSpeed;
	ALERT(at_console, "monster_modeltrain current yawspeed is:%d\n", pev->yaw_speed);
}

void CModelTrain::Blocked(CBaseEntity *pOther)

{
	if (gpGlobals->time < m_flActivateFinished)
		return;

	m_flActivateFinished = gpGlobals->time + 0.5;

	pOther->TakeDamage(pev, pev, pev->dmg, DMG_CRUSH);
}

void CModelTrain::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	// Dont allow same entity to trigger this more than once in a row
	if (pev->spawnflags & 65536)
	{
		//ALERT(at_console, "Door was activated by: %d", pActivator);
		if (prevActivator == pActivator)
			return;
	}

	prevActivator = pActivator;
	
	if (pev->spawnflags & 262144) {
		// Create the sprite
		if (m_Sprite == NULL)
		{
			m_Sprite = CSprite::SpriteCreate((char *)STRING(pev->model), pev->origin, true);
			if (m_Sprite)
			{
				m_Sprite->SetTransparency(pev->rendermode, pev->rendercolor.x, pev->rendercolor.y, pev->rendercolor.z, pev->renderamt, pev->renderfx); // this may be broken
				m_Sprite->SetAttachment(edict(), 0);
				m_Sprite->Animate(pev->framerate);
				m_Sprite->SetAttachment(pev->aiment, pev->body);
				m_Sprite->SetScale(pev->scale);
				m_Sprite->pev->framerate = pev->framerate;
				m_Sprite->TurnOn();
			}
		}
	}

	if (pev->spawnflags & SF_TRAIN_WAIT_RETRIGGER)
	{
		// Move toward my target
		pev->spawnflags &= ~SF_TRAIN_WAIT_RETRIGGER;
		Next();
	}
	else
	{
		pev->spawnflags |= SF_TRAIN_WAIT_RETRIGGER;
		// Pop back to last target if it's available
		if (pev->enemy)
			pev->target = pev->enemy->v.targetname;
		pev->nextthink = 0;
		pev->velocity = g_vecZero;
	}
}


void CModelTrain::Wait(void)
{
	// Fire the pass target if there is one
	if (m_pevCurrentTarget->message)
	{
		FireTargets(STRING(m_pevCurrentTarget->message), this, this, USE_TOGGLE, 0);
		if (FBitSet(m_pevCurrentTarget->spawnflags, SF_CORNER_FIREONCE))
			m_pevCurrentTarget->message = 0;
	}

	// need pointer to LAST target.
	if (FBitSet(m_pevCurrentTarget->spawnflags, SF_TRAIN_WAIT_RETRIGGER) || (pev->spawnflags & SF_TRAIN_WAIT_RETRIGGER))
	{
		pev->spawnflags |= SF_TRAIN_WAIT_RETRIGGER;
		pev->nextthink = 0;
		return;
	}

	// ALERT ( at_console, "%f\n", m_flWait );

	if (m_flWait != 0)
	{// -1 wait will wait forever!		
		pev->nextthink = pev->ltime + m_flWait;
		SetThink(&CModelTrain::Next);
	}
	else
	{
		Next();// do it RIGHT now!
	}
}


//
// Train next - path corner needs to change to next target 
//
void CModelTrain::Next(void)
{
	CBaseEntity	*pTarg;

	// now find our next target
	pTarg = GetNextTarget();

	// This makes it so if an incorrect target is specified
	// The train goes to the worldspawn, but if theres no target, it just stops
	if (locked) return;

	if (pTarg && FStringNull(pTarg->pev->target))
	{
		locked = true;
	}

	if (!pTarg)
	{
		pTarg = CBaseEntity::Instance(FIND_ENTITY_BY_CLASSNAME(NULL, "worldspawn"));
		locked = true;
	}

	// Save last target in case we need to find it again
	pev->message = pev->target;

	//if(pTarg->pev->target)
	pev->target = pTarg->pev->target;
	m_flWait = pTarg->GetDelay();

	if (m_pevCurrentTarget && m_pevCurrentTarget->speed != 0)
	{// don't copy speed from target if it is 0 (uninitialized)
		pev->speed = m_pevCurrentTarget->speed;
		ALERT(at_aiconsole, "Train %s speed to %4.2f\n", STRING(pev->targetname), pev->speed);
	}
	m_pevCurrentTarget = pTarg->pev;// keep track of this since path corners change our target for us.

	pev->enemy = pTarg->edict(); // hack

	// PS2HLU
	// Always use the angles of the next target (path)
	pev->angles = pTarg->pev->angles;
	


	if (FBitSet(m_pevCurrentTarget->spawnflags, SF_CORNER_TELEPORT))
	{
		// Path corner has indicated a teleport to the next corner.
		SetBits(pev->effects, EF_NOINTERP);
		UTIL_SetOrigin(pev, pTarg->pev->origin - (pev->mins + pev->maxs)* 0.5);
		Wait(); // Get on with doing the next path corner.
	}
	else
	{
		// Normal linear move.

		// this is not a hack or temporary fix, this is how things should be. (sjb).
		ClearBits(pev->effects, EF_NOINTERP);
		SetMoveDone(&CModelTrain::Wait);
		LinearMove(pTarg->pev->origin - (pev->mins + pev->maxs)* 0.5, pev->speed);
	}
}


void CModelTrain::Activate(void)
{

	// Not yet active, so teleport to first target
	if (!m_activated)
	{
		m_activated = true;
		entvars_t	*pevTarg = VARS(FIND_ENTITY_BY_TARGETNAME(NULL, STRING(pev->target)));

		pev->target = pevTarg->target;
		m_pevCurrentTarget = pevTarg;// keep track of this since path corners change our target for us.

		UTIL_SetOrigin(pev, pevTarg->origin - (pev->mins + pev->maxs) * 0.5);

		if (FStringNull(pev->targetname))
		{	// not triggered, so start immediately
			pev->nextthink = pev->ltime + 0.1;
			SetThink(&CModelTrain::Next);
		}
		else
			pev->spawnflags |= SF_TRAIN_WAIT_RETRIGGER;
	}
}

// Can this code run if the model is a sprite?
void CModelTrain::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case 0:
	default:
		CBaseMonster::HandleAnimEvent(pEvent);
		break;
	}
}