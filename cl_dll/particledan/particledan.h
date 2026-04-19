#if !defined( PARTICLEDANH )
#define PARTICLEDANH
#ifdef _WIN32
#pragma once
#endif

//===============================
//	ParticleDan Constants
//===============================

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
//	Particle Types.
//===============================

typedef enum
{
	// Draw a normal particle.
	NORMAL_PARTICLE = 0,

	// Draw a particle using the same scaling 
	// calculations that GlQuake uses.
	QUAKE_PARTICLE = 1
} part_type_t;

//===============================
//	Animation behavior
//===============================

typedef enum
{
	// Don't animate.
	DO_NOTHING = 0,

	// Animate once.
	ANIMATE_ONCE = 1,

	// Animate once and die.
	ANIMATE_DIE = 2,

	// Animate in a loop.
	ANIMATE_LOOP = 3,

	// Animate in a loop, but switch direction 
	// when hitting start or end of animation.
	ANIMATE_LOOP_REVERSE = 4
} anim_think_t;

//===============================
//	Light-level behavior
//===============================

typedef enum
{
	// Completely disregard the light level
	// information below us!!!
	IGNORE_LIGHT = 0,

	// Check for the ground's light level
	// information once, and then set particle
	// colors accordingly.
	LIGHT_CHECK_ONCE = 1,

	// Check for the ground's light level
	// information every frame, and then set 
	// particle colors accordingly.
	LIGHT_CHECK_EVERY_FRAME = 2
} light_think_t;

//===============================
//	Color Ramps
// 
//	This is for the people that who'd like to have
//	Quake1's particle ramps, not hardcoded to any 
//	color pallete.
//===============================

struct color_ramp_s
{
	int			size;			// How many colors a ramp has.
	color24		colors[10];		// User defined colors. Can go up to as much as 10.
};

//===============================
//	Anim. Info Struct.
// 
//	Added to reduce the ammount of fields I have
//	to type out.
//===============================

struct anim_info_s
{
	unsigned int	frame;		// Sprite frame.
	float			alpha;		// Particle's transparency.
	Vector2D		scale;		// Particle's scale.
	vec3_t			color;		// Partice's color.
	int				rampindex;	// Particle's color ramp.
};

//===============================
//	Particle struct.
//===============================

typedef struct partdan_s
{
	//=====================================================
	// Fields that users can freely modify.
	//=====================================================

	// Fields that can move particles around.
	vec3_t	org;		// Current origin.
	vec3_t	vel;		// Current velocity.
	vec3_t	accel;		// Current acceleration.

	float	angles;		// Current angles.
	float	angvel;		// Current angular Velocity.

	float	bounce;		// Current bounce-Factor.
	float	friction;	// Current friction for sliding-type collisions.

	// Fields that alter the particle's appearence
	const char		*sprite;		// Sprite name.
	unsigned int	rendermode;		// TRIAPI rendermode.
	float			brightness;		// TRIAPI brightness.

	struct anim_info_s	cur;		// Current frame, alpha, scale, and color.
	struct anim_info_s	start;		// Starting frame, alpha, scale, and color.
	struct anim_info_s	end;		// Ending frame, alpha, scale, and color.

	vec3_t			color_step;		// Color rate of change.
	float			alpha_step;		// Alpha rate of change.
	Vector2D		scale_step;		// Scale rate of change.
	unsigned int	framerate;		// Animation framerate.

	struct color_ramp_s		*ramp;	// Color ramp of a Particle.
	float			ramp_step;		// Particle's color ramp step.

	// Fields that control the particle's behaviour.
	unsigned int	idx;			// Entity particle is tied to.
	float			ltime;			// Particle's lifetime

	unsigned int	death_flags;	// How a particle dies.
	unsigned int	col_flags;		// How Particles interact with the world.
	anim_think_t	anim_think;		// Animation behavior
	anim_think_t	alpha_think;	// Alpha behavior
	anim_think_t	scale_think;	// Scaling behavior
	anim_think_t	color_think;	// Color thinking behavior
	light_think_t	light_think;	// Light-level behavior

	void			( *think ) ( struct partdan_s* p ); // Custom think function for particles.

	//=====================================================
	// Fields that only the code can modify.
	//=====================================================

	vec3_t		old_org;			// old org - used for bouncing
	float		nextanim;			// Particle's animation thinker
	struct		model_s		*model;	// Particle's model pointer
	struct		partdan_s	*next;	// pointer to next particle in particle pool
} ParticleDan;

//=========================================================================
//
//	EMITTERS
//
//=========================================================================

//===============================
//	Particle emission behavior
//===============================

typedef enum
{
	// Emit particles normally.
	EMIT_NORMAL = 0,

	// Emit particles in a spiral.
	EMIT_SPIRAL = 1,

	// Emit particles in a sphere.
	EMIT_SPHERE = 2,

	// Emit particles in a stream.
	EMIT_STREAM = 3,

	// Emit particles in a field.
	// Relies on being tied to an entity
	// with a defined "absmin" and "absmax."
	EMIT_FIELD = 4
} emit_mode_t;

//===============================
//	Emitter struct.
//===============================

typedef struct emitter_s
{
	//=====================================================
	// Fields that users can freely modify.
	//=====================================================

	// Fields that can move emitters around.
	vec3_t	org;		// Current origin.
	vec3_t	vel;		// Current velocity.
	vec3_t	accel;		// Current acceleration.

	float	bounce;		// Current bounce-Factor.
	float	friction;	// Current friction for sliding-type collisions.

	// Fields that controll the emitter's behaviors.
	unsigned int	idx;			// Entity that the emitter is tied to.

	unsigned int	max_emit;		// How many particles an emitter can emit.
	float			emit_intv;		// How frequently an emitter emits particles.

	emit_mode_t		emit_mode;		// How the emitter emits particles.

	//=====================================================
	// Fields that only the code can modify.
	//=====================================================

	struct emitter_s* next;			// Pointer to the next emitter in a emitter pool.
} emitter_t;

//===============================
//	CParticleDan Class.
//===============================

class CParticleDan
{
public:
	// Pointer to the classic Quake particle...
	model_s* m_pClassicParticle;

	//=====================================================
	// Utility functions.
	//=====================================================

	vec3_t		RGBToColor4f( float r, float g, float b );
	void		SetColor( ParticleDan* p, vec3_t start, vec3_t end, vec3_t step, anim_think_t mode = DO_NOTHING );
	void		SetScale( ParticleDan* p, Vector2D start, Vector2D end, Vector2D step, anim_think_t mode = DO_NOTHING );
	void		SetAlpha( ParticleDan* p, float start, float end, float step, anim_think_t mode = DO_NOTHING );
	void		SetAnimate( ParticleDan* p, int start, int end, int framerate, anim_think_t mode = DO_NOTHING );

	//=====================================================
	// Basic parts of the particle's thinking routine.
	//=====================================================

	void		TransformParticle( ParticleDan* p );
	void		AnimateParticle( ParticleDan* p );

	//=====================================================
	// Public internals
	//=====================================================

	int			Init( void );
	int			VidInit(void);

	ParticleDan*	GetParticlePointer( void );
	void			ManageParticles( void );
	int				MsgFunc_TieToEmitter( const char *pszName,  int iSize, void *pbuf );
private:
	//=====================================================
	// Private internals
	//=====================================================

	emitter_t		m_Emitters[MAX_EMITTERS];
	ParticleDan		m_Particles[MAX_PARTICLES];
	ParticleDan*	m_ActiveParticles;
	ParticleDan*	m_FreeParticles;
	int				m_iActiveParticles;

	void		ParticleThink( ParticleDan* p );
	void		ParticleDraw( ParticleDan* p );
	void		ParseScripts( void );

	cvar_t*		m_pCvarTestParticles;
	void		CreateTestParticles( void );
};

extern CParticleDan gParticleDan;

#endif