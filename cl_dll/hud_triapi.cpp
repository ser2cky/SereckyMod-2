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
//	hud_triapi.cpp
//	Purpose: Replacement drawing functions for HUD
//	graphics that support scaling controlled by
//	the hud_scale CVAR. Thanks to ScriptedSnark's
//	Goldsrc reverse engineer btw.
//
//	History:
//	FEB-21-26: Started. It all works now...
//	FEB-22-26: Further refinement.
//
//=======================================

#include "hud.h"
#include "cl_util.h"
#include "com_model.h"
#include "r_studioint.h"
#include "triangleapi.h"

extern engine_studio_api_t IEngineStudio;
model_s *gpSprite;

//=======================================
//	R_GetSpriteframe
//=======================================

mspriteframe_t *R_GetSpriteFrame(model_s* pModel, int frame)
{
	mspriteframe_t	*pspriteframe;
	msprite_t		*pSprite;

	pSprite = (msprite_t*)pModel->cache.data;

	// Invalid frame
	if ((frame >= pSprite->numframes) || (frame < 0))
	{
		gEngfuncs.Con_Printf("Sprite: no such frame %d\n", frame);
		frame = 0;
	}

	if (pSprite->frames[frame].type == SPR_SINGLE)
		pspriteframe = pSprite->frames[frame].frameptr;
	else
		pspriteframe = NULL;

	return pspriteframe;
}

//=======================================
//	RGBAToColor4f
//=======================================

void RGBAToColor4f( float *r, float *g, float *b, float *a )
{
	if ( r != NULL ) *r *= ( 1.0f / 255.0f );
	if ( g != NULL ) *g *= ( 1.0f / 255.0f );
	if ( b != NULL ) *b *= ( 1.0f / 255.0f );
	if ( a != NULL ) *a *= ( 1.0f / 255.0f );
}

//=======================================
//	AdjustTransformations
//=======================================

void AdjustTransformations( unsigned int *x, unsigned int *y, unsigned int *w, unsigned int *h )
{
	*x *= gHUD.GetXScale();
	*y *= gHUD.GetYScale();
	*w *= gHUD.GetXScale();
	*h *= gHUD.GetYScale();
}

//=======================================
//	DrawQuad
//=======================================

void DrawQuad( unsigned int x, unsigned int y, unsigned int w, unsigned int h, float fLeft, float fRight, float fTop, float fBottom )
{
	AdjustTransformations( &x, &y, &w, &h );

	gEngfuncs.Con_Printf("%.2f %.2f %.2f %.2f\n", fLeft, fRight, fTop, fBottom);

	gEngfuncs.pTriAPI->Begin( TRI_QUADS );

	gEngfuncs.pTriAPI->TexCoord2f( fLeft, fTop );
	gEngfuncs.pTriAPI->Vertex3f( x, y, 0 );

	gEngfuncs.pTriAPI->TexCoord2f( fRight, fTop );
	gEngfuncs.pTriAPI->Vertex3f( x + w, y, 0 );

	gEngfuncs.pTriAPI->TexCoord2f( fRight, fBottom );
	gEngfuncs.pTriAPI->Vertex3f( x + w, y + h, 0 );

	gEngfuncs.pTriAPI->TexCoord2f( fLeft, fBottom );
	gEngfuncs.pTriAPI->Vertex3f( x, y + h, 0 );

	gEngfuncs.pTriAPI->End();
	gEngfuncs.pTriAPI->RenderMode( kRenderNormal );
}

//=======================================
//	TriApi_SetSprite
//=======================================

void TriApi_SetSprite( HSPRITE hPic, int r, int g, int b )
{
	float r_, g_, b_;

	if ( !IEngineStudio.IsHardware() )
	{
		gEngfuncs.pfnSPR_Set( hPic, r, g, b );
		return;
	}

	gpSprite = (model_s*)gEngfuncs.GetSpritePointer( hPic );
	r_ = r; b_ = b; g_ = g;
	RGBAToColor4f(&r_, &g_, &b_, NULL);
	gEngfuncs.pTriAPI->Color4f( r_, g_, b_, 1.0f );
}

//=======================================
//	ValidateWRect
//	Verify that this is a valid, properly ordered rectangle.
//=======================================

int ValidateWRect( const wrect_t* prc )
{
	if (!prc)
		return FALSE;

	if ((prc->left >= prc->right) || (prc->top >= prc->bottom))
	{
		//!!!UNDONE Dev only warning msg
		return FALSE;
	}

	return TRUE;
}

//=======================================
//	IntersectWRect
//	classic interview question
//=======================================

int IntersectWRect( const wrect_t* prc1, const wrect_t* prc2, wrect_t* prc )
{
	wrect_t rc;

	if (!prc)
		prc = &rc;

	prc->left = max(prc1->left, prc2->left);
	prc->right = min(prc1->right, prc2->right);

	if (prc->left < prc->right)
	{
		prc->top = max(prc1->top, prc2->top);
		prc->bottom = min(prc1->bottom, prc2->bottom);

		if (prc->top < prc->bottom)
			return TRUE;
	}

	return FALSE;
}

//=======================================
//	AdjustSubRect
//=======================================

void AdjustSubRect( mspriteframe_t* pFrame, float* pfLeft, float* pfRight, float* pfTop, float* pfBottom, int* pw, int* ph, const wrect_t* prcSubRect )
{
	wrect_t rc;

	if (!ValidateWRect(prcSubRect))
		return;

	// clip sub rect to sprite

	rc.top = rc.left = 0;
	rc.right = *pw;
	rc.bottom = *ph;

	if (!IntersectWRect(prcSubRect, &rc, &rc))
		return;

	*pw = rc.right - rc.left;
	*ph = rc.bottom - rc.top;

	*pfLeft = rc.left / (float)pFrame->width;
	*pfRight = rc.right / (float)pFrame->width;
	*pfTop = rc.top / (float)pFrame->height;
	*pfBottom = rc.bottom / (float)pFrame->height;
}

//=======================================
//	TriApi_DrawFrame
//=======================================

void TriApi_DrawFrame( mspriteframe_t* pFrame, int x, int y, const wrect_t* prcSubRect )
{
	float	fLeft = 0.0f;
	float	fRight = 1.0f;
	float	fTop = 0.0f;
	float	fBottom = 1.0f;

	int		iWidth;
	int		iHeight;

	iWidth = pFrame->width;
	iHeight = pFrame->height;

	if ( prcSubRect != NULL )
		AdjustSubRect(pFrame, &fLeft, &fRight, &fTop, &fBottom, &iWidth, &iHeight, prcSubRect);

	DrawQuad( x, y, iWidth, iHeight, fLeft, fRight, fTop, fBottom );
}

//=======================================
//	TriApi_DrawSprite
//=======================================

void TriApi_DrawSprite( int frame, int x, int y, const wrect_t* prc )
{
	mspriteframe_t* pFrame;

	if ( !IEngineStudio.IsHardware() )
	{
		gEngfuncs.pfnSPR_Draw( frame, x, y, prc );
		return;
	}

	if (gpSprite == NULL)
		return;
	
	pFrame = R_GetSpriteFrame(gpSprite, frame);

	if (pFrame == NULL)
		gEngfuncs.Con_Printf("TriApi_DrawSprite: invalid frame\n");

	gEngfuncs.pTriAPI->SpriteTexture( gpSprite, frame );
	TriApi_DrawFrame(pFrame, x, y, prc);
}

//=======================================
//	TriApi_DrawHoles
//=======================================

void TriApi_DrawHoles( int frame, int x, int y, const wrect_t* prc )
{
	mspriteframe_t* pFrame;

	if ( !IEngineStudio.IsHardware() )
	{
		gEngfuncs.pfnSPR_DrawHoles( frame, x, y, prc );
		return;
	}

	if (gpSprite == NULL)
		return;

	pFrame = R_GetSpriteFrame(gpSprite, frame);

	gEngfuncs.pTriAPI->SpriteTexture( gpSprite, frame );
	TriApi_DrawFrame(pFrame, x, y, prc);
}

//=======================================
//	TriApi_DrawAdditive
//=======================================

void TriApi_DrawAdditive( int frame, int x, int y, const wrect_t* prcSubRect )
{
	mspriteframe_t* pFrame;

	if ( !IEngineStudio.IsHardware() )
	{
		gEngfuncs.pfnSPR_DrawAdditive( frame, x, y, prcSubRect );
		return;
	}

	if (gpSprite == NULL)
		return;

	pFrame = R_GetSpriteFrame(gpSprite, frame);

	gEngfuncs.pTriAPI->SpriteTexture( gpSprite, frame );
	gEngfuncs.pTriAPI->RenderMode( kRenderTransAdd );
	TriApi_DrawFrame(pFrame, x, y, prcSubRect);
}

//=======================================
//	TriApi_FillRGBA
//	New FillRGBA function that lets you define a different rendermode, incase you'd
//	like to fill in something that isn't additive.
//=======================================

void TriApi_FillRGBA( int x, int y, int width, int height, int r, int g, int b, int a, int rendermode )
{
	model_s* sprite;
	float r_, g_, b_, a_;

	if ( !IEngineStudio.IsHardware() )
	{
		gEngfuncs.pfnFillRGBA( x, y, width, height, r, g, b, a );
		return;
	}

	if ((sprite = (struct model_s*)gEngfuncs.GetSpritePointer(SPR_Load("sprites/white.spr"))) == NULL)
	{
		gEngfuncs.Con_Printf("TriApi_FillRGBA: Missing \"sprites/white.spr\"!!!");
		return;
	}

	r_ = r; b_ = b; g_ = g; a_ = a;
	RGBAToColor4f( &r_, &g_, &b_, &a_ );

	gEngfuncs.pTriAPI->RenderMode( rendermode );
	gEngfuncs.pTriAPI->CullFace( TRI_NONE );
	gEngfuncs.pTriAPI->Color4f( r_, g_, b_, a_ );
	gEngfuncs.pTriAPI->SpriteTexture( sprite, 0 );
	DrawQuad( x, y, width, height, 0.0f, 1.0f, 0.0f, 1.0f);
}

//=======================================
//	DrawTest
//	Un-if 0 this out if you'd like to puke all over your screen.
//=======================================

void DrawTest( void )
{
#if 0
	if (gHUD.m_pCvarScale)
	{
		gHUD.m_pCvarScale->value = ((sin(gHUD.m_flTime * 2.0f) + 1.0f) / 2.0f) + 1.0f;
		gEngfuncs.Con_Printf("%.2f\n", gHUD.m_pCvarScale->value);
	}

	TriApi_FillRGBA( 0, gHUD.m_iScreenHeight - 64, 320, 64, 255, 0, 0, 50, kRenderTransAdd );
#endif
}