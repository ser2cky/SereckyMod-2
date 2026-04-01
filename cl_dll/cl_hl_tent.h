#if !defined( TEMP_ENTITY_H )
#define TEMP_ENTITY_H
#ifdef _WIN32
#pragma once
#endif

typedef struct tempent_s	TEMPENTITY;
typedef struct model_s		model_t;

class CTempEntities
{
public:
	void	Init( void );
	void	VidInit( void );
	void	Shutdown( void );
	int		MsgFunc_ParseTEnt( const char* pszName, int iSize, void* pbuf );

	// Tracers
	void	R_TracerEffect( vec_t* start, vec_t* end );
	void	R_Implosion( vec_t* end, float radius, int count, float life );
	void	R_StreakSplash( vec_t* pos, vec_t* dir, int color, int count, float speed, int velocityMin, int velocityMax );

	// QParticles
	void	R_ParticleExplosion( vec_t* org );
	void	R_ParticleExplosion2( vec_t* org, int colorStart, int colorLength );
	void	R_BlobExplosion( vec_t* org );
	void	R_RunParticleEffect( vec_t* org, vec_t* dir, int color, int count );
	void	R_FlickerParticles( vec_t* org );
	void	R_LavaSplash( vec_t* org );
	void	R_LargeFunnel( vec_t* org, int reverse );
	void	R_TeleportSplash( vec_t* org );
	void	R_ShowLine( vec_t* start, vec_t* end );
	void	R_BloodStream( vec_t* org, vec_t* dir, int pcolor, int speed );
	void	R_Blood( vec_t* org, vec_t* dir, int pcolor, int speed );
	void	R_RocketTrail( vec_t *start, vec_t *end, int type );

	// Temp Ents.
	void	R_FizzEffect( cl_entity_t* pent, int modelIndex, int density );
	void	R_Bubbles( vec_t* mins, vec_t* maxs, float height, int modelIndex, int count, float speed );
	void	R_BubbleTrail( vec_t* start, vec_t* end, float height, int modelIndex, int count, float speed );
	void	R_Sprite_Trail(int type, vec_t* start, vec_t* end, int modelIndex, int count, float life, float size, float amplitude, int renderamt, float speed);
	void	R_TempSphereModel( float* pos, float speed, float life, int count, int modelIndex );
	void	R_BreakModel( float* pos, float* size, float* dir, float random, float life, int count, int modelIndex, char flags );
	TEMPENTITY*		R_TempSprite( float* pos, float* dir, float scale, int modelIndex, int rendermode, int renderfx, float a, float life, int flags );
	void	R_Sprite_Spray( vec_t* pos, vec_t* dir, int modelIndex, int count, int speed, int iRand );
	void	R_SparkEffect( float* pos, int count, int velocityMin, int velocityMax );
	void	R_FunnelSprite( float* org, int modelIndex, int reverse );
	void	R_RicochetSprite( float* pos, model_t* pmodel, float duration, float scale );
	void	R_BloodSprite( vec_t* org, int colorindex, int modelIndex, int modelIndex2, float size );
	TEMPENTITY*		R_DefaultSprite( float* pos, int spriteIndex, float framerate );
	void	R_Sprite_Smoke( TEMPENTITY* pTemp, float scale );
	void	R_Sprite_Explode( TEMPENTITY* pTemp, float scale, int flags );
	void	R_Sprite_WallPuff( TEMPENTITY* pTemp, float scale );
	void	R_MuzzleFlash( float* pos1, int type );
private:
	// important pointer thingys
	cvar_t*		m_pCvarDecals;
	cvar_t*		m_pCvarTracerSpeed;
	cvar_t*		m_pCvarTracerOffset;

	// Muzzle flash sprites
	model_t*	cl_sprite_muzzleflash[3];
	model_t*	cl_sprite_ricochet;
	model_t*	cl_sprite_shell;

	// particle ramps
	int		ramp1[8] = { 0x6F, 0x6D, 0x6B, 0x69, 0x67, 0x65, 0x63, 0x61 };
	int		ramp2[8] = { 0x6F, 0x6E, 0x6D, 0x6C, 0x6B, 0x6A, 0x68, 0x66 };
	int		ramp3[8] = { 0x6D, 0x6B, 6, 5, 4, 3, 0, 0 };

	// spark ramps
	int		gSparkRamp[9] = { 0xFE, 0xFD, 0xFC, 0x6F, 0x6E, 0x6D, 0x6C, 0x67, 0x60 };
};

extern CTempEntities gTempEnt;

#endif