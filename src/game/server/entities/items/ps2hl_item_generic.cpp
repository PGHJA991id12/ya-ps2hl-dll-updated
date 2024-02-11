// Header
#include "ps2hl_item_generic.h"

// Link entity
LINK_ENTITY_TO_CLASS(item_generic, CItemGeneric);


// Methods //

// Precache handler
void CItemGeneric::Precache(void)
{
	PRECACHE_MODEL((char *)STRING(pev->model));
}

// Spawn handler
void CItemGeneric::Spawn(void)
{
	// Precache model
	Precache();

	// Set the model
	SET_MODEL(ENT(pev), STRING(pev->model));

	// Check if sequence is loaded
	if (pev->sequence == -1)
	{
		// Failed to load sequence
		ALERT(at_console, "item_generic: cant load animation sequence ...");
		pev->sequence = 0;
	}

	// Prepare sequence
	pev->sequence = LookupSequence(STRING(m_iSequence));
	pev->frame = 0;
	m_fSequenceLoops = 1;
	ResetSequenceInfo();

	// BBox
	Vector Zero;
	Zero.x = Zero.y = Zero.z = 0;
	UTIL_SetSize(pev, Zero, Zero);
	pev->solid = SOLID_NOT;
	
	// PS2HLU Drop to floor flag, required by Decay
	if (FBitSet(pev->spawnflags, SF_ITEM_GENERIC_DROP_TO_FLOOR) && !FStrEq(STRING(pev->targetname), "satchel")) // PS2HLU satchel position fix for ht10focus
	{
		if( DROP_TO_FLOOR(ENT( pev ) ) == 0 )
		{
			ALERT(at_error, "Item %s fell out of level at %f,%f,%f\n", STRING( pev->classname ), pev->origin.x, pev->origin.y, pev->origin.z);
			//UTIL_Remove( this );
		}
	}

	// Set think delay
	pev->nextthink = gpGlobals->time + ITGN_DELAY_THINK;
}

// Parse keys
bool CItemGeneric::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "model"))
	{
		// Set model
		pev->model = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "body"))
	{
		// Set body
		pev->body = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "sequencename"))
	{
		// Set sequence
		m_iSequence = ALLOC_STRING(pkvd->szValue);
		return true;
	}

		return CBaseEntity::KeyValue(pkvd);
}

// Think handler
void CItemGeneric::Think(void)
{
	// Set delay
	pev->nextthink = gpGlobals->time + ITGN_DELAY_THINK;

	// Call animation handler
	StudioFrameAdvance(0);
}
