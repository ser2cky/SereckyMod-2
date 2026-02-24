
//=======================================
//	particle_dan3.cpp
// 
//	Purpose: Latest version of my "ParticleDan"
//	system. Particles can now be read from .pdan
//	scripts
//
//	History:
//	FEB-21-26: Started. Redid all the emitter and
//	particle structs from version2, and got rid of
//	the flags field in favour of typedef enums for
//	specific behaviours that are easier to deal with
//	in scripts.
//	FEB-22-26: 
//
//=======================================

#include "../hud.h"
#include "../cl_util.h"
#include "triangleapi.h"
#include "com_model.h"
#include "r_studioint.h"
#include "../parsemsg.h"
#include "pm_defs.h"
#include "particledan.h"

extern engine_studio_api_t IEngineStudio;
CParticleDan	gParticleDan;

//===============================
//	Color24ToVector
//===============================

vec3_t Color24ToVector( color24 color )
{
	vec3_t vecColor;

	vecColor[0] = color.r * ( 1.0f / 255.0f );
	vecColor[1] = color.g * ( 1.0f / 255.0f );
	vecColor[2] = color.b * ( 1.0f / 255.0f );

	return vecColor;
}

//===============================
//	IsAnimationDoneYet
//===============================

int IsAnimationDoneYet( float current, float max, float direction )
{
	if ( ( current >= max ) && ( direction == INCREASING ) )
		return 1;
	if ( ( current <= max ) && ( direction == DECREASING ) )
		return 1;
	return 0;
}

//===============================
//	GetAnimDirection
//===============================

void GetAnimDirection( float current, float max, int* dir ) 
{ 
	if ( current >= max )
	{
		*dir = DECREASING;
		gEngfuncs.Con_Printf("increasing\n");
	}
	if ( current <= max )
	{
		*dir = INCREASING;
		gEngfuncs.Con_Printf("decreasing\n");
	}
}

//===============================
//	GetParticlePointer
//===============================

int CParticleDan::Init( void )
{
	m_iShownYet = 0;
	m_pCvarTestParticles = CVAR_CREATE( "test_particles", "0", FCVAR_ARCHIVE );
	return 1;
}

//===============================
//	VidInit
//===============================

int CParticleDan::VidInit( void )
{
	m_iActiveParticles = 0;

	memset( m_Particles, 0, sizeof(m_Particles) );

	for (int i = 0; i < MAX_PARTICLES; i++)
		m_Particles[i].alive = 0;

	return 1;
}

//===============================
//	MsgFunc_AddEmitter
//===============================

int CParticleDan::MsgFunc_AddEmitter( const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	return 1;
}

//===============================
//	GetParticlePointer
//===============================

partdan_t *CParticleDan::GetParticlePointer( void )
{
	int i;

	for ( i = 0; i < MAX_PARTICLES; i++ )
	{
		if ( !m_Particles[i].alive )
		{
			memset(&m_Particles[i], 0, sizeof(m_Particles[i]));
			m_iActiveParticles++;
			m_Particles[i].alive = 1;
			return &m_Particles[i];
		}
		
	}

	gEngfuncs.Con_Printf("ParticleDan: Unable to grab a particle pointer! Particle limit is 8192!");

	return NULL;
}

//===============================
//	ParseScript
//===============================

void CParticleDan::ParseScript( void )
{

}

//===============================
//	ManageParticles
//===============================

void CParticleDan::ManageParticles( void )
{
	int i;

	for ( i = 0; i < MAX_PARTICLES; i++ )
	{
		if ( m_Particles[i].alive )
		{
			ParticleThink( &m_Particles[i] );
			ParticleDraw( &m_Particles[i] );

			if ( m_Particles[i].ltime <= 0.0f )
			{
				m_Particles[i].alive = 0;
				m_iActiveParticles--;
				memset( &m_Particles[i], 0, sizeof(m_Particles[i]) );
			}
		}
	}

	if (m_pCvarTestParticles->value)
		CreateTestParticles();
}

//===============================
//	ParticleThink
//===============================

void CParticleDan::ParticleThink( partdan_t* p )
{
	// Move our particle around.

	VectorCopy( p->org, p->old_org );

	VectorMA( p->vel, gHUD.m_flTimeDelta, p->accel, p->vel );
	VectorMA( p->org, gHUD.m_flTimeDelta, p->vel, p->org );
	p->ang += p->angvel * gHUD.m_flTimeDelta;

	// Get animation directions if we haven't already.
	if ( !p->anim_dir ) GetAnimDirection( p->frame, p->frame_max, &p->anim_dir );
	if ( !p->scale_dir ) GetAnimDirection( p->scale.x, p->scale_max.x, &p->scale_dir);
	if ( !p->alpha_dir ) GetAnimDirection( p->alpha, p->alpha_max, &p->alpha_dir);

	// Scaling
	if ( p->scale_think != DONT_SCALE )
	{
		p->scale.x += p->scale_step.x * gHUD.m_flTimeDelta;
		p->scale.y += p->scale_step.y * gHUD.m_flTimeDelta;

		if ( IsAnimationDoneYet( p->scale.x, p->scale_max.x, p->anim_dir ) )
			p->anim_finished = 1;

		switch ( p->scale_think )
		{
			case SCALE_ONCE:
			{
				if ( p->anim_finished )
					p->scale_step = { 0, 0 };
				break;
			}
			case SCALE_DIE:
			{
				if ( p->anim_finished )
					p->ltime = 0.0f;
			}
		}
	}

	// Animating
	if ( p->anim_think != DONT_ANIMATE )
	{
		if ( p->nextanim < gHUD.m_flTime )
		{
			p->frame += 1 * p->anim_dir;
			p->nextanim = gHUD.m_flTime + ( 1.0f / (float)p->fps );
		}

		switch ( p->anim_think )
		{
			case ANIMATE_ONCE:
			{
				if ( p->frame > p->frame_max )
					p->frame = p->frame_max;
				break;
			}
			case ANIMATE_DIE:
			{
				if ( p->frame > p->frame_max )
					p->ltime = 0.0f;
				break;
			}
			case ANIMATE_LOOP:
			{
				if ( p->frame > p->frame_max )
					p->frame = 0;
				break;
			}
		}
	}

	// Fading
	if ( p->alpha_think != DONT_FADE )
	{
		p->alpha += p->alpha_step * gHUD.m_flTimeDelta;

		if ( IsAnimationDoneYet( p->alpha, p->alpha_max, p->alpha_dir ) )
		{
			p->alpha_finished = 1;
			gEngfuncs.Con_Printf("ParticleDan: Finished fading\n");
		}

		switch ( p->alpha_think )
		{
			case FADE_DIE:
			{
				if ( p->alpha_finished )
					p->ltime = 0.0f;
				break;
			}
			case FADE_LOOP:
			{
				if ( p->alpha_finished )
					p->alpha = 0.0f;
				break;
			}
			case FADE_IN_OUT:
			case FADE_IN_OUT_DIE:
			{
				if ( p->alpha_finished )
				{
					p->alpha_step *= -1.0f;
					p->alpha_finished = 0;
					
				}
				break;
			}
		}
	}

	// Light level
	if ( p->light_think != IGNORE_LIGHT )
	{
		cl_entity_t* light_ent;
		alight_t lighting;
		vec3_t dir;

		if ( ( light_ent = gEngfuncs.GetLocalPlayer() ) != NULL )
		{
			light_ent->origin = p->org;
			lighting.plightvec = dir;

			IEngineStudio.StudioDynamicLight( light_ent, &lighting );
			IEngineStudio.StudioSetupLighting( &lighting );
			IEngineStudio.StudioEntityLight( &lighting );

			p->color.r = lighting.color[0] * lighting.shadelight;
			p->color.g = lighting.color[1] * lighting.shadelight;
			p->color.b = lighting.color[2] * lighting.shadelight;

			// Stop checking once we're done.
			if ( p->light_think == LIGHT_CHECK_ONCE )
				p->light_think = IGNORE_LIGHT;
		}
	}

	// Colliding
	if ( p->collide_think != DONT_COLLIDE )
	{
		switch ( p->collide_think )
		{
			case COLLIDE_STICK:
			{
				if ( gEngfuncs.PM_PointContents( p->org, NULL ) != ( CONTENTS_EMPTY | CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) )
					p->vel = p->accel = { 0.0f, 0.0f, 0.0f };
				break;
			}
			case COLLIDE_DIE:
			{
				if ( gEngfuncs.PM_PointContents( p->org, NULL ) != ( CONTENTS_EMPTY | CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) )
					p->ltime = 0.0f;
				break;
			}
			case COLLIDE_BOUNCE:
			{
				pmtrace_t* tr;

				tr = gEngfuncs.PM_TraceLine( p->old_org, p->org, PM_TRACELINE_PHYSENTSONLY, 2, -1 );

				if (tr && tr->fraction < 1.0f)
				{
					vec3_t vecNormal = tr->plane.normal;

					p->org = tr->endpos;
					p->vel = p->vel - 2 * DotProduct(p->vel, vecNormal) * vecNormal;
					p->vel = p->vel * p->bounce;
				}
				break;
			}
		}
	}

	p->ltime -= gHUD.m_flTimeDelta;
}

//===============================
//	ParticleDraw
//===============================

void CParticleDan::ParticleDraw( partdan_t* p )
{
	vec3_t v_angles, right, up;
	vec3_t color;

	// Convert color to a vector...
	color = Color24ToVector( p->color );

	// Setup AngleVector doohickey,
	gEngfuncs.GetViewAngles( ( float* )v_angles );
	AngleVectors( v_angles, NULL, right, up );

	// Transform sprite.
	VectorScale( right, p->scale.x, right );
	VectorScale( up, p->scale.y, up );

	// Sprite model check.
	if ( p->model == NULL )
	{
		if ( ( p->model = (model_s*)gEngfuncs.GetSpritePointer( SPR_Load( p->sprite ) ) ) == NULL )
		{
			p->ltime = 0.0f;
			gEngfuncs.Con_Printf( "ParticleDraw: Particle sprite \"%s\" does not exist, or cannot be loaded.\n", p->sprite );
			return;
		}
	}

	// Render particle.
	gEngfuncs.pTriAPI->RenderMode( p->rendermode );
	gEngfuncs.pTriAPI->SpriteTexture( p->model, p->frame );
	gEngfuncs.pTriAPI->Color4f( color[0], color[1], color[2], p->alpha );
	gEngfuncs.pTriAPI->CullFace( TRI_NONE );
	gEngfuncs.pTriAPI->Begin( TRI_QUADS );

	gEngfuncs.pTriAPI->TexCoord2f( 0, 0 );
	gEngfuncs.pTriAPI->Vertex3f( p->org[0] - right[0] + up[0] ,  p->org[1] - right[1] + up[1] ,  p->org[2] - right[2] + up[2] );

	gEngfuncs.pTriAPI->TexCoord2f( 0, 1 );
	gEngfuncs.pTriAPI->Vertex3f( p->org[0] - right[0] - up[0] ,  p->org[1] - right[1] - up[1] ,  p->org[2] - right[2] - up[2] );

	gEngfuncs.pTriAPI->TexCoord2f( 1, 1 );
	gEngfuncs.pTriAPI->Vertex3f( p->org[0] + right[0] - up[0] , p->org[1] + right[1] - up[1] , p->org[2] + right[2] - up[2] );

	gEngfuncs.pTriAPI->TexCoord2f( 1, 0 );
	gEngfuncs.pTriAPI->Vertex3f( p->org[0] + right[0] + up[0] , p->org[1] + right[1] + up[1] , p->org[2] + right[2] + up[2] );

	gEngfuncs.pTriAPI->End(); //end our list of vertexes
	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
}

//===============================
//	CreateTestParticles
//	Shoot plasmaballs...
//===============================

void CParticleDan::CreateTestParticles(void)
{
	if ( !m_iShownYet )
	{
		gEngfuncs.Con_Printf("CreateTestParticles: click to create particles\n");
		m_iShownYet = 1;
	}

	if (gHUD.m_iKeyBits & IN_ATTACK)
	{
		if (m_flEmissionRate <= 0.0f)
		{
			cl_entity_t* ent = gEngfuncs.GetLocalPlayer();
			partdan_t* p;

			vec3_t forward, right, v_angles;
			gEngfuncs.GetViewAngles((float*)v_angles);
			AngleVectors(v_angles, forward, right, NULL);

			p = GetParticlePointer();

			if (p)
			{
				p->org = ent->origin + Vector(0, 0, 18) + forward * 16.0f + right * 8.0f;
				p->vel = ent->curstate.velocity + forward * 800.0f;
				p->accel = { 0.0f, 0.0f, -400.0f };
				p->ang = 0;
				p->angvel = 0;
				p->bounce = 0.5f;

				p->sprite = "sprites/exit1.spr";
				p->color.r = p->color.g = p->color.b = 255;
				p->rendermode = kRenderTransAdd;

				p->fps = 30;
				p->frame_max = 24;
				p->frame = 0;

				p->alpha = 1.0;
				p->alpha_step = -2.0f;
				p->alpha_max = 0.0f;

				p->scale = { 0.0f, 0.0f };
				p->scale_step = { 8.0f, 8.0f };
				p->scale_max = { 16.0f, 16.0f };

				p->idx = 0;
				p->ltime = 5.0f;

				p->collide_think = COLLIDE_BOUNCE;
				p->anim_think = ANIMATE_LOOP;
				p->alpha_think = FADE_IN_OUT;
				p->scale_think = SCALE_ONCE;
				p->light_think = IGNORE_LIGHT;
			}
			m_flEmissionRate = 0.1f;
		}

		m_flEmissionRate -= gHUD.m_flTimeDelta;

		gHUD.m_iKeyBits &= ~IN_ATTACK;
	}
}

#if 0

//===============================
//	DrawCoolParticles
//	This is an example of creating particles using my system...
//===============================

void DrawCoolParticles( void )
{
	partdan_t* p;
	p = gParticleDan.GetParticlePointer();

	cl_entity_t* ent = gEngfuncs.GetLocalPlayer();
	vec3_t forward, v_angles;

	gEngfuncs.GetViewAngles((float*)v_angles);
	AngleVectors(v_angles, forward, NULL, NULL);

	if (p)
	{
		p->org = ent->origin + Vector(0, 0, 28) + forward * 16.0f;
		p->vel = ent->curstate.velocity + forward * 300.0f;
		p->accel = { 0, 0, 0 };
		p->ang = 0;
		p->angvel = 0;

		p->sprite = "sprites/exit1.spr";
		p->color.r = p->color.g = p->color.b = 255;
		p->rendermode = kRenderTransAdd;

		p->fps = 20;
		p->frame_max = 24;
		p->frame = 0;

		p->alpha = 1.0;
		p->alpha_step = 0.0f;
		p->alpha_max = 0.0f;

		p->scale = { 8.0f, 8.0f };
		p->scale_step = { 0.0f, 0.0f };
		p->scale_max = { 0.0f, 0.0f };

		p->idx = 0;
		p->ltime = 1.0f;

		p->collide_think = COLLIDE_DIE;
		p->anim_think = ANIMATE_LOOP;
		p->alpha_think = FADE_IN_OUT;
		p->scale_think = SCALE_IN_OUT;
		p->light_think = IGNORE_LIGHT;
	}
}

#endif