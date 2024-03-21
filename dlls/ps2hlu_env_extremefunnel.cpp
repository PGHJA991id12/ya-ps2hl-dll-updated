#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "UserMessages.h"

class CExtremeFunnel : public CBaseDelay
{
	void Spawn() override;
	void Precache() override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	void EXPORT FirstFunnelThink();
	void EXPORT SecondFunnelThink();
	void EXPORT EndFunnelThink();

	int m_iSprite; // Don't save, precache
	int m_iFloater;
	int m_iUses = 1;
};

LINK_ENTITY_TO_CLASS(env_extremefunnel, CExtremeFunnel);

void CExtremeFunnel::Spawn()
{
	Precache();
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;
}

void CExtremeFunnel::Precache()
{
	m_iSprite = PRECACHE_MODEL("sprites/flare6.spr");
	m_iFloater = PRECACHE_MODEL("sprites/dot.spr");
}

void CExtremeFunnel::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	switch (m_iUses)
	{
	case 1:
		SetThink(&CExtremeFunnel::FirstFunnelThink);
		break;
	case 2:
		SetThink(&CExtremeFunnel::SecondFunnelThink);
		break;
	case 3:
		SetThink(&CExtremeFunnel::EndFunnelThink);
		break;
	default:
		break;
	}

	m_iUses++;
	
	if (m_iUses <= 3)
	pev->nextthink = gpGlobals->time + 0.1f;
	else
	pev->nextthink = gpGlobals->time + 1.0f;
}

void CExtremeFunnel::FirstFunnelThink()
{
	if (gmsgExtremeFunnel)
	{
		MESSAGE_BEGIN(MSG_PVS, gmsgExtremeFunnel, pev->origin);
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z);
		WRITE_COORD(0);
		WRITE_COORD(0);
		WRITE_COORD(0);
		WRITE_SHORT(m_iSprite);
		WRITE_SHORT(0);
		WRITE_SHORT(m_iFloater);
		MESSAGE_END();
	}

	SetThink(&CExtremeFunnel::FirstFunnelThink);
	pev->nextthink = gpGlobals->time + 1.2f;
}

void CExtremeFunnel::SecondFunnelThink()
{
	// There seems to be a countdown in the
	// nextthink in the ps2 version, which after
	// it ends speeds up this function
	// TODO: Implement that
	for (int i =0; i < 4; i++)
	{
		if (gmsgExtremeFunnel)
		{
			MESSAGE_BEGIN(MSG_PVS, gmsgExtremeFunnel, pev->origin);
			WRITE_COORD(pev->origin.x);
			WRITE_COORD(pev->origin.y);
			WRITE_COORD(pev->origin.z);
			WRITE_COORD(0);
			WRITE_COORD(0);
			WRITE_COORD(0);
			WRITE_SHORT(m_iSprite);
			WRITE_SHORT(0);
			WRITE_SHORT(m_iFloater);
			MESSAGE_END();
		}
	}

	SetThink(&CExtremeFunnel::SecondFunnelThink);
	pev->nextthink = gpGlobals->time + 0.6f;
}

void CExtremeFunnel::EndFunnelThink()
{
	if (gmsgExtremeFunnel)
	{
		MESSAGE_BEGIN(MSG_PVS, gmsgExtremeFunnel, pev->origin);
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z);
		WRITE_COORD(0);
		WRITE_COORD(0);
		WRITE_COORD(0);
		WRITE_SHORT(m_iSprite);
		WRITE_SHORT(1);
		WRITE_SHORT(m_iFloater);
		MESSAGE_END();
	}

	SetThink(0);
	UTIL_Remove(this);
	pev->nextthink = gpGlobals->time + 0.1;
}