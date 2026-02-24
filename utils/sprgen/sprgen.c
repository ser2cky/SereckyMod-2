/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
****/

//
// spritegen.c: generates a .spr file from a series of .lbm frame files.
// Result is stored in /raid/quake/id1/sprites/<scriptname>.spr.
//
#pragma warning(disable : 4244)     // type conversion warning.
#define INCLUDELIBS


#ifdef NeXT
#include <libc.h>
#endif

#include "spritegn.h"

#define MAX_BUFFER_SIZE		0x100000
#define MAX_FRAMES			1000

dsprite_t		sprite;
byte			*byteimage, *lbmpalette;
int				byteimagewidth, byteimageheight;
byte			*lumpbuffer = NULL, *plump, *lump_p;
char			spritedir[1024];
char			spriteoutname[1024];
char			fontoutname[1024];
int				framesmaxs[2];
int				framecount;

qboolean do16bit = true;
qboolean writefont = false;

typedef struct {
	spriteframetype_t	type;		// single frame or group of frames
	void				*pdata;		// either a dspriteframe_t or group info
	float				interval;	// only used for frames in groups
	int					numgroupframes;	// only used by group headers
} spritepackage_t;

#define NUM_GLYPHS 256
const unsigned kFontMarker = 254;

typedef struct
{
	short startoffset;
	short charwidth;
} charinfo;

typedef struct
{
	int 		width, height;
	int			rowcount;
	int			rowheight;
	charinfo	fontinfo[ NUM_GLYPHS ];
	byte 		data[4];
} qfont_t;

typedef struct
{
	int 		width, height;
	int			rowcount;
	int			rowheight;
	charinfo	fontinfo[ NUM_GLYPHS ];
} qfont_2_t;

qfont_2_t font;

spritepackage_t	frames[MAX_FRAMES];

void FinishSprite (void);
void Cmd_Spritename (void);


/*
============
WriteFrame
============
*/
void WriteFrame (FILE *spriteouthandle, int framenum)
{
	dspriteframe_t	*pframe;
	dspriteframe_t	frametemp;

	pframe = (dspriteframe_t *)frames[framenum].pdata;
	frametemp.origin[0] = LittleLong (pframe->origin[0]);
	frametemp.origin[1] = LittleLong (pframe->origin[1]);
	frametemp.width = LittleLong (pframe->width);
	frametemp.height = LittleLong (pframe->height);

	SafeWrite (spriteouthandle, &frametemp, sizeof (frametemp));
	SafeWrite (spriteouthandle,
			   (byte *)(pframe + 1),
			   pframe->height * pframe->width);
}


/*
============
WriteSprite
============
*/
void WriteSprite (FILE *spriteouthandle)
{
	int			i, groupframe, curframe;
	dsprite_t	spritetemp;

	sprite.boundingradius = sqrt (((framesmaxs[0] >> 1) *
								   (framesmaxs[0] >> 1)) +
								  ((framesmaxs[1] >> 1) *
								   (framesmaxs[1] >> 1)));

//
// write out the sprite header
//
	spritetemp.type = LittleLong (sprite.type);
	spritetemp.texFormat = LittleLong (sprite.texFormat);
	spritetemp.boundingradius = LittleFloat (sprite.boundingradius);
	spritetemp.width = LittleLong (framesmaxs[0]);
	spritetemp.height = LittleLong (framesmaxs[1]);
	spritetemp.numframes = LittleLong (sprite.numframes);
	spritetemp.beamlength = LittleFloat (sprite.beamlength);
	spritetemp.synctype = LittleFloat (sprite.synctype);
	spritetemp.version = LittleLong (SPRITE_VERSION);
	spritetemp.ident = LittleLong (IDSPRITEHEADER);

	SafeWrite (spriteouthandle, &spritetemp, sizeof(spritetemp));

	if( do16bit )
	{
		// Write out palette in 16bit mode
		short cnt = 256;
		SafeWrite( spriteouthandle, (void *) &cnt, sizeof(cnt) );
		SafeWrite( spriteouthandle, lbmpalette, cnt * 3 );
	}

//
// write out the frames
//
	curframe = 0;

	for (i=0 ; i<sprite.numframes ; i++)
	{
		SafeWrite (spriteouthandle, &frames[curframe].type,
				   sizeof(frames[curframe].type));

		if (frames[curframe].type == SPR_SINGLE)
		{
		//
		// single (non-grouped) frame
		//
			WriteFrame (spriteouthandle, curframe);
			curframe++;
		}
		else
		{
			int					j, numframes;
			dspritegroup_t		dsgroup;
			float				totinterval;

			groupframe = curframe;
			curframe++;
			numframes = frames[groupframe].numgroupframes;

		//
		// set and write the group header
		//
			dsgroup.numframes = LittleLong (numframes);

			SafeWrite (spriteouthandle, &dsgroup, sizeof(dsgroup));

		//
		// write the interval array
		//
			totinterval = 0.0;

			for (j=0 ; j<numframes ; j++)
			{
				dspriteinterval_t	temp;

				totinterval += frames[groupframe+1+j].interval;
				temp.interval = LittleFloat (totinterval);

				SafeWrite (spriteouthandle, &temp, sizeof(temp));
			}

			for (j=0 ; j<numframes ; j++)
			{
				WriteFrame (spriteouthandle, curframe);
				curframe++;
			}
		}
	}

}

/*
============
WriteFont
============
*/
void WriteFont (FILE *outfont)
{
	qfont_2_t	temp_font;
	int			i;

	// Aint no damn need to write data we already
	// got the image from the . SPR Font output

	// write qfont header my way B*tch

	temp_font.width = LittleLong(font.width);
	temp_font.height = LittleLong(font.height);
	temp_font.rowcount = LittleLong(font.rowcount);
	temp_font.rowheight = LittleLong(font.rowheight);

	printf("%d %d %d %d\n", temp_font.width, temp_font.height, temp_font.rowcount, temp_font.rowheight);

	for ( i = 0; i < NUM_GLYPHS; i++ )
	{
		temp_font.fontinfo[i].startoffset = LittleShort(font.fontinfo[i].startoffset);
		temp_font.fontinfo[i].charwidth = LittleShort(font.fontinfo[i].charwidth);
	}

	SafeWrite (outfont, &temp_font, sizeof(temp_font));
}

/*
============
ExecCommand
============
*/
int	cmdsrun;

void ExecCommand (char *cmd, ...)
{
	int		ret;
	char	cmdline[1024];
	va_list	argptr;
	
	cmdsrun++;
	
	va_start (argptr, cmd);
	vsprintf (cmdline,cmd,argptr);
	va_end (argptr);
	
//	printf ("=============================================================\n");
//	printf ("spritegen: %s\n",cmdline);
	fflush (stdout);
	ret = system (cmdline);
//	printf ("=============================================================\n");
	
	if (ret)
		Error ("spritegen: exiting due to error");
}

/*
==============
LoadScreen
==============
*/
void LoadScreen (char *name)
{
	static byte origpalette[256*3];
	int iError;

	printf ("grabbing from %s...\n",name);
	iError = LoadBMP( name, &byteimage, &lbmpalette );
	if (iError)
		Error( "unable to load file \"%s\"\n", name );

	byteimagewidth = bmhd.w;
	byteimageheight = bmhd.h;

	if (sprite.numframes == 0)
		memcpy( origpalette, lbmpalette, sizeof( origpalette ));
	else if (memcmp( origpalette, lbmpalette, sizeof( origpalette )) != 0)
	{
		Error( "bitmap \"%s\" doesn't share a pallette with the previous bitmap\n", name );
	}
}


/*
===============
Cmd_Type
===============
*/
void Cmd_Type (void)
{
	GetToken (false);
	if (!strcmp (token, "vp_parallel_upright"))
		sprite.type = SPR_VP_PARALLEL_UPRIGHT;
	else if (!strcmp (token, "facing_upright"))
		sprite.type = SPR_FACING_UPRIGHT;
	else if (!strcmp (token, "vp_parallel"))
		sprite.type = SPR_VP_PARALLEL;
	else if (!strcmp (token, "oriented"))
		sprite.type = SPR_ORIENTED;
	else if (!strcmp (token, "vp_parallel_oriented"))
		sprite.type = SPR_VP_PARALLEL_ORIENTED;
	else
		Error ("Bad sprite type\n");
}


/*
===============
Cmd_Texture
===============
*/
void Cmd_Texture (void)
{
	GetToken (false);

	if (!strcmp (token, "additive"))
		sprite.texFormat = SPR_ADDITIVE;
	else if (!strcmp (token, "normal"))
		sprite.texFormat = SPR_NORMAL;
	else if (!strcmp (token, "indexalpha"))
		sprite.texFormat = SPR_INDEXALPHA;
	else if (!strcmp (token, "alphatest"))
		sprite.texFormat = SPR_ALPHTEST;
	else
		Error ("Bad sprite texture type\n");
}


/*
===============
Cmd_Beamlength
===============
*/
void Cmd_Beamlength ()
{
	GetToken (false);
	sprite.beamlength = atof (token);
}


/*
===============
Cmd_Load
===============
*/
void Cmd_Load (void)
{
	GetToken (false);
	LoadScreen (ExpandPathAndArchive(token));
}

/*
===============
Cmd_Font
===============
*/
void Cmd_Font (void)
{
	int		x, y, y2, xl, x2, yl, xh, yh, i, j;
	int		index, offset;
	int		width;
	int		iCurX;	// current x in destination
	int		iMaxX;  // max x in destination
	
	byte	*pbuf, *pCur;
	qfont_t 			*header;
	dspriteframe_t		*pframe;
	byte	*pFont, *pSpr;

	int 	w, h;

	iMaxX = 255;
	iCurX = 0;

	// sprite setup
	pframe = (dspriteframe_t *)plump;
	frames[framecount].pdata = pframe;
	frames[framecount].type = SPR_SINGLE;
	frames[framecount].interval = (float)0.1;

	// Set up header
	header = (qfont_t *)lump_p;
	memset( header, 0, sizeof(qfont_t) );

	GetToken( false );
	header->width = header->rowheight = atoi( token );  //mwh why does width equal rowheight? 
	header->height = 1;
	lump_p = (byte *)header->data;
	pCur = (byte *)lump_p;
	memset( lump_p, 0xFF, 256 * 160);

	GetToken( false );
	index = atoi( token );

	while( index != -1 )
	{
		// Get/Process source bitmap coordinates
		GetToken (false);
		xl = atoi (token);
		GetToken (false);
		yl = atoi (token);
		GetToken (false);
		xh = xl-1+atoi (token);
		GetToken (false);
		yh = atoi (token) - 1;
		if (xl == -1)
		{
			xl = yl = 0;
			xh = byteimagewidth;
			yh = byteimageheight;
		}

		if( xh<xl || yh<yl || xl < 0 || yl<0 )
			Error( "GrabFont line %1: Bad size: %i, %i, %i, %i", scriptline, xl, yl, xh, yh );

		//
		// Fill in font information
		// Create a bitmap that is up to 256 wide and as tall as we need to accomadate the font.
		// We limit the bitmap to 256 because some 3d boards have problems with textures bigger 
		// than that. 
		//
		for( y=yl; y<yh; y+=header->rowheight+1 )
		{
			// Make sure we're at a marker
			if( y != yl )
			{
				for( y2=y-header->rowheight; y2<yh; y2++ )
					if( kFontMarker == (unsigned) SCRN(xl,y2) )
						break;

				if( y2 == yh )
					break;
				else if( y2 != y )
					Error( "GrabFont line %d: rowheight doesn't seem to match bitmap (%d, %d)\n", scriptline, y, y2 );
			}

			for( x=xl; x<xh; )
			{
				// find next marker
				for( x2=x+1; x2<xh; x2++ )
					if( kFontMarker == (unsigned) SCRN(x2,y) )
						break;

				// check for end of row
				if( x2 == xh )
					break;

				// Set up glyph information
				if( index >= NUM_GLYPHS )
				{
					printf( "GrabFont: Glyph out of range\n" );
					goto getout;
				}
		
				// Fill in glyph info
				header->fontinfo[ index ].charwidth = x2 - x - 1;
				
				// update header				

				// output glyph data
				iCurX += header->fontinfo[index].charwidth;
				
				// Will this glyph fit on this row?
				if (iCurX >= iMaxX)
				{	
					// Nope -- move to next row
					pCur = (byte *)lump_p + 256 * header->rowheight * header->height;
					header->height++;
					iCurX = header->fontinfo[index].charwidth;
				} 
			
				// copy over the glyph bytes
				pbuf = pCur;
				header->fontinfo[ index ].startoffset = pCur - (byte *) header->data;
				

				for(j = 1; j <= header->rowheight; j++)
				{
					byte *psrc = byteimage + (y + j) * byteimagewidth + (x + 1);

					for(i = x + 1; i < x2; i++)
						*pbuf++ = *psrc++;

					pbuf = pCur + j * 256;
				}
				
				// move the lump pointer to point at the next possible glyph
				pCur += header->fontinfo[index].charwidth;
				x = x2;
				index++;
			}
		}

		// Get next ASCII index
getout:
		GetToken (false);
		index = atoi (token);
	}

	// advance the lump pointer so that the last row is saved.
	lump_p += (256 * header->rowheight) * header->height;
	
	// JAY: Round up to the next power of 2 for GL
	offset = header->height * header->rowheight;

	y = (offset>128)?256:(offset>64)?128:(offset>32)?64:(offset>16)?32:16;
	if ( offset != y )
	{
		printf("Rounding font from 256x%d to 256x%d\n", offset, y );
		lump_p += (256 * (y - offset));
	}
	header->rowcount = header->height;
	header->height = y;
	
	// sprite
	
	w = 256;
	h = header->height;
	
	// file write shit

	font.width = header->width;
	font.height = header->height;
	font.rowcount = header->rowcount;
	font.rowheight = header->rowheight;

	for ( i = 0; i < NUM_GLYPHS; i++ )
	{
		font.fontinfo[i].startoffset = header->fontinfo[i].startoffset;
		font.fontinfo[i].charwidth = header->fontinfo[i].charwidth;
		//printf("%d %d\n", font.fontinfo[i].charwidth, font.fontinfo[i].startoffset);
	}

	//printf("%d %d %d %d\n", font.width, font.height, font.rowcount, font.rowheight);

	// ok now we're done
	pFont = (byte*)header->data;
	pSpr = (byte*)(pframe + 1);

	memcpy(pSpr, pFont, w * h);

	plump = pSpr + (w*h);

	//printf("%d %d\n", header->width, header->height);

	pframe->origin[0] = -(w >> 1);
	pframe->origin[1] = h >> 1;

	pframe->width = w;
	pframe->height = h;

	if (w > framesmaxs[0])
		framesmaxs[0] = w;
	
	if (h > framesmaxs[1])
		framesmaxs[1] = h;
}

/*
===============
Cmd_Frame
===============
*/
void Cmd_Frame ()
{
	int             x,y,xl,yl,xh,yh,w,h;
	byte            *screen_p, *source;
	int             linedelta;
	dspriteframe_t	*pframe;
	int				pix;
	
	GetToken (false);
	xl = atoi (token);
	GetToken (false);
	yl = atoi (token);
	GetToken (false);
	w = atoi (token);
	GetToken (false);
	h = atoi (token);

	//if ((xl & 0x07) || (yl & 0x07) || (w & 0x07) || (h & 0x07))
	//	Error ("Sprite dimensions not multiples of 8\n");

	if ((w > 256) || (h > 256))
		Error ("Sprite has a dimension longer than 256");

	xh = xl+w;
	yh = yl+h;

	pframe = (dspriteframe_t *)plump;
	frames[framecount].pdata = pframe;
	frames[framecount].type = SPR_SINGLE;

	if (TokenAvailable ())
	{
		GetToken (false);
		frames[framecount].interval = atof (token);
		if (frames[framecount].interval <= 0.0)
			Error ("Non-positive interval");
	}
	else
	{
		frames[framecount].interval = (float)0.1;
	}
	
	if (TokenAvailable ())
	{
		GetToken (false);
		pframe->origin[0] = -atoi (token);
		GetToken (false);
		pframe->origin[1] = atoi (token);
	}
	else
	{
		pframe->origin[0] = -(w >> 1);
		pframe->origin[1] = h >> 1;
	}

	pframe->width = w;
	pframe->height = h;

	if (w > framesmaxs[0])
		framesmaxs[0] = w;
	
	if (h > framesmaxs[1])
		framesmaxs[1] = h;
	
	plump = (byte *)(pframe + 1);

	screen_p = byteimage + yl*byteimagewidth + xl;
	linedelta = byteimagewidth - w;

	source = plump;

	for (y=yl ; y<yh ; y++)
	{
		for (x=xl ; x<xh ; x++)
		{
			pix = *screen_p;
			*screen_p++ = 0;
//			if (pix == 255)
//				pix = 0;
			*plump++ = pix;
		}
		screen_p += linedelta;
	}

	framecount++;
	if (framecount >= MAX_FRAMES)
		Error ("Too many frames; increase MAX_FRAMES\n");
}


/*
===============
Cmd_GroupStart	
===============
*/
void Cmd_GroupStart (void)
{
	int			groupframe;

	groupframe = framecount++;

	frames[groupframe].type = SPR_GROUP;
	frames[groupframe].numgroupframes = 0;

	while (1)
	{
		GetToken (true);
		if (endofscript)
			Error ("End of file during group");

		if (!strcmp (token, "$frame"))
		{
			Cmd_Frame ();
			frames[groupframe].numgroupframes++;
		}
		else if (!strcmp (token, "$load"))
		{
			Cmd_Load ();
		}
		else if (!strcmp (token, "$groupend"))
		{
			break;
		}
		else
		{
			Error ("$frame, $load, or $groupend expected\n");
		}

	}

	if (frames[groupframe].numgroupframes == 0)
		Error ("Empty group\n");
}


/*
===============
ParseScript	
===============
*/
void ParseScript (void)
{
	while (1)
	{
		GetToken (true);
		if (endofscript)
			break;
	
		if (!strcmp (token, "$load"))
		{
			Cmd_Load ();
		}
		if (!strcmp (token, "$spritename"))
		{
			Cmd_Spritename ();
		}
		else if (!strcmp (token, "$type"))
		{
			Cmd_Type ();
		}
		else if (!strcmp (token, "$texture"))
		{
			Cmd_Texture ();
		}
		else if (!strcmp (token, "$beamlength"))
		{
			Cmd_Beamlength ();
		}
		else if (!strcmp (token, "$sync"))
		{
			sprite.synctype = ST_SYNC;
		}
		else if (!strcmp (token, "$font"))
		{
			Cmd_Font ();
			sprite.numframes++;
			writefont = true;
		}
		else if (!strcmp (token, "$frame"))
		{
			if (!writefont)
			{
				Cmd_Frame ();
				sprite.numframes++;
			}
			else
			{
				printf("found font, skipping default frame loading\n");
			}
		}
		else if (!strcmp (token, "$load"))
		{
			Cmd_Load ();
		}
		else if (!strcmp (token, "$groupstart"))
		{
			if (!writefont)
			{
				Cmd_GroupStart ();
				sprite.numframes++;
			}
			else
			{
				printf("found font, skipping default frame loading\n");
			}
		}
	}
}

/*
==============
Cmd_Spritename
==============
*/
void Cmd_Spritename (void)
{
	if (sprite.numframes)
		FinishSprite ();

	GetToken (false);
	sprintf (spriteoutname, "%s%s.spr", spritedir, token);
	sprintf (fontoutname, "%s%s.qfnt", spritedir, token);
	memset (&sprite, 0, sizeof(sprite));
	memset (&font, 0, sizeof(font));
	framecount = 0;

	framesmaxs[0] = -9999999;
	framesmaxs[1] = -9999999;

	if ( !lumpbuffer )
		lumpbuffer = malloc (MAX_BUFFER_SIZE * 2);	// *2 for padding

	if (!lumpbuffer)
		Error ("Couldn't get buffer memory");

	plump = lumpbuffer;
	lump_p = lumpbuffer;
	sprite.synctype = ST_RAND;	// default
}

/*
==============
FinishSprite	
==============
*/
void FinishSprite (void)
{
	FILE	*spriteouthandle;
	FILE	*outfont;

	if (sprite.numframes == 0)
		Error ("no frames\n");

	if (!strlen(spriteoutname))
		Error ("Didn't name sprite file");
		
	if ((plump - lumpbuffer) > MAX_BUFFER_SIZE)
		Error ("Sprite package too big; increase MAX_BUFFER_SIZE");

	spriteouthandle = SafeOpenWrite (spriteoutname);
	printf ("saving in %s\n", spriteoutname);
	WriteSprite (spriteouthandle);
	fclose (spriteouthandle);
	
	if (writefont)
	{
		outfont = SafeOpenWrite (fontoutname);
		printf ("saving font info in %s\n", fontoutname);
		WriteFont (outfont);
		fclose (outfont);
	}

	printf ("spritegen: successful\n");
	printf ("%d frame(s)\n", sprite.numframes);
	printf ("%d ungrouped frame(s), including group headers\n", framecount);
	
	spriteoutname[0] = 0;		// clear for a new sprite
	fontoutname[0] = 0;		// clear for a new sprite
}

/*
==============
main
	
==============
*/
extern char qproject[];

int main (int argc, char **argv)
{
	int		i;

	printf( "sprgen.exe v 1.2 font edition (%s)\n", __DATE__ );
	printf ("----- Sprite Gen ----\n");

	if (argc < 2 || argc > 7 )
		Error ("usage: sprgen [-archive directory] [-no16bit] [-proj <project>] file.qc");
		
	for( i=1; i<argc; i++ )
	{
		if( *argv[ i ] == '-' )
		{
			if( !strcmp( argv[ i ], "-archive" ) )
			{
				archive = true;
				strcpy (archivedir, argv[i+1]);
				printf ("Archiving source to: %s\n", archivedir);
				i++;
			}
			else if( !strcmp( argv[ i ], "-proj" ) )
			{
				strcpy( qproject, argv[i+1] );
				i++;
			}
			else if( !strcmp( argv[ i ], "-16bit" ) )
			{
				do16bit = true;
			}
			else if( !strcmp( argv[ i ], "-no16bit" ) )
			{
				do16bit = false;
			} 
			else
			{
				printf( "Unsupported command line flag: '%s'", argv[i] );
			}

		}
		else
			break;
	}

	// SetQdirFromPath (argv[i]);
	ExtractFilePath (argv[i], spritedir);	// chop the filename

//
// load the script
//
	LoadScriptFile (argv[i]);
	
	ParseScript ();
	FinishSprite ();

	return 0;
}

