
//====================================================================
//	serecky_vehicle.cpp
// 
//	Purpose: Give modders a template for creating their own vehicles.
// 
//	TODO: 
//	add friction/drifting to the cars somehow
//	maybe find a way to have studiomodel car models + movetype_push collisions.
//
//	History:
//	APR-1-26: 
// 
//	STARTED
// 
//	APR-2-26: 
// 
//	friction
// 
//	APR-3-26:
// 
//	Changed the Car's movetype from MOVETYPE_PUSHSTEP to MOVETYPE_PUSH,
//	since the MOVETYPE_PUSH	can collide with entites at an angle. the only
//	con of this though is that we have to do collision detect with the world
//	ourself now. also split up the car's thinking into Main_Think, and Driver_Think.
// 
//====================================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "player.h"

//=========================================================
//	Accelerate
//=========================================================

static float Accelerate( float cur_speed, float max_speed, float time )
{
	return cur_speed + time * ( max_speed - cur_speed );
}

//=========================================================
//	FixYaw
//=========================================================

static float FixYaw( float yaw )
{
	if ( yaw > 360 )
	{
		yaw -= 360;
	}

	if ( yaw < -360 )
	{
		yaw += 360;
	}

	return yaw;
}

typedef struct
{
	int			m_iNumOfGears;
	float		m_flMultiplier[12];
} gear_info_t;

//=========================================================
//	CVehicle
//=========================================================

class CVehicle : public CBaseEntity
{
public:
	void		Spawn( void );
	int			ObjectCaps ( void ) { return (CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | FCAP_IMPULSE_USE; }

	//=====================================================
	//	Car logic / internals.
	//=====================================================

	void		EXPORT Main_Think( void );
	void		Driver_Think( void );
	void		Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void		Blocked( CBaseEntity *pOther );
	//BOOL		CheckMove ( Vector vecMove );

	float		m_flFwdSpeed;
	float		m_flSteerYaw;

	EHANDLE		m_Driver;
	BOOL		m_bBlocked;

	//=====================================================
	//	Car constants.
	//=====================================================

	float		m_flMaxFwdSpeed;
	float		m_flAccelSpeed;
	float		m_flMaxSteer;
	float		m_flSteerSpeed;
private:
};

LINK_ENTITY_TO_CLASS( m44, CVehicle );

//=========================================================
//	Spawn
//=========================================================

void CVehicle::Spawn( void )
{
	pev->movetype	= MOVETYPE_PUSH;
	pev->solid		= SOLID_BBOX;
	pev->health		= 100.0f;
	pev->friction	= 0.25f;

	m_flMaxFwdSpeed	= 1200.0f;

	SET_MODEL( ENT(pev), STRING(pev->model) );
	UTIL_SetOrigin( pev, pev->origin );

	SetThink( &CVehicle::Main_Think );
	pev->nextthink	= pev->ltime + 0.1f;
}

//=========================================================
//	Main_Think
// 
//	This is the main thinking function of the Car. If we've got a driver, we 
//	jump to Driver_Think. If not, the code will gradually zero out the car's
//	forward and steering yaw, if it's non-zero.
//=========================================================

void CVehicle::Main_Think( void )
{
	// Goto Driver_Think if we've got a driver.
	if ( m_Driver != NULL )
	{
		Driver_Think();
		pev->nextthink = pev->ltime + 0.01f;
		return;
	}

	// Zero out car controls If we've got some.
	if ( ( m_flFwdSpeed != 0.0f ) && ( m_flSteerYaw != 0.0f ) )
	{
		m_flFwdSpeed = Accelerate( m_flFwdSpeed, 0.0f, gpGlobals->frametime * 1.5f );
		pev->velocity = gpGlobals->v_forward * m_flFwdSpeed;

		float flSteerFrac;

		flSteerFrac = m_flFwdSpeed / m_flMaxFwdSpeed;

		m_flSteerYaw = Accelerate( m_flSteerYaw, 0.0f, gpGlobals->frametime * 3.0f );
		pev->angles.y += m_flSteerYaw * flSteerFrac;

		pev->angles.y = FixYaw( pev->angles.y );
		m_flSteerYaw = FixYaw( m_flSteerYaw );

		pev->nextthink = pev->ltime + 0.01f;
		return;
	}

	// We can think less now.
	pev->nextthink = pev->ltime + 0.5f;
}

//=========================================================
//	Driver_Think
//	This function handles all driver inputs that affect forward speed,
//	and steering.
//=========================================================

void CVehicle::Driver_Think( void )
{
	// Return in-case m_Driver somehow becomes NULL.
	if ( m_Driver == NULL )
	{
		return;
	}

	// Clear driver if they're dead.
	if ( m_Driver->pev->deadflag != DEAD_NO )
	{
		m_Driver = NULL;
		return;
	}

	UTIL_MakeVectors( pev->angles );

	m_Driver->pev->origin = ( pev->absmin + pev->absmax ) * 0.5f;
	m_Driver->pev->origin.z += 48;
	m_Driver->pev->solid = SOLID_NOT;

	// Exiting car.
	if ( m_Driver->pev->button & IN_USE )
	{
		m_Driver->pev->solid = SOLID_SLIDEBOX;
		m_Driver->pev->origin.z += 48;
		m_Driver = NULL;
		pev->nextthink = pev->ltime + 0.5f;
		return;
	}

	// Going forward and backwards.
	if ( m_Driver->pev->button & IN_FORWARD )
	{
		m_flFwdSpeed = Accelerate( m_flFwdSpeed, m_flMaxFwdSpeed, gpGlobals->frametime * 1.5f );
	}
	else if ( m_Driver->pev->button & IN_BACK )
	{
		m_flFwdSpeed = Accelerate( m_flFwdSpeed, -m_flMaxFwdSpeed, gpGlobals->frametime * 1.5f );
	}
	else
	{
		m_flFwdSpeed = Accelerate( m_flFwdSpeed, 0.0f, gpGlobals->frametime * 1.5f );
	}

	pev->velocity = gpGlobals->v_forward * m_flFwdSpeed;

	// Steering
	float flSteerFrac;

	flSteerFrac = m_flFwdSpeed / m_flMaxFwdSpeed;

	if ( m_Driver->pev->button & ( IN_LEFT | IN_MOVELEFT ) )
	{
		m_flSteerYaw = Accelerate( m_flSteerYaw, 2.0f, gpGlobals->frametime * 8.0f );
	}
	else if ( m_Driver->pev->button & ( IN_RIGHT | IN_MOVERIGHT ) )
	{
		m_flSteerYaw = Accelerate( m_flSteerYaw, -2.0f, gpGlobals->frametime * 8.0f );
	}
	else
	{
		m_flSteerYaw = Accelerate( m_flSteerYaw, 0.0f, gpGlobals->frametime * 3.0f );
	}

	pev->angles.y += m_flSteerYaw * flSteerFrac;

	pev->angles.y = FixYaw( pev->angles.y );
	m_flSteerYaw = FixYaw( m_flSteerYaw );

	pev->nextthink = pev->ltime + 0.01f;
}

void CVehicle::Blocked( CBaseEntity* pOther )
{
	ALERT( at_console, "shit\n");
}

//=========================================================
//	Use
//=========================================================

void CVehicle::Use( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value )
{
	if ( pActivator->IsPlayer() && !m_Driver )
	{
		m_Driver = pActivator;
		pev->nextthink = pev->ltime + 0.1f;
		ALERT( at_console, "DRIVER NETNAME: %s\n", STRING(m_Driver->pev->netname) );
	}
}