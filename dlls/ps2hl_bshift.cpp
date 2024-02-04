/*

13.09.18

Blue Shift's "item_armorvest", "item_helmet" and "monster_rosenberg" entities for ps2hl

Taken directly from here:
https://github.com/FWGS/hlsdk-xash3d/blob/blueshift/dlls/

*/

// Includes for armor
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
//#include "skill.h"
#include "items.h"
#include "gamerules.h"

// Includes for Rosenberg
#include "monsters.h"
#include "talkmonster.h"
#include "schedule.h"
#include "defaultai.h"
#include "scripted.h"
#include "animation.h"
#include "soundent.h"

extern int gmsgItemPickup;



/////////////////////
// Barney vest
/////////////////////

class CItemArmorVest : public CItem
{
	void Spawn(void)
	{
		Precache();
		SET_MODEL(ENT(pev), "models/barney_vest.mdl");
		CItem::Spawn();
	}
	void Precache(void)
	{
		PRECACHE_MODEL("models/barney_vest.mdl");
		PRECACHE_SOUND("items/gunpickup2.wav");
	}
	bool MyTouch(CBasePlayer *pPlayer)
	{
		if ((pPlayer->pev->armorvalue < MAX_NORMAL_BATTERY) &&
			(pPlayer->pev->weapons & (1 << WEAPON_SUIT)))
		{
			pPlayer->pev->armorvalue += 60;
			pPlayer->pev->armorvalue = V_min(pPlayer->pev->armorvalue, MAX_NORMAL_BATTERY);

			EMIT_SOUND(pPlayer->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM);

			MESSAGE_BEGIN(MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev);
			WRITE_STRING(STRING(pev->classname));
			MESSAGE_END();
			return true;
		}
		return false;
	}
};

LINK_ENTITY_TO_CLASS(item_armorvest, CItemArmorVest)



/////////////////////
// Barney helmet
/////////////////////

class CItemHelmet : public CItem
{
	void Spawn(void)
	{
		Precache();
		SET_MODEL(ENT(pev), "models/barney_helmet.mdl");
		CItem::Spawn();
	}
	void Precache(void)
	{
		PRECACHE_MODEL("models/barney_helmet.mdl");
		PRECACHE_SOUND("items/gunpickup2.wav");
	}
	bool MyTouch(CBasePlayer *pPlayer)
	{
		if ((pPlayer->pev->armorvalue < MAX_NORMAL_BATTERY) &&
			(pPlayer->pev->weapons & (1 << WEAPON_SUIT)))
		{
			pPlayer->pev->armorvalue += 40;
			pPlayer->pev->armorvalue = V_min(pPlayer->pev->armorvalue, MAX_NORMAL_BATTERY);

			EMIT_SOUND(pPlayer->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM);

			MESSAGE_BEGIN(MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev);
			WRITE_STRING(STRING(pev->classname));
			MESSAGE_END();
			return true;
		}
		return false;
	}
};

LINK_ENTITY_TO_CLASS(item_helmet, CItemHelmet)



/////////////////////
// Rosenberg
/////////////////////

#define NUM_SCIENTIST_HEADS		4 // four heads available for scientist model

enum
{
	HEAD_GLASSES = 0,
	HEAD_EINSTEIN = 1,
	HEAD_LUTHER = 2,
	HEAD_SLICK = 3
};

enum
{
	SCHED_HIDE = LAST_TALKMONSTER_SCHEDULE + 1,
	SCHED_FEAR,
	SCHED_PANIC,
	SCHED_STARTLE,
	SCHED_TARGET_CHASE_SCARED,
	SCHED_TARGET_FACE_SCARED
};

enum
{
	TASK_SAY_HEAL = LAST_TALKMONSTER_TASK + 1,
	TASK_HEAL,
	TASK_SAY_FEAR,
	TASK_RUN_PATH_SCARED,
	TASK_SCREAM,
	TASK_RANDOM_SCREAM,
	TASK_MOVE_TO_TARGET_RANGE_SCARED
};

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		SCIENTIST_AE_HEAL	( 1 )
#define		SCIENTIST_AE_NEEDLEON	( 2 )
#define		SCIENTIST_AE_NEEDLEOFF	( 3 )

//=======================================================
// Scientist
//=======================================================
class CRosenberg : public CTalkMonster
{
public:
	void Spawn(void) override;
	void Precache(void) override;

	void SetYawSpeed(void) override;
	int Classify(void) override;
	void HandleAnimEvent(MonsterEvent_t *pEvent);
	void RunTask(Task_t *pTask) override;
	void StartTask(Task_t *pTask) override;
	int ObjectCaps(void) { return CTalkMonster::ObjectCaps() | FCAP_IMPULSE_USE; }
	bool TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType) override;
	virtual int FriendNumber(int arrayNumber) override;
	void SetActivity(Activity newActivity) override;
	Activity GetStoppedActivity(void) override;
	int ISoundMask(void);
	void DeclineFollowing(void);

	float CoverRadius(void) { return 1200; }		// Need more room for cover because scientists want to get far away!
	bool DisregardEnemy(CBaseEntity *pEnemy) { return !pEnemy->IsAlive() || (gpGlobals->time - m_fearTime) > 15; }

	bool CanHeal(void);
	void Heal(void);
	void Scream(void);

	// Override these to set behavior
	Schedule_t *GetScheduleOfType(int Type);
	Schedule_t *GetSchedule(void);
	MONSTERSTATE GetIdealState(void);

	void DeathSound(void);
	void PainSound(void);

	void TalkInit(void);

	void Killed(entvars_t *pevAttacker, int iGib);

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	CUSTOM_SCHEDULES

private:
	float m_painTime;
	float m_healTime;
	float m_fearTime;
};

LINK_ENTITY_TO_CLASS(monster_rosenberg, CRosenberg)

TYPEDESCRIPTION	CRosenberg::m_SaveData[] =
{
	DEFINE_FIELD(CRosenberg, m_painTime, FIELD_TIME),
	DEFINE_FIELD(CRosenberg, m_healTime, FIELD_TIME),
	DEFINE_FIELD(CRosenberg, m_fearTime, FIELD_TIME),
};

IMPLEMENT_SAVERESTORE(CRosenberg, CTalkMonster)

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
Task_t	tlRFollow[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_CANT_FOLLOW },	// If you fail, bail out of follow
	{ TASK_MOVE_TO_TARGET_RANGE, (float)128 },	// Move within 128 of target ent (client)
												//{ TASK_SET_SCHEDULE, (float)SCHED_TARGET_FACE },
};

Schedule_t slRFollow[] =
{
	{
		tlRFollow,
		ARRAYSIZE(tlRFollow),
	bits_COND_NEW_ENEMY |
	bits_COND_LIGHT_DAMAGE |
	bits_COND_HEAVY_DAMAGE |
	bits_COND_HEAR_SOUND,
	bits_SOUND_COMBAT |
	bits_SOUND_DANGER,
	"Follow"
	},
};

Task_t tlRFollowScared[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_TARGET_CHASE },// If you fail, follow normally
	{ TASK_MOVE_TO_TARGET_RANGE_SCARED, (float)128 },	// Move within 128 of target ent (client)
														//{ TASK_SET_SCHEDULE, (float)SCHED_TARGET_FACE_SCARED },
};

Schedule_t slRFollowScared[] =
{
	{
		tlRFollowScared,
		ARRAYSIZE(tlRFollowScared),
	bits_COND_NEW_ENEMY |
	bits_COND_HEAR_SOUND |
	bits_COND_LIGHT_DAMAGE |
	bits_COND_HEAVY_DAMAGE,
	bits_SOUND_DANGER,
	"FollowScared"
	},
};

Task_t tlRFaceTargetScared[] =
{
	{ TASK_FACE_TARGET, (float)0 },
	{ TASK_SET_ACTIVITY, (float)ACT_CROUCHIDLE },
	{ TASK_SET_SCHEDULE, (float)SCHED_TARGET_CHASE_SCARED },
};

Schedule_t slRFaceTargetScared[] =
{
	{
		tlRFaceTargetScared,
		ARRAYSIZE(tlRFaceTargetScared),
	bits_COND_HEAR_SOUND |
	bits_COND_NEW_ENEMY,
	bits_SOUND_DANGER,
	"FaceTargetScared"
	},
};

Task_t tlRStopFollowing[] =
{
	{ TASK_CANT_FOLLOW, (float)0 },
};

Schedule_t slRStopFollowing[] =
{
	{
		tlRStopFollowing,
		ARRAYSIZE(tlRStopFollowing),
	0,
	0,
	"StopFollowing"
	},
};

Task_t tlRHeal[] =
{
	{ TASK_MOVE_TO_TARGET_RANGE, (float)50 },	// Move within 60 of target ent (client)
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_TARGET_CHASE },	// If you fail, catch up with that guy! (change this to put syringe away and then chase)
	{ TASK_FACE_IDEAL, (float)0 },
	{ TASK_SAY_HEAL, (float)0 },
	{ TASK_PLAY_SEQUENCE_FACE_TARGET, (float)ACT_ARM },			// Whip out the needle
	{ TASK_HEAL, (float)0 },	// Put it in the player
	{ TASK_PLAY_SEQUENCE_FACE_TARGET, (float)ACT_DISARM },			// Put away the needle
};

Schedule_t slRHeal[] =
{
	{
		tlRHeal,
		ARRAYSIZE(tlRHeal),
	0,	// Don't interrupt or he'll end up running around with a needle all the time
	0,
	"Heal"
	},
};

Task_t tlRFaceTarget[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_FACE_TARGET, (float)0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_SET_SCHEDULE, (float)SCHED_TARGET_CHASE },
};

Schedule_t slRFaceTarget[] =
{
	{
		tlRFaceTarget,
		ARRAYSIZE(tlRFaceTarget),
	bits_COND_CLIENT_PUSH |
	bits_COND_NEW_ENEMY |
	bits_COND_HEAR_SOUND,
	bits_SOUND_COMBAT |
	bits_SOUND_DANGER,
	"FaceTarget"
	},
};

Task_t tlRSciPanic[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_SCREAM, (float)0 },
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY, (float)ACT_EXCITED },	// This is really fear-stricken excitement
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
};

Schedule_t slRSciPanic[] =
{
	{
		tlRSciPanic,
		ARRAYSIZE(tlRSciPanic),
	0,
	0,
	"SciPanic"
	},
};

Task_t tlRIdleSciStand[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_WAIT, (float)2 }, // repick IDLESTAND every two seconds.
	{ TASK_TLK_HEADRESET, (float)0 }, // reset head position
};

Schedule_t slRIdleSciStand[] =
{
	{
		tlRIdleSciStand,
		ARRAYSIZE(tlRIdleSciStand),
	bits_COND_NEW_ENEMY |
	bits_COND_LIGHT_DAMAGE |
	bits_COND_HEAVY_DAMAGE |
	bits_COND_HEAR_SOUND |
	bits_COND_SMELL |
	bits_COND_CLIENT_PUSH |
	bits_COND_PROVOKED,
	bits_SOUND_COMBAT |// sound flags
					   //bits_SOUND_PLAYER |
					   //bits_SOUND_WORLD |
	bits_SOUND_DANGER |
	bits_SOUND_MEAT |// scents
	bits_SOUND_CARCASS |
	bits_SOUND_GARBAGE,
	"IdleSciStand"

	},
};

Task_t tlRScientistCover[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_PANIC },		// If you fail, just panic!
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_FIND_COVER_FROM_ENEMY, (float)0 },
	{ TASK_RUN_PATH_SCARED, (float)0 },
	{ TASK_TURN_LEFT, (float)179 },
	{ TASK_SET_SCHEDULE, (float)SCHED_HIDE },
};

Schedule_t slRScientistCover[] =
{
	{
		tlRScientistCover,
		ARRAYSIZE(tlRScientistCover),
	bits_COND_NEW_ENEMY,
	0,
	"ScientistCover"
	},
};

Task_t tlRScientistHide[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_PANIC },		// If you fail, just panic!
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_CROUCH },
	{ TASK_SET_ACTIVITY, (float)ACT_CROUCHIDLE },	// FIXME: This looks lame
	{ TASK_WAIT_RANDOM, (float)10.0 },
};

Schedule_t slRScientistHide[] =
{
	{
		tlRScientistHide,
		ARRAYSIZE(tlRScientistHide),
	bits_COND_NEW_ENEMY |
	bits_COND_HEAR_SOUND |
	bits_COND_SEE_ENEMY |
	bits_COND_SEE_HATE |
	bits_COND_SEE_FEAR |
	bits_COND_SEE_DISLIKE,
	bits_SOUND_DANGER,
	"ScientistHide"
	},
};

Task_t tlRScientistStartle[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_PANIC },		// If you fail, just panic!
	{ TASK_RANDOM_SCREAM, (float)0.3 },				// Scream 30% of the time
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY, (float)ACT_CROUCH },
	{ TASK_RANDOM_SCREAM, (float)0.1 },				// Scream again 10% of the time
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY, (float)ACT_CROUCHIDLE },
	{ TASK_WAIT_RANDOM, (float)1.0 },
};

Schedule_t slRScientistStartle[] =
{
	{
		tlRScientistStartle,
		ARRAYSIZE(tlRScientistStartle),
	bits_COND_NEW_ENEMY |
	bits_COND_SEE_ENEMY |
	bits_COND_SEE_HATE |
	bits_COND_SEE_FEAR |
	bits_COND_SEE_DISLIKE,
	0,
	"ScientistStartle"
	},
};

Task_t tlRFear[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_SAY_FEAR, (float)0 },
	//{ TASK_PLAY_SEQUENCE, (float)ACT_FEAR_DISPLAY },
};

Schedule_t slRFear[] =
{
	{
		tlRFear,
		ARRAYSIZE(tlRFear),
	bits_COND_NEW_ENEMY,
	0,
	"Fear"
	},
};

DEFINE_CUSTOM_SCHEDULES(CRosenberg)
{
	slRFollow,
		slRFaceTarget,
		slRIdleSciStand,
		slRFear,
		slRScientistCover,
		slRScientistHide,
		slRScientistStartle,
		slRHeal,
		slRStopFollowing,
		slRSciPanic,
		slRFollowScared,
		slRFaceTargetScared,
};

IMPLEMENT_CUSTOM_SCHEDULES(CRosenberg, CTalkMonster)

void CRosenberg::DeclineFollowing(void)
{
	Talk(10);
	m_hTalkTarget = m_hEnemy;
	//if (FClassnameIs(pev, "monster_rosenberg"))
		PlaySentence("RO_POK", 2, VOL_NORM, ATTN_NORM);
	//else	PlaySentence("SC_POK", 2, VOL_NORM, ATTN_NORM);
}

void CRosenberg::Scream(void)
{
	if (FOkToSpeak())
	{
		Talk(10);
		m_hTalkTarget = m_hEnemy;
		//if (FClassnameIs(pev, "monster_rosenberg"))
			PlaySentence("RO_SCREAM", RANDOM_FLOAT(3, 6), VOL_NORM, ATTN_NORM);
		//else	PlaySentence("SC_SCREAM", RANDOM_FLOAT(3, 6), VOL_NORM, ATTN_NORM);
	}
}

Activity CRosenberg::GetStoppedActivity(void)
{
	if (m_hEnemy != NULL)
		return ACT_EXCITED;
	return CTalkMonster::GetStoppedActivity();
}

void CRosenberg::StartTask(Task_t *pTask)
{
	switch (pTask->iTask)
	{
	case TASK_SAY_HEAL:
		//if( FOkToSpeak() )
		Talk(2);
		m_hTalkTarget = m_hTargetEnt;
		//if (FClassnameIs(pev, "monster_rosenberg"))
			PlaySentence("RO_HEAL", 2, VOL_NORM, ATTN_IDLE);
		//else	PlaySentence("SC_HEAL", 2, VOL_NORM, ATTN_IDLE);
		TaskComplete();
		break;
	case TASK_SCREAM:
		Scream();
		TaskComplete();
		break;
	case TASK_RANDOM_SCREAM:
		if (RANDOM_FLOAT(0, 1) < pTask->flData)
			Scream();
		TaskComplete();
		break;
	case TASK_SAY_FEAR:
		if (FOkToSpeak())
		{
			Talk(2);
			m_hTalkTarget = m_hEnemy;
			//if (FClassnameIs(pev, "monster_rosenberg"))
			//{
				PlaySentence("RO_FEAR", 5, VOL_NORM, ATTN_NORM);
			//}
			//else
			//{
			//	if (m_hEnemy->IsPlayer())
			//		PlaySentence("SC_PLFEAR", 5, VOL_NORM, ATTN_NORM);
			//	else	PlaySentence("SC_FEAR", 5, VOL_NORM, ATTN_NORM);
			//}
		}
		TaskComplete();
		break;
	case TASK_HEAL:
		m_IdealActivity = ACT_MELEE_ATTACK1;
		break;
	case TASK_RUN_PATH_SCARED:
		m_movementActivity = ACT_RUN_SCARED;
		break;
	case TASK_MOVE_TO_TARGET_RANGE_SCARED:
	{
		if ((m_hTargetEnt->pev->origin - pev->origin).Length() < 1)
			TaskComplete();
		else
		{
			m_vecMoveGoal = m_hTargetEnt->pev->origin;
			if (!MoveToTarget(ACT_WALK_SCARED, 0.5))
				TaskFail();
		}
	}
	break;
	default:
		CTalkMonster::StartTask(pTask);
		break;
	}
}

void CRosenberg::RunTask(Task_t *pTask)
{
	switch (pTask->iTask)
	{
	case TASK_RUN_PATH_SCARED:
		if (MovementIsComplete())
			TaskComplete();
		if (RANDOM_LONG(0, 31) < 8)
			Scream();
		break;
	case TASK_MOVE_TO_TARGET_RANGE_SCARED:
	{
		if (RANDOM_LONG(0, 63) < 8)
			Scream();

		if (m_hEnemy == NULL)
		{
			TaskFail();
		}
		else
		{
			float distance;

			distance = (m_vecMoveGoal - pev->origin).Length2D();
			// Re-evaluate when you think your finished, or the target has moved too far
			if ((distance < pTask->flData) || (m_vecMoveGoal - m_hTargetEnt->pev->origin).Length() > pTask->flData * 0.5)
			{
				m_vecMoveGoal = m_hTargetEnt->pev->origin;
				distance = (m_vecMoveGoal - pev->origin).Length2D();
				FRefreshRoute();
			}

			// Set the appropriate activity based on an overlapping range
			// overlap the range to prevent oscillation
			if (distance < pTask->flData)
			{
				TaskComplete();
				RouteClear();		// Stop moving
			}
			else if (distance < 190 && m_movementActivity != ACT_WALK_SCARED)
				m_movementActivity = ACT_WALK_SCARED;
			else if (distance >= 270 && m_movementActivity != ACT_RUN_SCARED)
				m_movementActivity = ACT_RUN_SCARED;
		}
	}
	break;
	case TASK_HEAL:
		if (m_fSequenceFinished)
		{
			TaskComplete();
		}
		else
		{
			if (TargetDistance() > 90)
				TaskComplete();
			pev->ideal_yaw = UTIL_VecToYaw(m_hTargetEnt->pev->origin - pev->origin);
			ChangeYaw(pev->yaw_speed);
		}
		break;
	default:
		CTalkMonster::RunTask(pTask);
		break;
	}
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int CRosenberg::Classify(void)
{
	return CLASS_HUMAN_PASSIVE;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CRosenberg::SetYawSpeed(void)
{
	int ys;

	ys = 90;

	switch (m_Activity)
	{
	case ACT_IDLE:
		ys = 120;
		break;
	case ACT_WALK:
		ys = 180;
		break;
	case ACT_RUN:
		ys = 150;
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 120;
		break;
	default:
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CRosenberg::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case SCIENTIST_AE_HEAL:		// Heal my target (if within range)
		Heal();
		break;
	case SCIENTIST_AE_NEEDLEON:
	{
		int oldBody = pev->body;
		pev->body = (oldBody % NUM_SCIENTIST_HEADS) + NUM_SCIENTIST_HEADS * 1;
	}
	break;
	case SCIENTIST_AE_NEEDLEOFF:
	{
		int oldBody = pev->body;
		pev->body = (oldBody % NUM_SCIENTIST_HEADS) + NUM_SCIENTIST_HEADS * 0;
	}
	break;
	default:
		CTalkMonster::HandleAnimEvent(pEvent);
	}
}

//=========================================================
// Spawn
//=========================================================
void CRosenberg::Spawn(void)
{
	Precache();

	SET_MODEL(ENT(pev), "models/scientist_rosenberg.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	//if (FClassnameIs(pev, "monster_rosenberg"))
		pev->health = gSkillData.scientistHealth * 2;
	//else	pev->health = gSkillData.scientistHealth;
	pev->view_ofs = Vector(0, 0, 50);// position of the eyes relative to monster's origin.
	m_flFieldOfView = VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so scientists will notice player and say hello
	m_MonsterState = MONSTERSTATE_NONE;

	//m_flDistTooFar = 256.0;

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_OPEN_DOORS | bits_CAP_AUTO_DOORS | bits_CAP_USE;

	// White hands
	pev->skin = 0;

	if (pev->body == -1)
	{
		// -1 chooses a random head
		pev->body = RANDOM_LONG(0, NUM_SCIENTIST_HEADS - 1);// pick a head, any head
	}

	// Luther is black, make his hands black
	if (pev->body == HEAD_LUTHER)
		pev->skin = 1;

	MonsterInit();
	SetUse(&CTalkMonster::FollowerUse);
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CRosenberg::Precache(void)
{
	PRECACHE_MODEL("models/scientist_rosenberg.mdl");
	//PRECACHE_SOUND("scientist/sci_pain1.wav");
	//PRECACHE_SOUND("scientist/sci_pain2.wav");
	//PRECACHE_SOUND("scientist/sci_pain3.wav");
	//PRECACHE_SOUND("scientist/sci_pain4.wav");
	//PRECACHE_SOUND("scientist/sci_pain5.wav");
	PRECACHE_SOUND("rosenberg/ro_pain1.wav");

	// every new scientist must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();

	CTalkMonster::Precache();
}

// Init talk data
void CRosenberg::TalkInit()
{
	CTalkMonster::TalkInit();

	// scientist will try to talk to friends in this order:

	m_szFriends[0] = "monster_scientist";
	m_szFriends[1] = "monster_sitting_scientist";
	m_szFriends[2] = "monster_barney";

	// scientists speach group names (group names are in sentences.txt)
	//if (FClassnameIs(pev, "monster_rosenberg"))
	//{
		m_szGrp[TLK_ANSWER] = "RO_ANSWER";
		m_szGrp[TLK_QUESTION] = "RO_QUESTION";
		m_szGrp[TLK_IDLE] = "RO_IDLE";
		m_szGrp[TLK_STARE] = "RO_STARE";
		m_szGrp[TLK_USE] = "RO_OK";
		m_szGrp[TLK_UNUSE] = "RO_WAIT";
		m_szGrp[TLK_STOP] = "RO_STOP";
		m_szGrp[TLK_NOSHOOT] = "RO_SCARED";
		m_szGrp[TLK_HELLO] = "RO_HELLO";

		m_szGrp[TLK_PLHURT1] = "!RO_CUREA";
		m_szGrp[TLK_PLHURT2] = "!RO_CUREB";
		m_szGrp[TLK_PLHURT3] = "!RO_CUREC";

		m_szGrp[TLK_PHELLO] = "RO_PHELLO";
		m_szGrp[TLK_PIDLE] = "RO_PIDLE";
		m_szGrp[TLK_PQUESTION] = "RO_PQUEST";
		m_szGrp[TLK_SMELL] = "RO_SMELL";

		m_szGrp[TLK_WOUND] = "RO_WOUND";
		m_szGrp[TLK_MORTAL] = "RO_MORTAL";
	//}
	/*else
	{
		m_szGrp[TLK_ANSWER] = "SC_ANSWER";
		m_szGrp[TLK_QUESTION] = "SC_QUESTION";
		m_szGrp[TLK_IDLE] = "SC_IDLE";
		m_szGrp[TLK_STARE] = "SC_STARE";
		m_szGrp[TLK_USE] = "SC_OK";
		m_szGrp[TLK_UNUSE] = "SC_WAIT";
		m_szGrp[TLK_STOP] = "SC_STOP";
		m_szGrp[TLK_NOSHOOT] = "SC_SCARED";
		m_szGrp[TLK_HELLO] = "SC_HELLO";

		m_szGrp[TLK_PLHURT1] = "!SC_CUREA";
		m_szGrp[TLK_PLHURT2] = "!SC_CUREB";
		m_szGrp[TLK_PLHURT3] = "!SC_CUREC";

		m_szGrp[TLK_PHELLO] = "SC_PHELLO";
		m_szGrp[TLK_PIDLE] = "SC_PIDLE";
		m_szGrp[TLK_PQUESTION] = "SC_PQUEST";
		m_szGrp[TLK_SMELL] = "SC_SMELL";

		m_szGrp[TLK_WOUND] = "SC_WOUND";
		m_szGrp[TLK_MORTAL] = "SC_MORTAL";
	}*/

	// get voice for head
	switch (pev->body % 3)
	{
	default:
	case HEAD_GLASSES:
		m_voicePitch = 105;
		break;	//glasses
	case HEAD_EINSTEIN:
		m_voicePitch = 100;
		break;	//einstein
	case HEAD_LUTHER:
		m_voicePitch = 95;
		break;	//luther
	case HEAD_SLICK:
		m_voicePitch = 85;
		break;	//slick
	}
}

bool CRosenberg::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
{
	if (pevInflictor && pevInflictor->flags & FL_CLIENT)
	{
		//if (!FClassnameIs(pev, "monster_rosenberg"))
		//{
		//	Remember(bits_MEMORY_PROVOKED);
			StopFollowing(true);
		//}
	}

	// make sure friends talk about it if player hurts scientist...
	return CTalkMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}

//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. In the base class implementation,
// monsters care about all sounds, but no scents.
//=========================================================
int CRosenberg::ISoundMask(void)
{
	return bits_SOUND_WORLD |
		bits_SOUND_COMBAT |
		bits_SOUND_DANGER |
		bits_SOUND_PLAYER;
}

//=========================================================
// PainSound
//=========================================================
void CRosenberg::PainSound(void)
{
	if (gpGlobals->time < m_painTime)
		return;

	//if (FClassnameIs(pev, "monster_rosenberg"))
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "rosenberg/ro_pain1.wav", 1, ATTN_NORM, 0, GetVoicePitch());
	//else
	//	switch (RANDOM_LONG(0, 4))
	//	{
	//	case 0:
	//		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "scientist/sci_pain1.wav", 1, ATTN_NORM, 0, GetVoicePitch());
	//		break;
	//	case 1:
	//		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "scientist/sci_pain2.wav", 1, ATTN_NORM, 0, GetVoicePitch());
	//		break;
	//	case 2:
	//		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "scientist/sci_pain3.wav", 1, ATTN_NORM, 0, GetVoicePitch());
	//		break;
	//	case 3:
	//		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "scientist/sci_pain4.wav", 1, ATTN_NORM, 0, GetVoicePitch());
	//		break;
	//	case 4:
	//		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "scientist/sci_pain5.wav", 1, ATTN_NORM, 0, GetVoicePitch());
	//		break;
	//	}
}

//=========================================================
// DeathSound 
//=========================================================
void CRosenberg::DeathSound(void)
{
	PainSound();
}

void CRosenberg::Killed(entvars_t *pevAttacker, int iGib)
{
	SetUse(NULL);
	CTalkMonster::Killed(pevAttacker, iGib);
}

void CRosenberg::SetActivity(Activity newActivity)
{
	int iSequence;

	iSequence = LookupActivity(newActivity);

	// Set to the desired anim, or default anim if the desired is not present
	if (iSequence == ACTIVITY_NOT_AVAILABLE)
		newActivity = ACT_IDLE;
	CTalkMonster::SetActivity(newActivity);
}

Schedule_t *CRosenberg::GetScheduleOfType(int Type)
{
	Schedule_t *psched;

	switch (Type)
	{
		// Hook these to make a looping schedule
	case SCHED_TARGET_FACE:
		// call base class default so that scientist will talk
		// when 'used'
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
			return slRFaceTarget;	// override this for different target face behavior
		else
			return psched;
	case SCHED_TARGET_CHASE:
		return slRFollow;
	case SCHED_CANT_FOLLOW:
		return slRStopFollowing;
	case SCHED_PANIC:
		return slRSciPanic;
	case SCHED_TARGET_CHASE_SCARED:
		return slRFollowScared;
	case SCHED_TARGET_FACE_SCARED:
		return slRFaceTargetScared;
	case SCHED_IDLE_STAND:
		// call base class default so that scientist will talk
		// when standing during idle
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
			return slRIdleSciStand;
		else
			return psched;
	case SCHED_HIDE:
		return slRScientistHide;
	case SCHED_STARTLE:
		return slRScientistStartle;
	case SCHED_FEAR:
		return slRFear;
	}

	return CTalkMonster::GetScheduleOfType(Type);
}

Schedule_t *CRosenberg::GetSchedule(void)
{
	// so we don't keep calling through the EHANDLE stuff
	CBaseEntity *pEnemy = m_hEnemy;

	if (HasConditions(bits_COND_HEAR_SOUND))
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT(pSound != NULL);
		if (pSound && (pSound->m_iType & bits_SOUND_DANGER))
			return GetScheduleOfType(SCHED_TAKE_COVER_FROM_BEST_SOUND);
	}

	switch (m_MonsterState)
	{
	case MONSTERSTATE_ALERT:
	case MONSTERSTATE_IDLE:
		if (pEnemy)
		{
			if (HasConditions(bits_COND_SEE_ENEMY))
				m_fearTime = gpGlobals->time;
			else if (DisregardEnemy(pEnemy))		// After 15 seconds of being hidden, return to alert
			{
				m_hEnemy = NULL;
				pEnemy = NULL;
			}
		}

		if (HasConditions(bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE))
		{
			// flinch if hurt
			return GetScheduleOfType(SCHED_SMALL_FLINCH);
		}

		// Cower when you hear something scary
		if (HasConditions(bits_COND_HEAR_SOUND))
		{
			CSound *pSound;
			pSound = PBestSound();

			ASSERT(pSound != NULL);
			if (pSound)
			{
				if (pSound->m_iType & (bits_SOUND_DANGER | bits_SOUND_COMBAT))
				{
					if (gpGlobals->time - m_fearTime > 3)	// Only cower every 3 seconds or so
					{
						m_fearTime = gpGlobals->time;		// Update last fear
						return GetScheduleOfType(SCHED_STARTLE);	// This will just duck for a second
					}
				}
			}
		}

		// Behavior for following the player
		if (IsFollowing())
		{
			if (!m_hTargetEnt->IsAlive())
			{
				// UNDONE: Comment about the recently dead player here?
				StopFollowing(false);
				break;
			}

			int relationship = R_NO;

			// Nothing scary, just me and the player
			if (pEnemy != NULL)
				relationship = IRelationship(pEnemy);

			// UNDONE: Model fear properly, fix R_FR and add multiple levels of fear
			if (relationship != R_DL && relationship != R_HT)
			{
				// If I'm already close enough to my target
				if (TargetDistance() <= 128)
				{
					if (CanHeal())	// Heal opportunistically
						return slRHeal;
					if (HasConditions(bits_COND_CLIENT_PUSH))	// Player wants me to move
						return GetScheduleOfType(SCHED_MOVE_AWAY_FOLLOW);
				}
				return GetScheduleOfType(SCHED_TARGET_FACE);	// Just face and follow.
			}
			else //if (!FClassnameIs(pev, "monster_rosenberg")) // UNDONE: When afraid, scientist won't move out of your way.  Keep This?  If not, write move away scared
			{
				if (HasConditions(bits_COND_NEW_ENEMY)) // I just saw something new and scary, react
					return GetScheduleOfType(SCHED_FEAR);					// React to something scary
				return GetScheduleOfType(SCHED_TARGET_FACE_SCARED);	// face and follow, but I'm scared!
			}
		}

		if (HasConditions(bits_COND_CLIENT_PUSH))	// Player wants me to move
			return GetScheduleOfType(SCHED_MOVE_AWAY);

		// try to say something about smells
		TrySmellTalk();
		break;
	case MONSTERSTATE_COMBAT:
		if (HasConditions(bits_COND_NEW_ENEMY))
			return slRFear;					// Point and scream!
		if (HasConditions(bits_COND_SEE_ENEMY))
			return slRScientistCover;		// Take Cover

		if (HasConditions(bits_COND_HEAR_SOUND))
			return slTakeCoverFromBestSound;	// Cower and panic from the scary sound!

		return slRScientistCover;			// Run & Cower
		break;
	default:
		break;
	}

	return CTalkMonster::GetSchedule();
}

MONSTERSTATE CRosenberg::GetIdealState(void)
{
	switch (m_MonsterState)
	{
	case MONSTERSTATE_ALERT:
	case MONSTERSTATE_IDLE:
		if (HasConditions(bits_COND_NEW_ENEMY))
		{
			if (IsFollowing())
			{
				int relationship = IRelationship(m_hEnemy);
				if (relationship != R_FR || (relationship != R_HT && !HasConditions(bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE)))
				{
					// Don't go to combat if you're following the player
					m_IdealMonsterState = MONSTERSTATE_ALERT;
					return m_IdealMonsterState;
				}
				StopFollowing(true);
			}
		}
		else if (HasConditions(bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE))
		{
			// Stop following if you take damage
			if (IsFollowing())
				StopFollowing(true);
		}
		break;
	case MONSTERSTATE_COMBAT:
	{
		CBaseEntity *pEnemy = m_hEnemy;
		if (pEnemy != NULL)
		{
			if (DisregardEnemy(pEnemy))		// After 15 seconds of being hidden, return to alert
			{
				// Strip enemy when going to alert
				m_IdealMonsterState = MONSTERSTATE_ALERT;
				m_hEnemy = NULL;
				return m_IdealMonsterState;
			}

			// Follow if only scared a little
			if (m_hTargetEnt != NULL)
			{
				m_IdealMonsterState = MONSTERSTATE_ALERT;
				return m_IdealMonsterState;
			}

			if (HasConditions(bits_COND_SEE_ENEMY))
			{
				m_fearTime = gpGlobals->time;
				m_IdealMonsterState = MONSTERSTATE_COMBAT;
				return m_IdealMonsterState;
			}
		}
	}
	break;
	default:
		break;
	}

	return CTalkMonster::GetIdealState();
}

bool CRosenberg::CanHeal(void)
{
	if ((m_healTime > gpGlobals->time) || (m_hTargetEnt == NULL) || (m_hTargetEnt->pev->health > (m_hTargetEnt->pev->max_health * 0.5)))
		return false;

	return true;
}

void CRosenberg::Heal(void)
{
	if (!CanHeal())
		return;

	Vector target = m_hTargetEnt->pev->origin - pev->origin;
	if (target.Length() > 100)
		return;

	m_hTargetEnt->TakeHealth(gSkillData.scientistHeal, DMG_GENERIC);
	// Don't heal again for 1 minute
	m_healTime = gpGlobals->time + 60;
}

int CRosenberg::FriendNumber(int arrayNumber)
{
	static int array[3] = { 1, 2, 0 };
	if (arrayNumber < 3)
		return array[arrayNumber];
	return arrayNumber;
}
