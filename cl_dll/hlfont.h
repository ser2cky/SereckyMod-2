#if !defined( HLFONTH )
#define HLFONTH
#ifdef _WIN32
#pragma once
#endif

//=======================================
//	Font struct.
//=======================================

#define NUM_GLYPHS 256
#define MAX_FONTS 12

#include "qfont.h"

typedef struct hl_font_s
{
	HSPRITE		hPic;
	int 		width, height;
	int			rowcount;
	int			rowheight;
	charinfo	fontinfo[ NUM_GLYPHS ];
} hl_font_t;

//=======================================
//	Class def.
//=======================================

class CHudFont
{
public:
	hl_font_t*	GetFontPointer( char* fontname );
	void		VidInit( void );
	int			DrawCharacter( hl_font_t* font, int x, int y, int num, int r, int g, int b, int frame, int rendermode );
	int			DrawString( hl_font_t* font, int x, int y, int r, int g, int b, int frame, int rendermode, char* str );
	hl_font_t*	gCreditsFont;
private:
	hl_font_t	m_Fonts[MAX_FONTS];
	int			m_iNumFonts;
};

extern CHudFont gFont;

#endif