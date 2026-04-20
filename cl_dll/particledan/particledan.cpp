
//====================================================================
//	particle_dan.cpp
// 
//	Purpose: TODO
//
//	TODO: add emitters, implment particle .cfgs....
// 
//	History:
// 
//	FEB-21-26: 
// 
//	Started. Redid all the emitter and
//	particle structs from version2, and got rid of
//	the flags field in favour of typedef enums for
//	specific behaviours that are easier to deal with
//	in scripts.
// 
//	FEB-27-26: 
// 
//	Reworked particle animation code a bit..
//	now it's all controlled with think enums... also made
//	a proper active and free particle pool so every frame
//	we can just iterate on less particles. also started proper
//	work on script parsing.
// 
//	APR-1-26: 
// 
//	PARTICLE CHANGES:
//	Split up the Particle thinking code into
//	AnimateParticle, and Transform Particle. Removed collision
//	mode field from ParticleDan struct in favor of collision flags.
//	Moved a lot of particledan fields into anim_info_s. New Flamethrower
//	code that looks a lot better. 
// 
//	EMITTER CHANGES:
//	completely rewrote the whole emitter struct.
//
//====================================================================

#include "../hud.h"
#include "../cl_util.h"
#include "triangleapi.h"
#include "com_model.h"
#include "r_studioint.h"
#include "../parsemsg.h"
#include "pm_defs.h"
#include "particledan.h"
#include "../in_defs.h"

extern engine_studio_api_t IEngineStudio;
CParticleDan	gPDan;

//=========================================================
//	Init
//=========================================================

int CParticleDan::Init( void )
{
	m_pCvarTestParticles = CVAR_CREATE( "test_particles", "0", FCVAR_ARCHIVE );
	return 1;
}

//=========================================================
//	VidInit
//=========================================================

int CParticleDan::VidInit( void )
{
	memset( m_Particles, 0, sizeof(m_Particles) );

	m_FreeParticles = &m_Particles[ 0 ];
	m_ActiveParticles = NULL;

	for (int i = 0; i < MAX_PARTICLES; i++)
		m_Particles[i].next = &m_Particles[ i + 1 ];
	m_Particles[ MAX_PARTICLES - 1 ].next = NULL;

	m_iActiveParticles = 0;
	return 1;
}

//=========================================================
//	ParseScript
// 
//	Read all Particle .cfg files inside the "particledef/"
//	folder, so we can add them to our list of user-defined emitters.
//=========================================================

void CParticleDan::ParseScripts( void )
{
	//TODO
}

//=========================================================
//	MsgFunc_TieToEmitter
//	
//	Read a server entity's index, and the given path to a
//	particle definition, so we can tie an emitter to our entity.
//=========================================================

int CParticleDan::MsgFunc_TieToEmitter( const char *pszName, int iSize, void *pbuf )
{
	// TODO

	return 1;
}

//=========================================================
//	GetParticlePointer
// 
//	Return a particle pointer if we're still able to nab particles
//	from the free particles list, so users can cool particely stuff.
//=========================================================

ParticleDan *CParticleDan::GetParticlePointer( void )
{
	ParticleDan* p;

	if ( !m_FreeParticles )
	{
		gEngfuncs.Con_Printf("ParticleDan: Unable to grab a particle pointer! Particle limit is 8192!");
		return NULL;
	}

	p = m_FreeParticles;
	m_FreeParticles = p->next;
	memset( p, 0, sizeof(ParticleDan) );
	p->next = m_ActiveParticles;
	m_ActiveParticles = p;

	p->ltime = 1.0f;
	m_iActiveParticles++;
	return p;
}

//=========================================================
//	ManageParticleDan
// 
//	Function to manage both emitters and particles.
//=========================================================

void CParticleDan::ManageParticleDan( void )
{
	ParticleDan		*p, *pNext, *pTail, *pActive;
	PDan_Emitter	*emitter, *pEmitterNext, *pEmitterTail, *pEmitterActive;

	pActive = NULL;
	pTail = NULL;

	for ( p = m_ActiveParticles; p; p = pNext )
	{
		pNext = p->next;

		if ( p->ltime <= 0.0f )
		{
			p->next = m_FreeParticles;
			m_FreeParticles = p;
			m_iActiveParticles--;
			continue;
		}

		p->next = NULL;

		if ( !pTail )
		{
			pActive = pTail = p;
		}
		else
		{
			pTail->next = p;
			pTail = p;
		}

		if ( p->think == NULL )
		{
			ParticleThink( p );
			p->ltime -= gHUD.m_flTimeDelta;
		}
		else
		{
			p->think( p );
			p->ltime -= gHUD.m_flTimeDelta;
		}
		ParticleDraw( p );
	}

	m_ActiveParticles = pActive;

	pEmitterActive = NULL;
	pEmitterTail = NULL;

	for ( emitter = m_ActiveEmitters; emitter; emitter = pEmitterNext )
	{
		pEmitterNext = emitter->next;

		if ( emitter->ltime )
		{
			emitter->next = m_FreeEmitters;
			m_FreeEmitters = emitter;
			continue;
		}

		emitter->next = NULL;

		if ( !pEmitterTail )
		{
			pEmitterActive = pEmitterTail = emitter;
		}
		else
		{
			pEmitterTail->next = emitter;
			pEmitterTail = emitter;
		}
	}

	m_ActiveEmitters = pEmitterActive;

	if (m_pCvarTestParticles->value)
		CreateTestParticles();
}

//=========================================================
//	ParticleThink
// 
//	This is esentially the brain of every single standard particle. Because users
//	who write their own thinking functions for particles may only want to retain
//	parts of the particle thinking code, it has been split up into TransformParticle,
//	and AnimateParticle.
//=========================================================

void CParticleDan::ParticleThink( ParticleDan* p )
{
	TransformParticle( p );
	AnimateParticle( p );
}

//=========================================================
//	TransformParticle
// 
//	This function is where a particle handles velocity, origin, acceleration,
//	and collisions.
//=========================================================

void CParticleDan::TransformParticle( ParticleDan* p )
{
	VectorCopy( p->org, p->old_org );
	VectorMA( p->vel, gHUD.m_flTimeDelta, p->accel, p->vel );
	VectorMA( p->org, gHUD.m_flTimeDelta, p->vel, p->org );
	p->angles += p->angvel * gHUD.m_flTimeDelta;

	if ( p->col_flags & COLLIDE_AND_DIE )
	{
		p->ltime = 0.0f;
	}

	if ( p->col_flags & COLLIDE_STICK )
	{
		if ( gEngfuncs.PM_PointContents( p->org, NULL ) != ( CONTENTS_EMPTY | CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) )
			p->vel = p->accel = { 0.0f, 0.0f, 0.0f };
	}

	if ( p->col_flags & COLLIDE_BOUNCE )
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
	}

	if ( p->col_flags & COLLIDE_SLIDE )
	{
		pmtrace_t* tr;

		tr = gEngfuncs.PM_TraceLine( p->old_org, p->org, PM_TRACELINE_PHYSENTSONLY, 2, -1 );

		if (tr && tr->fraction < 1.0f)
		{
			vec3_t vecNormal = tr->plane.normal;

			p->org = tr->endpos;
			p->vel = p->vel - DotProduct(p->vel, vecNormal) * vecNormal;
			p->vel = p->vel * p->friction;
		}
	}
}

//=========================================================
//	AnimateParticle
// 
//	This function is where a particle handles velocity, origin, acceleration,
//	and collisions.
//=========================================================

void CParticleDan::AnimateParticle( ParticleDan* p )
{
	int i;

	// Animating
	if ( p->anim_think != DO_NOTHING )
	{
		if ( p->nextanim <= 0.0f )
		{
			p->cur.frame += 1;
			p->nextanim = 1.0f / p->framerate;
		}

		p->nextanim -= gHUD.m_flTimeDelta;

		if ( p->cur.frame > p->end.frame )
		{
			if ( p->anim_think == ANIMATE_DIE )
				p->ltime = 0.0f;
			if ( p->anim_think == ANIMATE_ONCE )
				p->nextanim = -1;
			if ( p->anim_think == ANIMATE_LOOP )
				p->cur.frame = p->start.frame;
		}
	}

	// Fading in & out
	if ( p->alpha_think != DO_NOTHING )
	{
		p->cur.alpha += gHUD.m_flTimeDelta * p->alpha_step;
	
		if ( p->alpha_think != ANIMATE_LOOP_REVERSE )
		{
			// Fading out
			if ( ( p->start.alpha > p->end.alpha ) && ( p->cur.alpha < p->end.alpha ) )
			{
				if ( p->alpha_think == ANIMATE_DIE )
					p->ltime = 0.0f;
				if ( p->alpha_think == ANIMATE_ONCE )
					p->alpha_step = 0.0f;
				if ( p->alpha_think == ANIMATE_LOOP )
					p->cur.alpha = p->start.alpha;
			}
			// Fading in.
			if ( ( p->start.alpha < p->end.alpha ) && ( p->cur.alpha > p->end.alpha ) )
			{
				if ( p->alpha_think == ANIMATE_DIE )
					p->ltime = 0.0f;
				if ( p->alpha_think == ANIMATE_ONCE )
					p->alpha_step = 0.0f;
				if ( p->alpha_think == ANIMATE_LOOP )
					p->cur.alpha = p->start.alpha;
			}
		}
		else
		{
			if ( p->start.alpha > p->end.alpha ) // fading out
			{
				// high to low
				if ( p->cur.alpha < p->end.alpha )
				{
					p->alpha_step *= -1.0f;
					p->cur.alpha = p->end.alpha;
				}
				// low to high
				if ( p->cur.alpha > p->start.alpha )
				{
					p->alpha_step *= -1.0f;
					p->cur.alpha = p->start.alpha;
				}
			}
			else if ( p->start.alpha < p->end.alpha ) // fading in
			{
				// low to high
				if ( p->cur.alpha > p->end.alpha )
				{
					p->alpha_step *= -1.0f;
					p->cur.alpha = p->end.alpha;
				}
				// high to low
				if ( p->cur.alpha < p->start.alpha )
				{
					p->alpha_step *= -1.0f;
					p->cur.alpha = p->start.alpha;
				}
			}
		}
	}

	// Scaling
	if ( p->scale_think != DO_NOTHING )
	{
		for ( i = 0; i < 2; i++ )
		{
			p->cur.scale[i] += gHUD.m_flTimeDelta * p->scale_step[i];
		
			if ( p->scale_think != ANIMATE_LOOP_REVERSE )
			{
				// Fading out
				if ( ( p->start.scale[i] > p->end.scale[i] ) && ( p->cur.scale[i] < p->end.scale[i] ) )
				{
					if ( p->scale_think == ANIMATE_DIE )
						p->ltime = 0.0f;
					if ( p->scale_think == ANIMATE_ONCE )
						p->scale_step[i] = 0.0f;
					if ( p->scale_think == ANIMATE_LOOP )
						p->cur.scale[i] = p->start.scale[i];
				}
				// Fading in.
				if ( ( p->start.scale[i] < p->end.scale[i] ) && ( p->cur.scale[i] > p->end.scale[i] ) )
				{
					if ( p->scale_think == ANIMATE_DIE )
						p->ltime = 0.0f;
					if ( p->scale_think == ANIMATE_ONCE )
						p->scale_step[i] = 0.0f;
					if ( p->scale_think == ANIMATE_LOOP )
						p->cur.scale[i] = p->start.scale[i];
				}
			}
			else
			{
				if ( p->start.scale[i] > p->end.scale[i] ) // fading out
				{
					// high to low
					if ( p->cur.scale[i] < p->end.scale[i] )
					{
						p->scale_step[i] *= -1.0f;
						p->cur.scale[i] = p->end.scale[i];
					}
					// low to high
					if ( p->cur.scale[i] > p->start.scale[i] )
					{
						p->scale_step[i] *= -1.0f;
						p->cur.scale[i] = p->start.scale[i];
					}
				}
				else if ( p->start.scale[i] < p->end.scale[i] ) // fading in
				{
					// low to high
					if ( p->cur.scale[i] > p->end.scale[i] )
					{
						p->scale_step[i] *= -1.0f;
						p->cur.scale[i] = p->start.scale[i];
					}
					// high to low
					if ( p->cur.scale[i] < p->start.scale[i] )
					{
						p->scale_step[i] *= -1.0f;
						p->cur.scale[i] = p->start.scale[i];
					}
				}
			}
		}
	}

	// Color changing stuff
	if ( p->color_think != DO_NOTHING )
	{
		for ( i = 0; i < 3; i++ )
		{
			p->cur.color[i] += gHUD.m_flTimeDelta * p->color_step[i];
		
			if ( p->color_think != ANIMATE_LOOP_REVERSE )
			{
				// Fading out
				if ( ( p->start.color[i] > p->end.color[i] ) && ( p->cur.color[i] < p->end.color[i] ) )
				{
					if ( p->color_think == ANIMATE_DIE )
						p->ltime = 0.0f;
					if ( p->color_think == ANIMATE_ONCE )
						p->color_step[i] = 0.0f;
					if ( p->color_think == ANIMATE_LOOP )
						p->cur.color[i] = p->start.color[i];
				}
				// Fading in.
				if ( ( p->start.color[i] < p->end.color[i] ) && ( p->cur.color[i] > p->end.color[i] ) )
				{
					if ( p->color_think == ANIMATE_DIE )
						p->ltime = 0.0f;
					if ( p->color_think == ANIMATE_ONCE )
						p->color_step[i] = 0.0f;
					if ( p->color_think == ANIMATE_LOOP )
						p->cur.color[i] = p->start.color[i];
				}
			}
			else
			{
				if ( p->start.color[i] > p->end.color[i] ) // fading out
				{
					// high to low
					if ( p->cur.color[i] < p->end.color[i] )
					{
						p->color_step[i] *= -1.0f;
						p->cur.color[i] = p->end.color[i];
					}
					// low to high
					if ( p->cur.color[i] > p->start.color[i] )
					{
						p->color_step[i] *= -1.0f;
						p->cur.color[i] = p->start.color[i];
					}

				}
				else if ( p->start.color[i] < p->end.color[i] ) // fading in
				{
					// low to high
					if ( p->cur.color[i] > p->end.color[i] )
					{
						p->color_step[i] *= -1.0f;
						p->cur.color[i] = p->start.color[i];
					}
					// high to low
					if ( p->cur.color[i] < p->start.color[i] )
					{
						p->color_step[i] *= -1.0f;
						p->cur.color[i] = p->start.color[i];
					}
				}
			}
		}
	}

	// Grabbing our current lightlevel..
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

			// Stop checking once we're done.
			if ( p->light_think == LIGHT_CHECK_ONCE )
				p->light_think = IGNORE_LIGHT;

			// TODO: Actually change the light here...
		}
	}
}

//=========================================================
//	ParticleDraw
// 
//	This function is where Particles get drawn. If a particle's type is
//	set to Quake, it'll override the current scale to use GlQuake's particle
//	scaling code instead.
// 
//	TODO: Figure out how to add Particle angles. Maybe use AngleVectors with
//	[ROLL] being set to p->angles?
//=========================================================

void CParticleDan::ParticleDraw( ParticleDan* p )
{
	vec3_t v_angles, right, up, vertex;

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

	// Setup AngleVector doohickey,
	gEngfuncs.GetViewAngles( ( float* )v_angles );
	v_angles[ROLL] += p->angles;

	AngleVectors( v_angles, NULL, right, up );

	// Transform sprite.
	VectorScale( right, p->cur.scale.x, right );
	VectorScale( up, p->cur.scale.y, up );

	// Render particle.
	gEngfuncs.pTriAPI->RenderMode( p->rendermode );
	gEngfuncs.pTriAPI->SpriteTexture( p->model, p->cur.frame );
	gEngfuncs.pTriAPI->Color4f( p->cur.color[0], p->cur.color[1], p->cur.color[2], p->cur.alpha );
	gEngfuncs.pTriAPI->Brightness( p->brightness );
	gEngfuncs.pTriAPI->CullFace( TRI_NONE );
	gEngfuncs.pTriAPI->Begin( TRI_QUADS );

	vertex = p->org - right + up;
	gEngfuncs.pTriAPI->TexCoord2f( 0, 0 );
	gEngfuncs.pTriAPI->Vertex3fv( vertex );

	vertex = p->org - right - up;
	gEngfuncs.pTriAPI->TexCoord2f( 0, 1 );
	gEngfuncs.pTriAPI->Vertex3fv( vertex );

	vertex = p->org + right - up;
	gEngfuncs.pTriAPI->TexCoord2f( 1, 1 );
	gEngfuncs.pTriAPI->Vertex3fv( vertex );

	vertex = p->org + right + up;
	gEngfuncs.pTriAPI->TexCoord2f( 1, 0 );
	gEngfuncs.pTriAPI->Vertex3fv( vertex );

	gEngfuncs.pTriAPI->End();
	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
}

//=========================================================
//	Below this point are various helper functions that make particles
//	slightly less tedious to code in...
//=========================================================

//===============================
//	RGBToColor4f
//===============================

vec3_t CParticleDan::RGBToColor4f( float r, float g, float b )
{
	vec3_t vecColor;

	vecColor[0] = r * ( 1.0f / 255.0f );
	vecColor[1] = g * ( 1.0f / 255.0f );
	vecColor[2] = b * ( 1.0f / 255.0f );

	return vecColor;
}

//===============================
//	SetColor
//===============================

void CParticleDan::SetColor( ParticleDan* p, vec3_t start, vec3_t end, vec3_t step, anim_think_t mode )
{
	p->cur.color = start;
	p->start.color = start;
	p->end.color = end;
	p->color_step = step;
	p->color_think = mode;
}

//===============================
//	SetScale
//===============================

void CParticleDan::SetScale( ParticleDan* p, Vector2D start, Vector2D end, Vector2D step, anim_think_t mode )
{
	p->cur.scale = start;
	p->start.scale = start;
	p->end.scale = end;
	p->scale_step = step;
	p->scale_think = mode;
}

//===============================
//	SetFade
//===============================

void CParticleDan::SetAlpha( ParticleDan* p, float start, float end, float step, anim_think_t mode )
{
	p->cur.alpha = start;
	p->start.alpha = start;
	p->end.alpha = end;
	p->alpha_step = step;
	p->alpha_think = mode;
}

//===============================
//	SetAnimate
//===============================

void CParticleDan::SetAnimate( ParticleDan* p, int start, int end, int framerate, anim_think_t mode )
{
	p->cur.frame = start;
	p->start.frame = start;
	p->end.frame = end;
	p->framerate = framerate;
	p->anim_think = mode;
}

//=========================================================
//	ShootFlameThrower
//	
//	SERECKY APR-1-26: This is a neat little Flamethrower effect I
//	made, based on Kingpin's flamethrower effect. Really proud of 
//	how this turned out, because I've been trying for months to
//	figure out how to make this look good!
//=========================================================

#define MAX_FLAMES_PER_FRAME			30
#define FLAME_MAX_MAKEUP_PERIOD			0.125
#define FLAME_NOISE						96.0

static float last_fire_time;
static float fire_time;

extern vec3_t v_angles;
extern vec3_t v_lastAngles;

float Interpolate( float a, float b, float t )
{
	return a + t * ( b - a );
}

void ShootFlameThrower(void)
{
	float lerpfrac, delta;
	int i;

	vec3_t old_fwd, cur_fwd, lerp_fwd;
	vec3_t old_rt, cur_rt, lerp_rt;
	vec3_t old_up, cur_up, lerp_up;
	vec3_t old_org, cur_org, lerp_org;

	cl_entity_t* ent;
	ParticleDan* p;

	if ( ( ent = gEngfuncs.GetLocalPlayer() ) == NULL )
		return;

	// set up vectors
	AngleVectors( v_angles, cur_fwd, cur_rt, cur_up );
	AngleVectors( v_lastAngles, old_fwd, old_rt, old_up );

	VectorCopy( ent->curstate.origin, cur_org );
	VectorCopy( ent->prevstate.origin, old_org );

	cur_org = cur_org + Vector(0, 0, 18) + cur_fwd * 16.0f + cur_rt * 8.0f;
	old_org = old_org + Vector(0, 0, 18) + old_fwd * 16.0f + old_rt * 8.0f;

	// set up time
	last_fire_time = fire_time;
	fire_time = gHUD.m_flTime;
	delta = fire_time - last_fire_time;

	// SERECKY APR-1-26: to make up for the time lost between frames, we make extra flame 
	// particles so that gaps created by spinning the camera around won't create ugly gaps 
	// in our flame trail.

	if (delta < FLAME_MAX_MAKEUP_PERIOD)
	{
		while ( last_fire_time <= fire_time )
		{
			if (delta < 0.0)
				break;

			lerpfrac = (fire_time - last_fire_time) / delta;
			lerpfrac = 1.0f - max(lerpfrac, 0.0f);
			last_fire_time += gHUD.m_flTimeDelta / (float)MAX_FLAMES_PER_FRAME;

			gEngfuncs.Con_Printf("%.2f %.2f frac %f\n", last_fire_time, fire_time, lerpfrac);

			if ((p = gPDan.GetParticlePointer()) != NULL)
			{
				for ( i = 0; i < 3; i++ )
				{
					lerp_fwd[i] = Interpolate( old_fwd[i], cur_fwd[i], lerpfrac );
					lerp_rt[i] = Interpolate( old_rt[i], cur_rt[i], lerpfrac );
					lerp_up[i] = Interpolate( old_up[i], cur_up[i], lerpfrac );
					lerp_org[i] = Interpolate( old_org[i], cur_org[i], lerpfrac );
				}

				p->org = lerp_org;
				p->vel = lerp_fwd * 1000.0f + lerp_rt * gEngfuncs.pfnRandomFloat( -FLAME_NOISE, FLAME_NOISE ) + lerp_up * gEngfuncs.pfnRandomFloat( -FLAME_NOISE, FLAME_NOISE );
				p->friction = 0.25f;

				p->sprite = "sprites/kp_explode1.spr";
				p->rendermode = kRenderTransAdd;
				p->brightness = 1.0f;
				p->angles = gEngfuncs.pfnRandomFloat(-360.0f, 360.0f);

				gPDan.SetColor(p, gPDan.RGBToColor4f(255, 255, 255), vec3_origin, vec3_origin);
				gPDan.SetScale(p, { 1.0f, 1.0f }, { 128.0f, 128.0f }, { 128.0f, 128.0f }, ANIMATE_ONCE);
				gPDan.SetAlpha(p, 1.0f, 0.0f, -8.0f);
				gPDan.SetAnimate(p, 0, 12, 30, ANIMATE_DIE);

				p->ltime = 2.0f;
				p->col_flags = COLLIDE_SLIDE;

				if ( gEngfuncs.pfnRandomLong(0, 1) == 1 )
					p->col_flags |= COLLIDE_AND_DIE;
			}
		}
	}
	else
	{
		if ( ( p = gPDan.GetParticlePointer() ) != NULL )
		{
			p->org = cur_org;
			p->vel = cur_fwd * 1000.0f + cur_rt * gEngfuncs.pfnRandomFloat( -FLAME_NOISE, FLAME_NOISE ) + cur_up * gEngfuncs.pfnRandomFloat( -FLAME_NOISE, FLAME_NOISE );
			p->friction = 0.25f;

			p->sprite = "sprites/kp_explode1.spr";
			p->rendermode = kRenderTransAdd;
			p->brightness = 1.0f;

			gPDan.SetColor(p, gPDan.RGBToColor4f(255, 255, 255), vec3_origin, vec3_origin);
			gPDan.SetScale(p, { 2, 2 }, { 128.0f, 128.0f }, { 128.0f, 128.0f }, ANIMATE_ONCE);
			gPDan.SetAlpha(p, 1.0f, 0.0f, -8.0f);
			gPDan.SetAnimate(p, 0, 12, 30, ANIMATE_DIE);

			p->ltime = 2.0f;
			p->col_flags = COLLIDE_SLIDE;

			if ( gEngfuncs.pfnRandomLong(0, 1) == 1 )
				p->col_flags |= COLLIDE_AND_DIE;
		}
	}
}

//=========================================================
//	CreateTestParticles
//	
//	SERECKY APR-1-26:
//	TODO: Add a command or cvar that lets you load up specific particle
//	definitions, that you can then shoot out. For now, you'll have to deal
//	with my flamethrower though...
//=========================================================

void CParticleDan::CreateTestParticles(void)
{
	if (gHUD.m_iKeyBits & IN_ATTACK)
	{
		ShootFlameThrower();
		gHUD.m_iKeyBits &= ~IN_ATTACK;
	}
}
