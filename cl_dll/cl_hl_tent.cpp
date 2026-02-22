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

//=======================================
//	cl_hl_tent.cpp
//	Purpose: Let people modify Temp-Ents
//	to their heart's content. Code taken from
//	the Half-Life NetTest reverse engineer.
//
//	History:
//	FEB-22-26: Started
//
//=======================================

#include "hud.h"
#include "cl_util.h"
#include "com_model.h"
#include "r_studioint.h"
#include "r_efx.h"
#include "event_api.h"
#include "parsemsg.h"

cvar_t*		r_decals;
int __MsgFunc_ParseTEnt(const char* pszName, int iSize, void* pbuf);

// particle ramps
int		ramp1[8] = { 0x6F, 0x6D, 0x6B, 0x69, 0x67, 0x65, 0x63, 0x61 };
int		ramp2[8] = { 0x6F, 0x6E, 0x6D, 0x6C, 0x6B, 0x6A, 0x68, 0x66 };
int		ramp3[8] = { 0x6D, 0x6B, 6, 5, 4, 3, 0, 0 };

// spark ramps
int		gSparkRamp[9] = { 0xFE, 0xFD, 0xFC, 0x6F, 0x6E, 0x6D, 0x6C, 0x67, 0x60 };

//=======================================
//	TempEnt_Init
//=======================================

void TempEnt_Init( void )
{
	r_decals = gEngfuncs.pfnGetCvarPointer("r_decals");
	HOOK_MESSAGE(ParseTEnt);
}

//=======================================
//	S_StartDynamicSound
//	Yeah I kinda got lazy with this one... How else would I play sounds
//	on the client anyways?
//=======================================

void S_StartDynamicSound( int entnum, int entchannel, const char *sample, vec_t* origin, float fvol, float attenuation, int flags, int pitch )
{
	gEngfuncs.pEventAPI->EV_PlaySound( entnum, (float*)origin, entchannel, sample, fvol, attenuation, flags, pitch );
}

/*
===============
R_ParticleExplosion

===============
*/
void R_ParticleExplosion( vec_t* org )
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

/*
===============
R_ParticleExplosion2

===============
*/
void R_ParticleExplosion2( vec_t* org, int colorStart, int colorLength )
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

/*
===============
R_BlobExplosion

===============
*/
void R_BlobExplosion( vec_t* org )
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

void R_RunParticleEffect( vec_t* org, vec_t* dir, int color, int count )
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

/*
===============
R_FlickerParticles
===============
*/
void R_FlickerParticles( vec_t* org )
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

/*
===============
R_LavaSplash

===============
*/
void R_LavaSplash( vec_t* org )
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

/*
===============
R_LargeFunnel
===============
*/
void R_LargeFunnel( vec_t* org, int reverse )
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

/*
===============
R_TeleportSplash

Quake1 teleport splash
===============
*/
void R_TeleportSplash( vec_t* org )
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

/*
===============
R_ShowLine
===============
*/
void R_ShowLine( vec_t* start, vec_t* end )
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

/*
===============
R_BloodStream

===============
*/
void R_BloodStream( vec_t* org, vec_t* dir, int pcolor, int speed )
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

/*
===============
R_Blood

===============
*/
void R_Blood( vec_t* org, vec_t* dir, int pcolor, int speed )
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

/*
===============
R_RocketTrail
===============
*/
void R_RocketTrail( vec_t *start, vec_t *end, int type )
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
//	MsgFunc_ParseTEnt
//	This whole function is a bit uhm.. bloated..
//=======================================

int __MsgFunc_ParseTEnt(const char* pszName, int iSize, void* pbuf)
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
			gEngfuncs.pEfxAPI->R_Sprite_Explode(gEngfuncs.pEfxAPI->R_DefaultSprite(pos, modelindex, frameRate), scale, flags);

			R_FlickerParticles(pos);

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

		// sound
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
		gEngfuncs.pEfxAPI->R_Sprite_Smoke(gEngfuncs.pEfxAPI->R_DefaultSprite(pos, modelindex, frameRate), scale);
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
		gEngfuncs.pEfxAPI->R_SparkEffect(pos, 8, -200, 200);
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

		if (r_decals->value)
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
		gEngfuncs.pEfxAPI->R_Implosion(pos, radius, count, life);
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

		gEngfuncs.pEfxAPI->R_Sprite_Trail(type, pos, endpos, modelindex, count, life, scale, amplitude, 255, flSpeed);
		break;
	}

	case TE_SPRITE:
		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();
		modelindex = READ_SHORT();
		scale = READ_BYTE() * 0.1;
		a = READ_BYTE() / 255.0;
		gEngfuncs.pEfxAPI->R_TempSprite(pos, vec3_origin, scale, modelindex, kRenderTransAdd, kRenderFxNone, a, 0, FTENT_SPRANIMATE);
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
		gEngfuncs.pEfxAPI->R_TempSprite(endpos, vec3_origin, 0.1, modelindex2, kRenderTransAdd, kRenderFxNone, 0.35, 0.01, 0);
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

		gEngfuncs.pEfxAPI->R_TempSprite(pos, vec3_origin, scale, modelindex, kRenderGlow, kRenderFxNoDissipation, a, life, FTENT_FADEOUT);
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
		gEngfuncs.pEfxAPI->R_StreakSplash(pos, dir, color, count, speed, -iRand, iRand);
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
			gEngfuncs.pEfxAPI->R_FizzEffect(ent, modelindex, density);
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

		gEngfuncs.pEfxAPI->R_TempSphereModel(pos, flSpeed, life, count, modelindex);
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
		gEngfuncs.pEfxAPI->R_BreakModel(pos, size, dir, frandom, life, count, modelindex, c);
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

		if ((model = gEngfuncs.CL_LoadModel("sprites/wallpuff.spr", &modelindex)) != NULL)
			pTemp = gEngfuncs.pEfxAPI->CL_TempEntAlloc(pos, model);
		gEngfuncs.pEfxAPI->R_Sprite_WallPuff(pTemp, 0.3);

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

		if (r_decals->value)
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
		gEngfuncs.pEfxAPI->R_Sprite_Spray(pos, dir, modelindex, count, speed * 2, iRand);
		break;

	case TE_ARMOR_RICOCHET:
		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();
		scale = READ_BYTE() * 0.1;

		if ((model = gEngfuncs.CL_LoadModel("sprites/richo1.spr", &modelindex)) != NULL)
			gEngfuncs.pEfxAPI->R_RicochetSprite(pos, model, 0.1, scale);

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
		gEngfuncs.pEfxAPI->R_Bubbles(pos, endpos, height, modelindex, count, flSpeed);
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
		gEngfuncs.pEfxAPI->R_BubbleTrail(pos, endpos, height, modelindex, count, flSpeed);
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
		gEngfuncs.pEfxAPI->R_BloodSprite(pos, color, modelindex, modelindex2, fsize);
		break;
	}

	default:
		gEngfuncs.Con_Printf("CL_ParseTEnt: bad type %d", type);
		break;
	}

	return 1;
}