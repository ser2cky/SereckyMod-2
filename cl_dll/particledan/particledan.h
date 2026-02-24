#if !defined( PARTICLEDANH )
#define PARTICLEDANH
#ifdef _WIN32
#pragma once
#endif

#define	MAX_PARTICLES	8192
#define	MAX_EMITTERS	800

typedef vec_t vec2_t[2];

enum
{
	INCREASING = 1,
	DECREASING = -1
};

//===============================
//	Collision behavior for particles.
//===============================

typedef enum
{
	DONT_COLLIDE = 0,
	COLLIDE_STICK = 1,
	COLLIDE_DIE = 2,
	COLLIDE_BOUNCE = 3
} collide_type_t;

//===============================
//	Animation behavior for particles.
//===============================

typedef enum
{
	DONT_ANIMATE = 0,
	ANIMATE_ONCE = 1,
	ANIMATE_DIE = 2,
	ANIMATE_LOOP = 3,
} anim_think_t;

//===============================
//	Fading/Alpha behavior for particles.
//===============================

typedef enum
{
	DONT_FADE = 0,		// don't fade
	FADE_ONCE = 1,		// just fade
	FADE_DIE = 2,		// die when alpha reaches it's max
	FADE_LOOP = 3,		// set alpha back to either 1 or 0 when hitting max
	FADE_IN_OUT = 4,	// fade in and out once
	FADE_IN_OUT_DIE = 5	// fade in and out then die.
} fade_think_t;

//===============================
//	Scaling behavior for particles.
//===============================

typedef enum
{
	DONT_SCALE = 0,
	SCALE_ONCE = 1,
	SCALE_DIE = 2,
	SCALE_LOOP = 3,
	SCALE_IN_OUT = 4,
	SCALE_IN_OUT_DIE = 5
} scale_think_t;

//===============================
//	Light-level behavior for particles.
//===============================

typedef enum
{
	IGNORE_LIGHT = 0,
	LIGHT_CHECK_ONCE = 1,
	LIGHT_CHECK_ALWAYS = 2
} light_think_t;

//===============================
//	Emitter struct.
//===============================

typedef struct emitter_s
{
	// Movement
	vec3_t	org;		// Emitter's origin
	vec3_t	vel;		// Emitter's velocity
	vec3_t	accel;		// Emitter's rate of acceleration

	// Behavior
	int		idx;		// Entity that emitter is tied to.
	float	ltime;		// Emitter's lifetime.
	int		max_emit;	// Total amount of particles that can be emitted. Value of 0 disregards this.
	int		max_emit_frame;		// How much an emitter can emit in a frame.
	float	emit_freq;  // How frequently an emitter emits particles.

	collide_type_t	collide_mode;	// Emitter collision behavior.

	struct	emitter_s* next;
} emitter_t;

//===============================
//	Particle struct.
//===============================

typedef struct partdan_s
{
	// Movement
	vec3_t	org;		// Particle's origin
	vec3_t	vel;		// Particle's velocity
	vec3_t	accel;		// Particle's rate of acceleration

	float	ang;		// Particle's current angles
	float	angvel;		// Particle's angular velocity

	float	bounce;		// Particle's bouncefactor

	// Appearence
	const char		*sprite;	// Particle sprite name.
	color24			color;		// Particle's RGB
	unsigned int	rendermode;	// Particle's rendermode

	// Animation
	unsigned int	fps;		// Particle's animation framerate
	unsigned int	frame_max;	// Max. amount of particle frames
	unsigned int	frame;		// Current Frame particle's on

	float	alpha;			// Particle's current transparency
	float	alpha_step;		// Particle transparency rate of change
	float	alpha_max;		// Particle's max transparency

	Vector2D	scale;			// Particle's current size.
	Vector2D	scale_step;		// Particle size rate of change
	Vector2D	scale_max;		// Particle's max size.

	// Behavior
	unsigned int	idx;		// Entity that particle is tied to
	float			ltime;		// Particle's lifetime

	collide_type_t	collide_think;	// Particle's collision behavior
	anim_think_t	anim_think;		// Particle's animation behavior
	fade_think_t	alpha_think;	// Particle's transparency behavior.
	scale_think_t	scale_think;	// Particle's scaling behavior
	light_think_t	light_think;	// Particle's light-level behavior.

	// Logic - don't touch!!
	vec3_t	old_org;				// old org - used for bouncing

	int		scale_dir;				// are we scaling up or down?
	int		alpha_dir;				// are we fading in or out?
	int		anim_dir;				// are we animating in reverse?

	int		scale_finished;			// done scaling?
	int		alpha_finished;			// done fading?
	int		anim_finished;			// done animating?

	int		alive;					// Well... Is our particle dead?
	float	nextanim;				// Particle's animation thinker.
	struct	model_s		*model;		// Particle model pointer.
	struct	partdan_s	*next;
} partdan_t;

class CParticleDan
{
public:
	partdan_t*	GetParticlePointer( void );
	void		ManageParticles( void );
	int			MsgFunc_AddEmitter( const char *pszName,  int iSize, void *pbuf );
	int			Init( void );
	int			VidInit(void);
private:
	emitter_t	m_Emitters[MAX_EMITTERS];
	partdan_t	m_Particles[MAX_PARTICLES];
	int			m_iActiveParticles;

	void		ParticleThink( partdan_t* p );
	void		ParticleDraw( partdan_t* p );
	void		ParseScript( void );

	cvar_t*		m_pCvarTestParticles;
	void		CreateTestParticles( void );
	int			m_iShownYet;
	float		m_flEmissionRate;
};

extern CParticleDan gParticleDan;

#endif