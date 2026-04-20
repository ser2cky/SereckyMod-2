
//=======================================
//	hud_drawfont.cpp
//	Purpose: Let people load up their own
//	custom bit-map based fonts. Font bitmaps are
//	loaded from .spr files, and font information
//	pertaining to offsets & what have it are stored
//	in .qfnt files.
//	
//	Also thanks to the people behind the NetTest R.E
//	for reversing the qfont drawing stuff!!!
//
//	History:
//	FEB-23-26: Started.
//
//=======================================

#include "hud.h"
#include "cl_util.h"
#include "hlfont.h"
#include "triangleapi.h"
#include "r_studioint.h"

extern engine_studio_api_t IEngineStudio;

CHudFont gFont;

//=======================================
//	VidInit
//	
//=======================================

void CHudFont::VidInit( void )
{
	m_iNumFonts = 0;
	memset( m_Fonts, 0, sizeof( m_Fonts ) );

	gCreditsFont = GetFontPointer( "fonts/font" );
}

//=======================================
//	GetFontPointer
//	well. it does what it says... looks for a .qfnt associated with
//	our sprite, so it can then load in the rest like an hsprite and
//	offset info...
//=======================================

hl_font_t* CHudFont::GetFontPointer( char* fontname )
{
	char load_font[256];
	qfont_t* font;
	int i, j;

	sprintf( load_font, "%s.qfnt", fontname );
	font = (qfont_t*)gEngfuncs.COM_LoadFile( load_font, 5, NULL );

	if (!font)
	{
		gEngfuncs.Con_Printf( "GetFontPointer: Failed to get font data at \"\".qfnt!\n", fontname );
		return NULL;
	}

	for ( i = 0; i < MAX_FONTS; i++ )
	{
		if ( !m_Fonts[i].hPic )
		{
			sprintf(load_font, "%s.spr", fontname);
			gEngfuncs.Con_Printf("%s\n", load_font);
			m_Fonts[i].hPic = SPR_Load( load_font );

			m_Fonts[i].width = font->width;
			m_Fonts[i].height = font->height;
			m_Fonts[i].rowcount = font->rowcount;
			m_Fonts[i].rowheight = font->rowheight;
			//gEngfuncs.Con_Printf("width: %d height: %d rowcount: %d rowheight: %d\n", font->width, font->height, font->rowcount, font->rowheight);

			for ( j = 0; j < NUM_GLYPHS; j++ )
			{
				m_Fonts[i].fontinfo[j].startoffset = font->fontinfo[j].startoffset;
				m_Fonts[i].fontinfo[j].charwidth = font->fontinfo[j].charwidth;
				//gEngfuncs.Con_Printf( "GLYPH:%d start ofs: %d charwidth: %d\n", j, font->fontinfo[j].startoffset, font->fontinfo[j].charwidth );
			}

			gEngfuncs.COM_FreeFile( font );
			return &m_Fonts[i];
		}
	}

	gEngfuncs.COM_FreeFile(font);
	return NULL;
}

//=======================================
//	DrawCharacter
//=======================================

int CHudFont::DrawCharacter( hl_font_t* font, int x, int y, int num, int r, int g, int b, int frame, int rendermode, int flags )
{
	int			rowheight, charwidth;
	int			row, col;
	wrect_t		rc;

	num &= 255;

	rowheight = font->rowheight;

	if ( y <= -rowheight )
		return 0;			// totally off screen

	charwidth = font->fontinfo[num].charwidth;

	if ( y < 0 || num == 32 )
		return charwidth;		// space

	col = font->fontinfo[num].startoffset & 255;
	row = ( font->fontinfo[num].startoffset & ~255 ) >> 8;
	
	rc.left = col;
	rc.right = col + charwidth;
	rc.top = row;
	rc.bottom = row + rowheight;

	SPR_Set( font->hPic, r, g, b );
	switch ( rendermode )
	{
		case kRenderTransAdd:
			SPR_DrawAdditive( frame, x, y, &rc, flags );
			break;
		case kRenderTransTexture:
			SPR_DrawHoles( frame, x, y, &rc, flags );
			break;
		case kRenderNormal:
		default:
			SPR_Draw( frame, x, y, &rc, rendermode, flags );
			break;
	}

	return charwidth;
}

//=======================================
//	DrawString
//=======================================

int CHudFont::DrawString( hl_font_t* font, char* str, int x, int y, int r, int g, int b, int frame, int rendermode, int flags )
{
	if ( font == NULL )
		return 0;

	while (*str)
	{
		x += DrawCharacter( font, x, y, *str, r, g, b, frame, rendermode, flags );
		str++;
	}
	return x;
}
