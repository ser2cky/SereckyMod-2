#if !defined( PARTICLEDANH )
#define PARTICLEDANH
#ifdef _WIN32
#pragma once
#endif

#define	MAX_PARTICLES	4096
#define	MAX_EMITTERS	500

//===============================
//	Collision Flags
//===============================

#define COLLIDE_STICK				( 1 << 0 ) // Stick to surfaces.
#define COLLIDE_BOUNCE				( 1 << 1 ) // Bounce off surfaces.
#define COLLIDE_SLIDE				( 1 << 2 ) // Slide off surfaces.
#define COLLIDE_AND_DIE				( 1 << 3 ) // Hit something and die right away.
#define COLLIDE_AGAINST_PLAYERS		( 1 << 4 ) // Does what it says.

//===============================
//	Animation behavior for particles.
//===============================

typedef enum
{
	DO_NOTHING = 0,				// Don't animate.
	ANIMATE_ONCE = 1,			// Animate once.
	ANIMATE_DIE = 2,			// Animate once and die.
	ANIMATE_LOOP = 3,			// Animate in a loop.
	ANIMATE_LOOP_REVERSE = 4	// Animate in a loop, but switch direction when hitting start or end of animation.
} anim_think_t;

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
	int		idx;				// Entity that emitter is tied to
	float	ltime;				// Emitter's lifetime
	int		max_emit;			// Total amount of particles that can be emitted. Value of 0 disregards this.
	int		max_emit_frame;		// How much an emitter can emit during a frame
	float	emit_freq;			// How frequently an emitter can emit particles

	unsigned int col_flags;		// How Emitters interact with the world.

	struct emitter_s* next;
} emitter_t;

//===============================
//	Color Ramps
//	For those who miss using color ramps from
//	good ol' QParticles.
//===============================

struct color_ramp_s
{
	int			size;			// How many colors our given ramp has.
	color24		colors[10];		// Array where you can define your own colors.
};

struct anim_info_s
{
	unsigned int	frame;
	float			alpha;
	Vector2D		scale;
	vec3_t			color;
};

//===============================
//	Particle struct.
//===============================

typedef struct partdan_s
{
	//
	// Variables that users can freely modify.
	//

	// Movement
	vec3_t	org;		// Particle's origin
	vec3_t	vel;		// Particle's velocity
	vec3_t	accel;		// Particle's rate of acceleration

	float	ang;		// Particle's current angles
	float	angvel;		// Particle's angular velocity

	float	bounce;		// Particle's bouncefactor

	// Appearence
	const char		*sprite;		// Particle sprite name
	unsigned int	rendermode;		// Particle's rendermode
	float			brightness;		// Particle's brightness for TriAPI

	// Animation
	struct anim_info_s		cur;	// Current frame, alpha, scale, and color.
	struct anim_info_s		start;	// start frame, alpha, scale, and color.
	struct anim_info_s		end;	// Ending frame, alpha, scale, and color.

	vec3_t			color_step;		// Particle color rate of change
	unsigned int	framerate;		// Particle's framerate
	float			alpha_step;		// Particle transparency rate of change
	Vector2D		scale_step;		// Particle's size rate of change

	// Behavior
	unsigned int	idx;			// Entity that particle is tied to
	float			ltime;			// Particle's lifetime

	unsigned int	col_flags;		// How Particles interact with the world.
	anim_think_t	anim_think;		// Particle's animation behavior
	anim_think_t	alpha_think;	// Particle's transparency behavior
	anim_think_t	scale_think;	// Particle's scaling behavior
	anim_think_t	color_think;	// Particle's color thinking behavior
	light_think_t	light_think;	// Particle's light-level behavior

	//
	// Variables that only the code can modify.
	//

	vec3_t		old_org;			// old org - used for bouncing

	float		timestart;			// when were we first created?
	float		nextanim;			// Particle's animation thinker
	struct		model_s		*model;	// Particle's model pointer
	struct		partdan_s	*next;	// pointer to next particle in particle pointer pool
} ParticleDan;

class CParticleDan
{
public:
	ParticleDan*	GetParticlePointer( void );
	void			ManageParticles( void );
	int				MsgFunc_AddEmitter( const char *pszName,  int iSize, void *pbuf );

	vec3_t		RGBToColor4f( float r, float g, float b );
	void		SetColor( ParticleDan* p, vec3_t start, vec3_t end, vec3_t step, anim_think_t mode = DO_NOTHING );
	void		SetScale( ParticleDan* p, Vector2D start, Vector2D end, Vector2D step, anim_think_t mode = DO_NOTHING );
	void		SetAlpha( ParticleDan* p, float start, float end, float step, anim_think_t mode = DO_NOTHING );
	void		SetAnimate( ParticleDan* p, int start, int end, int framerate, anim_think_t mode = DO_NOTHING );

	int			Init( void );
	int			VidInit(void);
private:
	emitter_t		m_Emitters[MAX_EMITTERS];
	ParticleDan		m_Particles[MAX_PARTICLES];
	ParticleDan*	m_ActiveParticles;
	ParticleDan*	m_FreeParticles;
	int				m_iActiveParticles;

	void		ParticleThink( ParticleDan* p );
	void		ParticleDraw( ParticleDan* p );
	void		ParseScript( const char* pszScript );

	cvar_t*		m_pCvarTestParticles;
	void		CreateTestParticles( void );
};

extern CParticleDan gParticleDan;

#endif