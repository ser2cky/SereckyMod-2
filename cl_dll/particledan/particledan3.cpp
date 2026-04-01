
//=======================================
//	particle_dan3.cpp
// 
//	Purpose: Particle System for Half-Life 1 that
//	allows you to create particles from either inside
//	the client, or from scripts.
//
//	History:
//	FEB-21-26: Started. Redid all the emitter and
//	particle structs from version2, and got rid of
//	the flags field in favour of typedef enums for
//	specific behaviours that are easier to deal with
//	in scripts.
//	FEB-27-26: Reworked particle animation code a bit..
//	now it's all controlled with think enums... also made
//	a proper active and free particle pool so every frame
//	we can just iterate on less particles. also started proper
//	work on script parsing.
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
extern vec3_t v_angles;
extern vec3_t v_lastAngles;

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
	p->color = start;
	p->color_start = start;
	p->color_end = end;
	p->color_step = step;
	p->color_think = mode;
}

//===============================
//	SetScale
//===============================

void CParticleDan::SetScale( ParticleDan* p, Vector2D start, Vector2D end, Vector2D step, anim_think_t mode )
{
	p->scale = start;
	p->scale_start = start;
	p->scale_end = end;
	p->scale_step = step;
	p->scale_think = mode;
}

//===============================
//	SetFade
//===============================

void CParticleDan::SetFade( ParticleDan* p, float start, float end, float step, anim_think_t mode )
{
	p->alpha = start;
	p->alpha_start = start;
	p->alpha_end = end;
	p->alpha_step = step;
	p->alpha_think = mode;
}

//===============================
//	SetAnimate
//===============================

void CParticleDan::SetAnimate( ParticleDan* p, int start, int end, int framerate, anim_think_t mode )
{
	p->frame = start;
	p->frame_start = start;
	p->frame_end = end;
	p->framerate = framerate;
	p->anim_think = mode;
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
	memset( m_Particles, 0, sizeof(m_Particles) );

	m_FreeParticles = &m_Particles[ 0 ];
	m_ActiveParticles = NULL;

	for (int i = 0; i < MAX_PARTICLES; i++)
		m_Particles[i].next = &m_Particles[ i + 1 ];
	m_Particles[ MAX_PARTICLES - 1 ].next = NULL;

	m_iActiveParticles = 0;
	return 1;
}

//=======================================
//	MsgFunc_AddEmitter
//=======================================

int CParticleDan::MsgFunc_AddEmitter( const char *pszName, int iSize, void *pbuf )
{
	char *szScript, *szScriptName;
	vec3_t vecOrg;
	int iEntIndex;

	BEGIN_READ( pbuf, iSize );

	vecOrg[0] = READ_COORD();
	vecOrg[1] = READ_COORD();
	vecOrg[2] = READ_COORD();
	iEntIndex = READ_SHORT();
	szScriptName = READ_STRING();

	if ( ( szScript = ( char* )gEngfuncs.COM_LoadFile( szScriptName, 0, NULL ) ) != NULL )
		ParseScript( szScript );
	else
		gEngfuncs.Con_Printf( "ParticleDan: Cannot load \"%s\"!\n", szScriptName );

	return 1;
}

//=======================================
//	ParseScript
//	Read .PDAN scripts to create both emitters and particles.
//=======================================

void CParticleDan::ParseScript( const char* pszScript )
{
	//TODO

	gEngfuncs.COM_FreeFile( ( void* )pszScript );
}

//=======================================
//	GetParticlePointer
//	return particle pointers for people to use them in good things.
//=======================================

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

//=======================================
//	ManageParticles
//	Manage all particles we have right now, and run their
//	respective thinking and drawing functions. memory code's
//	knicked from quake2. it's solid code that's ripe for the
//	picking...
//=======================================

void CParticleDan::ManageParticles( void )
{
	ParticleDan *p, *next, *tail, *active;

	active = NULL;
	tail = NULL;

	for ( p = m_ActiveParticles; p; p = next )
	{
		next = p->next;

		if ( p->ltime <= 0.0f )
		{
			p->next = m_FreeParticles;
			m_FreeParticles = p;
			m_iActiveParticles--;
			continue;
		}

		p->next = NULL;

		if ( !tail )
		{
			active = tail = p;
		}
		else
		{
			tail->next = p;
			tail = p;
		}

		ParticleThink( p );
		ParticleDraw( p );
	}

	m_ActiveParticles = active;

	if (m_pCvarTestParticles->value)
		CreateTestParticles();
}

//=======================================
//	ParticleThink
//	this is where particles do their little funny thinking code
//=======================================

void CParticleDan::ParticleThink( ParticleDan* p )
{
	int i;

	VectorCopy( p->org, p->old_org );
	p->ltime -= gHUD.m_flTimeDelta;

	VectorMA( p->vel, gHUD.m_flTimeDelta, p->accel, p->vel );
	VectorMA( p->org, gHUD.m_flTimeDelta, p->vel, p->org );
	p->ang += p->angvel * gHUD.m_flTimeDelta;
}

//===============================
//	ParticleDraw
//===============================

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
	AngleVectors( v_angles, NULL, right, up );

	// Transform sprite.
	VectorScale( right, p->scale.x, right );
	VectorScale( up, p->scale.y, up );

	// Render particle.
	gEngfuncs.pTriAPI->RenderMode( p->rendermode );
	gEngfuncs.pTriAPI->SpriteTexture( p->model, p->frame );
	gEngfuncs.pTriAPI->Color4f( p->color[0], p->color[1], p->color[2], p->alpha );
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

//===============================
//	ShootFlameThrower
//	create a fancy flamethrower effect similar to that of
//	kingpin's.
//===============================

#define MAX_FLAMES_PER_FRAME			30
#define FLAME_MAX_MAKEUP_PERIOD			0.125
#define FLAME_NOISE						64.0

static float last_fire_time;
static float fire_time;

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

			if ((p = gParticleDan.GetParticlePointer()) != NULL)
			{
				for ( i = 0; i < 3; i++ )
				{
					lerp_fwd[i] = Interpolate( old_fwd[i], cur_fwd[i], lerpfrac );
					lerp_rt[i] = Interpolate( old_rt[i], cur_rt[i], lerpfrac );
					lerp_up[i] = Interpolate( old_up[i], cur_up[i], lerpfrac );
					lerp_org[i] = Interpolate( old_org[i], cur_org[i], lerpfrac );
				}

				p->org = lerp_org = lerp_org + lerp_fwd * 4.0f;
				p->vel = lerp_fwd * 1000.0f + lerp_rt * gEngfuncs.pfnRandomFloat( -FLAME_NOISE, FLAME_NOISE) + lerp_up * gEngfuncs.pfnRandomFloat( -FLAME_NOISE, FLAME_NOISE);

				p->sprite = "sprites/kp_explode1.spr";
				p->rendermode = kRenderTransAdd;
				p->brightness = 1.0f;

				gParticleDan.SetColor(p, gParticleDan.RGBToColor4f(255, 255, 255), vec3_origin, vec3_origin);
				gParticleDan.SetScale(p, { 2.0f, 2.0f }, { 128.0f, 128.0f }, { 128.0f, 128.0f }, ANIMATE_ONCE);
				gParticleDan.SetFade(p, 1.0f, 0.0f, 0.0f);
				gParticleDan.SetAnimate(p, 0, 12, 30, ANIMATE_DIE);

				p->ltime = 2.0f;
				p->collide_think = COLLIDE_STICK;
			}
		}
	}
	else
	{
		if ( ( p = gParticleDan.GetParticlePointer() ) != NULL )
		{
			p->org = cur_org;
			p->vel = cur_fwd * 1000.0f + cur_rt * gEngfuncs.pfnRandomFloat( -FLAME_NOISE, FLAME_NOISE) + cur_up * gEngfuncs.pfnRandomFloat( -FLAME_NOISE, FLAME_NOISE);

			p->sprite = "sprites/kp_explode1.spr";
			p->rendermode = kRenderTransAdd;
			p->brightness = 1.0f;

			gParticleDan.SetColor(p, gParticleDan.RGBToColor4f(255, 255, 255), vec3_origin, vec3_origin);
			gParticleDan.SetScale(p, { 2, 2 }, { 128.0f, 128.0f }, { 128.0f, 128.0f }, ANIMATE_ONCE);
			gParticleDan.SetFade(p, 1.0f, 0.0f, 0.0f);
			gParticleDan.SetAnimate(p, 0, 12, 30, ANIMATE_DIE);

			p->ltime = 2.0f;
			p->collide_think = COLLIDE_STICK;

		}
	}
}


void CParticleDan::CreateTestParticles(void)
{
	if ( !m_iShownYet )
	{
		gEngfuncs.Con_Printf("CreateTestParticles: click to create particles\n");
		m_iShownYet = 1;
	}

	if (gHUD.m_iKeyBits & IN_ATTACK)
	{
		ShootFlameThrower();
		gHUD.m_iKeyBits &= ~IN_ATTACK;
	}
}

#if defined( TEST_IT )

//===============================
//	DrawCoolParticles
//	This is an example of creating particles using my system...
//===============================

void DrawCoolParticles( void )
{
	cl_entity_t* ent = gEngfuncs.GetLocalPlayer();
	ParticleDan* p;

	for ( int i = 0; i < 256; i++ )
	{
		if ( ( p = gParticleDan.GetParticlePointer() ) != NULL )
		{
			for ( int j = 0; j < 3; j++ )
				p->vel[j] = gEngfuncs.pfnRandomFloat( -512.0f, 512.0f );
			p->org = ent->origin;
			p->accel = { 0.0f, 0.0f, -400.0f };
			p->bounce = 0.5f;

			p->sprite = "sprites/exit1.spr";
			p->rendermode = kRenderTransAdd;
			p->brightness = 1.0f;

			gParticleDan.SetColor( p, gParticleDan.RGBToColor4f(0, 255, 255), gParticleDan.RGBToColor4f(255, 255, 0), { 0.5f, 0.0f, -0.5f }, ANIMATE_ONCE );
			gParticleDan.SetScale( p, { 16.0f, 16.0f }, { 0.0f, 0.0f }, { -8.0f, -8.0f }, ANIMATE_DIE );
			gParticleDan.SetFade( p, 0.0f, 1.0f, 1.0f, ANIMATE_LOOP_REVERSE );
			gParticleDan.SetAnimate( p, 0, 24, 20, ANIMATE_LOOP );

			p->ltime = 5.0f;
			p->collide_think = COLLIDE_BOUNCE;
			p->light_think = IGNORE_LIGHT;
		}
	}
}

#endif