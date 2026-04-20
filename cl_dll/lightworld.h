#if !defined( LIGHTWORLDH )
#define LIGHTWORLDH
#ifdef _WIN32
#pragma once
#endif
//================================================================

#define MAX_FANCY_LIGHTS	500
#define FLARE_CHECK_FREQ	0.5

#include "dlight.h"

//================================================================
//	NewLight struct.
//================================================================

typedef struct new_light_s
{
	struct model_s*		model;		// Pointer to newlight sprite. Used for TriAPI only.
	int			entindex;		// Entity newlight is tied to.

	vec3_t		org;
	dlight_t*	light;		// Pointer to newlight's dlight/elight.
	color24		color;		// Color that newlight's dlight/elight uses.
	float		radius;		// How big either the flare, or dlight/elight is going to be.
	int			flags;		// Flags to tell LightWorld what to do.
	float		alpha;		// sprite transparency for flares
	float		ltime;		// lifetime for the dlight/elight

	qboolean	has_light;
} new_light_t;

//================================================================
//	CLightWorld Class def.
//================================================================

class CLightWorld
{
public:
	//=====================================================
	// Public internals
	//=====================================================

	void	Init ( void );
	void	ResetLights ( void );
	void	FixLights ( void );
	void	ManageLights ( void );

	//=====================================================
	// Tri-API drawing functions
	//=====================================================

	void	DrawFlare ( new_light_t* pLight );
	void	DrawLightInfo ( new_light_t* pLight );

	//=====================================================
	// Light-fixing shenanagins.
	//=====================================================

	new_light_t*	GetLightPointer( int iEntIndex );
	int				Parse_LightInfo( const char *pszName,  int iSize, void *pbuf );
private:
	//=====================================================
	// Private internals
	//=====================================================

	new_light_t		m_Lights[MAX_FANCY_LIGHTS];
	cvar_t*			m_pCvarLightInfo;
	int				m_iLights;

	float			m_flNextFlareCheck;
	qboolean		m_bAddedLights;
};

extern CLightWorld gLightWorld;

#endif