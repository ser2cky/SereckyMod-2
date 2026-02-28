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
//	Purpose: Let people modify Temp-Ents
//	to their heart's content. Code taken from
//	the Half-Life NetTest reverse engineer, and
//	Xash3D FWGS...
//
//	History:
//	FEB-22-26: Started
//	FEB-24-26: moved all temp-ents into a class
//	so people can simply call them from the
//	"gTempEnt" global, like how you'd call something
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
	int i;
	char* disk_basepal;

	disk_basepal = (char*)gEngfuncs.COM_LoadFile("gfx/palette.lmp", 1, NULL);

	if (!disk_basepal)
	{
		gEngfuncs.pfnConsolePrint("Couldn't load gfx/palette.lmp");
	}
	else
	{
		host_basepal = (unsigned short*)malloc(sizeof(PackedColorVec) * 256);

		for (i = 0; i < 256; i++)
		{
			host_basepal[i * 4 + 0] = disk_basepal[i * 3 + 2];
			host_basepal[i * 4 + 1] = disk_basepal[i * 3 + 1];
			host_basepal[i * 4 + 2] = disk_basepal[i * 3 + 0];
			host_basepal[i * 4 + 3] = 0;
		}
	}

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
	if ( host_basepal )
		free( host_basepal );
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
	int count;

	count = 1;

	if (model)
	{
		if (model->type == mod_sprite)
		{
			msprite_t* psprite;

			psprite = (msprite_t*)model->cache.data;
			count = psprite->numframes;
		}
		else if (model->type == mod_studio)
		{
			count = g_StudioRenderer.StudioBodyVariations(model);
			count = 0;
		}

		if (count < 1)
			count = 1;
	}

	return count;
}

//=======================================
//	R_FizzEffect
//	Create a fizz effect
//=======================================

void CTempEntities::R_FizzEffect( cl_entity_t* pent, int modelIndex, int density )
{
	TEMPENTITY* pTemp;
	model_t* model;
	int				i, width, depth, count, frameCount;
	float			maxHeight, speed, xspeed, yspeed, zspeed;
	vec3_t			origin;

	if (!pent->model || !modelIndex)
		return;

	model = IEngineStudio.GetModelByIndex(modelIndex);
	model = IEngineStudio.GetModelByIndex(modelIndex);
	if (!model)
		return;

	count = density + 1;

	maxHeight = pent->model->maxs[2] - pent->model->mins[2];
	width = pent->model->maxs[0] - pent->model->mins[0];
	depth = pent->model->maxs[1] - pent->model->mins[1];
	speed = (float)pent->baseline.rendercolor.r * 256.0 + (float)pent->baseline.rendercolor.g;
	if (pent->baseline.rendercolor.b)
		speed = -speed;

	xspeed = cos(pent->angles[YAW] * M_PI / 180) * speed;
	yspeed = sin(pent->angles[YAW] * M_PI / 180) * speed;

	frameCount = ModelFrameCount(model);

	for (i = 0; i < count; i++)
	{
		origin[0] = pent->model->mins[0] + gEngfuncs.pfnRandomLong(0, width - 1);
		origin[1] = pent->model->mins[1] + gEngfuncs.pfnRandomLong(0, depth - 1);
		origin[2] = pent->model->mins[2];
		pTemp = gEngfuncs.pEfxAPI->CL_TempEntAlloc(origin, model);
		if (!pTemp)
			return;

		pTemp->flags |= FTENT_SINEWAVE;

		pTemp->x = origin[0];
		pTemp->y = origin[1];

		zspeed = gEngfuncs.pfnRandomLong(80, 140);
		pTemp->entity.baseline.origin[0] = xspeed;
		pTemp->entity.baseline.origin[1] = yspeed;
		pTemp->entity.baseline.origin[2] = zspeed;
		pTemp->die = gEngfuncs.GetClientTime() + (maxHeight / zspeed) - 0.1;
		pTemp->entity.curstate.frame = gEngfuncs.pfnRandomLong(0, frameCount - 1);
		// Set sprite scale
		pTemp->entity.curstate.scale = 1.0 / gEngfuncs.pfnRandomFloat(2, 5);
		pTemp->entity.curstate.rendermode = kRenderTransAlpha;
		pTemp->entity.curstate.renderamt = 255;
	}
}

//=======================================
//	R_Bubbles
//	Create bubbles
//=======================================

void CTempEntities::R_Bubbles( vec_t* mins, vec_t* maxs, float height, int modelIndex, int count, float speed )
{
	TEMPENTITY* pTemp;
	model_t* model;
	int					i, frameCount;
	float				angle;
	vec3_t				origin;

	if (!modelIndex)
		return;

	model = IEngineStudio.GetModelByIndex(modelIndex);
	if (!model)
		return;

	frameCount = ModelFrameCount(model);

	for (i = 0; i < count; i++)
	{
		origin[0] = gEngfuncs.pfnRandomLong(mins[0], maxs[0]);
		origin[1] = gEngfuncs.pfnRandomLong(mins[1], maxs[1]);
		origin[2] = gEngfuncs.pfnRandomLong(mins[2], maxs[2]);
		pTemp = gEngfuncs.pEfxAPI->CL_TempEntAlloc(origin, model);
		if (!pTemp)
			return;

		pTemp->flags |= FTENT_SINEWAVE;

		pTemp->x = origin[0];
		pTemp->y = origin[1];

		angle = gEngfuncs.pfnRandomLong(-3, 3);

		pTemp->entity.baseline.origin[0] = cos(angle) * speed;
		pTemp->entity.baseline.origin[1] = sin(angle) * speed;
		pTemp->entity.baseline.origin[2] = gEngfuncs.pfnRandomLong(80, 140);
		pTemp->die = gEngfuncs.GetClientTime() + ((height - (origin[2] - mins[2])) / pTemp->entity.baseline.origin[2]) - 0.1;
		pTemp->entity.curstate.frame = gEngfuncs.pfnRandomLong(0, frameCount - 1);

		// Set sprite scale
		pTemp->entity.curstate.scale = 1.0 / gEngfuncs.pfnRandomFloat(2, 5);
		pTemp->entity.curstate.rendermode = kRenderTransAlpha;
		pTemp->entity.curstate.renderamt = 255;
	}
}

//=======================================
//	R_BubbleTrail
//	Create bubble trail
//=======================================

void CTempEntities::R_BubbleTrail( vec_t* start, vec_t* end, float height, int modelIndex, int count, float speed )
{
	TEMPENTITY* pTemp;
	model_t* model;
	int					i, frameCount;
	float				dist, angle, zspeed;
	vec3_t				origin;

	if (!modelIndex)
		return;

	model = IEngineStudio.GetModelByIndex(modelIndex);
	if (!model)
		return;

	frameCount = ModelFrameCount(model);

	for (i = 0; i < count; i++)
	{
		dist = gEngfuncs.pfnRandomFloat(0, 1.0);
		origin[0] = start[0] + dist * (end[0] - start[0]);
		origin[1] = start[1] + dist * (end[1] - start[1]);
		origin[2] = start[2] + dist * (end[2] - start[2]);
		pTemp = gEngfuncs.pEfxAPI->CL_TempEntAlloc(origin, model);
		if (!pTemp)
			return;

		pTemp->flags |= FTENT_SINEWAVE;

		pTemp->x = origin[0];
		pTemp->y = origin[1];
		angle = gEngfuncs.pfnRandomLong(-3, 3);

		zspeed = gEngfuncs.pfnRandomLong(80, 140);
		pTemp->entity.baseline.origin[0] = speed * cos(angle);
		pTemp->entity.baseline.origin[1] = speed * sin(angle);
		pTemp->entity.baseline.origin[2] = zspeed;
		pTemp->die = gEngfuncs.GetClientTime() + ((height - (origin[2] - start[2])) / zspeed) - 0.1;
		pTemp->entity.curstate.frame = gEngfuncs.pfnRandomLong(0, frameCount - 1);

		// Set sprite scale
		pTemp->entity.curstate.scale = 1.0 / gEngfuncs.pfnRandomFloat(2, 5);
		pTemp->entity.curstate.rendermode = kRenderTransAlpha;
		pTemp->entity.curstate.renderamt = 255;
	}
}

//=======================================
//	R_Sprite_Trail
//	Line of moving glow sprites with gravity, fadeout, and collisions
//=======================================

void CTempEntities::R_Sprite_Trail( int type, vec_t* start, vec_t* end, int modelIndex, int count, float life, float size, float amplitude, int renderamt, float speed )
{
	TEMPENTITY* ptemp;
	model_t* model;
	int				i, frameCount;
	vec3_t			delta, pos, dir;

	model = IEngineStudio.GetModelByIndex(modelIndex);
	if (!model)
		return;

	frameCount = ModelFrameCount(model);

	VectorSubtract(end, start, delta);
	VectorCopy(delta, dir);
	VectorNormalize(dir);

	amplitude /= 256.0;

	for (i = 0; i < count; i++)
	{
		// Be careful of divide by 0 when using 'count' here...
		if (i == 0)
		{
			VectorMA(start, 0, delta, pos);
		}
		else
		{
			float scale = (float)i / ((float)count - 1.0);
			VectorMA(start, scale, delta, pos);
		}

		ptemp = gEngfuncs.pEfxAPI->CL_TempEntAlloc(pos, model);
		if (!ptemp)
			return;

		ptemp->flags |= (FTENT_COLLIDEWORLD | FTENT_SPRCYCLE | FTENT_FADEOUT | FTENT_SLOWGRAVITY);

		VectorScale(dir, speed, ptemp->entity.baseline.origin);

		ptemp->entity.baseline.origin[0] += gEngfuncs.pfnRandomLong(-127, 128) * amplitude;
		ptemp->entity.baseline.origin[1] += gEngfuncs.pfnRandomLong(-127, 128) * amplitude;
		ptemp->entity.baseline.origin[2] += gEngfuncs.pfnRandomLong(-127, 128) * amplitude;

		ptemp->entity.curstate.rendermode = kRenderGlow;
		ptemp->entity.curstate.renderfx = kRenderFxNoDissipation;
		ptemp->entity.curstate.renderamt = renderamt;
		ptemp->entity.baseline.renderamt = renderamt;
		ptemp->entity.curstate.scale = size;

		ptemp->entity.curstate.frame = gEngfuncs.pfnRandomLong(0, frameCount - 1);
		ptemp->frameMax = frameCount;
		ptemp->die = gEngfuncs.GetClientTime() + life + gEngfuncs.pfnRandomFloat(0, 4);

		ptemp->entity.curstate.rendercolor.r = 255;
		ptemp->entity.curstate.rendercolor.g = 255;
		ptemp->entity.curstate.rendercolor.b = 255;
	}
}

//=======================================
//	R_TempSphereModel
//	Spherical shower of models, picks from set
//=======================================

void CTempEntities::R_TempSphereModel( float* pos, float speed, float life, int count, int modelIndex )
{
	int					i, frameCount;
	TEMPENTITY* pTemp;
	model_t* model;

	if (!modelIndex)
		return;

	model = IEngineStudio.GetModelByIndex(modelIndex);
	if (!model)
		return;

	frameCount = ModelFrameCount(model);

	// Create temporary models
	for (i = 0; i < count; i++)
	{
		pTemp = gEngfuncs.pEfxAPI->CL_TempEntAlloc(pos, model);
		if (!pTemp)
			return;

		pTemp->entity.curstate.body = gEngfuncs.pfnRandomLong(0, frameCount - 1);

		if (gEngfuncs.pfnRandomLong(0, 255) < 10)
			pTemp->flags |= FTENT_SLOWGRAVITY;
		else
			pTemp->flags |= FTENT_GRAVITY;

		if (gEngfuncs.pfnRandomLong(0, 255) < 220)
		{
			pTemp->flags |= FTENT_ROTATE;
			pTemp->entity.baseline.angles[0] = gEngfuncs.pfnRandomFloat(-256, -255);
			pTemp->entity.baseline.angles[1] = gEngfuncs.pfnRandomFloat(-256, -255);
			pTemp->entity.baseline.angles[2] = gEngfuncs.pfnRandomFloat(-256, -255);
		}

		if (gEngfuncs.pfnRandomLong(0, 255) < 100)
		{
			pTemp->flags |= FTENT_SMOKETRAIL;
		}

		pTemp->flags |= FTENT_FLICKER | FTENT_COLLIDEWORLD;

		pTemp->entity.curstate.effects = i & 0x1F;
		pTemp->entity.curstate.rendermode = kRenderNormal;

		pTemp->entity.baseline.origin[0] = gEngfuncs.pfnRandomFloat(-1, 1);
		pTemp->entity.baseline.origin[1] = gEngfuncs.pfnRandomFloat(-1, 1);
		pTemp->entity.baseline.origin[2] = gEngfuncs.pfnRandomFloat(-1, 1);

		VectorNormalize(pTemp->entity.baseline.origin);
		VectorScale(pTemp->entity.baseline.origin, speed, pTemp->entity.baseline.origin);

		pTemp->die = gEngfuncs.GetClientTime() + life;
	}
}

#define SHARD_VOLUME 12.0	// on shard ever n^3 units

//=======================================
//	R_BreakModel
//	Create model shattering shards
//=======================================

void CTempEntities::R_BreakModel( float* pos, float* size, float* dir, float random, float life, int count, int modelIndex, char flags )
{
	int					i, frameCount;
	TEMPENTITY* pTemp;
	model_t* pModel;
	char				type;

	if (!modelIndex)
		return;

	type = flags & BREAK_TYPEMASK;

	pModel = IEngineStudio.GetModelByIndex(modelIndex);
	if (!pModel)
		return;

	frameCount = ModelFrameCount(pModel);

	if (count == 0)
		// assume surface (not volume)
		count = (size[0] * size[1] + size[1] * size[2] + size[2] * size[0]) / (3 * SHARD_VOLUME * SHARD_VOLUME);

	for (i = 0; i < count; i++)
	{
		vec3_t vecSpot;

		// fill up the box with stuff
		
		vecSpot[0] = pos[0] + gEngfuncs.pfnRandomFloat(-0.5, 0.5) * size[0];
		vecSpot[1] = pos[1] + gEngfuncs.pfnRandomFloat(-0.5, 0.5) * size[1];
		vecSpot[2] = pos[2] + gEngfuncs.pfnRandomFloat(-0.5, 0.5) * size[2];

		pTemp = gEngfuncs.pEfxAPI->CL_TempEntAlloc(vecSpot, pModel);
		if (!pTemp)
			break;

		// keep track of break_type, so we know how to play sound on collision
		pTemp->hitSound = type;

		if (pModel->type == mod_sprite)
			pTemp->entity.curstate.frame = gEngfuncs.pfnRandomLong(0, frameCount - 1);
		else if (pModel->type == mod_studio)
			pTemp->entity.curstate.body = gEngfuncs.pfnRandomLong(0, frameCount - 1);

		pTemp->flags |= FTENT_COLLIDEWORLD | FTENT_FADEOUT | FTENT_SLOWGRAVITY;

		if (gEngfuncs.pfnRandomLong(0, 255) < 200)
		{
			pTemp->flags |= FTENT_ROTATE;
			pTemp->entity.baseline.angles[0] = gEngfuncs.pfnRandomFloat(-256, 255);
			pTemp->entity.baseline.angles[1] = gEngfuncs.pfnRandomFloat(-256, 255);
			pTemp->entity.baseline.angles[2] = gEngfuncs.pfnRandomFloat(-256, 255);
		}

		if ((gEngfuncs.pfnRandomLong(0, 255) < 100) && (flags & BREAK_SMOKE))
			pTemp->flags |= FTENT_SMOKETRAIL;

		if ((type == BREAK_GLASS) || (flags & BREAK_TRANS))
		{
			pTemp->entity.curstate.rendermode = kRenderTransTexture;
			pTemp->entity.curstate.renderamt = 128;
			pTemp->entity.baseline.renderamt = 128;
		}
		else
		{
			pTemp->entity.curstate.rendermode = kRenderNormal;
			pTemp->entity.baseline.renderamt = 255;		// Set this for fadeout
		}

		pTemp->entity.baseline.origin[0] = dir[0] + gEngfuncs.pfnRandomFloat(-random, random);
		pTemp->entity.baseline.origin[1] = dir[1] + gEngfuncs.pfnRandomFloat(-random, random);
		pTemp->entity.baseline.origin[2] = dir[2] + gEngfuncs.pfnRandomFloat(0, random);

		pTemp->die = gEngfuncs.GetClientTime() + life + gEngfuncs.pfnRandomFloat(0, 1);	// Add an extra 0-1 secs of life
	}
}

//=======================================
//	R_TempSprite
//	Create sprite TE
//=======================================

TEMPENTITY* CTempEntities::R_TempSprite( float* pos, float* dir, float scale, int modelIndex, int rendermode, int renderfx, float a, float life, int flags )
{
	TEMPENTITY* pTemp;
	model_t* model;
	int					frameCount;

	if (!modelIndex)
		return NULL;

	model = IEngineStudio.GetModelByIndex(modelIndex);
	if (!model)
	{
		gEngfuncs.Con_Printf("No model %d!\n", modelIndex);
		return NULL;
	}

	frameCount = ModelFrameCount(model);

	pTemp = gEngfuncs.pEfxAPI->CL_TempEntAlloc(pos, model);
	if (!pTemp)
		return NULL;
	gEngfuncs.Con_Printf("%d\n", frameCount);
	pTemp->frameMax = frameCount;
	pTemp->entity.curstate.framerate = 10;
	pTemp->entity.curstate.rendermode = rendermode;
	pTemp->entity.curstate.renderfx = renderfx;
	pTemp->entity.curstate.scale = scale;
	pTemp->entity.baseline.renderamt = a * 255;
	pTemp->entity.curstate.rendercolor.r = 255;
	pTemp->entity.curstate.rendercolor.g = 255;
	pTemp->entity.curstate.rendercolor.b = 255;
	pTemp->entity.curstate.renderamt = a * 255;

	pTemp->flags |= flags;

	VectorCopy(dir, pTemp->entity.baseline.origin);
	VectorCopy(pos, pTemp->entity.origin);
	if (life)
		pTemp->die = gEngfuncs.GetClientTime() + life;
	else
		pTemp->die = gEngfuncs.GetClientTime() + (frameCount * 0.1) + 1;

	pTemp->entity.curstate.frame = 0;
	return pTemp;
}

//=======================================
//	R_Sprite_Spray
//	Spray sprite
//=======================================

void CTempEntities::R_Sprite_Spray( vec_t* pos, vec_t* dir, int modelIndex, int count, int speed, int iRand )
{
	TEMPENTITY* pTemp;
	model_t* pModel;
	float				noise;
	float				znoise;
	int					frameCount;
	int					i;

	noise = (float)iRand / 100;

	// more vertical displacement
	znoise = noise * 1.5;

	if (znoise > 1)
	{
		znoise = 1;
	}

	pModel = IEngineStudio.GetModelByIndex(modelIndex);
	if (!pModel)
	{
		gEngfuncs.Con_Printf("No model %d!\n", modelIndex);
		return;
	}

	frameCount = ModelFrameCount(pModel) - 1;

	for (i = 0; i < count; i++)
	{
		pTemp = gEngfuncs.pEfxAPI->CL_TempEntAlloc(pos, pModel);
		if (!pTemp)
			return;

		pTemp->entity.curstate.rendermode = kRenderTransAlpha;
		pTemp->entity.curstate.renderamt = 255;
		pTemp->entity.baseline.renderamt = 255;
		pTemp->entity.curstate.renderfx = kRenderFxNoDissipation;
		pTemp->entity.curstate.scale = 0.5;
		pTemp->flags |= FTENT_FADEOUT | FTENT_SLOWGRAVITY;
		pTemp->fadeSpeed = 2.0;

		// make the spittle fly the direction indicated, but mix in some noise.
		pTemp->entity.baseline.origin[0] = dir[0] + gEngfuncs.pfnRandomFloat(-noise, noise);
		pTemp->entity.baseline.origin[1] = dir[1] + gEngfuncs.pfnRandomFloat(-noise, noise);
		pTemp->entity.baseline.origin[2] = dir[2] + gEngfuncs.pfnRandomFloat(0, znoise);
		VectorScale(pTemp->entity.baseline.origin, gEngfuncs.pfnRandomFloat((speed * 0.8), (speed * 1.2)), pTemp->entity.baseline.origin);

		VectorCopy(pos, pTemp->entity.origin);

		pTemp->die = gEngfuncs.GetClientTime() + 0.35;

		pTemp->entity.curstate.frame = gEngfuncs.pfnRandomLong(0, frameCount);
	}
}

//=======================================
//	R_SparkEffect
//=======================================

void CTempEntities::R_SparkEffect( float* pos, int count, int velocityMin, int velocityMax )
{
	gEngfuncs.pEfxAPI->R_SparkStreaks(pos, count, velocityMin, velocityMax);
	R_RicochetSprite(pos, cl_sprite_ricochet, 0.1, gEngfuncs.pfnRandomFloat(0.5, 1.0));
}

//=======================================
//	R_FunnelSprite
//=======================================

void CTempEntities::R_FunnelSprite( float* org, int modelIndex, int reverse )
{
	TEMPENTITY* pTemp;
	int				i, j;
	float			flDist, vel;
	vec3_t			dir, dest;
	model_t* model;
	int				frameCount;

	if (!modelIndex)
	{
		gEngfuncs.Con_Printf("No modelindex for funnel!!\n");
		return;
	}

	model = IEngineStudio.GetModelByIndex(modelIndex);
	if (!model)
	{
		gEngfuncs.Con_Printf("No model %d!\n", modelIndex);
		return;
	}

	frameCount = ModelFrameCount(model);

	for (i = -256; i < 256; i += 32)
	{
		for (j = -256; j < 256; j += 32)
		{
			pTemp = gEngfuncs.pEfxAPI->CL_TempEntAlloc(org, model);
			if (!pTemp)
				return;

			if (reverse)
			{
				VectorCopy(org, pTemp->entity.origin);

				dest[0] = org[0] + i;
				dest[1] = org[1] + j;
				dest[2] = org[2] + gEngfuncs.pfnRandomFloat(100, 800);

				// send particle heading to dest at a random speed
				VectorSubtract(dest, pTemp->entity.origin, dir);

				vel = dest[2] / 8;// velocity based on how far particle has to travel away from org
			}
			else
			{
				pTemp->entity.origin[0] = org[0] + i;
				pTemp->entity.origin[1] = org[1] + j;
				pTemp->entity.origin[2] = org[2] + gEngfuncs.pfnRandomFloat(100, 800);

				// send particle heading to org at a random speed
				VectorSubtract(org, pTemp->entity.origin, dir);

				vel = pTemp->entity.origin[2] / 8;// velocity based on how far particle starts from org
			}

			pTemp->entity.curstate.framerate = 10;
			pTemp->entity.curstate.rendermode = kRenderGlow;
			pTemp->entity.curstate.renderfx = kRenderFxNoDissipation;
			pTemp->entity.curstate.renderamt = 200;
			pTemp->entity.baseline.renderamt = 200;

			pTemp->frameMax = frameCount;
			pTemp->entity.curstate.scale = gEngfuncs.pfnRandomFloat(0.1f, 0.4f);
			pTemp->flags = FTENT_ROTATE | FTENT_FADEOUT;

			flDist = VectorNormalize(dir);	// save the distance

			if (vel < 64)
			{
				vel = 64;
			}

			vel += gEngfuncs.pfnRandomFloat(64, 128);
			VectorScale(dir, vel, pTemp->entity.baseline.origin);

			pTemp->fadeSpeed = 2.0;
			pTemp->die = gEngfuncs.GetClientTime() + (flDist / vel) - 0.5;
		}
	}
}

//=======================================
//	R_RicochetSprite
//	Create ricochet sprite
//=======================================

void CTempEntities::R_RicochetSprite( float* pos, model_t* pmodel, float duration, float scale )
{
	TEMPENTITY* pTemp;

	pTemp = gEngfuncs.pEfxAPI->CL_TempEntAlloc(pos, pmodel);
	if (!pTemp)
		return;

	pTemp->entity.curstate.rendermode = kRenderGlow;
	pTemp->entity.curstate.renderfx = kRenderFxNoDissipation;
	pTemp->entity.curstate.renderamt = 200;
	pTemp->entity.baseline.renderamt = 200;
	pTemp->entity.curstate.scale = scale;
	pTemp->flags = FTENT_FADEOUT;

	VectorClear(pTemp->entity.baseline.origin);

	VectorCopy(pos, pTemp->entity.origin);

	pTemp->fadeSpeed = 8;
	pTemp->die = gEngfuncs.GetClientTime();

	pTemp->entity.curstate.frame = 0;
	pTemp->entity.angles[ROLL] = (45 * gEngfuncs.pfnRandomLong(0, 7));
}

//=======================================
//	R_BloodSprite
//	Create blood sprite
//=======================================

void CTempEntities::R_BloodSprite( vec_t* org, int colorindex, int modelIndex, int modelIndex2, float size )
{
	int				i, splatter;
	TEMPENTITY* pTemp;
	model_t* model;
	model_t* model2;
	int				frameCount, frameCount2;
	unsigned int	impactindex;
	unsigned int	spatterindex;
	color24			impactcolor;
	color24			splattercolor;

	impactindex = 4 * (colorindex + gEngfuncs.pfnRandomLong(1, 3));
	spatterindex = 4 * (colorindex + gEngfuncs.pfnRandomLong(1, 3)) + 4;

	impactcolor.r = host_basepal[impactindex + 2];
	impactcolor.g = host_basepal[impactindex + 1];
	impactcolor.b = host_basepal[impactindex + 0];

	splattercolor.r = host_basepal[spatterindex + 2];
	splattercolor.g = host_basepal[spatterindex + 1];
	splattercolor.b = host_basepal[spatterindex + 0];

	//Validate the model first
	if (modelIndex2 && (model2 = IEngineStudio.GetModelByIndex(modelIndex2)))
	{
		frameCount2 = ModelFrameCount(model2);

		// Random amount of drips
		splatter = size + gEngfuncs.pfnRandomLong(1, 8) + gEngfuncs.pfnRandomLong(1, 8);

		for (i = 0; i < splatter; i++)
		{
			// Make some blood drips
			if (pTemp = gEngfuncs.pEfxAPI->CL_TempEntAlloc(org, model2))
			{
				pTemp->entity.curstate.rendermode = kRenderNormal;
				pTemp->entity.curstate.renderfx = kRenderFxNone;
				pTemp->entity.curstate.scale = gEngfuncs.pfnRandomFloat(size / 15, size / 25);
				pTemp->flags = FTENT_ROTATE | FTENT_SLOWGRAVITY | FTENT_COLLIDEWORLD;

				pTemp->entity.curstate.rendercolor = splattercolor;
				pTemp->entity.baseline.renderamt = 250;
				pTemp->entity.curstate.renderamt = 250;

				pTemp->entity.baseline.origin[0] = gEngfuncs.pfnRandomFloat(-96, 95);
				pTemp->entity.baseline.origin[1] = gEngfuncs.pfnRandomFloat(-96, 95);
				pTemp->entity.baseline.origin[2] = gEngfuncs.pfnRandomFloat(-32, 95);

				pTemp->entity.baseline.angles[0] = gEngfuncs.pfnRandomFloat(-256, -255);
				pTemp->entity.baseline.angles[1] = gEngfuncs.pfnRandomFloat(-256, -255);
				pTemp->entity.baseline.angles[2] = gEngfuncs.pfnRandomFloat(-256, -255);

				pTemp->entity.curstate.framerate = 0;
				pTemp->die = gEngfuncs.GetClientTime() + gEngfuncs.pfnRandomFloat(1, 2);

				pTemp->entity.curstate.frame = gEngfuncs.pfnRandomLong(1, frameCount2 - 1);
				if (pTemp->entity.curstate.frame > 8)
					pTemp->entity.curstate.frame = frameCount2 - 1;

				pTemp->entity.angles[ROLL] = gEngfuncs.pfnRandomLong(0, 360);
			}
		}
	}

	// Impact particle
	if (modelIndex && (model = IEngineStudio.GetModelByIndex(modelIndex)))
	{
		frameCount = ModelFrameCount(model);

		//Large, single blood sprite is a high-priority tent
		if (pTemp = gEngfuncs.pEfxAPI->CL_TempEntAlloc(org, model))
		{
			pTemp->entity.curstate.rendermode = kRenderNormal;
			pTemp->entity.curstate.renderfx = kRenderFxNone;
			pTemp->entity.curstate.scale = gEngfuncs.pfnRandomFloat(size / 25, size / 35);
			pTemp->flags = FTENT_SPRANIMATE;

			pTemp->entity.curstate.rendercolor = impactcolor;
			pTemp->entity.baseline.renderamt = 250;
			pTemp->entity.curstate.renderamt = 250;

			VectorClear(pTemp->entity.baseline.origin);

			pTemp->entity.curstate.framerate = frameCount * 4; // Finish in 0.250 seconds
			pTemp->die = gEngfuncs.GetClientTime() + (frameCount / pTemp->entity.curstate.framerate); // Play the whole thing Once

			pTemp->entity.curstate.frame = 0;
			pTemp->frameMax = frameCount;
			pTemp->entity.angles[ROLL] = gEngfuncs.pfnRandomLong(0, 360);
		}
	}
}

//=======================================
//	R_DefaultSprite
//	Create default sprite TE
//=======================================

TEMPENTITY* CTempEntities::R_DefaultSprite( float* pos, int spriteIndex, float framerate )
{
	TEMPENTITY* pTemp;
	int				frameCount;
	model_t* pSprite;

	// don't spawn while paused
	//if (gEngfuncs.GetClientTime() == cl.oldtime)
	//	return NULL;

	pSprite = IEngineStudio.GetModelByIndex(spriteIndex);

	if (!spriteIndex || !pSprite || pSprite->type != mod_sprite)
	{
		gEngfuncs.Con_DPrintf("No Sprite %d!\n", spriteIndex);
		return NULL;
	}

	frameCount = ModelFrameCount(pSprite);

	pTemp = gEngfuncs.pEfxAPI->CL_TempEntAlloc(pos, pSprite);
	if (!pTemp)
		return NULL;

	pTemp->entity.curstate.scale = 1.0;
	pTemp->frameMax = frameCount;
	pTemp->flags |= FTENT_SPRANIMATE;
	if (framerate == 0)
		framerate = 10;

	VectorCopy(pos, pTemp->entity.origin);

	pTemp->entity.curstate.framerate = framerate;
	pTemp->entity.curstate.frame = 0;
	pTemp->die = gEngfuncs.GetClientTime() + (float)frameCount / framerate;
	
	return pTemp;
}

//=======================================
//	R_Sprite_Smoke
//	Create sprite smoke
//=======================================

void CTempEntities::R_Sprite_Smoke( TEMPENTITY* pTemp, float scale )
{
	int iColor;

	if (!pTemp)
		return;

	pTemp->entity.curstate.rendermode = kRenderTransAlpha;
	pTemp->entity.curstate.renderfx = kRenderFxNone;
	pTemp->entity.curstate.renderamt = 255;
	pTemp->entity.baseline.origin[2] = 30;
	iColor = gEngfuncs.pfnRandomLong(20, 35);
	pTemp->entity.curstate.rendercolor.r = iColor;
	pTemp->entity.curstate.rendercolor.g = iColor;
	pTemp->entity.curstate.rendercolor.b = iColor;
	pTemp->entity.origin[2] += 20;
	pTemp->entity.curstate.scale = scale;
}

//=======================================
//	R_Sprite_Explode
//	Create explosion sprite
//=======================================

void CTempEntities::R_Sprite_Explode( TEMPENTITY* pTemp, float scale, int flags )
{
	if (!pTemp)
		return;

	if ( flags & TE_EXPLFLAG_NOADDITIVE )
	{
		pTemp->entity.curstate.rendermode = kRenderTransAdd;
		pTemp->entity.curstate.renderfx = kRenderFxNone;
	}
	else
	{
		pTemp->entity.curstate.rendermode = kRenderTransAdd;
		pTemp->entity.curstate.renderamt = 180;
	}

	pTemp->entity.curstate.renderfx = kRenderFxNone;
	pTemp->entity.curstate.rendercolor.r = 0;
	pTemp->entity.curstate.rendercolor.g = 0;
	pTemp->entity.curstate.rendercolor.b = 0;
	pTemp->entity.curstate.scale = scale;
	pTemp->entity.origin[2] += 10;
	pTemp->entity.curstate.renderamt = 180;
	pTemp->entity.baseline.origin[2] = 8.0;
}

//=======================================
//	R_Sprite_WallPuff
//	Create a wallpuff
//=======================================

void CTempEntities::R_Sprite_WallPuff( TEMPENTITY* pTemp, float scale )
{
	if (!pTemp)
		return;

	pTemp->entity.curstate.rendermode = kRenderTransAlpha;
	pTemp->entity.curstate.renderamt = 255;
	pTemp->entity.curstate.rendercolor.r = 0;
	pTemp->entity.curstate.rendercolor.g = 0;
	pTemp->entity.curstate.rendercolor.b = 0;
	pTemp->entity.curstate.renderfx = kRenderFxNone;
	pTemp->entity.curstate.scale = scale;
	pTemp->entity.curstate.frame = 0;
	pTemp->entity.angles[ROLL] = gEngfuncs.pfnRandomLong(0, 359);
	pTemp->die = gEngfuncs.GetClientTime() + 0.01;
}

//=======================================
//	R_MuzzleFlash
//	Play muzzle flash
//=======================================

void CTempEntities::R_MuzzleFlash( float* pos1, int type )
{
	// SERECKY FEB-24-26: for some reason i can't get this to work properly in s.w mode,
	// so we'll have to fall back to the original method...
	if ( !IEngineStudio.IsHardware() )
	{
		gEngfuncs.pEfxAPI->R_MuzzleFlash( pos1, type );
		return;
	}

	TEMPENTITY* pTemp;
	int index;
	float scale;
	int			frameCount;

	index = (type % 10) % 3;
	scale = (type / 10) * 0.1;
	if (scale == 0)
		scale = 0.5;

	frameCount = ModelFrameCount(cl_sprite_muzzleflash[index]);

	//gEngfuncs.Con_DPrintf("%d %f\n", index, scale);
	pTemp = gEngfuncs.pEfxAPI->CL_TempEntAlloc(pos1, cl_sprite_muzzleflash[index]);
	if (!pTemp)
		return;	
	pTemp->entity.curstate.rendermode = kRenderTransAdd;
	pTemp->entity.curstate.renderamt = 255;
	pTemp->entity.curstate.renderfx = kRenderFxNone;
	pTemp->entity.curstate.rendercolor.r = 255;
	pTemp->entity.curstate.rendercolor.g = 255;
	pTemp->entity.curstate.rendercolor.b = 255;
	VectorCopy(pos1, pTemp->entity.origin);
	pTemp->die = gEngfuncs.GetClientTime() +0.01;
	pTemp->entity.curstate.frame = gEngfuncs.pfnRandomLong(0, frameCount - 1);
	pTemp->frameMax = frameCount;
	VectorCopy(vec3_origin, pTemp->entity.angles);

	pTemp->entity.curstate.scale = scale;

	if (index == 0)
	{
		// Rifle flash
		pTemp->entity.angles[ROLL] = gEngfuncs.pfnRandomLong(0, 20);
	}
	else
	{
		pTemp->entity.angles[ROLL] = gEngfuncs.pfnRandomLong(0, 359);
	}

	gEngfuncs.CL_CreateVisibleEntity( ET_TEMPENTITY, &pTemp->entity );
}


//=======================================
//	MsgFunc_ParseTEnt
//	This whole function is a bit uhm.. bloated..
//=======================================

int CTempEntities::MsgFunc_ParseTEnt(const char* pszName, int iSize, void* pbuf)
{
	char	c;
	int		type;
	int		soundtype;
	int		speed;
	int		color;
	int		iRand;
	int		entnumber;
	int		modelindex, modelindex2;
	vec3_t	pos;
	vec3_t	size;
	vec3_t	dir;
	vec3_t	endpos;
	int		colorStart, colorLength;
	dlight_t* dl;
	cl_entity_t* ent;
	model_s* model;
	int		startEnt, endEnt;
	int		startFrame;
	int		count;
	float	life;
	float	scale;
	float	flSpeed;
	float	r, g, b, a;
	float	frameRate;
	int		flags;

	BEGIN_READ( pbuf, iSize );

	type = READ_BYTE();
	switch (type)
	{
	case TE_BEAMPOINTS:
	case TE_BEAMENTPOINT:
	case TE_BEAMENTS:
	{
		float width;
		float amplitude;
		float flSpeed;

		if (type == TE_BEAMENTS)
		{
			startEnt = READ_SHORT();
			endEnt = READ_SHORT();
		}
		else if (type == TE_BEAMENTPOINT)
		{
			startEnt = READ_SHORT();

			endpos[0] = READ_COORD();
			endpos[1] = READ_COORD();
			endpos[2] = READ_COORD();
		}
		else
		{
			pos[0] = READ_COORD();
			pos[1] = READ_COORD();
			pos[2] = READ_COORD();

			endpos[0] = READ_COORD();
			endpos[1] = READ_COORD();
			endpos[2] = READ_COORD();
		}

		modelindex = READ_SHORT();

		startFrame = READ_BYTE();
		frameRate = READ_BYTE() * 0.1;

		life = READ_BYTE() * 0.1;
		width = READ_BYTE() * 0.1;
		amplitude = READ_BYTE() * 0.01;

		r = READ_BYTE() / 255.0;
		g = READ_BYTE() / 255.0;
		b = READ_BYTE() / 255.0;
		a = READ_BYTE() / 255.0;

		flSpeed = READ_BYTE() * 0.1;

		switch (type)
		{
		case TE_BEAMENTS:
			gEngfuncs.pEfxAPI->R_BeamEnts(startEnt, endEnt, modelindex, life, width, amplitude, a, flSpeed, startFrame, frameRate, r, g, b);
			break;
		case TE_BEAMENTPOINT:
			gEngfuncs.pEfxAPI->R_BeamEntPoint(startEnt, endpos, modelindex, life, width, amplitude, a, flSpeed, startFrame, frameRate, r, g, b);
			break;
		case TE_BEAMPOINTS:
			gEngfuncs.pEfxAPI->R_BeamPoints(pos, endpos, modelindex, life, width, amplitude, a, flSpeed, startFrame, frameRate, r, g, b);
			break;
		}
		break;
	}

	case TE_GUNSHOT:			// bullet hitting wall
		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();
		R_RunParticleEffect(pos, vec3_origin, 0, 20);

		iRand = gEngfuncs.pfnRandomLong(0, 0x7FFF);
		if (iRand < 0x3FFF)
		{
			switch (iRand % 5)
			{
			case 0:
				S_StartDynamicSound(-1, CHAN_AUTO, "weapons/ric1.wav", pos, VOL_NORM, 1.0, 0, PITCH_NORM);
				break;
			case 1:
				S_StartDynamicSound(-1, CHAN_AUTO, "weapons/ric2.wav", pos, VOL_NORM, 1.0, 0, PITCH_NORM);
				break;
			case 2:
				S_StartDynamicSound(-1, CHAN_AUTO, "weapons/ric3.wav", pos, VOL_NORM, 1.0, 0, PITCH_NORM);
				break;
			case 3:
				S_StartDynamicSound(-1, CHAN_AUTO, "weapons/ric4.wav", pos, VOL_NORM, 1.0, 0, PITCH_NORM);
				break;
			case 4:
				S_StartDynamicSound(-1, CHAN_AUTO, "weapons/ric5.wav", pos, VOL_NORM, 1.0, 0, PITCH_NORM);
				break;
			}
		}
		break;

	case TE_EXPLOSION:			// rocket explosion
		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();

		modelindex = READ_SHORT();

		scale = READ_BYTE() * 0.1;
		frameRate = READ_BYTE();
		flags = READ_BYTE();

		if (scale != 0)
		{
			// sprite
			R_Sprite_Explode(R_DefaultSprite(pos, modelindex, frameRate), scale, flags);

			if (!(flags & TE_EXPLFLAG_NOPARTICLES))
				R_FlickerParticles(pos);

			if (!(flags & TE_EXPLFLAG_NODLIGHTS))
			{
				// big flash
				dl = gEngfuncs.pEfxAPI->CL_AllocDlight(0);
				VectorCopy(pos, dl->origin);
				dl->radius = 200;
				dl->color.r = 250;
				dl->color.g = 250;
				dl->color.b = 150;
				dl->die = gEngfuncs.GetClientTime() + 0.01;
				dl->decay = 800;


				// red glow
				dl = gEngfuncs.pEfxAPI->CL_AllocDlight(0);
				VectorCopy(pos, dl->origin);
				dl->radius = 150;
				dl->color.r = 255;
				dl->color.g = 190;
				dl->color.b = 40;
				dl->die = gEngfuncs.GetClientTime() + 1.0;
				dl->decay = 200;
			}
		}

		// sound
		if (!(flags & TE_EXPLFLAG_NOSOUND))
		{
			switch (gEngfuncs.pfnRandomLong(0, 2))
			{
			case 0:
				S_StartDynamicSound(-1, CHAN_AUTO, "weapons/explode3.wav", pos, VOL_NORM, 0.3, 0, PITCH_NORM);
				break;
			case 1:
				S_StartDynamicSound(-1, CHAN_AUTO, "weapons/explode4.wav", pos, VOL_NORM, 0.3, 0, PITCH_NORM);
				break;
			case 2:
				S_StartDynamicSound(-1, CHAN_AUTO, "weapons/explode5.wav", pos, VOL_NORM, 0.3, 0, PITCH_NORM);
				break;
			}
		}
		break;

	case TE_TAREXPLOSION:			// Quake1 "tarbaby" explosion with sound
		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();
		R_BlobExplosion(pos);
		S_StartDynamicSound(-1, CHAN_AUTO, "weapons/explode3.wav", pos, VOL_NORM, 1.0, 0, PITCH_NORM);
		break;

	case TE_SMOKE:		// alphablend sprite, move vertically 30 pps
		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();
		modelindex = READ_SHORT();
		scale = READ_BYTE() * 0.1;
		frameRate = READ_BYTE();
		R_Sprite_Smoke(gEngfuncs.pEfxAPI->R_DefaultSprite(pos, modelindex, frameRate), scale);
		break;

	case TE_TRACER:
		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();
		endpos[0] = READ_COORD();
		endpos[1] = READ_COORD();
		endpos[2] = READ_COORD();
		gEngfuncs.pEfxAPI->R_TracerEffect(pos, endpos);
		break;

	case TE_LIGHTNING:				// lightning bolts
	{
		float width;
		float amplitude;

		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();
		endpos[0] = READ_COORD();
		endpos[1] = READ_COORD();
		endpos[2] = READ_COORD();
		life = READ_BYTE() * 0.1;
		width = READ_BYTE() * 0.1;
		amplitude = READ_BYTE() * 0.01;
		modelindex = READ_SHORT();
		gEngfuncs.pEfxAPI->R_BeamLightning(pos, endpos, modelindex, life, width, amplitude, 0.6, 3.5);
		break;
	}

	case TE_SPARKS:
		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();
		R_SparkEffect(pos, 8, -200, 200);
		break;

	case TE_LAVASPLASH:
		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();
		R_LavaSplash(pos);
		break;

	case TE_TELEPORT:
		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();
		R_TeleportSplash(pos);
		break;

	case TE_EXPLOSION2:
		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();

		colorStart = READ_BYTE();
		colorLength = READ_BYTE();
		R_ParticleExplosion2(pos, colorStart, colorLength);

		// spawn some dynamic light
		dl = gEngfuncs.pEfxAPI->CL_AllocDlight(0);
		VectorCopy(pos, dl->origin);
		dl->radius = 350;
		dl->decay = 300;
		dl->die = gEngfuncs.GetClientTime() + 0.5;
		S_StartDynamicSound(-1, CHAN_AUTO, "weapons/explode3.wav", pos, 1.0, 0.6f, 0, 100);
		break;

	case TE_BSPDECAL:
	case TE_DECAL:
	case TE_WORLDDECAL:
	case TE_WORLDDECALHIGH:
	case TE_DECALHIGH:
	{
		int decalTextureIndex;

		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();

		if (type == TE_BSPDECAL)
		{
			decalTextureIndex = READ_SHORT();

			modelindex = 0; // Never remove it
			flags = FDECAL_PERMANENT;
		}
		else
		{
			decalTextureIndex = READ_BYTE();

			modelindex = 0;
			flags = 0;
		}

		if (type == TE_DECAL || type == TE_DECALHIGH || type == TE_BSPDECAL)
		{
			entnumber = READ_SHORT();

			if (type == TE_BSPDECAL && entnumber)
			{
				modelindex = READ_SHORT();
			}
		}
		else
		{
			entnumber = 0;
		}

		if (type == TE_DECALHIGH || type == TE_WORLDDECALHIGH)
			decalTextureIndex += 256; // texture index is greater than 256

		if ((ent = gEngfuncs.GetEntityByIndex(entnumber)) == NULL)
		{
			gEngfuncs.Con_Printf("Decal: entity = %i", entnumber);
			break;
		}

		if (m_pCvarDecals->value)
		{
			gEngfuncs.pEfxAPI->R_DecalShoot(gEngfuncs.pEfxAPI->Draw_DecalIndex(decalTextureIndex), entnumber, modelindex, pos, flags);
		}
		break;
	}

	case TE_IMPLOSION:
	{
		float radius;

		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();
		radius = READ_BYTE();
		count = READ_BYTE();
		life = READ_BYTE() / 10.0;
		R_Implosion(pos, radius, count, life);
		break;
	}
	
	case TE_SPRITETRAIL:
	{
		float amplitude;

		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();

		endpos[0] = READ_COORD();
		endpos[1] = READ_COORD();
		endpos[2] = READ_COORD();

		modelindex = READ_SHORT();
		count = READ_BYTE();

		life = READ_BYTE() * 0.1;

		scale = READ_BYTE();
		if (scale == 0.0)
			scale = 1.0;
		else
			scale *= 0.1;

		amplitude = READ_BYTE() * 10.0;
		flSpeed = READ_BYTE() * 10.0;

		R_Sprite_Trail(type, pos, endpos, modelindex, count, life, scale, amplitude, 255, flSpeed);
		break;
	}

	case TE_SPRITE:
		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();
		modelindex = READ_SHORT();
		scale = READ_BYTE() * 0.1;
		a = READ_BYTE() / 255.0;
		R_TempSprite(pos, vec3_origin, scale, modelindex, kRenderTransAdd, kRenderFxNone, a, 0, FTENT_SPRANIMATE);
		break;

	case TE_BEAMSPRITE:
		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();

		endpos[0] = READ_COORD();
		endpos[1] = READ_COORD();
		endpos[2] = READ_COORD();

		modelindex = READ_SHORT();	// beam modelindex
		modelindex2 = READ_SHORT();	// sprite modelindex

		gEngfuncs.pEfxAPI->R_BeamPoints(pos, endpos, modelindex, 0.01, 0.4, 0, gEngfuncs.pfnRandomFloat(0.5, 0.655), 5, 0, 0, 1, 0, 0);
		R_TempSprite(endpos, vec3_origin, 0.1, modelindex2, kRenderTransAdd, kRenderFxNone, 0.35, 0.01, 0);
		break;

	case TE_BEAMTORUS:
	case TE_BEAMDISK:
	case TE_BEAMCYLINDER:
	{
		float width;
		float amplitude;

		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();

		endpos[0] = READ_COORD();
		endpos[1] = READ_COORD();
		endpos[2] = READ_COORD();

		modelindex = READ_SHORT();
		startFrame = READ_BYTE();
		frameRate = READ_BYTE() * 0.1;
		life = READ_BYTE() * 0.1;
		width = READ_BYTE();
		amplitude = READ_BYTE() * 0.01;

		r = READ_BYTE() / 255.0;
		g = READ_BYTE() / 255.0;
		b = READ_BYTE() / 255.0;
		a = READ_BYTE() / 225.0;

		flSpeed = READ_BYTE() * 0.1;

		gEngfuncs.pEfxAPI->R_BeamCirclePoints(type, pos, endpos, modelindex, life, width, amplitude, a, flSpeed, startFrame, frameRate, r, g, b);
		break;
	}

	case TE_BEAMFOLLOW:
	{
		float width;

		startEnt = READ_SHORT();
		modelindex = READ_SHORT();

		life = READ_BYTE() * 0.1;
		width = READ_BYTE();

		r = READ_BYTE() / 255.0;
		g = READ_BYTE() / 255.0;
		b = READ_BYTE() / 255.0;
		a = READ_BYTE() / 255.0;

		gEngfuncs.pEfxAPI->R_BeamFollow(startEnt, modelindex, life, width, r, g, b, a);
		break;
	}

	case TE_GLOWSPRITE:
		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();

		modelindex = READ_SHORT();
		life = READ_BYTE() * 0.1;
		scale = READ_BYTE() * 0.1;
		a = READ_BYTE() / 255.0;

		R_TempSprite(pos, vec3_origin, scale, modelindex, kRenderGlow, kRenderFxNoDissipation, a, life, FTENT_FADEOUT);
		break;

	case TE_BEAMRING:
	{
		float width;
		float amplitude;

		startEnt = READ_SHORT();
		endEnt = READ_SHORT();
		modelindex = READ_SHORT();

		startFrame = READ_BYTE();
		frameRate = READ_BYTE() * 0.1;
		life = READ_BYTE() * 0.1;
		width = READ_BYTE() * 0.1;
		amplitude = READ_BYTE() * 0.01;

		r = READ_BYTE() / 255.0;
		g = READ_BYTE() / 255.0;
		b = READ_BYTE() / 255.0;
		a = READ_BYTE() / 255.0;

		flSpeed = READ_BYTE() * 0.1;

		gEngfuncs.pEfxAPI->R_BeamRing(startEnt, endEnt, modelindex, life, width, amplitude, a, flSpeed, startFrame, frameRate, r, g, b);
		break;
	}

	case TE_STREAK_SPLASH:
		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();

		dir[0] = READ_COORD();
		dir[1] = READ_COORD();
		dir[2] = READ_COORD();

		color = READ_BYTE();
		count = READ_SHORT();
		speed = READ_SHORT();
		iRand = READ_SHORT();
		R_StreakSplash(pos, dir, color, count, speed, -iRand, iRand);
		break;

	case TE_DLIGHT:
		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();

		dl = gEngfuncs.pEfxAPI->CL_AllocDlight(0);
		VectorCopy(pos, dl->origin);
		dl->radius = READ_BYTE() * 10.0;
		dl->color.r = READ_BYTE();
		dl->color.g = READ_BYTE();
		dl->color.b = READ_BYTE();
		dl->die = gEngfuncs.GetClientTime() + READ_BYTE() * 0.1;
		dl->decay = READ_BYTE() * 10.0;
		break;

	case TE_ELIGHT:
	{
		float flTime;

		dl = gEngfuncs.pEfxAPI->CL_AllocElight(READ_SHORT());
		dl->origin[0] = READ_COORD();
		dl->origin[1] = READ_COORD();
		dl->origin[2] = READ_COORD();
		dl->radius = READ_COORD();
		dl->color.r = READ_BYTE();
		dl->color.g = READ_BYTE();
		dl->color.b = READ_BYTE();

		life = READ_BYTE() * 0.1;
		dl->die = gEngfuncs.GetClientTime() + life;
		flTime = READ_COORD();
		if (life != 0)
			flTime /= life;

		dl->decay = flTime;
		break;
	}

	case TE_TEXTMESSAGE:
		gEngfuncs.Con_Printf("TODO!!!\n");
		break;

	case TE_LINE:
		gEngfuncs.Con_Printf("TODO!!!\n");
		break;

	case TE_BOX:
		gEngfuncs.Con_Printf("TODO!!!\n");
		break;

	case TE_KILLBEAM:
		entnumber = READ_SHORT();
		gEngfuncs.pEfxAPI->R_BeamKill(entnumber);
		break;

	case TE_LARGEFUNNEL:
		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();
		modelindex = READ_SHORT();
		flags = READ_SHORT();
		R_LargeFunnel(pos, flags);
		gEngfuncs.pEfxAPI->R_FunnelSprite(pos, modelindex, flags);
		break;

	case TE_BLOODSTREAM:
		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();
		dir[0] = READ_COORD();
		dir[1] = READ_COORD();
		dir[2] = READ_COORD();
		color = READ_BYTE();
		speed = READ_BYTE();
		R_BloodStream(pos, dir, color, speed);
		break;
		
	case TE_SHOWLINE:
		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();
		endpos[0] = READ_COORD();
		endpos[1] = READ_COORD();
		endpos[2] = READ_COORD();
		R_ShowLine(pos, endpos);
		break;

	case TE_BLOOD:
		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();
		dir[0] = READ_COORD();
		dir[1] = READ_COORD();
		dir[2] = READ_COORD();
		color = READ_BYTE();
		speed = READ_BYTE();
		R_Blood(pos, dir, color, speed);
		break;

	case TE_FIZZ:
	{
		int density;

		entnumber = READ_SHORT();
		modelindex = READ_SHORT();
		density = READ_BYTE();

		if ((ent = gEngfuncs.GetEntityByIndex(entnumber)) != NULL)
			R_FizzEffect(ent, modelindex, density);
		break;
	}

	case TE_MODEL:
		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();

		dir[0] = READ_COORD();
		dir[1] = READ_COORD();
		dir[2] = READ_COORD();

		endpos[0] = 0.0;
		endpos[1] = READ_ANGLE();
		endpos[2] = 0.0;

		modelindex = READ_SHORT();
		soundtype = READ_BYTE();
		life = READ_BYTE() * 0.1;

		gEngfuncs.pEfxAPI->R_TempModel(pos, dir, endpos, life, modelindex, soundtype);
		break;

	case TE_EXPLODEMODEL:
		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();

		flSpeed = READ_COORD();

		modelindex = READ_SHORT();
		count = READ_SHORT();
		life = READ_BYTE() * 0.1;

		R_TempSphereModel(pos, flSpeed, life, count, modelindex);
		break;
		
	case TE_BREAKMODEL:
	{
		float frandom;

		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();

		size[0] = READ_COORD();
		size[1] = READ_COORD();
		size[2] = READ_COORD();

		dir[0] = READ_COORD();
		dir[1] = READ_COORD();
		dir[2] = READ_COORD();

		frandom = READ_BYTE() * 10.0;
		modelindex = READ_SHORT();
		count = READ_BYTE();
		life = READ_BYTE() * 0.1;
		c = READ_BYTE();
		R_BreakModel(pos, size, dir, frandom, life, count, modelindex, c);
		break;
	}

	case TE_GUNSHOTDECAL:
	{
		int decalTextureIndex;
		TEMPENTITY* pTemp;

		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();

		entnumber = READ_SHORT();
		decalTextureIndex = READ_BYTE();	

		pTemp = gEngfuncs.pEfxAPI->CL_TempEntAlloc(pos, cl_sprite_shell);
		R_Sprite_WallPuff(pTemp, 0.3);

		iRand = gEngfuncs.pfnRandomLong(0, 0x7FFF);
		if (iRand < 0x3FFF)
		{
			switch (iRand % 5)
			{
			case 0:
				S_StartDynamicSound(-1, CHAN_AUTO, "weapons/ric1.wav", pos, VOL_NORM, 1.0, 0, PITCH_NORM);
				break;
			case 1:
				S_StartDynamicSound(-1, CHAN_AUTO, "weapons/ric2.wav", pos, VOL_NORM, 1.0, 0, PITCH_NORM);
				break;
			case 2:
				S_StartDynamicSound(-1, CHAN_AUTO, "weapons/ric3.wav", pos, VOL_NORM, 1.0, 0, PITCH_NORM);
				break;
			case 3:
				S_StartDynamicSound(-1, CHAN_AUTO, "weapons/ric4.wav", pos, VOL_NORM, 1.0, 0, PITCH_NORM);
				break;
			case 4:
				S_StartDynamicSound(-1, CHAN_AUTO, "weapons/ric5.wav", pos, VOL_NORM, 1.0, 0, PITCH_NORM);
				break;
			}
		}

		if ((ent = gEngfuncs.GetEntityByIndex(entnumber)) == NULL)
		{
			gEngfuncs.Con_Printf("Decal: entity = %i", entnumber);
			break;
		}

		if (m_pCvarDecals->value)
		{
			gEngfuncs.pEfxAPI->R_DecalShoot(gEngfuncs.pEfxAPI->Draw_DecalIndex(decalTextureIndex), entnumber, 0, pos, 0);
		}
		break;
	}

	case TE_SPRITE_SPRAY:
		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();
		dir[0] = READ_COORD();
		dir[1] = READ_COORD();
		dir[2] = READ_COORD();
		modelindex = READ_SHORT();
		count = READ_BYTE();
		speed = READ_BYTE();
		iRand = READ_BYTE();
		R_Sprite_Spray(pos, dir, modelindex, count, speed * 2, iRand);
		break;

	case TE_ARMOR_RICOCHET:
		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();
		scale = READ_BYTE() * 0.1;

		R_RicochetSprite(pos, cl_sprite_ricochet, 0.1, scale);

		switch (gEngfuncs.pfnRandomLong(0, 4))
		{
		case 0:
			S_StartDynamicSound(-1, CHAN_AUTO, "weapons/ric1.wav", pos, VOL_NORM, 1.0, 0, PITCH_NORM);
			break;
		case 1:
			S_StartDynamicSound(-1, CHAN_AUTO, "weapons/ric2.wav", pos, VOL_NORM, 1.0, 0, PITCH_NORM);
			break;
		case 2:
			S_StartDynamicSound(-1, CHAN_AUTO, "weapons/ric3.wav", pos, VOL_NORM, 1.0, 0, PITCH_NORM);
			break;
		case 3:
			S_StartDynamicSound(-1, CHAN_AUTO, "weapons/ric4.wav", pos, VOL_NORM, 1.0, 0, PITCH_NORM);
			break;
		case 4:
			S_StartDynamicSound(-1, CHAN_AUTO, "weapons/ric5.wav", pos, VOL_NORM, 1.0, 0, PITCH_NORM);
			break;
		}
		break;

	case TE_PLAYERDECAL:
		gEngfuncs.Con_Printf("USE SVC_TEMPENTITY VERSION INSTEAD!!!\n");
		break;

	case TE_BUBBLES:
	{
		float height;

		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();

		endpos[0] = READ_COORD();
		endpos[1] = READ_COORD();
		endpos[2] = READ_COORD();

		height = READ_COORD();
		modelindex = READ_SHORT();
		count = READ_BYTE();
		flSpeed = READ_COORD();
		R_Bubbles(pos, endpos, height, modelindex, count, flSpeed);
		break;
	}

	case TE_BUBBLETRAIL:
	{
		float height;

		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();

		endpos[0] = READ_COORD();
		endpos[1] = READ_COORD();
		endpos[2] = READ_COORD();

		height = READ_COORD();
		modelindex = READ_SHORT();
		count = READ_BYTE();
		flSpeed = READ_COORD();
		R_BubbleTrail(pos, endpos, height, modelindex, count, flSpeed);
		break;
	}

	case TE_BLOODSPRITE:
	{
		float fsize;

		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();
		modelindex = READ_SHORT();
		modelindex2 = READ_SHORT();
		color = READ_BYTE();
		fsize = READ_BYTE();
		R_BloodSprite(pos, color, modelindex, modelindex2, fsize);
		break;
	}

	case TE_PROJECTILE:
		gEngfuncs.Con_Printf("TODO!!!\n");
		break;

	case TE_SPRAY:
		gEngfuncs.Con_Printf("TODO!!!\n");
		break;

	case TE_PLAYERSPRITES:
		gEngfuncs.Con_Printf("TODO!!!\n");
		break;

	case TE_PARTICLEBURST:
		gEngfuncs.Con_Printf("TODO!!!\n");
		break;

	case TE_FIREFIELD:
		gEngfuncs.Con_Printf("TODO!!!\n");
		break;

	case TE_PLAYERATTACHMENT:
		gEngfuncs.Con_Printf("TODO!!!\n");
		break;

	case TE_KILLPLAYERATTACHMENTS:
		gEngfuncs.Con_Printf("TODO!!!\n");
		break;

	case TE_MULTIGUNSHOT:
		gEngfuncs.Con_Printf("TODO!!!\n");
		break;

	case TE_USERTRACER:
		gEngfuncs.Con_Printf("TODO!!!\n");
		break;

	default:
		gEngfuncs.Con_Printf("CL_ParseTEnt: bad type %d", type);
		break;
	}

	return 1;
}