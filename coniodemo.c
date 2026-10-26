/* ================================================================================================================= */
/*                                                                                                                   */
/*  C O N I O X   T E X T M O D E   D E M O                                                                          */
/*  ---------------------------------------                                                                          */
/*                                                                                                                   */
/*  A spectacular text-mode demo exercising the public CONIO.H API through CONIOX v5.0                               */
/*  by Javier Gutierrez Chamorro (https://nikkhokkho.sourceforge.io).                                                */
/*  Runs on DOS16, DOS32, Win32 and Win64 through CONIOX. It builds with Watcom/OpenWatcom,                          */
/*  Borland/Turbo C and DJGPP on DOS, and with Microsoft Visual C/C++, MinGW-w64 GCC, LLVM                           */
/*  Clang, Pelles C, TinyCC, Digital Mars and Embarcadero C++Builder on Windows. The                                 */
/*  reference build is DOS16 real mode with OpenWatcom (tiny model).                                                 */
/*                                                                                                                   */
/*  No floating point is used anywhere: every effect runs on integer math and a precomputed                          */
/*  256-entry sine table, so it flies even on a 286. Full-screen frames are composed                                 */
/*  off-screen and blitted with a single puttext(1,1,80,25) call, which CONIOX                                       */
/*  turns into one direct far memcpy into video RAM.                                                                 */
/*                                                                                                                   */
/*  Effects:                                                                                                         */
/*     1. Shining CONIOX logo with typewriter intro                                                                  */
/*     2. CONIO information report in a framed window with the logo                                                  */
/*     3. Photograph converted to ANSI art (embedded SGR escape sequence)                                            */
/*     4. Classic textmode fire                                                                                      */
/*     5. 3D starfield warp                                                                                          */
/*     6. Color cycling plasma                                                                                       */
/*     7. Double sine scroller with bouncing module box                                                              */
/*     8. Matrix rain                                                                                                */
/*     9. ASCII Mandelbrot (16.12 fixed point)                                                                       */
/*    10. TUI desktop showcasing window/clrscr/gettext/puttext/delline/insline/ungetch/cursor                        */
/*    11. Speed benchmark: 50000 putch, 1000 cputs, 1000 cprintf plus puttext/gettext/movetext/gotoxy/clrscr         */
/*    12. Closing credits                                                                                            */
/*                                                                                                                   */
/*  Controls: ANY KEY = skip to the next effect, ESC = quit the demo.                                                */
/*  A live FPS counter is shown on the top right corner of the animated effects                                      */
/*  only: it is composited into every off-screen frame, so it never flickers.                                        */
/*                                                                                                                   */
/*  Reference build (DOS16 real mode with OpenWatcom, tiny model):                                                   */
/*     wcl -mt -bt=dos -d0 -os -ox -fe=coniodemo.exe coniodemo.c coniox.c                                            */
/*                                                                                                                   */
/*  This demo code is public domain. Use, copy, distribute and modify it free of charge.                             */
/* ================================================================================================================= */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "coniox.h"

/* ----------------------------------------------------------------------------------------------------------------- */
/* Geometry. textmode(C80) guarantees an 80x25 color text screen on DOS16.                                           */
/* ----------------------------------------------------------------------------------------------------------------- */
#define DEMO_COLS   80
#define DEMO_ROWS   25

#define KEY_NONE    0
#define KEY_SKIP    1
#define KEY_QUIT    2

static struct text_info demo_ti;
static int SW = DEMO_COLS;
static int SH = DEMO_ROWS;

/* Off-screen frame buffer in gettext()/puttext() word format: low byte = char, high byte = attribute. */
static unsigned short demo_fb[DEMO_ROWS * DEMO_COLS];

/* Live FPS counter: demo_wait() counts one frame per call and paints the count on the top right corner. */
static int demo_fps = 0;                     /* 1 while the current effect shows the FPS overlay */
static int demo_fpsframes = 0;               /* frames counted since the last reset */
static int demo_fpsrate = 0;                 /* frames counted in the last completed second */
static clock_t demo_fpsclock = 0;            /* clock() at the last reset */
static unsigned short demo_fpsbuf[8];        /* cells of the FPS overlay */

/* 256-entry sine table scaled to +/-127 (fixed point; no floating point anywhere in this demo). */
static signed char demo_sin[256] =
{
	   0,    3,    6,    9,   12,   16,   19,   22,   25,   28,   31,   34,   37,   40,   43,   46,
	  49,   51,   54,   57,   60,   63,   65,   68,   71,   73,   76,   78,   81,   83,   85,   88,
	  90,   92,   94,   96,   98,  100,  102,  104,  106,  107,  109,  111,  112,  113,  115,  116,
	 117,  118,  120,  121,  122,  122,  123,  124,  125,  125,  126,  126,  126,  127,  127,  127,
	 127,  127,  127,  127,  126,  126,  126,  125,  125,  124,  123,  122,  122,  121,  120,  118,
	 117,  116,  115,  113,  112,  111,  109,  107,  106,  104,  102,  100,   98,   96,   94,   92,
	  90,   88,   85,   83,   81,   78,   76,   73,   71,   68,   65,   63,   60,   57,   54,   51,
	  49,   46,   43,   40,   37,   34,   31,   28,   25,   22,   19,   16,   12,    9,    6,    3,
	   0,   -3,   -6,   -9,  -12,  -16,  -19,  -22,  -25,  -28,  -31,  -34,  -37,  -40,  -43,  -46,
	 -49,  -51,  -54,  -57,  -60,  -63,  -65,  -68,  -71,  -73,  -76,  -78,  -81,  -83,  -85,  -88,
	 -90,  -92,  -94,  -96,  -98, -100, -102, -104, -106, -107, -109, -111, -112, -113, -115, -116,
	-117, -118, -120, -121, -122, -122, -123, -124, -125, -125, -126, -126, -126, -127, -127, -127,
	-127, -127, -127, -127, -126, -126, -126, -125, -125, -124, -123, -122, -122, -121, -120, -118,
	-117, -116, -115, -113, -112, -111, -109, -107, -106, -104, -102, -100,  -98,  -96,  -94,  -92,
	 -90,  -88,  -85,  -83,  -81,  -78,  -76,  -73,  -71,  -68,  -65,  -63,  -60,  -57,  -54,  -51,
	 -49,  -46,  -43,  -40,  -37,  -34,  -31,  -28,  -25,  -22,  -19,  -16,  -12,   -9,   -6,   -3
};

/* Rainbow attribute palette: foreground colors on black background, hue ordered. */
static const unsigned char demo_rainbow[14] =
{
	0x04, 0x0C, 0x0E, 0x0A, 0x02, 0x03, 0x0B, 0x09, 0x01, 0x05, 0x0D, 0x0F, 0x07, 0x06
};

/* Defined later in the TUI desktop section, shared by the information report. */
static void demo_window(int l, int t, int r, int b, const char *title);

/* ----------------------------------------------------------------------------------------------------------------- */
/* Tiny 16-bit LCG random generator: deterministic, no CRT dependency.                                               */
/* ----------------------------------------------------------------------------------------------------------------- */
static unsigned int demo_seed = 0xACE1;

static int demo_rand(int range)
{
	demo_seed = (unsigned int) (demo_seed * 25173u + 13849u);
	return (int) ((demo_seed >> 2) % (unsigned int) range);
}

/* ----------------------------------------------------------------------------------------------------------------- */
/* Keyboard: kbhit()/getch() with extended key handling. Returns KEY_NONE/KEY_SKIP/KEY_QUIT.                         */
/* ----------------------------------------------------------------------------------------------------------------- */
static int demo_pollkey(void)
{
	int ch;

	if (!kbhit())
	{
		return KEY_NONE;
	}
	ch = getch();
	if (ch == 0)
	{
		getch();    /* extended key: consume its scan code */
	}
	if (ch == KEY_ESC)
	{
		return KEY_QUIT;
	}
	return KEY_SKIP;
}

/* Waits roughly one BIOS tick (55 ms) per count, polling the keyboard. Returns the key event, if any. */
/* It also counts one frame per call and, when demo_fps is set, refreshes the live FPS counter on the */
/* top right corner: effects drawing directly on screen get it through this puttext(), while fb_flush() */
/* composites the very same cells into every off-screen frame, so full-screen blits never erase it */
/* and it cannot flicker. */
static int demo_wait(int ticks)
{
	int k;

	while (ticks-- > 0)
	{
		k = demo_pollkey();
		if (k != KEY_NONE)
		{
			return k;
		}
		delay(1);
	}
	if (demo_fps)
	{
		int n, p;

		demo_fpsframes++;
		if (clock() - demo_fpsclock >= (clock_t) CLOCKS_PER_SEC)
		{
			demo_fpsrate = demo_fpsframes;
			demo_fpsframes = 0;
			demo_fpsclock = clock();
		}
		p = 7;
		demo_fpsbuf[p--] = (unsigned short) ((0x70 << 8) | 'S');
		demo_fpsbuf[p--] = (unsigned short) ((0x70 << 8) | 'P');
		demo_fpsbuf[p--] = (unsigned short) ((0x70 << 8) | 'F');
		demo_fpsbuf[p--] = (unsigned short) ((0x70 << 8) | ' ');
		n = demo_fpsrate;
		if (n > 9999)
		{
			n = 9999;
		}
		do
		{
			demo_fpsbuf[p--] = (unsigned short) ((0x70 << 8) | ('0' + (n % 10)));
			n /= 10;
		} while (n > 0);
		while (p >= 0)
		{
			demo_fpsbuf[p--] = (unsigned short) ((0x70 << 8) | ' ');
		}
		puttext(SW - 7, 1, SW, 1, demo_fpsbuf);
	}
	return KEY_NONE;
}

/* ----------------------------------------------------------------------------------------------------------------- */
/* Frame buffer helpers.                                                                                             */
/* ----------------------------------------------------------------------------------------------------------------- */
static void fb_set(int x, int y, int ch, int attr)
{
	if ((unsigned int) x < (unsigned int) SW && (unsigned int) y < (unsigned int) SH)
	{
		demo_fb[(y * SW) + x] = (unsigned short) (((attr & 0xFF) << 8) | (ch & 0xFF));
	}
}

static void fb_clear(void)
{
	memset(demo_fb, 0, sizeof(demo_fb));
}

static void fb_flush(void)
{
	if (demo_fps)
	{
		/* Composite the FPS overlay into the frame before the blit: the overlay cells are part of */
		/* every frame, so the screen never shows an intermediate state without them. */
		memcpy(demo_fb + SW - 8, demo_fpsbuf, sizeof(demo_fpsbuf));
	}
	puttext(1, 1, SW, SH, demo_fb);
}

static void fb_flush_row(int y)
{
	puttext(1, y + 1, SW, y + 1, demo_fb + (y * SW));
}

static void fb_text(int x, int y, const char *s, int attr)
{
	int i;

	for (i = 0; s[i] != 0; i++)
	{
		fb_set(x + i, y, s[i], attr);
	}
}

/* Draws a centered string into the frame buffer. Returns the leftmost column used. */
static int fb_text_centered(int y, const char *s, int attr)
{
	int x;

	x = SW / 2 - (int) strlen(s) / 2;
	fb_text(x, y, s, attr);
	return x;
}

/* Diagonal checker dissolve to black. The dissolve is progressive, so the snapshot taken with gettext() */
/* is dirtied in place: no second buffer is needed. It also works after direct (non-fb) drawing. */
static int demo_wipeout(void)
{
	int phase, x, y, k;

	gettext(1, 1, SW, SH, demo_fb);
	for (phase = 0; phase < 8; phase++)
	{
		for (y = 0; y < SH; y++)
		{
			for (x = 0; x < SW; x++)
			{
				if (((x + y) & 7) <= phase)
				{
					demo_fb[(y * SW) + x] = 0x0020;
				}
			}
		}
		fb_flush();
		k = demo_wait(1);
		if (k != KEY_NONE)
		{
			return k;
		}
	}
	return KEY_NONE;
}

/* ----------------------------------------------------------------------------------------------------------------- */
/* Effect 1: shining CONIOX logo + typewriter intro.                                                                 */
/* ----------------------------------------------------------------------------------------------------------------- */
static const char *demo_logo[6][5] =
{
	{ " \xDB\xDB\xDB\xDB ", "\xDB     ", "\xDB     ", "\xDB     ", " \xDB\xDB\xDB\xDB " },    /* C */
	{ " \xDB\xDB\xDB\xDB ", "\xDB   \xDB ", "\xDB   \xDB ", "\xDB   \xDB ", " \xDB\xDB\xDB\xDB " },    /* O */
	{ "\xDB   \xDB ", "\xDB\xDB  \xDB ", "\xDB \xDB \xDB ", "\xDB  \xDB\xDB ", "\xDB   \xDB " },       /* N */
	{ "\xDB\xDB\xDB\xDB  ", "  \xDB    ", "  \xDB    ", "  \xDB    ", "\xDB\xDB\xDB\xDB  " },          /* I */
	{ " \xDB\xDB\xDB\xDB ", "\xDB   \xDB ", "\xDB   \xDB ", "\xDB   \xDB ", " \xDB\xDB\xDB\xDB " },    /* O */
	{ "\xDB   \xDB ", " \xDB \xDB  ", "  \xDB    ", " \xDB \xDB  ", "\xDB   \xDB " }                   /* X */
};

static const unsigned char demo_shine[8] = { 0x09, 0x0B, 0x0F, 0x0B, 0x09, 0x01, 0x03, 0x01 };

static int fx_intro(void)
{
	static const char *lines[4] =
	{
		"TEXTMODE CONIO AT ITS FINEST",
		"16 COLORS - 80X25 - DIRECT VIDEO",
		"PUBLIC DOMAIN - JAVIER GUTIERREZ CHAMORRO",
		"DOS16 / DOS32 / WIN32 / WIN64 EDITION"
	};
	const char *s;
	int t, x, y, li, i, lim, len, k, blink;

	_setcursortype(_NOCURSOR);
	demo_fps = 1;
	fb_clear();
	fb_flush();

	k = KEY_NONE;
	for (t = 0; ; t++)
	{
		fb_clear();

		/* Shimmering block logo: 6 letters x 6 columns at column 22. */
		for (y = 0; y < 5; y++)
		{
			for (li = 0; li < 6; li++)
			{
				s = demo_logo[li][y];
				for (x = 0; x < 6; x++)
				{
					if (s[x] == '\xDB')
					{
						fb_set(22 + (li * 6) + x, 4 + y, 0xDB, demo_shine[(x + t) & 7]);
					}
				}
			}
		}

		/* Frame lines and version caption. */
		for (x = 0; x < SW; x++)
		{
			fb_set(x, 0, 0xCD, 0x08);
			fb_set(x, 24, 0xCD, 0x08);
		}
		fb_text_centered(10, "V E R S I O N   5 . 0", 0x0B);

		/* Typewriter lines, one character per frame, with blinking block cursor. */
		for (i = 0; i < 4; i++)
		{
			s = lines[i];
			len = (int) strlen(s);
			lim = t - 40 - (i * 35);
			if (lim > len)
			{
				lim = len;
			}
			if (lim < 0)
			{
				lim = 0;
			}
			for (x = 0; x < lim; x++)
			{
				fb_set(SW / 2 - len / 2 + x, 12 + i, s[x], 0x0B);
			}
			if (lim > 0 && lim < len && ((t / 4) & 1))
			{
				fb_set(SW / 2 - len / 2 + lim, 12 + i, 0xDB, 0x0F);
			}
		}

		/* Blinking invitation. */
		blink = ((t / 10) & 1) ? 0x70 : 0x07;
		fb_text_centered(22, "PRESS ANY KEY TO START THE SHOW", blink);

		fb_flush();
		k = demo_wait(1);
		if (k != KEY_NONE)
		{
			break;
		}
	}

	if (k == KEY_QUIT)
	{
		return k;
	}
	return demo_wipeout();
}

/* ----------------------------------------------------------------------------------------------------------------- */
/* Effect 2: CONIO information report, in a framed window crowned by the CONIOX logo.                                */
/* ----------------------------------------------------------------------------------------------------------------- */

/* Bright per-letter palette for the logo: dark rainbow colors vanish over the blue window */
/* background, and cyan reads poorly next to it, so it becomes light blue. */
static const unsigned char demo_logopal[6] = { 0x0C, 0x0E, 0x0F, 0x09, 0x0A, 0x0D };

static int fx_info(void)
{
	struct text_info ti;
	int i, x, y, k;

	_setcursortype(_NOCURSOR);
	demo_fps = 0;
	gettextinfo(&ti);

	/* Checkered desktop background, blitted in one shot. */
	fb_clear();
	for (y = 0; y < SH; y++)
	{
		for (x = 0; x < SW; x++)
		{
			fb_set(x, y, 0xB0, (((x ^ y) & 1) ? 0x08 : 0x00));
		}
	}
	fb_flush();

	/* Information window over the desktop, client area (7,3)-(73,21): 67x19 cells. */
	demo_window(6, 2, 74, 22, "CONIO INFORMATION");
	window(7, 3, 73, 21);

	/* Shining CONIOX ascii art, one bright color per letter, centered under the title bar. */
	for (y = 0; y < 5; y++)
	{
		gotoxy(16, 1 + y);
		for (i = 0; i < 6; i++)
		{
			textcolor(demo_logopal[i]);
			cputs(demo_logo[i][y]);
		}
	}

	/* Divider under the logo. */
	gotoxy(1, 7);
	textcolor(LIGHTCYAN);
	for (x = 0; x < 67; x++)
	{
		putch(0xC4);
	}

	/* Report: one labeled row per line, everything inside the window. Labels in yellow, values in white: */
	/* light cyan over the blue background read poorly, yellow keeps the contrast instead. */
	textcolor(YELLOW);
	gotoxy(3, 9);
	cputs("Version:     ");
	textcolor(WHITE);
	cprintf("%d.%02d (%s)", coniox_version / 100, coniox_version % 100, coniox_build);
	textcolor(YELLOW);
	gotoxy(3, 10);
	cputs("Window:      ");
	textcolor(WHITE);
	cprintf("%d,%d - %d,%d", ti.winleft, ti.wintop, ti.winright, ti.winbottom);
	textcolor(YELLOW);
	gotoxy(3, 11);
	cputs("Attribute:   ");
	textcolor(WHITE);
	cprintf("%d (normattr %d)", ti.attribute, ti.normattr);
	textcolor(YELLOW);
	gotoxy(3, 12);
	cputs("Mode:        ");
	textcolor(WHITE);
	cprintf("%d", ti.currmode);
	textcolor(YELLOW);
	gotoxy(3, 13);
	cputs("Screen:      ");
	textcolor(WHITE);
	cprintf("%dx%d", ti.screenwidth, ti.screenheight);
	textcolor(YELLOW);
	gotoxy(3, 14);
	cputs("Cursor:      ");
	textcolor(WHITE);
	cprintf("%d,%d", ti.curx, ti.cury);
	textcolor(YELLOW);
	gotoxy(3, 15);
	cputs("Video:       ");
	textcolor(WHITE);
	cprintf("directvideo %d, _wscroll %d", directvideo, _wscroll);
	textcolor(YELLOW);
	gotoxy(3, 16);
	cputs("Platforms:   ");
	textcolor(WHITE);
	cputs("DOS16 / DOS32 / WIN32 / WIN64");
	textcolor(YELLOW);
	gotoxy(3, 17);
	cputs("Compilers:   ");
	textcolor(WHITE);
	cputs("Watcom, OpenWatcom, Borland, DJGPP, MSVC, MinGW");
	textcolor(YELLOW);
	gotoxy(3, 18);
	cputs("             ");
	textcolor(WHITE);
	cputs("Clang, Pelles C, TinyCC, Digital Mars, Embarcadero");

	textcolor(YELLOW + BLINK);
	gotoxy(22, 19);
	cputs("Press any key to continue");
	do
	{
		k = demo_wait(1);
	} while (k == KEY_NONE);
	window(1, 1, SW, SH);
	if (k == KEY_QUIT)
	{
		return k;
	}
	return demo_wipeout();
}

/* ----------------------------------------------------------------------------------------------------------------- */
/*  Effect 3: photograph image-to-ANSI conversion. A real photograph is stored                                       */
/* as a single embedded ANSI SGR escape sequence using upper/lower half blocks and the 16 CGA colors,                */
/* interpreted at run time into the off-screen buffer with no floats and no extra buffers, revealed                  */
/* row by row.                                                                                                       */
/* ----------------------------------------------------------------------------------------------------------------- */

/* ANSI SGR 30-37/40-47 order (black red green yellow blue magenta cyan white) to CGA 0-7 (black blue green cyan red */
/* magenta yellow white): the permutation is self-inverse. */
static const unsigned char demo_sgr[8] = { 0x00, 0x04, 0x02, 0x06, 0x01, 0x05, 0x03, 0x07 };

static const char demo_ansi[] =
	"\x1B[30m                           \x1B[1m\x1B[37m\xDC\x1B[0m\x1B[30m                                                    \n"
	"                                                    \x1B[37m\xDC\xDC\x1B[30m             \x1B[37m\xDC\x1B[30m            \n"
	"\x1B[34m\xDF\xDF\x1B[37m\x1B[44m\xDC\x1B[34m\x1B[40m\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\x1B[1m\x1B[37m\x1B[44m\xDC\x1B[0m\x1B[34m\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\n"
	"\x1B[30m\x1B[44m                                                \x1B[37m\xDF\x1B[30m                               \n"
	"                       \x1B[1m\x1B[37m\xDF\x1B[0m\x1B[30m\x1B[44m                  \x1B[37m\xDF\x1B[30m                                     \n"
	"               \x1B[37m\xDF\x1B[30m                  \x1B[37m\xDF\x1B[30m                                       \x1B[37m\xDC\xDC\x1B[30m    \n"
	"  \x1B[35m\xDF\x1B[30m   \x1B[35m\xDF\x1B[30m   \x1B[35m\xDF\x1B[30m   \x1B[35m\xDF\x1B[30m   \x1B[37m\x1B[45m\xDC\x1B[30m\x1B[44m   \x1B[35m\xDF\x1B[30m   \x1B[35m\xDF\x1B[30m   \x1B[35m\xDF\x1B[30m   \x1B[35m\xDF\x1B[30m   \x1B[35m\xDF\x1B[30m   \x1B[35m\xDF\x1B[30m   \x1B[35m\xDF\x1B[30m   \x1B[35m\xDF\x1B[30m   \x1B[35m\xDF\x1B[30m   \x1B[35m\xDF\x1B[30m   \x1B[35m\xDF\x1B[30m   \x1B[35m\xDF\x1B[30m   \x1B[35m\xDF\x1B[30m   \x1B[35m\xDF\x1B[30m   \x1B[35m\xDF\x1B[37m\xDF\n"
	"\x1B[30m\x1B[45m  \x1B[35m\x1B[44m\xDF\x1B[30m\x1B[45m   \x1B[35m\x1B[44m\xDF\x1B[30m\x1B[45m   \x1B[35m\x1B[44m\xDF\x1B[30m\x1B[45m   \x1B[35m\x1B[44m\xDF\x1B[30m\x1B[45m   \x1B[35m\x1B[44m\xDF\x1B[30m\x1B[45m   \x1B[35m\x1B[44m\xDF\x1B[30m\x1B[45m   \x1B[35m\x1B[44m\xDF\x1B[30m\x1B[45m   \x1B[35m\x1B[44m\xDF\x1B[30m\x1B[45m   \x1B[35m\x1B[44m\xDF\x1B[30m\x1B[45m   \x1B[35m\x1B[44m\xDF\x1B[30m\x1B[45m   \x1B[35m\x1B[44m\xDF\x1B[30m\x1B[45m   \x1B[35m\x1B[44m\xDF\x1B[30m\x1B[45m   \x1B[35m\x1B[44m\xDF\x1B[30m\x1B[45m   \x1B[35m\x1B[44m\xDF\x1B[30m\x1B[45m   \x1B[35m\x1B[44m\xDF\x1B[30m\x1B[45m   \x1B[35m\x1B[44m\xDF\x1B[30m\x1B[45m   \x1B[35m\x1B[44m\xDF\x1B[30m\x1B[45m   \x1B[35m\x1B[44m\xDF\x1B[30m\x1B[45m   \x1B[35m\x1B[44m\xDF\x1B[30m\x1B[45m   \x1B[35m\x1B[44m\xDF\x1B[30m\x1B[45m \n"
	"                                                                                \n"
	"                                                                                \n"
	" \x1B[35m\x1B[41m\xDC\x1B[30m\x1B[45m \x1B[35m\x1B[41m\xDC\x1B[30m\x1B[45m \x1B[35m\x1B[41m\xDC\x1B[30m\x1B[45m \x1B[35m\x1B[41m\xDC\x1B[30m\x1B[45m \x1B[35m\x1B[41m\xDC\x1B[30m\x1B[45m \x1B[35m\x1B[41m\xDC\x1B[30m\x1B[45m \x1B[35m\x1B[41m\xDC\x1B[30m\x1B[45m \x1B[35m\x1B[41m\xDC\x1B[30m\x1B[45m \x1B[35m\x1B[41m\xDC\x1B[30m\x1B[45m \x1B[35m\x1B[41m\xDC\x1B[30m\x1B[45m \x1B[35m\x1B[41m\xDC\x1B[30m\x1B[45m \x1B[35m\x1B[41m\xDC\x1B[30m\x1B[45m \x1B[35m\x1B[41m\xDC\x1B[30m\x1B[45m \x1B[35m\x1B[41m\xDC\x1B[30m\x1B[45m \x1B[35m\x1B[41m\xDC\x1B[30m\x1B[45m \x1B[35m\x1B[41m\xDC\x1B[30m\x1B[45m \x1B[35m\x1B[41m\xDC\x1B[30m\x1B[45m \x1B[35m\x1B[41m\xDC\x1B[30m\x1B[45m \x1B[35m\x1B[41m\xDC\x1B[30m\x1B[45m \x1B[35m\x1B[41m\xDC\x1B[37m\x1B[45m\xDF\x1B[1m\x1B[30m\xDF\x1B[0m\x1B[45m\xDF\x1B[1m\x1B[30m\xDF\x1B[0m\x1B[45m\xDF\x1B[1m\x1B[30m\xDF\x1B[0m\x1B[45m\xDF\x1B[1m\x1B[30m\xDF\x1B[0m\x1B[45m\xDF\x1B[1m\x1B[30m\xDF\xDF\x1B[0m\x1B[35m\xDC\xDC\x1B[41m\xDC\x1B[30m\x1B[45m \x1B[35m\x1B[41m\xDC\x1B[30m\x1B[45m \x1B[35m\x1B[41m\xDC\x1B[30m\x1B[45m \x1B[35m\x1B[41m\xDC\x1B[30m\x1B[45m \x1B[35m\x1B[41m\xDC\x1B[30m\x1B[45m \x1B[35m\x1B[41m\xDC\x1B[30m\x1B[45m \x1B[35m\x1B[41m\xDC\x1B[30m\x1B[45m \x1B[35m\x1B[41m\xDC\x1B[30m\x1B[45m \x1B[35m\x1B[41m\xDC\x1B[30m\x1B[45m \x1B[35m\x1B[41m\xDC\x1B[30m\x1B[45m \x1B[35m\x1B[41m\xDC\x1B[30m\x1B[45m \x1B[35m\x1B[41m\xDC\x1B[30m\x1B[45m \x1B[35m\x1B[41m\xDC\x1B[30m\x1B[45m \x1B[35m\x1B[41m\xDC\n"
	"\x1B[30m                                    \x1B[37m\xDF\x1B[31m\x1B[40m\xDC\x1B[37m\xDC\xDC\x1B[1m\x1B[30m\xDC\x1B[0m\x1B[30m        \x1B[1m\xDC\xDC\x1B[0m\x1B[30m   \x1B[31m\xDC\x1B[1m\x1B[30m\x1B[41m\xDF\x1B[37m\xDF\x1B[0m\x1B[41m\xDF\xDF\x1B[1m\x1B[30m\xDF\xDF\xDF\xDF\xDF\x1B[0m\x1B[41m\xDF\x1B[1m\xDF\x1B[0m\x1B[30m\x1B[41m              \n"
	"              \x1B[1m\x1B[31m\xDF\xDF\xDF\x1B[0m\x1B[30m\x1B[41m           \x1B[37m\xDF\xDF\xDF\x1B[1m\x1B[30m\xDF\xDF\xDF\x1B[40m\xDB\xDC\x1B[0m\x1B[30m                   \x1B[1m\x1B[47m\xDF\xDF\xDF\x1B[0m\x1B[30m\x1B[47m \x1B[1m\x1B[37m\xDC\x1B[0m\x1B[30m \x1B[37m\x1B[41m\xDF\x1B[30m                  \n"
	"    \x1B[33m\xDF\x1B[30m \x1B[33m\xDF\x1B[30m \x1B[1m\x1B[31m\xDF\x1B[0m\x1B[30m\x1B[41m \x1B[33m\xDF\x1B[30m \x1B[1m\x1B[31m\xDF\x1B[40m\xDB\x1B[37m\xDB\xDB\x1B[47m\xDF\x1B[31m\xDC\x1B[30m\x1B[41m\xDF\xDF\x1B[0m\x1B[30m\x1B[47m  \x1B[1m\xDC\xDF\xDF\xDF\x1B[40m\xDB\xDC\xDC\xDC\xDC\xDC\x1B[0m\x1B[30m                           \x1B[1m\xDC\xDC\xDC\xDC\xDC\xDC\xDC\x1B[0m\x1B[30m \x1B[31m\xDC\x1B[30m\x1B[41m            \n"
	"\x1B[33m\xDF\xDF\xDF\x1B[30m\x1B[43m \x1B[1m\x1B[31m\x1B[41m\xDF\x1B[0m\x1B[30m\x1B[43m \x1B[1m\x1B[31m\xDF\x1B[40m\xDB\x1B[41m\xDF\x1B[0m\x1B[30m\x1B[43m \x1B[1m\x1B[31m\xDF\x1B[40m\xDB\x1B[0m\x1B[30m\x1B[47m \x1B[1m\x1B[31m\xDF\x1B[37m\xDC\xDC\xDC\x1B[0m\x1B[30m\x1B[47m \x1B[1m\xDC\x1B[0m\x1B[30m\x1B[47m \x1B[1m\x1B[40m\xDC\xDC\x1B[0m\x1B[30m                                              \x1B[33m\x1B[41m\xDF\xDF\xDF\x1B[30m\x1B[43m \x1B[33m\x1B[41m\xDF\xDF\xDF\x1B[30m\x1B[43m \x1B[33m\x1B[41m\xDF\x1B[30m \x1B[33m\xDF\x1B[30m \n"
	"\x1B[43m  \x1B[1m\x1B[31m\xDF\x1B[0m\x1B[30m\x1B[43m \x1B[1m\x1B[31m\xDF\x1B[40m\xDB\x1B[43m\xDF\x1B[40m\xDB\xDB\xDB\x1B[43m\xDF\x1B[40m\xDB\xDC\xDC\x1B[37m\x1B[43m\xDF\x1B[33m\x1B[40m\xDF\x1B[37m\xDF\x1B[33m\xDF\x1B[0m\x1B[30m                                                 \x1B[33m\xDF\x1B[1m\x1B[31m\x1B[43m\xDF\x1B[0m\x1B[30m\x1B[43m   \x1B[1m\x1B[31m\xDF\x1B[0m\x1B[30m\x1B[43m       \n"
	"  \x1B[1m\x1B[31m\xDF\xDC\xDF\x1B[40m\xDB\xDB\xDB\xDB\xDB\xDB\xDB\xDF\xDF\x1B[0m\xDC\x1B[1m\xDC\x1B[0m\xDC\x1B[1m\x1B[33m\xDC\x1B[0m\x1B[30m  \x1B[37m\xDF\x1B[1m\x1B[30m\xDF\x1B[0m\xDF\x1B[1m\x1B[30m\xDF\x1B[0m\x1B[30m                                   \x1B[1m\xDF\x1B[0m\xDF\x1B[1m\x1B[30m\xDF\x1B[0m\x1B[30m     \x1B[43m   \x1B[1m\x1B[31m\xDF\x1B[0m\x1B[30m\x1B[43m   \x1B[1m\x1B[31m\xDF\x1B[0m\x1B[30m\x1B[43m     \n"
	"\x1B[40m    \x1B[1m\xDF\x1B[0m\x1B[30m \x1B[1m\xDF\x1B[0m\x1B[30m             \x1B[1m\xDC\xDF\x1B[0m\xDF\xDC\x1B[30m                                   \x1B[1m\x1B[47m\xDF\x1B[0m\xDF\x1B[1m\x1B[30m\x1B[47m\xDF\x1B[0m\x1B[30m                  \n"
	"\x1B[1m\xDF\xDF\xDF\xDB\xDF\xDB\xDB\xDB\xDB\xDB\xDB\xDB\xDF\x1B[0m\x1B[30m                                                                   \n"
	"\x1B[1m\xDB\xDB\xDB\xDB\xDB\xDB\xDB\xDB\xDB\xDB\xDB\xDB\xDF\x1B[0m\x1B[30m                                                                   \n"
	"\x1B[1m\xDF\xDB\xDB\xDB\xDB\xDB\xDB\xDB\xDB\xDC\xDF\x1B[0m\x1B[30m                                                                     \n"
	"\x1B[1m\xDF\xDC\xDF\xDC\xDB\xDC\xDF\xDC\x1B[0m\x1B[30m                                                                        \n"
	"                                                                                \n"
	"                                                                                \n"
	"                                                                                ";

static int fx_ansi(void)
{
	const char *p;
	int ch, v, x, y, fg, bg, bright, blink, k;

	_setcursortype(_NOCURSOR);
	demo_fps = 0;
	fb_clear();
	fb_flush();

	x = 0;
	y = 0;
	fg = 7;
	bg = 0;
	bright = 0;
	blink = 0;
	for (p = demo_ansi; *p != 0; p++)
	{
		ch = (unsigned char) *p;
		if (ch == 0x1B)
		{
			v = 0;
			p += 2;                    /* skip ESC and '[' */
			while (*p >= '0' && *p <= '9')
			{
				v = (v * 10) + (*p - '0');
				p++;
			}
			/* p now points at 'm': the loop increment steps over it */
			if (v == 0)
			{
				fg = 7;
				bg = 0;
				bright = 0;
				blink = 0;
			}
			else if (v == 1)
			{
				bright = 1;
			}
			else if (v == 5)
			{
				blink = 1;
			}
			else if (v >= 30 && v <= 37)
			{
				fg = demo_sgr[v - 30];
			}
			else if (v >= 40 && v <= 47)
			{
				bg = demo_sgr[v - 40];
			}
			continue;
		}
		if (ch == '\n')
		{
			x = 0;
			y++;
			continue;
		}
		fb_set(x, y, ch, (blink << 7) | (bg << 4) | (bright << 3) | fg);
		x++;
	}

	/* Row by row reveal: SKIP shows the rest at once, ESC leaves, otherwise hold and go. */
	k = KEY_NONE;
	for (y = 0; y < SH && k == KEY_NONE; y++)
	{
		fb_flush_row(y);
		k = demo_wait(1);
	}
	if (k == KEY_QUIT)
	{
		return k;
	}
	if (k == KEY_SKIP)
	{
		fb_flush();
	}
	k = demo_wait(300);
	if (k == KEY_QUIT)
	{
		return k;
	}
	return demo_wipeout();
}

/* ----------------------------------------------------------------------------------------------------------------- */
/* Effect 4: classic textmode fire. Heat propagates upwards with random cooling;                                     */
/* values are painted through a char/color ramp.                                                                     */
/* ----------------------------------------------------------------------------------------------------------------- */
static int fx_fire(void)
{
	static unsigned char heat[DEMO_ROWS + 2][DEMO_COLS];
	static const unsigned char framp[16][2] =
	{
		{ ' ', 0x00 }, { '.', 0x04 }, { ':', 0x04 }, { '-', 0x04 },
		{ '=', 0x0C }, { '+', 0x0C }, { '*', 0x06 }, { '#', 0x06 },
		{ '%', 0x0E }, { '&', 0x0E }, { '$', 0x0E }, { '@', 0x0E },
		{ '@', 0x0F }, { '@', 0x0F }, { '@', 0x0F }, { '@', 0x0F }
	};
	int x, y, xl, xr, v, f, k;

	_setcursortype(_NOCURSOR);
	demo_fps = 1;
	fb_clear();
	fb_flush();
	memset(heat, 0, sizeof(heat));

	k = KEY_NONE;
	for (f = 0; f < 520; f++)
	{
		/* Seed (or starve) the two bottom heat rows: values live inside the ramp range, so the base glows instead of clamping to a flat wall. */
		for (x = 0; x < SW; x++)
		{
			v = (f < 430) ? (12 + demo_rand(4)) : 0;
			heat[SH][x] = (unsigned char) v;
			heat[SH + 1][x] = (unsigned char) v;
		}

		/* Average upward propagation, wrapping at the edges: a fixed plus a random cooling unit makes the flames die out in about ten rows. */
		for (y = 0; y < SH; y++)
		{
			for (x = 0; x < SW; x++)
			{
				xl = (x == 0) ? (SW - 1) : (x - 1);
				xr = (x == SW - 1) ? 0 : (x + 1);
				v = heat[y + 1][x] + heat[y + 1][xl] + heat[y + 1][xr] + heat[y + 2][x];
				v = (v >> 2) - 1 - (demo_rand(3) == 0);
				if (v < 0)
				{
					v = 0;
				}
				heat[y][x] = (unsigned char) v;
			}
		}

		/* Paint through the fire ramp. */
		for (y = 0; y < SH; y++)
		{
			for (x = 0; x < SW; x++)
			{
				v = heat[y][x];
				if (v > 15)
				{
					v = 15;
				}
				fb_set(x, y, framp[v][0], framp[v][1]);
			}
		}

		fb_flush();
		k = demo_wait(1);
		if (k != KEY_NONE)
		{
			break;
		}
	}

	if (k == KEY_QUIT)
	{
		return k;
	}
	return demo_wipeout();
}

/* ----------------------------------------------------------------------------------------------------------------- */
/* Effect 5: 3D starfield warp. Fixed point projection, speed and brightness by depth.                               */
/* ----------------------------------------------------------------------------------------------------------------- */
static int fx_stars(void)
{
	static int sx[140], sy[140], sz[140];
	int i, f, k, px, py, z;

	_setcursortype(_NOCURSOR);
	demo_fps = 1;
	fb_clear();
	fb_flush();

	for (i = 0; i < 140; i++)
	{
		sx[i] = demo_rand(1000) - 500;
		sy[i] = demo_rand(500) - 250;
		sz[i] = 4 + demo_rand(124);
	}

	k = KEY_NONE;
	for (f = 0; f < 600; f++)
	{
		fb_clear();
		for (i = 0; i < 140; i++)
		{
			z = sz[i];
			px = SW / 2 + ((sx[i] * 10) / z);
			py = SH / 2 + (((sy[i] * 10) / z) / 2);
			if (px >= 0 && px < SW && py >= 0 && py < SH)
			{
				if (z > 100)
				{
					fb_set(px, py, '.', 0x08);
				}
				else if (z > 60)
				{
					fb_set(px, py, '.', 0x07);
				}
				else if (z > 25)
				{
					fb_set(px, py, '+', 0x0F);
				}
				else
				{
					fb_set(px, py, '*', 0x0B);
				}
			}
			z -= (1 + (i & 3));
			if (z <= 2)
			{
				sx[i] = demo_rand(1000) - 500;
				sy[i] = demo_rand(500) - 250;
				z = 128;
			}
			sz[i] = z;
		}
		fb_flush();
		k = demo_wait(1);
		if (k != KEY_NONE)
		{
			break;
		}
	}

	if (k == KEY_QUIT)
	{
		return k;
	}
	return demo_wipeout();
}

/* ----------------------------------------------------------------------------------------------------------------- */
/* Effect 6: color cycling plasma. Four summed sine waves drive a character ramp                                     */
/* plus a rotating 8-color attribute palette.                                                                        */
/* ----------------------------------------------------------------------------------------------------------------- */
static int fx_plasma(void)
{
	static const unsigned char ppal[8] = { 0x01, 0x09, 0x0B, 0x0F, 0x0B, 0x09, 0x0D, 0x05 };
	static const char pramp[8] = { ' ', '.', ':', '-', '=', '+', '*', '#' };
	int x, y, t, v, ic, ii, k;

	_setcursortype(_NOCURSOR);
	demo_fps = 1;
	fb_clear();
	fb_flush();

	k = KEY_NONE;
	for (t = 0; t < 560; t++)
	{
		for (y = 0; y < SH; y++)
		{
			for (x = 0; x < SW; x++)
			{
				v = demo_sin[(x * 2 + t * 3) & 255]
				  + demo_sin[(y * 5 + t * 2) & 255]
				  + demo_sin[((x + y) * 3 - t * 4 + 4096) & 255]
				  + demo_sin[((x * x + y * y) / 3 + t * 5) & 255];
				ii = (v + 508) / 128;
				if (ii > 7)
				{
					ii = 7;
				}
				ic = (((v + 512) / 64 + t) & 7);
				fb_set(x, y, pramp[ii], ppal[ic]);
			}
		}
		fb_flush();
		k = demo_wait(1);
		if (k != KEY_NONE)
		{
			break;
		}
	}

	if (k == KEY_QUIT)
	{
		return k;
	}
	return demo_wipeout();
}

/* ----------------------------------------------------------------------------------------------------------------- */
/* Effect 7: double sine scroller with a bouncing module box.                                                        */
/* ----------------------------------------------------------------------------------------------------------------- */
static const char demo_msg[] =
	"CONIOX V5.0 *** THE ULTIMATE TEXTMODE CONIO LIBRARY *** DIRECT VIDEO AT FULL BLAST *** 100 PERCENT BORLAND STYLE *** GREETINGS TO ALL DOS CODERS AND DEMOSCENERS AROUND THE WORLD *** KEEP THE SCENE ALIVE!!! ***   ";

static int fx_scroller(void)
{
	int f, x, y, i, k, len, bx, by, dx, dy, edge;

	_setcursortype(_NOCURSOR);
	demo_fps = 1;
	fb_clear();
	fb_flush();
	len = (int) strlen(demo_msg);
	bx = 4;
	by = 17;
	dx = 1;
	dy = 1;

	k = KEY_NONE;
	for (f = 0; f < 900; f++)
	{
		fb_clear();
		for (x = 0; x < SW; x++)
		{
			fb_set(x, 0, 0xCD, 0x09);
			fb_set(x, 24, 0xCD, 0x09);
		}

		/* Main sine scroller with per-column rainbow color and drop shadow. */
		for (x = 0; x < SW; x++)
		{
			i = (f + x) % len;
			y = 11 + ((demo_sin[((f + x) * 3) & 255] * 6) / 128);
			fb_set(x, y, demo_msg[i], demo_rainbow[(x + f) % 14]);
			fb_set(x, y + 1, demo_msg[i], 0x08);
		}

		/* Counter-scrolling dim echo near the bottom. */
		for (x = 0; x < SW; x++)
		{
			i = ((f * 2) + 77 - x + (len * 4)) % len;
			y = 21 + ((demo_sin[((f + x) * 2 + 64) & 255] * 2) / 128);
			fb_set(x, y, demo_msg[i], 0x08);
		}

		/* Bouncing module box drawn on top of the scroller. */
		if (f & 1)
		{
			bx += dx;
			by += dy;
			if (bx <= 1 || bx >= SW - 10)
			{
				dx = -dx;
			}
			if (by <= 2 || by >= SH - 5)
			{
				dy = -dy;
			}
		}
		for (y = 0; y < 4; y++)
		{
			for (x = 0; x < 9; x++)
			{
				edge = (y == 0 || y == 3 || x == 0 || x == 8);
				fb_set(bx + x, by + y, edge ? 0xDB : ' ', 0x2F);
			}
		}
		fb_text(bx + 1, by + 1, "CONIOX", 0x2F);
		fb_text(bx + 1, by + 2, "V5.0", 0x2F);

		fb_flush();
		k = demo_wait(1);
		if (k != KEY_NONE)
		{
			break;
		}
	}

	if (k == KEY_QUIT)
	{
		return k;
	}
	return demo_wipeout();
}

/* ----------------------------------------------------------------------------------------------------------------- */
/* Effect 8: matrix rain. Staggered falling columns with white head, green body, dark tail.                          */
/* ----------------------------------------------------------------------------------------------------------------- */
static int fx_matrix(void)
{
	static const char mchars[] = "01<>#$%&*+=/\\|ABCDEFXYZ";
	static int mhead[DEMO_COLS], mspd[DEMO_COLS], mlen[DEMO_COLS], mchr[DEMO_COLS];
	int x, y, i, f, k, a, mcl;

	_setcursortype(_NOCURSOR);
	demo_fps = 1;
	fb_clear();
	fb_flush();
	mcl = (int) strlen(mchars);

	for (x = 0; x < SW; x++)
	{
		mhead[x] = -demo_rand(70);
		mspd[x] = 1 + demo_rand(3);
		mlen[x] = 6 + demo_rand(10);
		mchr[x] = demo_rand(mcl);
	}

	k = KEY_NONE;
	for (f = 0; f < 560; f++)
	{
		fb_clear();
		for (x = 0; x < SW; x++)
		{
			for (i = 0; i < mlen[x]; i++)
			{
				y = mhead[x] - i;
				if (y >= 0 && y < SH)
				{
					if (i == 0)
					{
						a = 0x0F;
					}
					else if (i == 1)
					{
						a = 0x0A;
					}
					else if (i < mlen[x] - 2)
					{
						a = 0x02;
					}
					else
					{
						a = 0x08;
					}
					fb_set(x, y, mchars[mchr[x]], a);
				}
			}
			if (((f & 3) == 0) && (demo_rand(3) == 0))
			{
				mchr[x] = demo_rand(mcl);
			}
			mhead[x] += mspd[x];
			if (mhead[x] - mlen[x] > SH)
			{
				mhead[x] = -demo_rand(50);
				mspd[x] = 1 + demo_rand(3);
				mlen[x] = 6 + demo_rand(10);
			}
		}
		fb_flush();
		k = demo_wait(1);
		if (k != KEY_NONE)
		{
			break;
		}
	}

	if (k == KEY_QUIT)
	{
		return k;
	}
	return demo_wipeout();
}

/* ----------------------------------------------------------------------------------------------------------------- */
/* Effect 9: ASCII Mandelbrot in 16.12 fixed point, revealed row by row.                                             */
/* ----------------------------------------------------------------------------------------------------------------- */
static int fx_mandel(void)
{
	static const unsigned char mpal[14] =
	{
		0x01, 0x09, 0x0B, 0x03, 0x02, 0x0A, 0x0E, 0x06, 0x0C, 0x04, 0x05, 0x0D, 0x0F, 0x07
	};
	static const char mramp[9] = { ' ', '.', ':', '-', '=', '+', '*', '#', '%' };
	long cx, cy, zx, zy, zx2, zy2, tx;
	int x, y, it, k;

	_setcursortype(_NOCURSOR);
	demo_fps = 0;
	fb_clear();
	fb_flush();

	k = KEY_NONE;
	for (y = 0; y < SH; y++)
	{
		cy = -4710L + ((long) y * 392L);
		for (x = 0; x < SW; x++)
		{
			cx = -8192L + ((long) x * 137L);
			zx = 0;
			zy = 0;
			for (it = 0; it < 48; it++)
			{
				zx2 = (zx * zx) >> 12;
				zy2 = (zy * zy) >> 12;
				if (zx2 + zy2 > 16384L)
				{
					break;
				}
				tx = zx2 - zy2 + cx;
				zy = ((zx * zy) >> 12) + cy;
				zx = tx;
			}
			if (it >= 48)
			{
				fb_set(x, y, '@', 0x0F);    /* inside the set: bright silhouette */
			}
			else
			{
				fb_set(x, y, mramp[it > 8 ? 8 : it], mpal[it % 14]);
			}
		}
		fb_flush_row(y);
		if ((y & 1) == 1)
		{
			k = demo_wait(1);
			if (k == KEY_QUIT)
			{
				return k;
			}
		}
	}

	k = demo_wait(240);
	if (k == KEY_QUIT)
	{
		return k;
	}
	return demo_wipeout();
}

/* ----------------------------------------------------------------------------------------------------------------- */
/* Effect 10: TUI desktop. Showcases window(), clrscr(), textattr(), putch(), cprintf(),                             */
/* ungetch()/getch(), wherex()/wherey(), delline()/insline(), the cursor types and                                   */
/* gettext()/puttext() save & restore.                                                                               */
/* ----------------------------------------------------------------------------------------------------------------- */
static void demo_window(int l, int t, int r, int b, const char *title)
{
	int x, y;

	/* Drop shadow: black cells with dark gray attribute. */
	textattr(0x08);
	window(l + 1, b + 1, r + 1, b + 1);
	clrscr();
	window(r + 1, t + 1, r + 1, b + 1);
	clrscr();

	/* White on blue frame. */
	textattr(0x1F);
	window(l, t, r, b);
	clrscr();
	gotoxy(1, 1);
	putch(0xC9);
	for (x = 0; x < r - l - 1; x++)
	{
		putch(0xCD);
	}
	putch(0xBB);
	for (y = 2; y < b - t + 1; y++)
	{
		gotoxy(1, y);
		putch(0xBA);
		gotoxy(r - l + 1, y);
		putch(0xBA);
	}
	gotoxy(1, b - t + 1);
	putch(0xC8);
	for (x = 0; x < r - l - 1; x++)
	{
		putch(0xCD);
	}
	putch(0xBC);

	/* Light gray on blue client area, via textbackground()/textcolor(). */
	textbackground(BLUE);
	textcolor(LIGHTGRAY);
	window(l + 1, t + 1, r - 1, b - 1);
	clrscr();

	/* The title sits on the top border: restore the full screen window first, as gotoxy() takes window relative coordinates. */
	window(1, 1, SW, SH);
	if (title != NULL && title[0] != 0)
	{
		gotoxy(l + 2, t);
		textattr(0x1E);
		cprintf(" %s ", title);
	}
}

/* Types a string cell by cell at window relative coordinates. Fast-forwards on SKIP, aborts on ESC. */
static int demo_type(int x, int y, const char *s, int attr)
{
	int i, k, fast;

	fast = 0;
	textattr(attr);
	for (i = 0; s[i] != 0; i++)
	{
		gotoxy(x + i, y);
		putch(s[i]);
		if ((!fast) && ((i & 1) == 1))
		{
			k = demo_wait(1);
			if (k == KEY_QUIT)
			{
				return KEY_QUIT;
			}
			if (k == KEY_SKIP)
			{
				fast = 1;
			}
		}
	}
	return KEY_NONE;
}

static int fx_desktop(void)
{
	static unsigned short ballbuf[10 * 6];
	static const char *about[6] =
	{
		"CONIOX V5.0 - TEXT MODE CONIO",
		"DOS16 / DOS32 / WIN32 / WIN64",
		"WATCOM OPENWATCOM BORLAND DJGPP",
		"PUBLIC DOMAIN (C) 1995-2026",
		"JAVIER GUTIERREZ CHAMORRO",
		"NIKKHOKKHO.SOURCEFORGE.IO"
	};
	int x, y, i, k, bx, by, dx, dy, ch;

	_setcursortype(_NORMALCURSOR);
	demo_fps = 1;

	/* Checkered desktop background, blitted in one shot. */
	fb_clear();
	for (y = 0; y < SH; y++)
	{
		for (x = 0; x < SW; x++)
		{
			fb_set(x, y, 0xB0, (((x ^ y) & 1) ? 0x08 : 0x00));
		}
	}
	fb_flush();

	/* About window with typewriter text. clreol() must stay inside the About window, */
	/* so all its drawing happens in window relative coordinates. */
	demo_window(4, 3, 38, 11, "ABOUT CONIOX");
	window(5, 4, 37, 10);
	for (i = 0; i < 6; i++)
	{
		k = demo_type(2, 2 + i, about[i], (i == 0) ? 0x1F : 0x1B);
		if (k == KEY_QUIT)
		{
			window(1, 1, SW, SH);
			return k;
		}
		gotoxy(2 + (int) strlen(about[i]), 2 + i);
		textattr(0x17);
		clreol();                       /* wipe the rest of the line, inside the window */
		k = demo_wait(8);
		if (k == KEY_QUIT)
		{
			window(1, 1, SW, SH);
			return k;
		}
	}
	window(1, 1, SW, SH);
	k = demo_wait(40);
	if (k == KEY_QUIT)
	{
		return k;
	}

	/* Live API test window. */
	demo_window(43, 13, 76, 23, "LIVE API TEST");

	textattr(0x1A);
	gotoxy(45, 15);
	cprintf("UNGETCH+GETCH: ");
	ungetch('X');
	ch = getch();
	textattr(0x1F);
	cprintf("%c", ch);

	k = demo_wait(24);
	if (k == KEY_QUIT)
	{
		return k;
	}

	textattr(0x1E);
	gotoxy(45, 16);
	cprintf("WHEREX/Y: X=%d Y=%d", wherex(), wherey());

	k = demo_wait(24);
	if (k == KEY_QUIT)
	{
		return k;
	}

	/* lowvideo()/highvideo()/normvideo() intensity cycling. */
	textcolor(LIGHTCYAN);
	textbackground(BLUE);
	gotoxy(45, 17);
	cprintf("INTENSITY: ");
	lowvideo();
	cprintf("LOW ");
	highvideo();
	cprintf("HIGH");
	normvideo();

	k = demo_wait(24);
	if (k == KEY_QUIT)
	{
		return k;
	}

	/* delline()/insline() scrolling inside the panel window only. */
	window(44, 14, 75, 22);
	textattr(0x1B);
	for (i = 0; i < 3; i++)
	{
		gotoxy(2, 5 + i);
		cprintf("ROW %d ALIVE", i + 1);
	}
	k = demo_wait(24);
	if (k == KEY_QUIT)
	{
		window(1, 1, SW, SH);
		return k;
	}
	gotoxy(2, 6);
	delline();                      /* ROW 2 removed, ROW 3 slides up */
	k = demo_wait(30);
	if (k == KEY_QUIT)
	{
		window(1, 1, SW, SH);
		return k;
	}
	gotoxy(2, 6);
	insline();                      /* blank line re-inserted */
	k = demo_wait(30);
	if (k == KEY_QUIT)
	{
		window(1, 1, SW, SH);
		return k;
	}

	/* Cursor shapes. */
	window(1, 1, SW, SH);
	gotoxy(45, 22);
	textattr(0x1F);
	cprintf("CURSOR: ");
	_setcursortype(_SOLIDCURSOR);
	cprintf("SOLID");
	k = demo_wait(20);
	if (k == KEY_QUIT)
	{
		return k;
	}
	_setcursortype(_NORMALCURSOR);
	cprintf(" NORM");
	k = demo_wait(20);
	if (k == KEY_QUIT)
	{
		return k;
	}
	_setcursortype(_NOCURSOR);

	/* Bouncing ball: gettext()/puttext() save & restore over any background. */
	bx = 3;
	by = 15;
	dx = 1;
	dy = 1;
	for (i = 0; i < 240; i++)
	{
		gettext(bx, by, bx + 7, by + 2, ballbuf);
		textattr(0x2F);
		window(bx, by, bx + 7, by + 2);
		clrscr();
		gotoxy(3, 2);
		cprintf("<O>");
		k = demo_wait(2);
		puttext(bx, by, bx + 7, by + 2, ballbuf);
		if (k == KEY_QUIT)
		{
			window(1, 1, SW, SH);
			return k;
		}
		if (k == KEY_SKIP)
		{
			break;
		}
		bx += dx;
		by += dy;
		if (bx <= 2 || bx >= SW - 10)
		{
			dx = -dx;
			bx += dx * 2;
		}
		if (by <= 13 || by >= SH - 4)
		{
			dy = -dy;
			by += dy * 2;
		}
	}
	window(1, 1, SW, SH);

	/* Slide the About window across the desktop with movetext(), repainting the track. */
	for (i = 0; i < 41; i++)
	{
		movetext(4, 3, 39, 12, 5 + i, 3);
		for (y = 3; y <= 12; y++)
		{
			gotoxy(4 + i, y);
			textattr(((((4 + i) ^ y) & 1) ? 0x08 : 0x00));
			putch(0xB0);
		}
		k = demo_wait(1);
		if (k == KEY_QUIT)
		{
			return k;
		}
		if (k == KEY_SKIP)
		{
			break;
		}
	}

	k = demo_wait(60);
	if (k == KEY_QUIT)
	{
		return k;
	}
	return demo_wipeout();
}

/* ----------------------------------------------------------------------------------------------------------------- */
/* Effect 11: CONIO speed benchmark, integrated from the classic coniodemo. The original 50000 putch,                */
/* 1000 cputs and 1000 cprintf tests keep their exact conditions, and new timed tests cover                          */
/* puttext, gettext, movetext, gotoxy + putch and clrscr. The FPS overlay is disabled: its own                       */
/* writes would pollute the measurements.                                                                            */
/* ----------------------------------------------------------------------------------------------------------------- */
static int fx_benchmark(void)
{
	clock_t start, stop;
	unsigned int i;
	int k;

	demo_fps = 0;
	_wscroll = 1;
	_setcursortype(_NOCURSOR);

	/* 50000 chars using putch(). */
	window(1, 2, SW, SH - 1);
	textbackground(BLUE);
	clrscr();

	start = clock();
	textcolor(WHITE);
	clrscr();
	for (i = 0; i < 50000U; i++)
	{
		putch(254);
	}
	stop = clock();
	textcolor(YELLOW);
	cputs("\r\nThose was 50000 254 (\xFE) chars using putch\r\n");
	cprintf("It took %ld clocks (%d clocks are a second)\r\n", stop - start, (int) CLOCKS_PER_SEC);
	textcolor(YELLOW + BLINK);
	cputs("---- Press any key to continue ----");
	do
	{
		k = demo_wait(1);
	} while (k == KEY_NONE);
	if (k == KEY_QUIT)
	{
		_wscroll = 0;
		window(1, 1, SW, SH);
		return k;
	}

	/* 1000 cputs with automatic wrapping and scrolling. */
	window(1, 2, SW, SH - 1);
	textbackground(BLUE);
	clrscr();

	window(5, 5, SW - 5, SH - 5);
	textcolor(WHITE);
	textbackground(MAGENTA);
	clrscr();
	for (i = 0; i < 1000; i++)
	{
		cputs("All functions support automatic scrolling on screen or window ");
	}
	textcolor(YELLOW);
	cputs("\r\nAll functions support automatic scrolling on screen or window\r\n");
	textcolor(YELLOW + BLINK);
	cputs("---- Press any key to continue ----");
	do
	{
		k = demo_wait(1);
	} while (k == KEY_NONE);
	if (k == KEY_QUIT)
	{
		_wscroll = 0;
		window(1, 1, SW, SH);
		return k;
	}

	/* 1000 cputs with explicit newlines. */
	start = clock();
	window(6, 6, SW - 6, SH - 6);
	textcolor(WHITE);
	textbackground(GREEN);
	clrscr();
	for (i = 0; i < 1000; i++)
	{
		cputs("Some cputs here and there\r\n");
	}
	stop = clock();
	textcolor(YELLOW);
	cputs("\r\nThose was 1000 cputs\r\n");
	cprintf("It took %ld clocks (%d clocks are a second)\r\n", stop - start, (int) CLOCKS_PER_SEC);
	textcolor(YELLOW + BLINK);
	cputs("---- Press any key to continue ----");
	do
	{
		k = demo_wait(1);
	} while (k == KEY_NONE);
	if (k == KEY_QUIT)
	{
		_wscroll = 0;
		window(1, 1, SW, SH);
		return k;
	}

	/* 1000 cprintf with formatting. */
	start = clock();
	window(7, 7, SW - 7, SH - 7);
	textcolor(WHITE);
	textbackground(RED);
	clrscr();
	for (i = 0; i < 1000; i++)
	{
		cprintf("Line %u with cprintf writting here\r\n", i);
	}
	stop = clock();
	textcolor(YELLOW);
	cputs("\r\nThose was 1000 cprintf\r\n");
	cprintf("It took %ld clocks (%d clocks are a second)\r\n", stop - start, (int) CLOCKS_PER_SEC);
	textcolor(YELLOW + BLINK);
	cputs("---- Press any key to continue ----");
	do
	{
		k = demo_wait(1);
	} while (k == KEY_NONE);
	if (k == KEY_QUIT)
	{
		_wscroll = 0;
		window(1, 1, SW, SH);
		return k;
	}

	/* 100 whole screen puttext blits. */
	window(1, 2, SW, SH - 1);
	textbackground(BLUE);
	clrscr();
	for (i = 0; i < (unsigned int) (SW * SH); i++)
	{
		demo_fb[i] = (unsigned short) (0x2000 | ((i * 7) & 0xFF));
	}
	start = clock();
	for (i = 0; i < 100; i++)
	{
		puttext(1, 1, SW, SH, demo_fb);
	}
	stop = clock();
	textcolor(YELLOW);
	cputs("\r\nThose was 100 puttext of the whole screen\r\n");
	cprintf("It took %ld clocks (%d clocks are a second)\r\n", stop - start, (int) CLOCKS_PER_SEC);
	textcolor(YELLOW + BLINK);
	cputs("---- Press any key to continue ----");
	do
	{
		k = demo_wait(1);
	} while (k == KEY_NONE);
	if (k == KEY_QUIT)
	{
		_wscroll = 0;
		window(1, 1, SW, SH);
		return k;
	}

	/* 100 whole screen gettext reads. */
	start = clock();
	for (i = 0; i < 100; i++)
	{
		gettext(1, 1, SW, SH, demo_fb);
	}
	stop = clock();
	textcolor(YELLOW);
	cputs("\r\nThose was 100 gettext of the whole screen\r\n");
	cprintf("It took %ld clocks (%d clocks are a second)\r\n", stop - start, (int) CLOCKS_PER_SEC);
	textcolor(YELLOW + BLINK);
	cputs("---- Press any key to continue ----");
	do
	{
		k = demo_wait(1);
	} while (k == KEY_NONE);
	if (k == KEY_QUIT)
	{
		_wscroll = 0;
		window(1, 1, SW, SH);
		return k;
	}

	/* 100 whole screen movetext scrolls. */
	start = clock();
	for (i = 0; i < 100; i++)
	{
		movetext(1, 1, SW, SH - 1, 1, 2);
	}
	stop = clock();
	textcolor(YELLOW);
	cputs("\r\nThose was 100 movetext of the whole screen\r\n");
	cprintf("It took %ld clocks (%d clocks are a second)\r\n", stop - start, (int) CLOCKS_PER_SEC);
	textcolor(YELLOW + BLINK);
	cputs("---- Press any key to continue ----");
	do
	{
		k = demo_wait(1);
	} while (k == KEY_NONE);
	if (k == KEY_QUIT)
	{
		_wscroll = 0;
		window(1, 1, SW, SH);
		return k;
	}

	/* 10000 positioned chars using gotoxy() + putch(). */
	window(1, 1, SW, SH);
	textattr(0x07);
	clrscr();
	start = clock();
	for (i = 0; i < 10000; i++)
	{
		gotoxy(1 + demo_rand(SW), 1 + demo_rand(SH));
		putch(254);
	}
	stop = clock();
	textcolor(YELLOW);
	gotoxy(1, 1);
	cputs("\r\nThose was 10000 gotoxy + putch\r\n");
	cprintf("It took %ld clocks (%d clocks are a second)\r\n", stop - start, (int) CLOCKS_PER_SEC);
	textcolor(YELLOW + BLINK);
	cputs("---- Press any key to continue ----");
	do
	{
		k = demo_wait(1);
	} while (k == KEY_NONE);
	if (k == KEY_QUIT)
	{
		_wscroll = 0;
		window(1, 1, SW, SH);
		return k;
	}

	/* 250 clrscr of the working window. */
	window(1, 2, SW, SH - 1);
	textbackground(BLUE);
	textcolor(YELLOW);
	start = clock();
	for (i = 0; i < 250; i++)
	{
		clrscr();
	}
	stop = clock();
	cputs("\r\nThose was 250 clrscr\r\n");
	cprintf("It took %ld clocks (%d clocks are a second)\r\n", stop - start, (int) CLOCKS_PER_SEC);
	textcolor(YELLOW + BLINK);
	cputs("---- Press any key to continue ----");
	do
	{
		k = demo_wait(1);
	} while (k == KEY_NONE);

	_wscroll = 0;
	window(1, 1, SW, SH);
	if (k == KEY_QUIT)
	{
		return k;
	}
	return demo_wipeout();
}

/* ----------------------------------------------------------------------------------------------------------------- */
/* Effect 12: closing credits.                                                                                       */
/* ----------------------------------------------------------------------------------------------------------------- */
static int fx_credits(void)
{
	static const char *cred_fx[8] =
	{
		"PORSCHE PHOTO TO ANSI",
		"TEXTMODE FIRE",
		"3D STARFIELD WARP",
		"COLOR CYCLING PLASMA",
		"DOUBLE SINE SCROLLER",
		"MATRIX RAIN",
		"FIXED POINT MANDELBROT",
		"TUI DESKTOP SHOWCASE"
	};
	static const char *cred_api[6] =
	{
		"CLRSCR GOTOXY PUTCH CPRINTF CPUTS",
		"GETTEXT PUTTEXT MOVETEXT WINDOW",
		"TEXTMODE TEXTCOLOR TEXTBACKGROUND",
		"TEXTATTR HIGHVIDEO LOWVIDEO NORMVIDEO",
		"GETCH KBHIT UNGETCH DELAY",
		"DELLINE INSLINE CLREOL CURSOR"
	};
	int i, t, k, blink;

	_setcursortype(_NOCURSOR);
	demo_fps = 1;
	fb_clear();
	fb_flush();

	k = KEY_NONE;
	for (t = 0; ; t++)
	{
		fb_clear();
		for (i = 0; i < SW; i++)
		{
			fb_set(i, 0, 0xCD, 0x08);
			fb_set(i, 24, 0xCD, 0x08);
		}

		if (t > 8)
		{
			fb_text_centered(2, "C O N I O X   D E M O", 0x0F);
			fb_text_centered(3, "DOS16 / DOS32 / WIN32 / WIN64", 0x08);
		}
		for (i = 0; i < 8; i++)
		{
			if (t > 20 + (i * 11))
			{
				fb_text_centered(5 + i, cred_fx[i], demo_rainbow[i % 14]);
			}
		}
		if (t > 104)
		{
			fb_text_centered(13, "CONIO.H API EXERCISED", 0x0B);
		}
		for (i = 0; i < 6; i++)
		{
			if (t > 120 + (i * 12))
			{
				fb_text_centered(14 + i, cred_api[i], 0x07);
			}
		}
		if (t > 190)
		{
			fb_text_centered(21, "WATCOM - BORLAND - DJGPP - MSVC - MINGW - CLANG - PELLES", 0x08);
		}
		if (t > 210)
		{
			blink = ((t / 10) & 1) ? 0x70 : 0x07;
			fb_text_centered(22, "PRESS ANY KEY TO EXIT - THANK YOU", blink);
		}

		fb_flush();
		k = demo_wait(1);
		if (k != KEY_NONE)
		{
			break;
		}
	}

	if (k == KEY_QUIT)
	{
		return k;
	}
	return KEY_SKIP;    /* credits are the last stop: fall through to the goodbye screen */
}

/* ----------------------------------------------------------------------------------------------------------------- */
/* Main: mode setup, effect chain, clean shutdown.                                                                   */
/* ----------------------------------------------------------------------------------------------------------------- */
int main(void)
{
	static int (*const fxlist[])(void) = { fx_intro, fx_info, fx_ansi, fx_fire, fx_stars, fx_plasma, fx_scroller, fx_matrix, fx_mandel, fx_desktop, fx_benchmark, fx_credits };
	int i, r;

	textmode(C80);
	gettextinfo(&demo_ti);
	SW = (int) demo_ti.screenwidth;
	SH = (int) demo_ti.screenheight;
	if (SW > DEMO_COLS)
	{
		SW = DEMO_COLS;
	}
	if (SH > DEMO_ROWS)
	{
		SH = DEMO_ROWS;
	}
	inittextinfo();
	_wscroll = 0;
	_setcursortype(_NOCURSOR);

	r = KEY_NONE;
	for (i = 0; i < (int) (sizeof(fxlist) / sizeof(fxlist[0])); i++)
	{
		r = fxlist[i]();
		if (r == KEY_QUIT)
		{
			break;
		}
	}

	/* Clean shutdown: restore the standard text screen. */
	textmode(C80);
	normvideo();
	clrscr();
	_setcursortype(_NORMALCURSOR);
	gotoxy(1, 1);
	textcolor(LIGHTGREEN);
	cprintf("CONIOX demo finished - thanks for watching!\r\n");
	textcolor(LIGHTGRAY);
	cprintf("conio.h engine: CONIOX v%d.%02d by Javier Gutierrez Chamorro\r\n", coniox_version / 100, coniox_version % 100);
	cprintf("Demo build date: %s\r\n", coniox_build);
	return 0;
}