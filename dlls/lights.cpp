/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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

===== lights.cpp ========================================================

  spawn and think functions for editor-placed lights

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "usermsg.h"

//================================================================
// 
//	CLight
// 
//================================================================

class CLight : public CPointEntity
{
public:
	virtual void	KeyValue( KeyValueData* pkvd ); 
	virtual void	Spawn( void );
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	
	static	TYPEDESCRIPTION m_SaveData[];
	void	SendClientInfo( CBaseEntity* pEntity = NULL );
private:
	int		m_iStyle;
	int		m_iszPattern;
	int		m_iClientFlags;
};
LINK_ENTITY_TO_CLASS( light, CLight );

//================================================================

TYPEDESCRIPTION	CLight::m_SaveData[] = 
{
	DEFINE_FIELD( CLight, m_iStyle, FIELD_INTEGER ),
	DEFINE_FIELD( CLight, m_iszPattern, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CLight, CPointEntity );

//================================================================

//================================================================
//	KeyValue
// 
//	Cache user-entity-field values until spawn is called.
//================================================================

void CLight :: KeyValue( KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "style"))
	{
		m_iStyle = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "pitch"))
	{
		pev->angles.x = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "pattern"))
	{
		m_iszPattern = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "_light"))
	{
		int r, g, b, v, j;
		j = sscanf( pkvd->szValue, "%d %d %d %d\n", &r, &g, &b, &v );

		if (j == 1)
		{
			g = b = r;
		}

		// simulate qrad direct, ambient,and gamma adjustments, as well as engine scaling
		pev->rendercolor[0] = r;
		pev->rendercolor[1] = g;
		pev->rendercolor[2] = b;
		pev->renderamt = v;
	}
	else
	{
		CPointEntity::KeyValue( pkvd );
	}
}

//================================================================
//	Spawn
// 
//	QUAKED light (0 1 0) (-8 -8 -8) (8 8 8) LIGHT_START_OFF
//	Non-displayed light.
//	Default light value is 300
//	Default style is 0
//	If targeted, it will toggle between on or off.
//
//================================================================

void CLight :: Spawn( void )
{
	//if (FStringNull(pev->targetname))
	//{       // inert light
	//	REMOVE_ENTITY(ENT(pev));
	//	return;
	//}
	
	PRECACHE_MODEL( "sprites/null.spr" );
	SET_MODEL( ENT(pev), "sprites/null.spr" );
	pev->effects = EF_NODRAW;

	if (m_iStyle >= 32)
	{
		if (FBitSet(pev->spawnflags, SF_LIGHT_START_OFF))
			LIGHT_STYLE(m_iStyle, "a");
		else if (m_iszPattern)
			LIGHT_STYLE(m_iStyle, (char *)STRING( m_iszPattern ));
		else
			LIGHT_STYLE(m_iStyle, "m");
	}

	// SERECKY APR-19-26: update client flags to send to the server.
	if ( pev->spawnflags & ( SF_LIGHT_DRAW_FLARE | SF_LIGHT_EMIT_ELIGHT | SF_LIGHT_EMIT_DLIGHT | SF_LIGHT_EMIT_SUNFLARE ) )
	{
		pev->effects |= (pev->spawnflags & ~SF_LIGHT_START_OFF) << 7;
	}
}

//================================================================
//	Use
//================================================================

void CLight :: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (m_iStyle >= 32)
	{
		if ( !ShouldToggle( useType, !FBitSet(pev->spawnflags, SF_LIGHT_START_OFF) ) )
			return;

		if (FBitSet(pev->spawnflags, SF_LIGHT_START_OFF))
		{
			if (m_iszPattern)
				LIGHT_STYLE(m_iStyle, (char *)STRING( m_iszPattern ));
			else
				LIGHT_STYLE(m_iStyle, "m");
			ClearBits(pev->spawnflags, SF_LIGHT_START_OFF);
		}
		else
		{
			LIGHT_STYLE(m_iStyle, "a");
			SetBits(pev->spawnflags, SF_LIGHT_START_OFF);
		}

		if ( pev->effects & ( EF_EMIT_FLARE | EF_EMIT_ELIGHT | EF_EMIT_DLIGHT | EF_EMIT_SUNFLARE ) )
		{
			SendClientInfo( NULL );
		}
	}
}

//================================================================
//	SendClientInfo
//	
//	Purpose: Send light flags & ent-index to the client. Was inspired
//	by how Overfloater did this in his Half-Life Retail mod.
//================================================================

void CLight::SendClientInfo( CBaseEntity* pEntity )
{
	if ( !( pev->effects & ( EF_EMIT_FLARE | EF_EMIT_ELIGHT | EF_EMIT_DLIGHT | EF_EMIT_SUNFLARE ) ) )
	{
		return;
	}

	if ( pEntity != NULL )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgLight, NULL, pEntity->pev );
	}
	else
	{
		MESSAGE_BEGIN( MSG_ALL, gmsgLight, NULL );
	}

	WRITE_BYTE( ENTINDEX(this->edict()) );
	WRITE_SHORT( pev->effects );
	WRITE_COORD( pev->origin[0] );
	WRITE_COORD( pev->origin[1] );
	WRITE_COORD( pev->origin[2] );
	WRITE_BYTE( (int)pev->rendercolor[0] );
	WRITE_BYTE( (int)pev->rendercolor[1] );
	WRITE_BYTE( (int)pev->rendercolor[2] );
	WRITE_SHORT( (int)pev->renderamt );
	MESSAGE_END();
}

//
// shut up spawn functions for new spotlights
//
LINK_ENTITY_TO_CLASS( light_spot, CLight );
LINK_ENTITY_TO_CLASS( dir_light, CLight );

//================================================================
// 
//	CEnvLight
// 
//================================================================

class CEnvLight : public CLight
{
public:
	void	KeyValue( KeyValueData* pkvd ); 
	void	Spawn( void );
};

LINK_ENTITY_TO_CLASS( light_environment, CEnvLight );

//================================================================
//	KeyValue
//================================================================

void CEnvLight::KeyValue( KeyValueData* pkvd )
{
	if (FStrEq(pkvd->szKeyName, "_light"))
	{
		int r, g, b, v, j;
		char szColor[64];
		j = sscanf( pkvd->szValue, "%d %d %d %d\n", &r, &g, &b, &v );
		if (j == 1)
		{
			g = b = r;
		}
		else if (j == 4)
		{
			r = r * (v / 255.0);
			g = g * (v / 255.0);
			b = b * (v / 255.0);
		}

		// simulate qrad direct, ambient,and gamma adjustments, as well as engine scaling
		r = pow( r / 114.0, 0.6 ) * 264;
		g = pow( g / 114.0, 0.6 ) * 264;
		b = pow( b / 114.0, 0.6 ) * 264;

		pkvd->fHandled = TRUE;
		sprintf( szColor, "%d", r );
		CVAR_SET_STRING( "sv_skycolor_r", szColor );
		sprintf( szColor, "%d", g );
		CVAR_SET_STRING( "sv_skycolor_g", szColor );
		sprintf( szColor, "%d", b );
		CVAR_SET_STRING( "sv_skycolor_b", szColor );
	}
	else
	{
		CLight::KeyValue( pkvd );
	}
}

//================================================================
//	Spawn
//================================================================

void CEnvLight :: Spawn( void )
{
	char szVector[64];
	UTIL_MakeAimVectors( pev->angles );

	sprintf( szVector, "%f", gpGlobals->v_forward.x );
	CVAR_SET_STRING( "sv_skyvec_x", szVector );
	sprintf( szVector, "%f", gpGlobals->v_forward.y );
	CVAR_SET_STRING( "sv_skyvec_y", szVector );
	sprintf( szVector, "%f", gpGlobals->v_forward.z );
	CVAR_SET_STRING( "sv_skyvec_z", szVector );

	CLight::Spawn( );
}
