#pragma once
#ifndef OSPREY_H
#define OSPREY_H

typedef struct
{
	int isValid;
	EHANDLE hGrunt;
	Vector vecOrigin;
	Vector vecAngles;
} t_ospreygrunt;


#define SF_WAITFORTRIGGER	0x40


#define MAX_CARRY	24

class COsprey : public CBaseMonster
{
public:
	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];
	int ObjectCaps() override { return CBaseMonster::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	void Spawn() override;
	void Precache() override;
	int Classify() override { return CLASS_MACHINE; }
	int BloodColor() override { return DONT_BLEED; }
	void Killed(entvars_t* pevAttacker, int iGib) override;

	void UpdateGoal();
	bool HasDead();
	void EXPORT FlyThink();
	void EXPORT DeployThink();
	void Flight();
	void EXPORT HitTouch(CBaseEntity* pOther);
	void EXPORT FindAllThink();
	void EXPORT HoverThink();
	CBaseMonster* MakeGrunt(Vector vecSrc);
	void EXPORT CrashTouch(CBaseEntity* pOther);
	void EXPORT DyingThink();
	void EXPORT CommandUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);

	bool TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType) override;
	void TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType) override;
	void ShowDamage();
	void Update();

	CBaseEntity* m_pGoalEnt;
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

	float m_flRightHealth;
	float m_flLeftHealth;

	int m_iUnits;
	EHANDLE m_hGrunt[MAX_CARRY];
	Vector m_vecOrigin[MAX_CARRY];
	EHANDLE m_hRepel[4];

	int m_iSoundState;
	int m_iSpriteTexture;

	int m_iPitch;

	int m_iExplode;
	int m_iTailGibs;
	int m_iBodyGibs;
	int m_iEngineGibs;

	int m_iDoLeftSmokePuff;
	int m_iDoRightSmokePuff;
};

#endif