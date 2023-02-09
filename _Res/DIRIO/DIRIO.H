/*ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿*/
/*³  DIRIO v4.0      (c) Copyright 1995-2009 by Javier Gutiérrez Chamorro ³*/
/*³                                           http://guti.webcindario.com ³*/
/*³                                                /static.php?page=DIRIO ³*/ 
/*³                                                                       ³*/
/*³                                                                       ³*/
/*³                                                                       ³*/
/*³  Portable library replacement for CONIO functions.                    ³*/
/*³  Including direct video output for DOS16 and DOS32 and Windows API    ³*/
/*³  wrapper for Win32.                                                   ³*/
/*³                                                                       ³*/
/*³  ***   This source code is Public Domain, so you can use, copy  ***   ³*/
/*³  ***         distribute, and modify it free of charge.          ***   ³*/
/*³                                                                       ³*/
/*ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ*/


#if !defined(_DIRIO)
#define _DIRIO

#define _NOCURSOR			0
#define _SOLIDCURSOR		1
#define _NORMALCURSOR		2

struct text_info 
{
	unsigned char winleft;
	unsigned char wintop;
	unsigned char winright;
	unsigned char winbottom;
	unsigned char attribute;
	unsigned char normattr;
	unsigned char currmode;
	unsigned char screenheight;
	unsigned char screenwidth;
	unsigned char curx;
	unsigned char cury;
};


#if !defined(__COLORS)
#define __COLORS

enum COLORS 
{
	BLACK,          
	BLUE,
	GREEN,
	CYAN,
	RED,
	MAGENTA,
	BROWN,
	LIGHTGRAY,
	DARKGRAY,       
	LIGHTBLUE,
	LIGHTGREEN,
	LIGHTCYAN,
	LIGHTRED,
	LIGHTMAGENTA,
	YELLOW,
	WHITE
};
#endif

#define BLINK       128 /* blink bit */

enum text_modes 
{  
    LASTMODE = -1,
    BW40     = 0,
    C40,
    BW80,
    C80,
    MONO     = 7,
    C4350    = 64,
    /* New Color modes */
    C40X14   = 8,
    C40X21,
    C40X28,
    C40X43,
    C40X50,
    C40X60,
    C80X14,
    C80X21,
    C80X28,
    C80X43,
    C80X50,
    C80X60,
    /* New Black & White modes */
    BW40X14,
    BW40X21,
    BW40X28,
    BW40X43,
    BW40X50,
    BW40X60,
    BW80X14,
    BW80X21,
    BW80X28,
    BW80X43,
    BW80X50,
    BW80X60,
    /* New Monochrome modes */
    MONO14,             /* Invalid VGA mode */
    MONO21,
    MONO28,
    MONO43,
    MONO50,
    MONO60,
    _ORIGMODE = 65      /* original mode at program startup */
};

#define directvideo  _directvideo
#define _getch	getch
extern int _directvideo;
extern int _wscroll;

#define PASS_MAX    8   /* max. no. of chars returned by getpass */

/* ---------------------------------- dini -------------------------------- */
void dini (void);
void dend (void);
void dgotoxy (int piX, int piY);
void dtextattr (int piAttrib);
void dtextcolor (int piColor);
void dtextbackground (int piBackground);
void dwindow (int piX1, int piY1, int piX2, int piY2);
int dwherex (void);
int dwherey (void);
void dscrolldw (int piX1, int piY1, int piX2, int piY2);
void dscrollup (int piX1, int piY1, int piX2, int piY2);
void dputch (int piChar);
int dputs (char *pacString);
void dprintf (char *pacString, ...);
void dclreol (void);
void dclrscr (void);
int dkbhit (void);
int dgetch (void);
int dgetche (void);
void ddelline (void);
int dgettext (int piX1, int piY1, int piX2, int piY2, void *pacBuffer);
void dgettextinfo (struct text_info *pudtInfo);
void dhighvideo (void);
void dinsline (void);
void dlowvideo (void);
int dmovetext (int piX1, int piY1, int piX2, int piY2, int piX, int piY);
void dnormvideo (void);
int dputtext (int piX1, int piY1, int piX2, int piY2, void *pacBuffer);
void dtextmode (int piMode);
void _dsetcursortype (int piType);
char * dgets (char *pacString);
int dscanf (const char *pacString, ...);
char * dgetpass (const char *pacPrompt);
int dungetch (int pcChar);

#endif


