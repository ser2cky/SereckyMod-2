//====================================================================
//	lightworld.cpp
// 
//	Purpose: 
// 
//	Create fancy client-sided lighting effects. Lights from the server have spawn
//	flags that enable certain effects, like emitting either entity or dynamic lights,
//	flares, and sun flares.
// 
//	History:
// 
//	APR-19-26:
//	started. created defines, new_light_t struct, and clightworld class w/
//	all of the important functions ( Init, ResetLights, ManageLights, FixLights,
//	ManageLights, DrawLightInfo, GetLightPointer, Parse_LightInfo ).
// 
//====================================================================

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include "triangleapi.h"
#include "r_efx.h"
#include "cl_entity.h"
#include "r_studioint.h"
#include "lightworld.h"

//================================================================

extern engine_studio_api_t IEngineStudio;
CLightWorld gLightWorld;

//================================================================
//	Init
//	
//	Purpose: Intialize the lightworld stuff. i.e create cvars, 
//	hook messages, etc.
//================================================================

int __MsgFunc_LightInfo(const char* pszName, int iSize, void* pbuf)
{
	return gLightWorld.Parse_LightInfo(pszName, iSize, pbuf);
}

void CLightWorld::Init( void )
{
	m_pCvarLightInfo = CVAR_CREATE( "show_light_info", "0", FCVAR_ARCHIVE );
	HOOK_MESSAGE( LightInfo );
}

//================================================================
//	ResetLights
//	
//	Purpose: Resets everything relating to CLightWorld (on the client)
//================================================================

void CLightWorld::ResetLights( void )
{
	memset( m_Lights, 0, sizeof(m_Lights) );
	m_bAddedLights = false;
	m_iLights = 0;
}

//================================================================
//	GetLightPointer
//	
//	Purpose: Return a light pointer based the given ent. index. If the ent index
//	does not exist in the light list, find an element with an entindex of zero, and
//	return it.
//================================================================

new_light_t* CLightWorld::GetLightPointer( int iEntIndex )
{
	for ( int i = 0; i < MAX_FANCY_LIGHTS; i++ )
	{
		if ( m_Lights[i].entindex == iEntIndex )
		{
			memset( &m_Lights[i], 0, sizeof(m_Lights[i]) );
			return &m_Lights[i];
		}

		if ( m_Lights[i].entindex == 0 )
		{
			m_iLights++;
			memset( &m_Lights[i], 0, sizeof(m_Lights[i]) );
			return &m_Lights[i];
		}
	}
	gEngfuncs.Con_Printf( "GetLightPointer: Could not retrieve a light pointer.\n" );
	return NULL;
}

//================================================================
//	Parse_LightInfo
//	
//	Purpose: Read light info sent from server entities, so we can add them
//	to our light array.
//================================================================

int CLightWorld::Parse_LightInfo( const char* pszName, int iSize, void* pbuf )
{
	new_light_t* pLight;

	BEGIN_READ( pbuf, iSize );

	int iEntIndex = READ_BYTE();
	pLight = GetLightPointer( iEntIndex );

	if ( pLight )
	{
		pLight->entindex = iEntIndex;
		pLight->flags = READ_SHORT();
		pLight->org[0] = READ_COORD();
		pLight->org[1] = READ_COORD();
		pLight->org[2] = READ_COORD();
		pLight->color.r = READ_BYTE();
		pLight->color.g = READ_BYTE();
		pLight->color.b = READ_BYTE();
		pLight->radius = (int)READ_SHORT();
		gEngfuncs.Con_Printf( "Parse_LightInfo: succesfully added light with index %d.\n", iEntIndex );
	}

	return 1;
}

//================================================================
//	FixLights
//	
//	Purpose: Make sure that we've added ALL the special lights in the world,
//	and that all of our lights have correct properties.
//================================================================

void CLightWorld::FixLights( void )
{
	new_light_t* pLight;
	cl_entity_t* pEnt;
	
	qboolean in_list = false;
	int iEntIndex = 1;

	// iterate thru all entities in the server & check if they're in the lightlist
	// already. if they're not in the lightlist, add them in!
	
	while (1)
	{
		// hit max_edicts
		if ( !( pEnt = gEngfuncs.GetEntityByIndex( iEntIndex ) ) )
			break;

		if ( pEnt->curstate.effects & ( EF_EMIT_FLARE | EF_EMIT_ELIGHT | EF_EMIT_DLIGHT | EF_EMIT_SUNFLARE ) )
		{
			in_list = false;
			for ( int i = 0; i < m_iLights; i++ )
			{
				pLight = &m_Lights[i];
				
				if ( !( pEnt = gEngfuncs.GetEntityByIndex( pLight->entindex ) ) )
					continue;
					
				//pLight->color = pEnt->curstate.rendercolor;
				//pLight->radius = pEnt->curstate.renderamt;
				//pLight->flags = pEnt->curstate.effects;
				//VectorCopy( pEnt->origin, pLight->org );
					
				if ( pLight->entindex == iEntIndex )
					in_list = true;
			}
			
			if ( !in_list )
			{
				m_iLights++;
				m_Lights[m_iLights].entindex = iEntIndex;
				m_Lights[m_iLights].color = pEnt->curstate.rendercolor;
				m_Lights[m_iLights].radius = pEnt->curstate.renderamt;
				m_Lights[m_iLights].flags = pEnt->curstate.effects;
				VectorCopy( pEnt->origin, m_Lights[m_iLights].org );
			}
		}
		iEntIndex++;
	}
}

//================================================================
//	ManageLights
//	
//	Purpose: Function to manage lights (avoiding the drawing of lights
//	outside the player's pvs, and applying the appropiate F.X)
//================================================================

void CLightWorld::ManageLights( void )
{
	new_light_t* pLight;
	cl_entity_t* pPlayer;
	cl_entity_t* pEnt;

	pPlayer = gEngfuncs.GetLocalPlayer();

	// make sure that our lights don't have bogus properties.
	if ( !m_bAddedLights && gEngfuncs.GetClientTime() >= 1.5f )
	{
		FixLights();
		gEngfuncs.Con_Printf( "Lightworld: The current level has %d special light(s).\n", m_iLights );
		m_bAddedLights = true;
	}

	for ( int i = 0; i < m_iLights; i++ )
	{
		pLight = &m_Lights[i];

		pEnt = gEngfuncs.GetEntityByIndex( pLight->entindex );

		if ( !pEnt )
			continue;

		if ( !pPlayer )
			continue;

		// Destroy Elights & Dlights outside the player's P.V.S
		if ( pEnt->curstate.messagenum != pPlayer->curstate.messagenum )
		{
			if ( pLight->light != NULL )
			{
				pLight->light->decay = 25.0f;
				//pLight->has_light = false;
			}
			continue;
		}

		if ( ( pLight->flags & ( EF_EMIT_ELIGHT | EF_EMIT_DLIGHT ) ) )
		{
			if ( pLight->ltime < gEngfuncs.GetClientTime() )
			{
				if ( pLight->flags & EF_EMIT_DLIGHT )
					pLight->light = gEngfuncs.pEfxAPI->CL_AllocDlight( pLight->entindex );
				else
					pLight->light = gEngfuncs.pEfxAPI->CL_AllocElight( pLight->entindex);
			}

			if ( pLight->light != NULL )
			{
				VectorCopy ( pLight->org, pLight->light->origin );
				pLight->light->color = pLight->color;
				pLight->light->radius = pLight->radius;
				pLight->ltime = pLight->light->die = gEngfuncs.GetClientTime() + 1.0f;
			}
		}

		if ( pLight->flags & EF_EMIT_FLARE )
			DrawFlare( pLight );
	}
}

//================================================================
//	DrawFlare
//	
//	Purpose: Draw a lens flare with TriAPI that fades out if a solid
//  is blocking the distance between the light and player.
//================================================================

void CLightWorld::DrawFlare( new_light_t* pLight )
{
	// TODO
}

//================================================================
//	DrawLightInfo
//	
//	Purpose: Debug command that shows light information.
//================================================================

void CLightWorld::DrawLightInfo( new_light_t* pLight )
{
	cl_entity_t* pPlayer;
	cl_entity_t* pEnt;

	if ( !m_pCvarLightInfo->value )
		return;

	pPlayer = gEngfuncs.GetLocalPlayer();

	for (int i = 0; i < m_iLights; i++)
	{
		pLight = &m_Lights[i];

		pEnt = gEngfuncs.GetEntityByIndex( pLight->entindex );

		if ( !pEnt )
			continue;

		if ( !pPlayer )
			continue;

		// Destroy Elights & Dlights outside the player's P.V.S
		if ( pEnt->curstate.messagenum != pPlayer->curstate.messagenum )
			continue;

		vec3_t vecScreen;

		if ( !gEngfuncs.pTriAPI->WorldToScreen( pLight->org, vecScreen ) )
		{
			vecScreen[0] = XPROJECT( vecScreen[0] );
			vecScreen[1] = YPROJECT( vecScreen[1] );

			DrawConsoleString(vecScreen[0], vecScreen[1], VarArgs("light index: %d\n", i));
			DrawConsoleString(vecScreen[0], vecScreen[1] + 12, VarArgs("entity index: %d\n", pLight->entindex));
			DrawConsoleString(vecScreen[0], vecScreen[1] + 24, VarArgs("origin: %.2f %.2f %.2f\n", pLight->org[0], pLight->org[1], pLight->org[2]));
			DrawConsoleString(vecScreen[0], vecScreen[1] + 36, VarArgs("color: %d %d %d\n", pLight->color.r, pLight->color.g, pLight->color.b));
			DrawConsoleString(vecScreen[0], vecScreen[1] + 48, VarArgs("radius: %.2f\n", pLight->radius));

			//gEngfuncs.Con_Printf("%.2f %.2f\n", vecScreen[0], vecScreen[1] );
		}
	}
}