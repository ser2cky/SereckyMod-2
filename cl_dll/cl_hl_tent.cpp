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

//====================================================================
//	cl_hl_tent.cpp
// 
//	Purpose: Give modders access to the code of most Half-Life Temp-
//	entities. Credits to Xash3D FWGS and the Half-Life NetTest1 decompile
//	for making this possible.
//
//	History:
//	FEB-22-26: Started
//	FEB-24-26: moved all temp-ents into a class so people can simply 
//	call them from the "gTempEnt" global, like how you'd call something
//	from the pEfxApi...
//	FEB-26-26: added more
//
//====================================================================

#include "hud.h"
#include "cl_util.h"
#include "com_model.h"
#include "event_api.h"
#include "r_efx.h"
#include "r_studioint.h"
#include "parsemsg.h"
#include "cl_hl_tent.h"
#include "entity_types.h"

#include "studio_util.h"
#include "studio.h"
#include "StudioModelRenderer.h"
#include "GameStudioModelRenderer.h"

extern engine_studio_api_t IEngineStudio;
extern CGameStudioModelRenderer g_StudioRenderer;

CTempEntities gTempEnt;

//=======================================
//	S_StartDynamicSound
//	Yeah I kinda got lazy with this one... How else would I play sounds
//	on the client anyways?
//=======================================

void S_StartDynamicSound( int entnum, int entchannel, const char *sample, vec_t* origin, float fvol, float attenuation, int flags, int pitch )
{
	gEngfuncs.pEventAPI->EV_PlaySound( entnum, (float*)origin, entchannel, sample, fvol, attenuation, flags, pitch );
}

//=======================================
//	ParseTEnt
//=======================================

int __MsgFunc_ParseTEnt(const char* pszName, int iSize, void* pbuf)
{
	return gTempEnt.MsgFunc_ParseTEnt( pszName, iSize, pbuf );
}

//=======================================
//	Init
//=======================================

void CTempEntities::Init( void )
{
	m_pCvarDecals = gEngfuncs.pfnGetCvarPointer( "r_decals" );
	m_pCvarTracerSpeed = gEngfuncs.pfnGetCvarPointer( "tracer_speed" );
	m_pCvarTracerOffset = gEngfuncs.pfnGetCvarPointer( "tracer_offset" );
	HOOK_MESSAGE( ParseTEnt );
}

//=======================================
//	VidInit
//=======================================

void CTempEntities::VidInit( void )
{
	cl_sprite_muzzleflash[0] = IEngineStudio.Mod_ForName("sprites/muzzleflash1.spr", 0);
	cl_sprite_muzzleflash[1] = IEngineStudio.Mod_ForName("sprites/muzzleflash2.spr", 0);
	cl_sprite_muzzleflash[2] = IEngineStudio.Mod_ForName("sprites/muzzleflash3.spr", 0);

	cl_sprite_ricochet = IEngineStudio.Mod_ForName("sprites/richo1.spr", 0);
	cl_sprite_shell = IEngineStudio.Mod_ForName("sprites/wallpuff.spr", 0);
}

//=======================================
//	Shutdown
//=======================================

void CTempEntities::Shutdown( void )
{
}

//=======================================
//	Function Name
//=======================================

void CTempEntities::R_TracerEffect( vec_t* start, vec_t* end )
{
	vec3_t	temp, vel;
	float	len;

	if ( m_pCvarTracerSpeed->value <= 0 )
		m_pCvarTracerSpeed->value = 3;

	VectorSubtract(end, start, temp);
	len = Length(temp);

	VectorScale(temp, 1.0 / len, temp);
	VectorScale(temp, gEngfuncs.pfnRandomLong(-10, 9) + m_pCvarTracerOffset->value, vel);
	VectorAdd(start, vel, start);
	VectorScale(temp, m_pCvarTracerSpeed->value, vel);

	gEngfuncs.pEfxAPI->R_TracerParticles( start, vel, len / m_pCvarTracerSpeed->value );
}

//=======================================
//	Function Name
//=======================================

void CTempEntities::R_Implosion( vec_t* end, float radius, int count, float life )
{
	int		i;
	vec3_t	start, temp;
	vec3_t	vel;

	for (i = 0; i < count; i++)
	{
		temp[0] = (radius / 100) * gEngfuncs.pfnRandomFloat(-100, 100);
		temp[1] = (radius / 100) * gEngfuncs.pfnRandomFloat(-100, 100);
		temp[2] = (radius / 100) * gEngfuncs.pfnRandomFloat(0, 100);

		VectorAdd(temp, end, start);
		VectorScale(temp, -1.0 / life, vel);

		gEngfuncs.pEfxAPI->R_TracerParticles(start, vel, life);
	}
}

//=======================================
//	Function Name
//=======================================

void CTempEntities::R_StreakSplash( vec_t* pos, vec_t* dir, int color, int count, float speed, int velocityMin, int velocityMax )
{
	int		i, j;
	particle_t* p;
	vec3_t	initialVelocity;

	VectorScale(dir, speed, initialVelocity);

	for (i = 0; i < count; i++)
	{
		if ( ( p = gEngfuncs.pEfxAPI->R_TracerParticles( vec3_origin, vec3_origin, 1.0f ) ) == NULL )
			return;

		p->color = color;
		p->packedColor = 255;
		p->type = pt_grav;
		p->die = gEngfuncs.GetClientTime() + gEngfuncs.pfnRandomFloat(0.1, 0.5);
		p->ramp = 1;

		for (j = 0; j < 3; j++)
		{
			p->org[j] = pos[j];
			p->vel[j] = initialVelocity[j] + gEngfuncs.pfnRandomFloat(velocityMin, velocityMax);
		}
	}
}

//=======================================
//	Function Name
//=======================================

void CTempEntities::R_ParticleExplosion( vec_t* org )
{
	int			i, j;
	particle_t* p;

	for (i = 0; i < 1024; i++)
	{
		if ((p = gEngfuncs.pEfxAPI->R_AllocParticle(NULL)) == NULL)
			return;
		
		p->die = gEngfuncs.GetClientTime() + 5.0;
		p->color = ramp1[0];
		p->ramp = gEngfuncs.pfnRandomLong(0, 3);
		gEngfuncs.pEfxAPI->R_GetPackedColor(&p->packedColor, p->color);

		if (i & 1)
		{
			p->type = pt_explode;

			while (1)
			{
				for (j = 0; j < 3; j++)
				{
					p->vel[j] = gEngfuncs.pfnRandomFloat(-512, 512);
				}

				if (DotProduct(p->vel, p->vel) <= 262144.0)
					break;
			}

			for (j = 0; j < 3; j++)
			{
				p->org[j] = org[j] + p->vel[j] * 0.25;
			}
		}
		else
		{
			p->type = pt_explode2;

			while (1)
			{
				for (j = 0; j < 3; j++)
				{
					p->vel[j] = gEngfuncs.pfnRandomFloat(-512, 512);
				}

				if (DotProduct(p->vel, p->vel) <= 262144.0)
					break;
			}

			for (j = 0; j < 3; j++)
			{
				p->org[j] = org[j] + p->vel[j] * 0.25;
			}
		}
	}
}

//=======================================
//	Function Name
//=======================================

void CTempEntities::R_ParticleExplosion2( vec_t* org, int colorStart, int colorLength )
{
	int			i, j;
	particle_t* p;
	int			colorMod = 0;

	for (i = 0; i < 512; i++)
	{
		if ((p = gEngfuncs.pEfxAPI->R_AllocParticle(NULL)) == NULL)
			return;
		
		p->die = gEngfuncs.GetClientTime() + 0.3;
		p->color = colorStart + (colorMod % colorLength);
		gEngfuncs.pEfxAPI->R_GetPackedColor(&p->packedColor, p->color);

		colorMod++;

		p->type = pt_blob;

		for (j = 0; j < 3; j++)
		{
			p->org[j] = org[j] + gEngfuncs.pfnRandomFloat(-16, 16);
			p->vel[j] = gEngfuncs.pfnRandomFloat(-256, 256);
		}
	}
}

//=======================================
//	Function Name
//=======================================

void CTempEntities::R_BlobExplosion( vec_t* org )
{
	int			i, j;
	particle_t* p;

	for (i = 0; i < 1024; i++)
	{
		if ((p = gEngfuncs.pEfxAPI->R_AllocParticle(NULL)) == NULL)
			return;

		p->die = gEngfuncs.GetClientTime() + gEngfuncs.pfnRandomFloat(1, 1.4);

		if (i & 1)
		{
			p->type = pt_blob;
			p->color = gEngfuncs.pfnRandomLong(66, 71);
			gEngfuncs.pEfxAPI->R_GetPackedColor(&p->packedColor, p->color);

			for (j = 0; j < 3; j++)
			{
				p->org[j] = org[j] + gEngfuncs.pfnRandomFloat(-16, 16);
				p->vel[j] = gEngfuncs.pfnRandomFloat(-256, 256);
			}
		}
		else
		{
			p->type = pt_blob2;
			p->color = gEngfuncs.pfnRandomLong(150, 155);
			gEngfuncs.pEfxAPI->R_GetPackedColor(&p->packedColor, p->color);

			for (j = 0; j < 3; j++)
			{
				p->org[j] = org[j] + gEngfuncs.pfnRandomFloat(-16, 16);
				p->vel[j] = gEngfuncs.pfnRandomFloat(-256, 256);
			}
		}
	}
}

//=======================================
//	R_RunParticleEffect
//=======================================

void CTempEntities::R_RunParticleEffect( vec_t* org, vec_t* dir, int color, int count )
{
	int		i, j;
	particle_t* p;

	for (i = 0; i < count; i++)
	{
		if ((p = gEngfuncs.pEfxAPI->R_AllocParticle(NULL)) == NULL)
			return;

		if (count == 1024)
		{
			// rocket explosion
			p->die = gEngfuncs.GetClientTime() + 5.0;
			p->color = ramp1[0];
			gEngfuncs.pEfxAPI->R_GetPackedColor(&p->packedColor, p->color);
			p->ramp = gEngfuncs.pfnRandomLong(0, 3);

			if (i & 1)
			{
				p->type = pt_explode;
				for (j = 0; j < 3; j++)
				{
					p->org[j] = org[j] + gEngfuncs.pfnRandomFloat(-16, 16);
					p->vel[j] = gEngfuncs.pfnRandomFloat(-256, 256);
				}
			}
			else
			{
				p->type = pt_explode2;
				for (j = 0; j < 3; j++)
				{
					p->org[j] = org[j] + gEngfuncs.pfnRandomFloat(-16, 16);
					p->vel[j] = gEngfuncs.pfnRandomFloat(-256, 256);
				}
			}
		}
		else
		{
			p->die = gEngfuncs.GetClientTime() + gEngfuncs.pfnRandomFloat(0, 0.4);
			p->color = (color & ~7) + gEngfuncs.pfnRandomLong(0, 7);
			gEngfuncs.pEfxAPI->R_GetPackedColor(&p->packedColor, p->color);
			p->type = pt_slowgrav;

			for (j = 0; j < 3; j++)
			{
				p->org[j] = org[j] + gEngfuncs.pfnRandomFloat(-8, 8);
				p->vel[j] = dir[j] * 15;
			}
		}
	}
}

//=======================================
//	Function Name
//=======================================

void CTempEntities::R_FlickerParticles( vec_t* org )
{
	int		i, j;
	particle_t* p;

	for (i = 0; i < 15; i++)
	{
		if ((p = gEngfuncs.pEfxAPI->R_AllocParticle(NULL)) == NULL)
			return;

		for (j = 0; j < 3; j++)
		{
			p->org[j] = org[j];
			p->vel[j] = gEngfuncs.pfnRandomFloat(-32, 32);
		}

		p->vel[2] = gEngfuncs.pfnRandomFloat(80, 143);

		p->color = 254;
		p->ramp = 0;
		gEngfuncs.pEfxAPI->R_GetPackedColor(&p->packedColor, p->color);
		p->die = gEngfuncs.GetClientTime() + 2.0;
		p->type = pt_blob2;
	}
}

//=======================================
//	Function Name
//=======================================

void CTempEntities::R_LavaSplash( vec_t* org )
{
	int			i, j, k;
	particle_t* p;
	float		vel;
	vec3_t		dir;

	for (i = -16; i < 16; i++)
	{
		for (j = -16; j < 16; j++)
		{
			for (k = 0; k < 1; k++)
			{
				if ((p = gEngfuncs.pEfxAPI->R_AllocParticle(NULL)) == NULL)
					return;

				p->die = gEngfuncs.GetClientTime() + gEngfuncs.pfnRandomFloat(2, 2.62);
				p->type = pt_slowgrav;
				p->color = gEngfuncs.pfnRandomLong(224, 231);
				gEngfuncs.pEfxAPI->R_GetPackedColor(&p->packedColor, p->color);

				dir[0] = j * 8 + gEngfuncs.pfnRandomFloat(0, 7);
				dir[1] = i * 8 + gEngfuncs.pfnRandomFloat(0, 7);
				dir[2] = 256;

				p->org[0] = org[0] + dir[0];
				p->org[1] = org[1] + dir[1];
				p->org[2] = org[2] + gEngfuncs.pfnRandomFloat(0, 63);

				VectorNormalize(dir);
				vel = gEngfuncs.pfnRandomFloat(50, 113);
				VectorScale(dir, vel, p->vel);
			}
		}
	}
}

//=======================================
//	Function Name
//=======================================

void CTempEntities::R_LargeFunnel( vec_t* org, int reverse )
{
	int			i, j;
	particle_t* p;
	float		vel;
	vec3_t		dir, dest;
	float		flDist;

	for (i = -256; i <= 256; i += 32)
	{
		for (j = -256; j <= 256; j += 32)
		{
			if ((p = gEngfuncs.pEfxAPI->R_AllocParticle(NULL)) == NULL)
				return;

			if (reverse)
			{
				VectorCopy(org, p->org);

				dest[0] = org[0] + i;
				dest[1] = org[1] + j;
				dest[2] = org[2] + gEngfuncs.pfnRandomFloat(100, 800);

				// send particle heading to dest at a random speed
				VectorSubtract(dest, p->org, dir);

				vel = dest[2] / 8.0;// velocity based on how far particle starts from org
			}
			else
			{
				p->org[0] = org[0] + i;
				p->org[1] = org[1] + j;
				p->org[2] = org[2] + gEngfuncs.pfnRandomFloat(100, 800);

				// send particle heading to dest at a random speed
				VectorSubtract(org, p->org, dir);

				vel = p->org[2] / 8.0;// velocity based on how far particle starts from org
			}

			p->type = pt_static;
			p->color = 244;
			gEngfuncs.pEfxAPI->R_GetPackedColor(&p->packedColor, p->color);

			flDist = VectorNormalize(dir);	// save the distance

			if (vel < 64)
			{
				vel = 64;
			}

			vel += gEngfuncs.pfnRandomFloat(64, 128);
			VectorScale(dir, vel, p->vel);

			// die right when you get there
			p->die = gEngfuncs.GetClientTime() + (flDist / vel);
		}
	}
}

//=======================================
//	Function Name
//=======================================

void CTempEntities::R_TeleportSplash( vec_t* org )
{
	int			i, j, k;
	particle_t* p;
	float		vel;
	vec3_t		dir;

	for (i = -16; i < 16; i += 4)
	{
		for (j = -16; j < 16; j += 4)
		{
			for (k = -24; k < 32; k += 4)
			{
				if ((p = gEngfuncs.pEfxAPI->R_AllocParticle(NULL)) == NULL)
					return;

				p->die = gEngfuncs.GetClientTime() + gEngfuncs.pfnRandomFloat(0.2, 0.34);
				p->color = gEngfuncs.pfnRandomLong(7, 14);
				p->type = pt_slowgrav;
				gEngfuncs.pEfxAPI->R_GetPackedColor(&p->packedColor, p->color);

				dir[0] = j * 8;
				dir[1] = i * 8;
				dir[2] = k * 8;

				p->org[0] = org[0] + i + gEngfuncs.pfnRandomFloat(0, 3);
				p->org[1] = org[1] + j + gEngfuncs.pfnRandomFloat(0, 3);
				p->org[2] = org[2] + k + gEngfuncs.pfnRandomFloat(0, 3);

				VectorNormalize(dir);
				vel = gEngfuncs.pfnRandomFloat(50, 113);
				VectorScale(dir, vel, p->vel);
			}
		}
	}
}

//=======================================
//	Function Name
//=======================================

void CTempEntities::R_ShowLine( vec_t* start, vec_t* end )
{
	vec3_t		vec;
	float		len;
	particle_t* p;
	int			dec = 5;

	static int tracercount;

	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);
	VectorScale(vec, dec, vec);

	while (len > 0)
	{
		len -= dec;

		if ((p = gEngfuncs.pEfxAPI->R_AllocParticle(NULL)) == NULL)
			return;

		VectorCopy(vec3_origin, p->vel);

		p->die = gEngfuncs.GetClientTime() + 30;
		p->type = pt_static;
		p->color = 75;
		gEngfuncs.pEfxAPI->R_GetPackedColor(&p->packedColor, p->color);

		VectorCopy(start, p->org);
		VectorAdd(start, vec, start);
	}
}

//=======================================
//	Function Name
//=======================================

void CTempEntities::R_BloodStream( vec_t* org, vec_t* dir, int pcolor, int speed )
{
	// Add our particles
	vec3_t	dirCopy;
	float	arc;
	int		count;
	int		count2;
	particle_t* p;
	float	num;
	int		speedCopy = speed;

	VectorNormalize(dir);

	arc = 0.05;
	for (count = 0; count < 100; count++)
	{
		if ((p = gEngfuncs.pEfxAPI->R_AllocParticle(NULL)) == NULL)
			return;

		p->die = gEngfuncs.GetClientTime() + 2.0;
		p->color = pcolor + gEngfuncs.pfnRandomLong(0, 9);
		gEngfuncs.pEfxAPI->R_GetPackedColor(&p->packedColor, p->color);

		p->type = pt_vox_grav;

		VectorCopy(org, p->org);
		VectorCopy(dir, dirCopy);
		dirCopy[2] -= arc;

		arc -= 0.005;

		VectorScale(dirCopy, speedCopy, p->vel);
		speedCopy -= 0.00001; // make last few drip
	}

	arc = 0.075;
	for (count = 0; count < (speed / 5); count++)
	{
		if ((p = gEngfuncs.pEfxAPI->R_AllocParticle(NULL)) == NULL)
			return;

		p->die = gEngfuncs.GetClientTime() + 3.0;
		p->type = pt_vox_slowgrav;
		p->color = pcolor + gEngfuncs.pfnRandomLong(0, 9);
		gEngfuncs.pEfxAPI->R_GetPackedColor(&p->packedColor, p->color);

		VectorCopy(org, p->org);
		VectorCopy(dir, dirCopy);
		dirCopy[2] -= arc;

		arc -= 0.005;

		num = gEngfuncs.pfnRandomFloat(0, 1);
		speedCopy = speed * num;
		num *= 1.7;

		VectorScale(dirCopy, num, dirCopy); // randomize a bit
		VectorScale(dirCopy, speedCopy, p->vel);

		for (count2 = 0; count2 < 2; count2++)
		{
			if ((p = gEngfuncs.pEfxAPI->R_AllocParticle(NULL)) == NULL)
				return;

			p->die = gEngfuncs.GetClientTime() + 3.0;
			p->type = pt_vox_slowgrav;
			p->color = pcolor + gEngfuncs.pfnRandomLong(0, 9);
			gEngfuncs.pEfxAPI->R_GetPackedColor(&p->packedColor, p->color);

			p->org[0] = org[0] + gEngfuncs.pfnRandomFloat(-1, 1);
			p->org[1] = org[1] + gEngfuncs.pfnRandomFloat(-1, 1);
			p->org[2] = org[2] + gEngfuncs.pfnRandomFloat(-1, 1);

			VectorCopy(dir, dirCopy);
			dirCopy[2] -= arc;

			VectorScale(dirCopy, num, dirCopy); // randomize a bit
			VectorScale(dirCopy, speedCopy, p->vel);
		}
	}
}

//=======================================
//	Function Name
//=======================================

void CTempEntities::R_Blood( vec_t* org, vec_t* dir, int pcolor, int speed )
{
	vec3_t	dirCopy;
	vec3_t	orgCopy;
	float	arc;
	int		count;
	int		count2;
	particle_t* p;
	int		pspeed;

	VectorNormalize(dir);

	pspeed = speed * 3;

	arc = 0.06;
	for (count = 0; count < (speed / 2); count++)
	{
		orgCopy[0] = org[0] + gEngfuncs.pfnRandomFloat(-3, 3);
		orgCopy[1] = org[1] + gEngfuncs.pfnRandomFloat(-3, 3);
		orgCopy[2] = org[2] + gEngfuncs.pfnRandomFloat(-3, 3);

		dirCopy[0] = dir[0] + gEngfuncs.pfnRandomFloat(-arc, arc);
		dirCopy[1] = dir[1] + gEngfuncs.pfnRandomFloat(-arc, arc);
		dirCopy[2] = dir[2] + gEngfuncs.pfnRandomFloat(-arc, arc);

		for (count2 = 0; count2 < 8; count2++)
		{
			if ((p = gEngfuncs.pEfxAPI->R_AllocParticle(NULL)) == NULL)
				return;

			p->die = gEngfuncs.GetClientTime() + 1.5;
			p->color = pcolor + gEngfuncs.pfnRandomLong(0, 9);
			p->type = pt_vox_grav;
			gEngfuncs.pEfxAPI->R_GetPackedColor(&p->packedColor, p->color);

			p->org[0] = orgCopy[0] + gEngfuncs.pfnRandomFloat(-1, 1);
			p->org[1] = orgCopy[1] + gEngfuncs.pfnRandomFloat(-1, 1);
			p->org[2] = orgCopy[2] + gEngfuncs.pfnRandomFloat(-1, 1);

			VectorScale(dirCopy, pspeed, p->vel);
		}

		pspeed -= speed;
	}
}

//=======================================
//	Function Name
//=======================================

void CTempEntities::R_RocketTrail( vec_t *start, vec_t *end, int type )
{
	vec3_t	vec, right, up;
	float	len;
	int		j;
	particle_t* p;
	int		dec;

	static int tracercount;

	VectorSubtract(end, start, vec);

	len = VectorNormalize(vec);

	if (type == 7)
	{
		dec = 1;
		//VectorMatrix(vec, right, up);
	}
	else if (type < 128)
	{
		dec = 3;
	}
	else
	{
		dec = 1;
		type -= 128;
	}

	VectorScale(vec, dec, vec);

	while (len > 0)
	{
		len -= dec;

		if ((p = gEngfuncs.pEfxAPI->R_AllocParticle(NULL)) == NULL)
			return;

		VectorCopy(vec3_origin, p->vel);

		p->die = gEngfuncs.GetClientTime() + 2.0;

		switch (type)
		{
		case 0: // rocket trail
			p->ramp = gEngfuncs.pfnRandomLong(0, 3);
			p->type = pt_fire;
			p->color = ramp3[(int)p->ramp];
			for (j = 0; j < 3; j++)
				p->org[j] = start[j] + gEngfuncs.pfnRandomFloat(-3, 3);
			break;

		case 1:	// smoke
			p->ramp = gEngfuncs.pfnRandomLong(2, 5);
			p->type = pt_fire;
			p->color = ramp3[(int)p->ramp];
			for (j = 0; j < 3; j++)
				p->org[j] = start[j] + gEngfuncs.pfnRandomFloat(-3, 3);
			break;
	
		case 2:	// blood
			p->type = pt_grav;
			p->color = gEngfuncs.pfnRandomLong(67, 70);
			for (j = 0; j < 3; j++)
				p->org[j] = start[j] + gEngfuncs.pfnRandomFloat(-3, 3);
			break;

		case 3:
		case 5:	// tracer
			p->die = gEngfuncs.GetClientTime() + 0.5;
			p->type = pt_static;

			if (type == 3)
				p->color = 52 + (tracercount & 4) * 2;
			else
				p->color = 230 + (tracercount & 4) * 2;

			VectorCopy(start, p->org);
			tracercount++;

			if (tracercount & 1)
			{
				p->vel[0] = 30 * vec[1];
				p->vel[1] = 30 * -vec[0];
			}
			else
			{
				p->vel[0] = 30 * -vec[1];
				p->vel[1] = 30 * vec[0];
			}
			break;

		case 4: // slight blood
			p->type = pt_grav;
			p->color = gEngfuncs.pfnRandomLong(67, 70);
			for (j = 0; j < 3; j++)
				p->org[j] = start[j] + gEngfuncs.pfnRandomFloat(-3, 3);
			len -= 3;
			break;

		case 6:	// voor trail
			p->ramp = gEngfuncs.pfnRandomLong(0, 3);
			p->type = pt_fire;
			p->color = ramp3[(int)p->ramp];
			VectorCopy(start, p->org);
			break;

		case 7:	// explosion tracer
		{
			float s, c, x, y;

			j = gEngfuncs.pfnRandomLong(0, 0xFFFF);
			s = sin(j);
			c = cos(j);

			j = gEngfuncs.pfnRandomLong(8, 16);
			y = s * j;
			x = c * j;

			p->org[0] = start[0] + right[0] * y + up[0] * x;
			p->org[1] = start[1] + right[1] * y + up[1] * x;
			p->org[2] = start[2] + right[2] * y + up[2] * x;
			
			VectorSubtract(start, p->org, p->vel);
			VectorScale(p->vel, 2.0, p->vel);
			VectorMA(p->vel, gEngfuncs.pfnRandomFloat(96, 111), vec, p->vel);

			p->die = gEngfuncs.GetClientTime() + 2.0;
			p->ramp = gEngfuncs.pfnRandomLong(0, 3);
			p->color = ramp3[(int)p->ramp];
			p->type = pt_explode2;
			break;
		}
		}
		gEngfuncs.pEfxAPI->R_GetPackedColor(&p->packedColor, p->color);

		VectorAdd(start, vec, start);
	}
}

//=======================================
//	TEMP-ENTITY CODE STARTS PAST THIS POINT...
//=======================================

int ModelFrameCount( model_t* model )
{
	return 0;
}

//=======================================
//	R_FizzEffect
//	Create a fizz effect
//=======================================

void CTempEntities::R_FizzEffect( cl_entity_t* pent, int modelIndex, int density )
{

}

//=======================================
//	R_Bubbles
//	Create bubbles
//=======================================

void CTempEntities::R_Bubbles( vec_t* mins, vec_t* maxs, float height, int modelIndex, int count, float speed )
{

}

//=======================================
//	R_BubbleTrail
//	Create bubble trail
//=======================================

void CTempEntities::R_BubbleTrail( vec_t* start, vec_t* end, float height, int modelIndex, int count, float speed )
{

}

//=======================================
//	R_Sprite_Trail
//	Line of moving glow sprites with gravity, fadeout, and collisions
//=======================================

void CTempEntities::R_Sprite_Trail( int type, vec_t* start, vec_t* end, int modelIndex, int count, float life, float size, float amplitude, int renderamt, float speed )
{

}

//=======================================
//	R_TempSphereModel
//	Spherical shower of models, picks from set
//=======================================

void CTempEntities::R_TempSphereModel( float* pos, float speed, float life, int count, int modelIndex )
{

}

#define SHARD_VOLUME 12.0	// on shard ever n^3 units

//=======================================
//	R_BreakModel
//	Create model shattering shards
//=======================================

void CTempEntities::R_BreakModel( float* pos, float* size, float* dir, float random, float life, int count, int modelIndex, char flags )
{

}

//=======================================
//	R_TempSprite
//	Create sprite TE
//=======================================

TEMPENTITY* CTempEntities::R_TempSprite( float* pos, float* dir, float scale, int modelIndex, int rendermode, int renderfx, float a, float life, int flags )
{
	return NULL;
}

//=======================================
//	R_Sprite_Spray
//	Spray sprite
//=======================================

void CTempEntities::R_Sprite_Spray( vec_t* pos, vec_t* dir, int modelIndex, int count, int speed, int iRand )
{

}

//=======================================
//	R_SparkEffect
//=======================================

void CTempEntities::R_SparkEffect( float* pos, int count, int velocityMin, int velocityMax )
{

}

//=======================================
//	R_FunnelSprite
//=======================================

void CTempEntities::R_FunnelSprite( float* org, int modelIndex, int reverse )
{

}

//=======================================
//	R_RicochetSprite
//	Create ricochet sprite
//=======================================

void CTempEntities::R_RicochetSprite( float* pos, model_t* pmodel, float duration, float scale )
{

}

//=======================================
//	R_BloodSprite
//	Create blood sprite
//=======================================

void CTempEntities::R_BloodSprite( vec_t* org, int colorindex, int modelIndex, int modelIndex2, float size )
{

}

//=======================================
//	R_DefaultSprite
//	Create default sprite TE
//=======================================

TEMPENTITY* CTempEntities::R_DefaultSprite( float* pos, int spriteIndex, float framerate )
{
	return NULL;
}

//=======================================
//	R_Sprite_Smoke
//	Create sprite smoke
//=======================================

void CTempEntities::R_Sprite_Smoke( TEMPENTITY* pTemp, float scale )
{
}

//=======================================
//	R_Sprite_Explode
//	Create explosion sprite
//=======================================

void CTempEntities::R_Sprite_Explode( TEMPENTITY* pTemp, float scale, int flags )
{
}

//=======================================
//	R_Sprite_WallPuff
//	Create a wallpuff
//=======================================

void CTempEntities::R_Sprite_WallPuff( TEMPENTITY* pTemp, float scale )
{
}

//=======================================
//	R_MuzzleFlash
//	Play muzzle flash
//=======================================

void CTempEntities::R_MuzzleFlash( float* pos1, int type )
{
}


//=======================================
//	MsgFunc_ParseTEnt
//	This whole function is a bit uhm.. bloated..
//=======================================

int CTempEntities::MsgFunc_ParseTEnt(const char* pszName, int iSize, void* pbuf)
{
	return 0;
}