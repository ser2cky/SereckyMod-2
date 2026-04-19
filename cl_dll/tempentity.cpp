
//====================================================================
//	cl_hl_tent.cpp
// 
//	Credits: 
//	Xash3D FWGS Team, Half-Life NetTest Reverse Engineer Team
// 
//	Purpose: 
//	Give modders access to Half-Life's temporary entity code.
// 
//	These effects will have to be called from the server using "gmsgTempEntity",
//	not SVC_TEMPENTITY. You may also call these effects on the client,
//	since this is sorta like an exposed version of the pEfxApi. Effects that 
//	do not have their own equivalent will use their pEfxApi equivalent instead.
// 
//====================================================================

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include "r_efx.h"
#include "event_api.h"

#include "com_model.h"
#include "r_studioint.h"

#include "pmtrace.h"
#include "pm_defs.h"
#include "entity_types.h"
#include "customentity.h"

#include "studio_util.h"
#include "studio.h"
#include "StudioModelRenderer.h"
#include "GameStudioModelRenderer.h"

#include "tempentity.h"

//==================================================================

extern engine_studio_api_t			IEngineStudio;
extern CGameStudioModelRenderer		g_StudioRenderer;

CTempEntities		gTempEnt;
extern vec3_t		v_origin;

void VectorAngles(const float* forward, float* angles);

//==================================================================

int __MsgFunc_ParseTEnt(const char* pszName, int iSize, void* pbuf)
{
	return gTempEnt.MsgFunc_ParseTEnt(pszName, iSize, pbuf);
}

void CTempEntities::Init(void)
{
	m_pCvarDecals = gEngfuncs.pfnGetCvarPointer("r_decals");
	m_pCvarTracerSpeed = gEngfuncs.pfnGetCvarPointer("tracerspeed");
	m_pCvarTracerOffset = gEngfuncs.pfnGetCvarPointer("traceroffset");
	HOOK_MESSAGE( ParseTEnt );
}

void CTempEntities::VidInit(void)
{
	cl_sprite_muzzleflash[0] = IEngineStudio.Mod_ForName("sprites/muzzleflash1.spr", 0);
	cl_sprite_muzzleflash[1] = IEngineStudio.Mod_ForName("sprites/muzzleflash2.spr", 0);
	cl_sprite_muzzleflash[2] = IEngineStudio.Mod_ForName("sprites/muzzleflash3.spr", 0);

	cl_sprite_ricochet = IEngineStudio.Mod_ForName("sprites/richo1.spr", 0);
	cl_sprite_shell = IEngineStudio.Mod_ForName("sprites/wallpuff.spr", 0);
}

//==================================================================

/*
===============
ModelFrameCount

===============
*/

static int ModelFrameCount(model_t* model)
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
		}

		if (count < 1)
			count = 1;
	}

	return count;
}

/*
===============
S_StartDynamicSound

Seems to be the only way to play sounds on the client with proper channels...
===============
*/
static void S_StartDynamicSound(int entnum, int entchannel, const char *sample, vec_t* origin, float fvol, float attenuation, int flags, int pitch)
{
	gEngfuncs.pEventAPI->EV_PlaySound( entnum, (float*)origin, entchannel, sample, fvol, attenuation, flags, pitch );
}

/*
===============
R_ParticleExplosion

===============
*/
void CTempEntities::R_ParticleExplosion( vec_t* org )
{
	int			i, j;
	particle_t* p;

	for (i = 0; i < 1024; i++)
	{
		if (!(p = gEngfuncs.pEfxAPI->R_AllocParticle(NULL)))
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
void CTempEntities::R_ParticleExplosion2( vec_t* org, int colorStart, int colorLength )
{
	int			i, j;
	particle_t* p;
	int			colorMod = 0;

	for (i = 0; i < 512; i++)
	{
		if (!(p = gEngfuncs.pEfxAPI->R_AllocParticle(NULL)))
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
void CTempEntities::R_BlobExplosion( vec_t* org )
{
	int			i, j;
	particle_t* p;

	for (i = 0; i < 1024; i++)
	{
		if (!(p = gEngfuncs.pEfxAPI->R_AllocParticle(NULL)))
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

/*
===============
R_RunParticleEffect
===============
*/
void CTempEntities::R_RunParticleEffect( vec_t* org, vec_t* dir, int color, int count )
{
	int		i, j;
	particle_t* p;

	for (i = 0; i < count; i++)
	{
		if (!(p = gEngfuncs.pEfxAPI->R_AllocParticle(NULL)))
			return;

		if (count == 1024)
		{
			// rocket explosion
			p->die = gEngfuncs.GetClientTime() + 5.0;
			p->color = ramp1[0];
			gEngfuncs.pEfxAPI->R_GetPackedColor(&p->packedColor, ramp1[0]);
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
R_BulletImpactParticles
===============
*/
void CTempEntities::R_BulletImpactParticles( vec_t* org )
{
	int		i, quantity;
	int		color;
	float		dist;
	vec3_t		dir;
	particle_t	*p;

	VectorSubtract( org, v_origin, dir );
	dist = Length( dir );
	if( dist > 1000.0f ) dist = 1000.0f;

	quantity = (1000.0f - dist) / 100.0f;
	if( quantity == 0 ) quantity = 1;

	color = 3 - ((30 * quantity) / 100 );
	R_SparkStreaks( org, 2, -200, 200 );

	for( i = 0; i < quantity * 4; i++ )
	{
		p = gEngfuncs.pEfxAPI->R_AllocParticle( NULL );
		if( !p ) return;

		VectorCopy( org, p->org);

		p->vel[0] = gEngfuncs.pfnRandomFloat( -1.0f, 1.0f );
		p->vel[1] = gEngfuncs.pfnRandomFloat( -1.0f, 1.0f );
		p->vel[2] = gEngfuncs.pfnRandomFloat( -1.0f, 1.0f );
		VectorScale( p->vel, gEngfuncs.pfnRandomFloat( 50.0f, 100.0f ), p->vel );

		p->die = gEngfuncs.GetClientTime() + 0.5;
		p->color = 3 - color;
		p->type = pt_grav;
	}
}

/*
===============
R_FlickerParticles
===============
*/
void CTempEntities::R_FlickerParticles( vec_t* org )
{
	int		i, j;
	particle_t* p;

	for (i = 0; i < 15; i++)
	{
		if (!(p = gEngfuncs.pEfxAPI->R_AllocParticle(NULL)))
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
R_SparkStreaks
===============
*/
void CTempEntities::R_SparkStreaks( vec_t* pos, int count, int velocityMin, int velocityMax )
{
	int		i, j;
	particle_t* p;

	for (i = 0; i < count; i++)
	{
		if (!(p = gEngfuncs.pEfxAPI->R_TracerParticles(pos, vec3_origin, 0.5)))
			return;

		p->color = 5;
		p->type = pt_grav;
		p->packedColor = 255;
		p->die = gEngfuncs.GetClientTime() + gEngfuncs.pfnRandomFloat(0.1, 0.5);
		p->ramp = 0.5;

		VectorCopy(pos, p->org);

		for (j = 0; j < 3; j++)
			p->vel[j] = gEngfuncs.pfnRandomFloat(velocityMin, velocityMax);
	}
}

/*
===============
R_StreakSplash
===============
*/
void CTempEntities::R_StreakSplash( vec_t* pos, vec_t* dir, int color, int count, float speed, int velocityMin, int velocityMax )
{
	int		i, j;
	particle_t* p;
	vec3_t	initialVelocity;

	VectorScale(dir, speed, initialVelocity);

	for (i = 0; i < count; i++)
	{
		if (!(p = gEngfuncs.pEfxAPI->R_TracerParticles(pos, vec3_origin, 0.5)))
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

/*
===============
R_Implosion
===============
*/
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

/*
===============
R_LavaSplash

===============
*/
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
				if (!(p = gEngfuncs.pEfxAPI->R_AllocParticle(NULL)))
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
R_ParticleBurst
===============
*/
void CTempEntities::R_ParticleBurst( vec_t* org, int size, int color, float life )
{
	particle_t	*p;
	vec3_t		dir, dest;
	int		i, j;
	float		dist;

	for( i = 0; i < 32; i++ )
	{
		for( j = 0; j < 32; j++ )
		{
			p = gEngfuncs.pEfxAPI->R_AllocParticle( NULL );
			if( !p ) return;

			p->die = gEngfuncs.GetClientTime() + life + gEngfuncs.pfnRandomFloat( -0.5f, 0.5f );
			p->color = color + gEngfuncs.pfnRandomLong( 0, 10 );
			p->ramp = 1.0f;

			VectorCopy( org, p->org );

			dest[0] = gEngfuncs.pfnRandomFloat(-size, size) + org[0];
			dest[1] = gEngfuncs.pfnRandomFloat(-size, size) + org[1];
			dest[2] = gEngfuncs.pfnRandomFloat(-size, size) + org[2];

			VectorSubtract( dest, p->org, dir );
			dist = VectorNormalize( dir );
			VectorScale( dir, ( dist / life ), p->vel );
		}
	}
}

/*
===============
R_LargeFunnel
===============
*/
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
			if (!(p = gEngfuncs.pEfxAPI->R_AllocParticle(NULL)))
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
				if (!(p = gEngfuncs.pEfxAPI->R_AllocParticle(NULL)))
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

		if (!(p = gEngfuncs.pEfxAPI->R_AllocParticle(NULL)))
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
		if (!(p = gEngfuncs.pEfxAPI->R_AllocParticle(NULL)))
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
		if (!(p = gEngfuncs.pEfxAPI->R_AllocParticle(NULL)))
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
			if (!(p = gEngfuncs.pEfxAPI->R_AllocParticle(NULL)))
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
			if (!(p = gEngfuncs.pEfxAPI->R_AllocParticle(NULL)))
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

		if (!(p = gEngfuncs.pEfxAPI->R_AllocParticle(NULL)))
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

//-----------------------------------------------------------------------------
//
// Beams
//
//-----------------------------------------------------------------------------

void CTempEntities::R_BeamSetup( BEAM* pbeam, vec_t* start, vec_t* end, int modelIndex, float life, float width, float amplitude, float brightness, float speed )
{
	model_t* sprite;

	sprite = IEngineStudio.GetModelByIndex(modelIndex);
	if (!sprite)
		return;

	pbeam->type = TE_BEAMPOINTS;
	pbeam->modelIndex = modelIndex;
	pbeam->frame = 0;
	pbeam->frameRate = 0;
	pbeam->frameCount = ModelFrameCount(sprite);

	VectorCopy(start, pbeam->source);
	VectorCopy(end, pbeam->target);
	VectorSubtract(end, start, pbeam->delta);

	pbeam->freq = gEngfuncs.GetClientTime() * speed;
	pbeam->die = gEngfuncs.GetClientTime() + life;
	pbeam->width = width;
	pbeam->amplitude = amplitude;
	pbeam->brightness = brightness;
	pbeam->speed = speed;

	if (amplitude >= 0.5)
		pbeam->segments = Length(pbeam->delta) * 0.25 + 3;	// once per 4 pixels
	else
		pbeam->segments = Length(pbeam->delta) * 0.075 + 3; // once per 16 pixels

	pbeam->flags = 0;
}

void CTempEntities::SetBeamAttributes( BEAM* pbeam, float r, float g, float b, float framerate, int startFrame )
{
	pbeam->frameRate = framerate;
	pbeam->frame = startFrame;

	pbeam->r = r;
	pbeam->g = g;
	pbeam->b = b;
}

BEAM* CTempEntities::R_BeamEnts( int startEnt, int endEnt, int modelIndex, float life, float width, float amplitude, float brightness, float speed, int startFrame, float framerate, float r, float g, float b )
{
	BEAM* pbeam;
	cl_entity_t* start, * end;

	if (life != 0.0)
	{
		start = gEngfuncs.GetEntityByIndex(BEAMENT_ENTITY(startEnt));
		if (!start->model)
			return NULL;

		end = gEngfuncs.GetEntityByIndex(BEAMENT_ENTITY(endEnt));
		if (!end->model)
			return NULL;
	}

	pbeam = gEngfuncs.pEfxAPI->R_BeamLightning(vec3_origin, vec3_origin, modelIndex, life, width, amplitude, brightness, speed);
	if (!pbeam)
		return NULL;

	pbeam->type = TE_BEAMPOINTS;
	pbeam->flags = (FBEAM_STARTENTITY | FBEAM_ENDENTITY);

	if (life == 0.0)
	{
		pbeam->flags |= FBEAM_FOREVER;
	}

	pbeam->startEntity = startEnt;
	pbeam->endEntity = endEnt;

	SetBeamAttributes(pbeam, r, g, b, framerate, startFrame);

	return pbeam;
}

// Creates a beam between an entity and a point
BEAM* CTempEntities::R_BeamEntPoint( int startEnt, vec_t* end, int modelIndex, float life, float width, float amplitude, float brightness, float speed, int startFrame, float framerate, float r, float g, float b )
{
	BEAM* pbeam;
	cl_entity_t* start;

	if (life != 0.0)
	{
		start = gEngfuncs.GetEntityByIndex(BEAMENT_ENTITY(startEnt));
		if (!start->model)
			return NULL;
	}

	pbeam = gEngfuncs.pEfxAPI->R_BeamLightning(vec3_origin, end, modelIndex, life, width, amplitude, brightness, speed);
	if (!pbeam)
		return NULL;

	pbeam->type = TE_BEAMPOINTS;
	pbeam->flags = FBEAM_STARTENTITY;

	if (life == 0.0)
	{
		pbeam->flags |= FBEAM_FOREVER;
	}

	pbeam->startEntity = startEnt;
	pbeam->endEntity = 0;

	SetBeamAttributes(pbeam, r, g, b, framerate, startFrame);

	return pbeam;
}

BEAM* CTempEntities::R_BeamCirclePoints( int type, vec_t* start, vec_t* end, int modelIndex, float life, float width, float amplitude, float brightness, float speed, int startFrame, float framerate, float r, float g, float b )
{
	BEAM* pbeam;

	pbeam = gEngfuncs.pEfxAPI->R_BeamLightning(start, end, modelIndex, life, width, amplitude, brightness, speed);
	if (!pbeam)
		return NULL;

	pbeam->type = type;

	if (life == 0.0)
	{
		pbeam->flags |= FBEAM_FOREVER;
	}

	SetBeamAttributes(pbeam, r, g, b, framerate, startFrame);

	return pbeam;
}

BEAM* CTempEntities::R_BeamFollow( int startEnt, int modelIndex, float life, float width, float r, float g, float b, float brightness )
{
	BEAM* pbeam;

	pbeam = gEngfuncs.pEfxAPI->R_BeamLightning(vec3_origin, vec3_origin, modelIndex, life, width, life, brightness, 1.0);
	if (!pbeam)
		return NULL;

	pbeam->type = TE_BEAMFOLLOW;
	pbeam->flags = FBEAM_STARTENTITY;

	pbeam->startEntity = startEnt;

	SetBeamAttributes(pbeam, r, g, b, 1.0, 0);

	return pbeam;
}

// Create a beam ring between two entities
BEAM* CTempEntities::R_BeamRing( int startEnt, int endEnt, int modelIndex, float life, float width, float amplitude, float brightness, float speed, int startFrame, float framerate, float r, float g, float b )
{
	BEAM* pbeam;
	cl_entity_t* start, * end;

	if (life != 0.0)
	{
		start = gEngfuncs.GetEntityByIndex(startEnt);
		if (!start->model)
			return NULL;

		end = gEngfuncs.GetEntityByIndex(endEnt);
		if (!end->model)
			return NULL;
	}

	pbeam = gEngfuncs.pEfxAPI->R_BeamLightning(vec3_origin, vec3_origin, modelIndex, life, width, amplitude, brightness, speed);
	if (!pbeam)
		return NULL;

	pbeam->type = TE_BEAMRING;
	pbeam->flags = (FBEAM_STARTENTITY | FBEAM_ENDENTITY);

	if (life == 0.0)
	{
		pbeam->flags |= FBEAM_FOREVER;
	}

	pbeam->startEntity = startEnt;
	pbeam->endEntity = endEnt;
	
	SetBeamAttributes(pbeam, r, g, b, framerate, startFrame);

	return pbeam;
}

//-----------------------------------------------------------------------------
//
// Temporary entities
//
//-----------------------------------------------------------------------------

void CTempEntities::R_RicochetSound(vec_t* pos)
{
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
	default:
		break;
	}
}

/*
===============
R_TracerEffect

===============
*/
void CTempEntities::R_TracerEffect( vec_t* start, vec_t* end )
{
	vec3_t	temp, vel;
	float	len;

	if (m_pCvarTracerSpeed->value <= 0)
		m_pCvarTracerSpeed->value = 3;

	VectorSubtract(end, start, temp);
	len = Length(temp);

	VectorScale(temp, 1.0 / len, temp);
	VectorScale(temp, gEngfuncs.pfnRandomLong(-10, 9) + m_pCvarTracerOffset->value, vel);
	VectorAdd(start, vel, start);
	VectorScale(temp, m_pCvarTracerSpeed->value, vel);

	gEngfuncs.pEfxAPI->R_TracerParticles(start, vel, len / m_pCvarTracerSpeed->value);
}

/*
===============
R_UserTracerParticle

===============
*/
void R_UserTracerParticle( float *org, float *vel, float life, int colorIndex, float length, byte deathcontext, void (*deathfunc)( particle_t *p ))
{
	particle_t	*p;

	if( colorIndex < 0 )
		return;

	if(( p = gEngfuncs.pEfxAPI->R_TracerParticles( org, vel, life )) != NULL )
	{
		p->context = deathcontext;
		p->deathfunc = deathfunc;
		p->color = colorIndex;
		p->ramp = length;
	}
}

/*
===============
R_FizzEffect

Create a fizz effect
===============
*/
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
	if (!model)
		return;

	count = density + 1;

	maxHeight = pent->model->maxs[2] - pent->model->mins[2];
	width = pent->model->maxs[0] - pent->model->mins[0];
	depth = pent->model->maxs[1] - pent->model->mins[1];
	speed = (float)pent->curstate.rendercolor.r * 256.0 + (float)pent->curstate.rendercolor.g;
	if (pent->curstate.rendercolor.b)
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

/*
===============
R_Bubbles

Create bubbles
===============
*/
void CTempEntities::R_Bubbles( vec_t* mins, vec_t* maxs, float height, int modelIndex, int count, float speed )
{
	TEMPENTITY* pTemp;
	model_t* model;
	int					i, frameCount;
	float				angle;
	vec3_t				origin;

	if (!modelIndex)
		return;

	IEngineStudio.GetModelByIndex(modelIndex);

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

/*
===============
R_BubbleTrail

Create bubble trail
===============
*/
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

/*
===============
R_Sprite_Trail

Line of moving glow sprites with gravity, fadeout, and collisions
===============
*/
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

/*
==============
R_Projectile

Create an projectile entity
==============
*/
void R_Projectile( vec_t* origin, vec_t* velocity, int modelIndex, int life, int owner, void (*hitcallback)( TEMPENTITY*, pmtrace_t* ))
{
	TEMPENTITY	*pTemp;
	model_t		*pmodel;
	vec3_t		dir;

	if(( pmodel = IEngineStudio.GetModelByIndex( modelIndex )) == NULL )
		return;

	pTemp = gEngfuncs.pEfxAPI->CL_TempEntAllocHigh( origin, pmodel );
	if( !pTemp ) return;

	VectorCopy( velocity, pTemp->entity.baseline.origin );

	if( pmodel->type == mod_sprite )
	{
		pTemp->flags |= FTENT_SPRANIMATE;

		if( pTemp->frameMax < 10 )
		{
			pTemp->flags |= FTENT_SPRANIMATE | FTENT_SPRANIMATELOOP;
			pTemp->entity.curstate.framerate = 10;
		}
		else
		{
			pTemp->entity.curstate.framerate = pTemp->frameMax / life;
		}
	}
	else
	{
		pTemp->frameMax = 0;

		VectorCopy(velocity, dir);
		VectorNormalize( dir );
		VectorAngles( dir, pTemp->entity.angles );
	}

	pTemp->flags |= FTENT_COLLIDEALL|FTENT_PERSIST|FTENT_COLLIDEKILL;
	pTemp->clientIndex = owner;
	pTemp->entity.baseline.renderamt = 255;
	pTemp->hitcallback = hitcallback;
	pTemp->die = gEngfuncs.GetClientTime() + life;
}

/*
===============
R_TempSphereModel

Spherical shower of models, picks from set
===============
*/
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

/*
===============
R_BreakModel

Create model shattering shards
===============
*/
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

/*
====================
R_TempSprite

Create sprite TE
====================
*/
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

/*
===============
R_Sprite_Spray

Spray sprite
===============
*/
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

/*
===============
R_Spray
===============
*/
void CTempEntities::R_Spray( vec_t* pos, vec_t* dir, int modelIndex, int count, int speed, int iRand, int rendermode )
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

		pTemp->entity.curstate.rendermode = rendermode;
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

void CTempEntities::R_SparkEffect( float* pos, int count, int velocityMin, int velocityMax )
{
	R_SparkStreaks(pos, count, velocityMin, velocityMax);
	R_RicochetSprite(pos, cl_sprite_ricochet, 0.1, gEngfuncs.pfnRandomFloat(0.5, 1.0));
}

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

/*
=================
R_RicochetSprite

Create ricochet sprite
=================
*/
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

/*
===============
R_BloodSprite

Create blood sprite
===============
*/
void CTempEntities::R_BloodSprite( vec_t* org, int colorindex, int modelIndex, int modelIndex2, float size )
{
	// i could load the palette directly into the dll but that feels dirty so i'm just gonna
	// call the engine version instead...
	gEngfuncs.pEfxAPI->R_BloodSprite(org, colorindex, modelIndex, modelIndex2, size);
#if 0
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
	if (modelIndex2 && (model2 = cl.model_precache[modelIndex2]))
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
	if (modelIndex && (model = cl.model_precache[modelIndex]))
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
#endif
}

/*
=================
R_DefaultSprite

Create default sprite TE
=================
*/
TEMPENTITY* CTempEntities::R_DefaultSprite( float* pos, int spriteIndex, float framerate )
{
	TEMPENTITY* pTemp;
	int				frameCount;
	model_t* pSprite;

	// don't spawn while paused
	if (gEngfuncs.GetClientTime() == g_StudioRenderer.m_clOldTime)
		return NULL;

	pSprite = IEngineStudio.GetModelByIndex(spriteIndex);

	if (!spriteIndex || !pSprite || pSprite->type != mod_sprite)
	{
		gEngfuncs.Con_DPrintf("No Sprite %d!\n", spriteIndex);
		return NULL;
	}

	frameCount = ModelFrameCount(IEngineStudio.GetModelByIndex(spriteIndex));

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

/*
===============
R_Sprite_Smoke

Create sprite smoke
===============
*/
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

/*
===============
R_Sprite_Explode

Create explosion sprite
===============
*/
void CTempEntities::R_Sprite_Explode( TEMPENTITY* pTemp, float scale, int flags )
{
	qboolean noadditive, drawalpha, rotate;

	if (!pTemp)
		return;

	noadditive = flags & TE_EXPLFLAG_NOADDITIVE;
	drawalpha  = flags & TE_EXPLFLAG_DRAWALPHA;
	rotate     = flags & TE_EXPLFLAG_ROTATE;

	pTemp->entity.curstate.rendermode = noadditive ? kRenderNormal :
		drawalpha ? kRenderTransAlpha : kRenderTransAdd;
	pTemp->entity.curstate.renderfx = kRenderFxNone;
	pTemp->entity.curstate.rendercolor.r = 0;
	pTemp->entity.curstate.rendercolor.g = 0;
	pTemp->entity.curstate.rendercolor.b = 0;
	pTemp->entity.curstate.scale = scale;
	pTemp->entity.origin[2] += 10;
	pTemp->entity.curstate.renderamt = 180;

	pTemp->entity.baseline.origin[2] = 8.0;
}

/*
==============
R_PlayerSprites

Create a particle smoke around player
==============
*/
void CTempEntities::R_PlayerSprites( int client, int modelIndex, int count, int size )
{
	TEMPENTITY	*pTemp;
	cl_entity_t	*pEnt;
	vec3_t		position;
	vec3_t		dir;
	float		vel;
	int		i;

	pEnt = gEngfuncs.GetEntityByIndex( client );

	if( !pEnt || !pEnt->player )
		return;

	vel = 128;

	for( i = 0; i < count; i++ )
	{
		VectorCopy( pEnt->origin, position );
		position[0] += gEngfuncs.pfnRandomFloat( -10.0f, 10.0f );
		position[1] += gEngfuncs.pfnRandomFloat( -10.0f, 10.0f );
		position[2] += gEngfuncs.pfnRandomFloat( -20.0f, 36.0f );

		pTemp = gEngfuncs.pEfxAPI->CL_TempEntAlloc( position, IEngineStudio.GetModelByIndex( modelIndex ));
		if( !pTemp ) return;

		VectorSubtract( pTemp->entity.origin, pEnt->origin, pTemp->tentOffset );

		if ( i != 0 )
		{
			pTemp->flags |= FTENT_PLYRATTACHMENT;
			pTemp->clientIndex = client;
		}
		else
		{
			VectorSubtract( position, pEnt->origin, dir );
			VectorNormalize( dir );
			VectorScale( dir, 60, dir );
			VectorCopy( dir, pTemp->entity.baseline.origin );
			pTemp->entity.baseline.origin[1] = gEngfuncs.pfnRandomFloat( 20.0f, 60.0f );
		}

		pTemp->entity.curstate.renderfx = kRenderFxNoDissipation;
		pTemp->entity.curstate.framerate = gEngfuncs.pfnRandomFloat( 1.0f - (size / 100.0f ), 1.0f );

		if( pTemp->frameMax > 1 )
		{
			pTemp->flags |= FTENT_SPRANIMATE;
			pTemp->entity.curstate.framerate = 20.0f;
			pTemp->die = gEngfuncs.GetClientTime() + (pTemp->frameMax * 0.05f);
		}
		else
		{
			pTemp->die = gEngfuncs.GetClientTime() + 0.35f;
		}
	}
}

/*
==============
R_FireField

Makes a field of fire
==============
*/
void CTempEntities::R_FireField( float *org, int radius, int modelIndex, int count, int flags, float life )
{
	TEMPENTITY	*pTemp;
	model_t		*pmodel;
	float		time;
	vec3_t		pos;
	int		i;

	if(( pmodel = IEngineStudio.GetModelByIndex( modelIndex )) == NULL )
		return;

	for( i = 0; i < count; i++ )
	{
		VectorCopy( org, pos );
		pos[0] += gEngfuncs.pfnRandomFloat( -radius, radius );
		pos[1] += gEngfuncs.pfnRandomFloat( -radius, radius );

		if( !( flags & TEFIRE_FLAG_PLANAR ))
			pos[2] += gEngfuncs.pfnRandomFloat( -radius, radius );

		pTemp = gEngfuncs.pEfxAPI->CL_TempEntAlloc( pos, pmodel );
		if( !pTemp ) return;

		if (flags & TEFIRE_FLAG_ALPHA)
		{
			pTemp->entity.curstate.rendermode = kRenderTransAlpha;
			pTemp->entity.curstate.renderfx = kRenderFxNoDissipation;
			pTemp->entity.baseline.renderamt = pTemp->entity.curstate.renderamt = 128;
		}
		else if (flags & TEFIRE_FLAG_ADDITIVE)
		{
			pTemp->entity.curstate.rendermode = kRenderTransAdd;
			pTemp->entity.curstate.renderamt = 180;
		}
		else
		{
			pTemp->entity.curstate.rendermode = kRenderNormal;
			pTemp->entity.curstate.renderfx = kRenderFxNoDissipation;
			pTemp->entity.baseline.renderamt = pTemp->entity.curstate.renderamt = 255;
		}

		pTemp->entity.curstate.framerate = gEngfuncs.pfnRandomFloat( 0.75f, 1.25f );
		time = life + gEngfuncs.pfnRandomFloat( -0.25f, 0.5f );
		pTemp->die = gEngfuncs.GetClientTime() + time;

		if( pTemp->frameMax > 1 )
		{
			pTemp->flags |= FTENT_SPRANIMATE;

			if(flags & TEFIRE_FLAG_LOOP)
			{
				pTemp->entity.curstate.framerate = 15.0f;
				pTemp->flags |= FTENT_SPRANIMATELOOP;
			}
			else
			{
				pTemp->entity.curstate.framerate = pTemp->frameMax / time;
			}
		}

		if( ( flags & TEFIRE_FLAG_ALLFLOAT ) || ( flags & TEFIRE_FLAG_SOMEFLOAT ) && !gEngfuncs.pfnRandomLong( 0, 1 ))
		{
			// drift sprite upward
			pTemp->entity.baseline.origin[2] = gEngfuncs.pfnRandomFloat( 10.0f, 30.0f );
		}
	}
}

/*
==============
R_MultiGunshot

Client version of shotgun shot
==============
*/
void CTempEntities::R_MultiGunshot( vec_t* org, vec_t* dir, vec_t* noise, int count, int decalCount, int *decalIndices )
{
	pmtrace_t*	trace;
	vec3_t	right, up;
	vec3_t	vecSrc, vecDir, vecEnd, ang;
	int	i, j, decalIndex;

	VectorAngles(dir, ang);
	AngleVectors(ang, NULL, right, up);
	VectorCopy( org, vecSrc );

	for( i = 0; i < count; i++ )
	{
		// get circular gaussian spread
		float x, y, z;
		do {
			x = gEngfuncs.pfnRandomFloat( -0.5f, 0.5f ) + gEngfuncs.pfnRandomFloat( -0.5f, 0.5f );
			y = gEngfuncs.pfnRandomFloat( -0.5f, 0.5f ) + gEngfuncs.pfnRandomFloat( -0.5f, 0.5f );
			z = x * x + y * y;
		} while( z > 1.0f );

		for( j = 0; j < 3; j++ )
		{
			vecDir[j] = dir[j] + x * noise[0] * right[j] + y * noise[1] * up[j];
			vecEnd[j] = vecSrc[j] + 4096.0f * vecDir[j];
		}

		trace = gEngfuncs.PM_TraceLine(vecSrc, vecEnd, PM_TRACELINE_ANYVISIBLE, 2, PM_STUDIO_IGNORE);

		// paint decals
		if( trace->fraction != 1.0f )
		{
			physent_t	*pe = NULL;

			if( i & 2 ) 
				R_RicochetSound( trace->endpos );

			R_BulletImpactParticles( trace->endpos );

			if( pe && ( pe->solid == SOLID_BSP || pe->movetype == MOVETYPE_PUSHSTEP ))
			{
				cl_entity_t *e = gEngfuncs.GetEntityByIndex( pe->info );
				decalIndex = gEngfuncs.pEfxAPI->Draw_DecalIndex( decalIndices[gEngfuncs.pfnRandomLong( 0, decalCount-1 )] );
				gEngfuncs.pEfxAPI->R_DecalShoot( decalIndex, e->index, 0, trace->endpos, 0 );
			}
		}
	}
}


/*
===============
R_Sprite_WallPuff

Create a wallpuff
===============
*/
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

/*
===============
R_MuzzleFlash

Play muzzle flash
===============
*/
void CTempEntities::R_MuzzleFlash( float* pos1, int type )
{
	// This does not work in software mode. Fallback to engine version!
	if (!IEngineStudio.IsHardware())
	{
		gEngfuncs.pEfxAPI->R_MuzzleFlash(pos1, type);
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

	//Con_DPrintf("%d %f\n", index, scale);
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
	pTemp->die = gEngfuncs.GetClientTime() + 0.01;
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

	gEngfuncs.CL_CreateVisibleEntity(ET_TEMPENTITY, &pTemp->entity);
	//AppendTEntity(&pTemp->entity);
}

/*
=================
CL_ParseTEnt
=================
*/
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
			R_BeamEnts(startEnt, endEnt, modelindex, life, width, amplitude, a, flSpeed, startFrame, frameRate, r, g, b);
			break;
		case TE_BEAMENTPOINT:
			R_BeamEntPoint(startEnt, endpos, modelindex, life, width, amplitude, a, flSpeed, startFrame, frameRate, r, g, b);
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
			{
				R_FlickerParticles(pos);
			}

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
		R_Sprite_Smoke(R_DefaultSprite(pos, modelindex, frameRate), scale);
		break;

	case TE_TRACER:
		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();
		endpos[0] = READ_COORD();
		endpos[1] = READ_COORD();
		endpos[2] = READ_COORD();
		R_TracerEffect(pos, endpos);
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

		//if (entnumber >= cl.max_edicts)
		if (gEngfuncs.GetEntityByIndex(entnumber) == NULL)
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

		R_BeamCirclePoints(type, pos, endpos, modelindex, life, width, amplitude, a, flSpeed, startFrame, frameRate, r, g, b);
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

		R_BeamFollow(startEnt, modelindex, life, width, r, g, b, a);
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

		R_BeamRing(startEnt, endEnt, modelindex, life, width, amplitude, a, flSpeed, startFrame, frameRate, r, g, b);
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
	case TE_BOX:
		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();
		dir[0] = READ_COORD();
		dir[1] = READ_COORD();
		dir[2] = READ_COORD();
		life = (float)(READ_SHORT() * 0.1);
		r = READ_BYTE();
		g = READ_BYTE();
		b = READ_BYTE();
		if( type == TE_LINE ) gEngfuncs.pEfxAPI->R_ParticleLine( pos, dir, r, g, b, life );
		else gEngfuncs.pEfxAPI->R_ParticleBox( pos, dir, r, g, b, life );
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
		R_FunnelSprite(pos, modelindex, flags);
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
		//if (entnumber >= cl.max_edicts)
		if (gEngfuncs.GetEntityByIndex(entnumber) == NULL)
		{
			gEngfuncs.Con_Printf("Bubble: entity = %i", entnumber);
			break;
		}

		modelindex = READ_SHORT();
		density = READ_BYTE();
		R_FizzEffect(gEngfuncs.GetEntityByIndex(entnumber), modelindex, density);
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

		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();

		entnumber = READ_SHORT();
		decalTextureIndex = READ_BYTE();	

		R_BulletImpactParticles(pos);

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

		//if (entnumber >= cl.max_edicts)
		if (gEngfuncs.GetEntityByIndex(entnumber) == NULL)
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

	case TE_SPRAY:
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
		if (type == TE_SPRAY)
		{
			flags = READ_BYTE(); // rendermode
			R_Spray(pos, dir, modelindex, count, speed * 2, iRand, flags);
		}
		else
			R_Sprite_Spray(pos, dir, modelindex, count, speed * 2, iRand);
		break;

	case TE_ARMOR_RICOCHET:
		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();
		scale = READ_BYTE() * 0.1;
		R_RicochetSprite(pos, cl_sprite_ricochet, 0.1, scale);
		R_RicochetSound(pos);
		break;
		
	case TE_PLAYERDECAL:
	{
		static int decalTextureIndex;
		int playernum;
		customization_t* pCust = NULL;

		flags = (type == TE_BSPDECAL) ? FDECAL_PERMANENT : 0;

		playernum = READ_BYTE() - 1;
		if (playernum < MAX_CLIENTS)
			pCust = IEngineStudio.PlayerInfo(playernum)->customdata.pNext;

		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();

		entnumber = READ_SHORT();
		decalTextureIndex = READ_BYTE();

		if (entnumber && type == TE_BSPDECAL)
		{
			modelindex = READ_SHORT();
		}
		else
		{
			modelindex = 0;
		}

		//if (entnumber >= cl.max_edicts)
		if (gEngfuncs.GetEntityByIndex(entnumber) == NULL)
		{
			gEngfuncs.Con_Printf("Decal: entity = %i", entnumber);
			break;
		}

		if (pCust && pCust->pBuffer)
		{
			cachewad_t* pwad;

			pwad = (cachewad_t*)pCust->pInfo;
			if (pwad)
			{
				if ((pCust->resource.ucFlags & RES_CUSTOM) && pCust->resource.type == t_decal && pCust->bTranslated)
				{
					if (decalTextureIndex > pwad->lumpCount - 1)
						decalTextureIndex = pwad->lumpCount - 1;

					pCust->nUserData1 = decalTextureIndex;
					//pTexture = (texture_t*)Draw_CustomCacheGet(pwad, pCust->pBuffer, decalTextureIndex);
				}
			}
		}

		if (m_pCvarDecals->value)
		{
			if (pCust)
			{
				gEngfuncs.pEfxAPI->R_DecalShoot(gEngfuncs.pEfxAPI->Draw_DecalIndex(decalTextureIndex), entnumber, modelindex, pos, flags);
			}
			else
			{
				gEngfuncs.pEfxAPI->R_DecalShoot(gEngfuncs.pEfxAPI->Draw_DecalIndex(decalTextureIndex), entnumber, modelindex, pos, flags);
			}
		}
		break;
	}

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
	{
		int playernum;

		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();

		dir[0] = READ_COORD();
		dir[1] = READ_COORD();
		dir[2] = READ_COORD();

		modelindex = READ_SHORT();
		life = READ_BYTE();
		playernum = READ_BYTE();

		R_Projectile(pos, dir, modelindex, life, playernum, NULL);
		break;
	}

	case TE_PLAYERSPRITES:
	{
		color = READ_SHORT();
		modelindex = READ_SHORT();
		count = READ_BYTE();
		iRand = (float)READ_BYTE();
		R_PlayerSprites( color, modelindex, count, iRand );
		break;
	}

	case TE_PARTICLEBURST:
	{
		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();

		scale = (float)READ_SHORT();
		color = READ_BYTE();
		life = (float)(READ_BYTE() * 0.1);

		R_ParticleBurst(pos, scale, color, life);
		break;
	}

	case TE_FIREFIELD:
	{
		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();
		scale = (float)READ_SHORT();
		modelindex = READ_SHORT();
		count = READ_BYTE();
		flags = READ_BYTE();
		life = (float)(READ_BYTE( ) * 0.1f);
		R_FireField( pos, scale, modelindex, count, flags, life );
		break;
	}

	case TE_PLAYERATTACHMENT:
	{
		entnumber = READ_BYTE();	// playernum
		scale = READ_BYTE();	// height
		modelindex = READ_SHORT();
		life = (float)(READ_SHORT() * 0.1f);
		gEngfuncs.pEfxAPI->R_AttachTentToPlayer( entnumber, modelindex, scale, life );
		break;
	}

	case TE_KILLPLAYERATTACHMENTS:
	{
		entnumber = READ_BYTE();	// playernum
		gEngfuncs.pEfxAPI->R_KillAttachedTents( entnumber );
		break;
	}

	case TE_MULTIGUNSHOT:
	{
		int index_array[1];
		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();

		dir[0] = READ_COORD() * 0.1f;
		dir[1] = READ_COORD() * 0.1f;
		dir[2] = READ_COORD() * 0.1f;

		size[0] = READ_COORD() * 0.01f;
		size[1] = READ_COORD() * 0.01f;
		size[2] = 0.0f;

		count = READ_BYTE();
		index_array[0] = READ_BYTE();

		R_MultiGunshot(pos, dir, size, count, 1, index_array);
		break;
	}

	case TE_USERTRACER:
	{
		pos[0] = READ_COORD();
		pos[1] = READ_COORD();
		pos[2] = READ_COORD();

		dir[0] = READ_COORD();
		dir[1] = READ_COORD();
		dir[2] = READ_COORD();

		life = (float)(READ_BYTE() * 0.1f);
		color = READ_BYTE();
		scale = (float)(READ_BYTE() * 0.1f);

		R_UserTracerParticle(pos, dir, life, color, scale, 0, NULL);
		break;
	}

	default:
		gEngfuncs.Con_Printf("CL_ParseTEnt: bad type");
		break;
	}
	return 1;
}