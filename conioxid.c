/* ================================================================================= */
/*                                                                                   */
/*   ARKANOID CONIOX v1.0                                       (c) 2026 Public Domain */
/*                                                                                   */
/*   Juego de demostracion para la libreria CONIOX de Javier Gutierrez Chamorro.      */
/*   Ejerce practicamente toda la API de conio: clrscr, clreol, gotoxy, wherex/wherey,*/
/*   textattr/textcolor/textbackground, highvideo/lowvideo/normvideo, window,          */
/*   cprintf/cputs/putch/getch/getche/kbhit/ungetch, cgets/cscanf/getpass,             */
/*   gettext/puttext/movetext, insline/delline, _setcursortype, textmode, delay,       */
/*   gettextinfo y (solo Watcom+DOS) el altavoz PC mediante inportb/outportb.          */
/*                                                                                   */
/*   Compilacion con OpenWatcom (ver MAKEFILE y README.TXT):                          */
/*      DOS16 : wcl -bt=dos -ml        -os -fe=ark16.exe arkanoid.c coniox.c           */
/*      DOS32 : wcl -bt=dos -l=dos4g   -os -fe=ark32.exe arkanoid.c coniox.c           */
/*      WIN32 : wcl -bt=nt  -l=nt      -os -fe=arkw.exe  arkanoid.c coniox.c           */
/*                                                                                   */
/*   Textos en ingles a proposito: se ven identicos en CP437/CP850 (DOS) y CP1252     */
/*   (Win32), y evita problemas de codepage. Comentarios en espanol sin tildes.       */
/*                                                                                   */
/* ================================================================================= */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "coniox.h"

/* --------------------------------------------------------------------------------- */
/* PLATAFORMA OBJETIVO                                                                */
/* --------------------------------------------------------------------------------- */
#if ((__DOS__) || (__MSDOS__))
#define IS_DOS 1
#define IS_WIN 0
#else
#define IS_WIN 1
#define IS_DOS 0
#endif

#if IS_WIN
#include <windows.h>        /* Beep() para el sonido en Win32 */
#endif

/*
   Granularidad del bucle:
   - En DOS, delay() espera ticks del BIOS (18.2 Hz): cualquier valor < 55 ms redondea
     a 1 tick. Con FRAME_MS 50 pedimos exactamente 1 tick.
   - En Win32, delay() usa Sleep(): FRAME_MS 20 da ~50 fps, suficiente para el juego.
*/
#if IS_DOS
#define FRAME_MS 50
#else
#define FRAME_MS 20
#endif

/*
   Voz digitalizada (ark_pcm): PCM 8 bits mono a SND_RATE Hz, sin cabecera WAV.
   En DOS el ritmo son cuentas del timer 0 (1193180 / SND_RATE) y en Win32 la
   cabecera RIFF se compone en runtime con este mismo valor.
*/
#define SND_RATE 3000

/* --------------------------------------------------------------------------------- */
/* CONSTANTES DEL JUEGO                                                               */
/* --------------------------------------------------------------------------------- */
#define APP_NAME     "ARKANOID CONIOX"
#define APP_VERSION  "1.0"
#define HI_FILE      "ARKANOID.DAT"
#define CHEAT_CODE   "NIKKHO"         /* El nick del autor de CONIOX en SourceForge  */

#define PW       7                    /* Ancho de la pala, en celdas                  */
#define GW       66                   /* Ancho maximo del interior del campo          */
#define GH       46                   /* Alto  maximo del interior del campo (80x50)  */
#define PANELW   12                   /* Ancho del panel lateral, bordes incluidos    */
#define MAXROWS  8                    /* Filas de ladrillos maximo                    */
#define LIVES0   3                    /* Vidas iniciales                              */

/* Caracteres de marco: lineas de caja CP437 en las dos plataformas. En Win32        */
/* la consola pasa a CP 437 en main() para que se dibujen bien.                      */
#define B_TL '\xDA'
#define B_TR '\xBF'
#define B_BL '\xC0'
#define B_BR '\xD9'
#define B_HZ '\xC4'
#define B_VT '\xB3'
#define CH_FULL '\xDB'                /* Ladrillo solido: bloque lleno                */

/* --------------------------------------------------------------------------------- */
/* VARIABLES GLOBALES                                                                 */
/* --------------------------------------------------------------------------------- */
static struct text_info tinfo;
static int scrw, scrh;                /* Tamano de pantalla detectado                  */

static int opt_speed = 5;             /* 1..9                                          */
static int opt_sound = 1;             /* 0/1                                           */
static int opt_mode  = 0;             /* 0 = 80x25, 1 = 80x50                          */
static int ghost     = 0;             /* Modo fantasma (codigo secreto)                */

static struct { char name[16]; long score; } hi[3];

/* Geometria del campo (coordenadas absolutas de pantalla)                            */
static int fld_x1, fld_y1, fld_x2, fld_y2;
static int fwx, fwy;                  /* Interior del campo en celdas                  */
#define ABX(ix) (fld_x1 + (ix))       /* Interior -> pantalla                          */
#define ABY(iy) (fld_y1 + (iy))

/* Estado de la partida                                                               */
static unsigned char brick[GH + 1][GW + 1];   /* 0 libre, 1 normal, 2 dura, 3 solido   */
static int bricks_left;
static long score;
static int lives, level;
static int paddle_x;
static int ball_x, ball_y, ball_dx, ball_dy;
static int ball_stuck, ball_timer;
static int clear_pending;

static const int rowcolor[MAXROWS] = { LIGHTRED, RED, MAGENTA, LIGHTMAGENTA,
				       BROWN, YELLOW, LIGHTCYAN, GREEN };

static unsigned short dlgbuf[512];   /* Salvapantallas de dialogos: maximo 440 celdas  */

/* --------------------------------------------------------------------------------- */
/* PROTOTIPOS                                                                         */
/* --------------------------------------------------------------------------------- */
static void read_screen(void);
static void full_window(void);
static void putchxy(int x, int y, int ch);
static void cputsxy(int x, int y, const char *s);
static void cprintfxy(int x, int y, const char *fmt, ...);
static int  center_x(int len);
static int  center_y(int lines);
static void screen_bg(int attr);
static void draw_box(int x1, int y1, int x2, int y2, int attr, const char *title);
static int  read_key(void);
static int  str_ieq(const char *a, const char *b);
static int  dialog_yn(const char *title, const char *msg);

static void snd_play(int freq, int ticks);
static void snd_tick(void);
static void snd_off(void);
static void snd_menu(void);
static void snd_select(void);
static void snd_wall(void);
static void snd_paddle(void);
static void snd_crack(void);
static void snd_break(void);
static void snd_solid(void);
static void snd_voice(void);
static void jingle(int up);

static void records_default(void);
static void records_load(void);
static void records_save(void);
static void records_insert(const char *name, long sc);
static void records_show(void);
static void secret_code(void);

static void intro_screen(void);
static void menu_draw(int sel);
static int  menu_loop(void);
static void settings_draw(int sel, int n, int row);
static void settings_screen(void);
static void help_screen(void);
static void about_screen(void);

static void game_geometry(void);
static void init_level(void);
static void brick_draw(int ix, int iy);
static void bricks_draw_all(void);
static void paddle_draw(void);
static void paddle_erase(void);
static void ball_draw(void);
static void ball_erase(void);
static void panel_draw(void);
static void panel_score(void);
static void chrome_draw(void);
static void game_redraw(void);
static void game_start(void);
static void move_paddle(int dx);
static int  paddle_dx_for(int hitx);
static void hit_brick(int ix, int iy);
static int  move_one(int mdx, int mdy);
static int  ball_step(void);
static int  ball_interval(void);
static void center_msg(const char *s, int attr);
static void pause_overlay(void);
static int  lose_ball(void);
static void game_over(void);
static void level_clear(void);
static void game_run(void);

static int  demo_header(int n, const char *title);
static int  demo_wait(void);
static int  demo_s01(void);
static int  demo_s02(void);
static int  demo_s03(void);
static int  demo_s04(void);
static int  demo_s05(void);
static int  demo_s06(void);
static int  demo_s07(void);
static int  demo_s08(void);
static int  demo_s09(void);
static int  demo_s10(void);
static int  demo_s11(void);
static int  demo_s12(void);
static int  demo_s13(void);
static void demo_tour(void);

/* ================================================================================= */
/*  HELPERS GENERALES DE PANTALLA                                                     */
/* ================================================================================= */

/* --------------------------------------------------------------------------------- */
static void read_screen(void)
{
	gettextinfo(&tinfo);
	scrw = tinfo.screenwidth;
	scrh = tinfo.screenheight;
}

/* --------------------------------------------------------------------------------- */
/* Restaura la ventana activa a pantalla completa (coniox usa coordenadas relativas   */
/* a la ventana en gotoxy/putch/cprintf y absolutas en gettext/puttext/movetext).     */
/* --------------------------------------------------------------------------------- */
static void full_window(void)
{
	window(1, 1, scrw, scrh);
	textattr(0x07);
}

/* --------------------------------------------------------------------------------- */
static void putchxy(int x, int y, int ch)
{
	gotoxy(x, y);
	putch(ch);
}

/* --------------------------------------------------------------------------------- */
static void cputsxy(int x, int y, const char *s)
{
	gotoxy(x, y);
	cputs(s);
}

/* --------------------------------------------------------------------------------- */
/* cprintfxy() usa cvprintf() de la libreria: version va_list de cprintf.             */
/* --------------------------------------------------------------------------------- */
static void cprintfxy(int x, int y, const char *fmt, ...)
{
	va_list ap;

	gotoxy(x, y);
	va_start(ap, fmt);
	cvprintf(fmt, ap);
	va_end(ap);
}

/* --------------------------------------------------------------------------------- */
static int center_x(int len)
{
	int x = (scrw - len) / 2;

	return (x < 1) ? 1 : x;
}

/* --------------------------------------------------------------------------------- */
static int center_y(int lines)
{
	int y = (scrh - lines) / 2;

	return (y < 1) ? 1 : y;
}

/* --------------------------------------------------------------------------------- */
/* Fondo completo: coniox clrscr() rellena la ventana con el atributo en curso.       */
/* --------------------------------------------------------------------------------- */
static void screen_bg(int attr)
{
	textattr(attr);
	clrscr();
}

/* --------------------------------------------------------------------------------- */
/* Caja con marco y relleno. Si title != NULL se centra sobre el borde superior.      */
/* --------------------------------------------------------------------------------- */
static void draw_box(int x1, int y1, int x2, int y2, int attr, const char *title)
{
	int x, y, tl;

	if (x2 < x1 || y2 < y1 || x1 < 1 || y1 < 1 || x2 > scrw || y2 > scrh)
	{
		return;
	}
	textattr(attr);
	for (y = y1; y <= y2; y++)
	{
		gotoxy(x1, y);
		putch((y == y1) ? B_TL : ((y == y2) ? B_BL : B_VT));
		for (x = x1 + 1; x <= x2 - 1; x++)
		{
			putch((y == y1 || y == y2) ? B_HZ : ' ');
		}
		putch((y == y1) ? B_TR : ((y == y2) ? B_BR : B_VT));
	}
	if (title != NULL)
	{
		tl = (int) strlen(title) + 2;
		gotoxy(x1 + (x2 - x1 + 1 - tl) / 2, y1);
		cprintf(" %s ", title);
	}
}

/* --------------------------------------------------------------------------------- */
/* getch() devuelve 0 y a continuacion el scan code para las teclas extendidas, tanto  */
/* en DOS como en Win32 (comportamiento unificado de CONIOX). Sumamos KEY_SPECIAL     */
/* (0x100) para distinguirlas de los caracteres planos.                               */
/* --------------------------------------------------------------------------------- */
static int read_key(void)
{
	int c = getch();

	if (c == 0)
	{
		c = getch() + KEY_SPECIAL;
	}
	return c;
}

/* --------------------------------------------------------------------------------- */
static int str_ieq(const char *a, const char *b)
{
	if (a == NULL || b == NULL)
	{
		return 0;
	}
	while (*a && *b)
	{
		if (toupper((unsigned char) *a) != toupper((unsigned char) *b))
		{
			return 0;
		}
		a++;
		b++;
	}
	return (*a == *b);
}

/* --------------------------------------------------------------------------------- */
/* Dialogo de confirmacion Y/N: salva la zona con gettext(), dibuja, espera y         */
/* restaura con puttext(). Salvamento/restauracion de pantalla sin parpadeo.             */
/* --------------------------------------------------------------------------------- */
static int dialog_yn(const char *title, const char *msg)
{
	int w, x1, y1, x2, y2, c, r = 0;

	w = (int) strlen(msg) + 8;
	if (w < 26)                 /* Minimo: los botones miden 23 y empiezan en col 2  */
	{
		w = 26;
	}
	x1 = center_x(w);
	x2 = x1 + w - 1;
	y1 = center_y(5);
	y2 = y1 + 4;
	if (!gettext(x1, y1, x2, y2, dlgbuf))
	{
		return 0;
	}
	draw_box(x1, y1, x2, y2, 0x4E, title);
	textattr(0x4F);
	cprintfxy(x1 + 2, y1 + 2, "%s", msg);
	textattr(0x4E);
	cprintfxy(x1 + 2, y1 + 3, "  <Y> YES      <N> NO  ");
	snd_off();                  /* Silencia cualquier pitido activo al bloquearse   */
	for (;;)
	{
		c = read_key();
		if (c == 'y' || c == 'Y')
		{
			r = 1;
			break;
		}
		if (c == 'n' || c == 'N' || c == KEY_ESC)
		{
			break;
		}
	}
	puttext(x1, y1, x2, y2, dlgbuf);
	return r;
}

/* ================================================================================= */
/*  SONIDO: altavoz PC en DOS (puertos 0x42/0x43/0x61 via inportb/outportb de CONIOX) */
/*  y Beep() del API de consola en Win32.                                             */
/*                                                                                    */
/*  En DOS el sonido es NO bloqueante: snd_play() programa el timer 8254 y enciende   */
/*  la puerta; snd_tick() se llama una vez por frame y apaga el altavoz al agotarse   */
/*  la cuenta. delay() de DOS tiene granularidad de 55 ms y no serviria para pitidos  */
/*  cortos dentro del juego. Los menus y dialogos se bloquean en read_key(), donde    */
/*  nadie llama a snd_tick(): por eso snd_menu()/snd_select() se apagan solas y los   */
/*  overlays que esperan una tecla hacen snd_off() antes de bloquearse.               */
/* ================================================================================= */
#if (IS_DOS && defined(__WATCOMC__))

static unsigned int snd_count = 0;

static void snd_play(int freq, int ticks)
{
	unsigned long div;

	if (!opt_sound || freq < 20)
	{
		return;
	}
	outportb(0x61, (unsigned char) (inportb(0x61) & 0xFC));   /* Puerta cerrada: sin glitch */
	div = 1193180UL / (unsigned long) freq;
	if (div > 65535UL)
	{
		div = 65535UL;
	}
	outportb(0x43, 0xB6);                       /* Timer 2, modo 3 (onda cuadrada) */
	outportb(0x42, (unsigned char) (div & 0xFF));
	outportb(0x42, (unsigned char) ((div >> 8) & 0xFF));
	outportb(0x61, (unsigned char) (inportb(0x61) | 0x03));   /* Puerta + gate RAM */
	snd_count = (unsigned int) ticks;
}

static void snd_tick(void)
{
	if (snd_count > 0 && --snd_count == 0)
	{
		outportb(0x61, (unsigned char) (inportb(0x61) & 0xFC));
	}
}

static void snd_off(void)
{
	snd_count = 0;
	outportb(0x61, (unsigned char) (inportb(0x61) & 0xFC));
}

#elif IS_DOS
/* DOS sin Watcom: la cabecera no publica inportb/outportb. Sin sonido.               */
static void snd_play(int freq, int ticks)
{
	(void) freq;
	(void) ticks;
}

static void snd_tick(void)
{
}

static void snd_off(void)
{
}

#else
/* Win32: Beep() es bloqueante, dura ticks*FRAME_MS y simula el mismo API.            */
static void snd_play(int freq, int ticks)
{
	if (opt_sound && freq >= 20)
	{
		Beep((DWORD) freq, (DWORD) (ticks * FRAME_MS));
	}
}

static void snd_tick(void)
{
}

static void snd_off(void)
{
}

#endif

/* --------------------------------------------------------------------------------- */
static void snd_menu(void)
{
	snd_play(700, 1);
#if (IS_DOS && defined(__WATCOMC__))
	delay(FRAME_MS);            /* El menu se bloquea en read_key() y nadie llama   */
	snd_off();                  /* a snd_tick(): el pitido debe apagarse solo       */
#endif
}

static void snd_select(void)
{
	snd_play(1000, 2);
#if (IS_DOS && defined(__WATCOMC__))
	delay(FRAME_MS * 2);
	snd_off();
#endif
}

static void snd_wall(void)
{
	snd_play(250, 1);
}

static void snd_paddle(void)
{
	snd_play(450, 1);
}

static void snd_crack(void)
{
	snd_play(650, 1);
}

static void snd_break(void)
{
	snd_play(900, 2);
}

static void snd_solid(void)
{
	snd_play(120, 1);
}

/* --------------------------------------------------------------------------------- */
/* Voz digitalizada de arranque: dice "ARKANOID" con un sample PCM real de 8 bits  */
/* a SND_RATE Hz en ark_pcm[], pelado: sin cabecera WAV (en Win32 la cabecera RIFF */
/* se compone en runtime y va a BSS, sin ocupar sitio en el ejecutable) y con los  */
/* silencios de cabeza y cola recortados, que no se oyen. La senal se filtro y se  */
/* rebajo de 4 kHz a SND_RATE Hz: la voz no aporta apenas energia por encima de    */
/* 1.4 kHz, asi que suena practicamente igual con un tercio menos de datos, sin    */
/* compresion alguna.                                                              */
/* En DOS se reproduce por el altavoz PC con PWM: el timer 2 va a frecuencia       */
/* maxima (divisor 2) y se conmuta el bit 1 del puerto 0x61 al ritmo de cada       */
/* muestra, contada con el timer 0 y con el valor del puerto cacheado; en          */
/* Win32 se pasa la imagen WAV recompuesta a PlaySoundA() con winmm cargada en     */
/* runtime, sin tocar el enlace de wcl y sin bloquear la portada (SND_ASYNC).      */
#if ((IS_DOS && defined(__WATCOMC__)) || IS_WIN)
static const unsigned char ark_pcm[] = {
		0x80, 0x80, 0x80, 0x80, 0x7E, 0x80, 0x80, 0x7F, 0x81, 0x7F, 0x81, 0x7E, 0x82, 0x7E, 0x82, 0x81,
		0x75, 0x89, 0x7A, 0x7F, 0x87, 0x79, 0x86, 0x7F, 0x7F, 0x7E, 0x82, 0x81, 0x76, 0x86, 0x7D, 0x7E,
		0x85, 0x7C, 0x81, 0x83, 0x7D, 0x85, 0x7D, 0x80, 0x7E, 0x7D, 0x88, 0x6F, 0x85, 0x81, 0x73, 0x89,
		0x7B, 0x7F, 0x86, 0x79, 0x87, 0x80, 0x7A, 0x85, 0x7B, 0x87, 0x77, 0x82, 0x80, 0x73, 0x8B, 0x77,
		0x85, 0x85, 0x7A, 0x87, 0x7D, 0x7E, 0x7C, 0x84, 0x7C, 0x7A, 0x87, 0x7A, 0x83, 0x84, 0x7C, 0x84,
		0x82, 0x7E, 0x87, 0x7A, 0x82, 0x75, 0x83, 0x85, 0x67, 0x90, 0x77, 0x78, 0x90, 0x73, 0x8A, 0x87,
		0x75, 0x98, 0x76, 0x7D, 0x85, 0x77, 0x8E, 0x64, 0x8C, 0x7E, 0x6D, 0x95, 0x71, 0x87, 0x8C, 0x76,
		0x90, 0x7A, 0x71, 0x8A, 0x87, 0x71, 0x8F, 0x83, 0x75, 0x9A, 0x77, 0x86, 0x8A, 0x79, 0x86, 0x7A,
		0x71, 0x7F, 0x9C, 0x6B, 0x8B, 0x93, 0x66, 0x9A, 0x73, 0x7A, 0x92, 0x71, 0x7E, 0x82, 0x5B, 0x8F,
		0x86, 0x56, 0x9E, 0x6E, 0x7A, 0x9A, 0x6D, 0x92, 0x8A, 0x6F, 0x8E, 0x71, 0x77, 0xA9, 0x68, 0x8E,
		0x8B, 0x69, 0x9A, 0x72, 0x81, 0x8E, 0x6A, 0x82, 0x70, 0x6D, 0x9B, 0x68, 0x86, 0x92, 0x69, 0xA0,
		0x83, 0x7C, 0x9A, 0x73, 0x80, 0x76, 0x70, 0x9E, 0x61, 0x7E, 0x92, 0x5A, 0x94, 0x84, 0x70, 0x94,
		0x70, 0x7F, 0x78, 0x72, 0xB2, 0x67, 0x86, 0x9F, 0x63, 0x9A, 0x83, 0x76, 0x94, 0x67, 0x79, 0x70,
		0x72, 0x9F, 0x5F, 0x93, 0x8F, 0x68, 0xAF, 0x7D, 0x7F, 0x9C, 0x68, 0x7E, 0x68, 0x88, 0x8A, 0x56,
		0x99, 0x74, 0x76, 0x9F, 0x73, 0x8C, 0x88, 0x6C, 0x7C, 0x76, 0x95, 0x6D, 0x78, 0x93, 0x61, 0x90,
		0x8C, 0x72, 0x8D, 0x78, 0x72, 0x71, 0x8C, 0x92, 0x67, 0x96, 0x79, 0x76, 0x9B, 0x74, 0x84, 0x81,
		0x76, 0x6F, 0x72, 0xAD, 0x6C, 0x7E, 0xA7, 0x63, 0x97, 0x8D, 0x75, 0x8F, 0x6B, 0x7B, 0x62, 0x8A,
		0x9A, 0x55, 0xB0, 0x81, 0x6E, 0xB7, 0x75, 0x8A, 0x88, 0x6C, 0x77, 0x5A, 0xA3, 0x6B, 0x62, 0xAE,
		0x58, 0x99, 0x9E, 0x6B, 0xA5, 0x74, 0x73, 0x6C, 0x84, 0x96, 0x41, 0x9D, 0x7D, 0x55, 0xB0, 0x70,
		0x83, 0x93, 0x67, 0x7C, 0x72, 0xA1, 0x67, 0x6E, 0xA7, 0x47, 0x90, 0x98, 0x59, 0x9A, 0x75, 0x6A,
		0x74, 0x8A, 0xA0, 0x59, 0xA1, 0x84, 0x5F, 0xB1, 0x6A, 0x7A, 0x95, 0x57, 0x74, 0x73, 0xA7, 0x74,
		0x72, 0xBD, 0x5C, 0x92, 0xAC, 0x62, 0x9B, 0x7A, 0x62, 0x73, 0x85, 0x94, 0x58, 0xAB, 0x88, 0x61,
		0xC6, 0x7A, 0x79, 0xAA, 0x64, 0x6D, 0x74, 0x9A, 0x6B, 0x67, 0xAC, 0x59, 0x8F, 0xAA, 0x61, 0x9B,
		0x8B, 0x5C, 0x6D, 0x97, 0x84, 0x49, 0xA2, 0x7B, 0x52, 0xB4, 0x77, 0x6A, 0xA1, 0x68, 0x59, 0x89,
		0x9E, 0x4D, 0x82, 0xA1, 0x40, 0x99, 0x9C, 0x52, 0x94, 0x83, 0x51, 0x71, 0xAA, 0x6E, 0x5F, 0xB2,
		0x5B, 0x71, 0xB5, 0x61, 0x79, 0x99, 0x5B, 0x5A, 0xA1, 0x95, 0x50, 0xA3, 0x8F, 0x57, 0xB0, 0x8A,
		0x62, 0x9A, 0x79, 0x4D, 0x8E, 0xB5, 0x53, 0x89, 0xB5, 0x54, 0x9B, 0xAE, 0x61, 0x8C, 0x94, 0x4F,
		0x73, 0xC6, 0x65, 0x6B, 0xC5, 0x64, 0x7F, 0xC2, 0x6E, 0x77, 0xA2, 0x5F, 0x58, 0xC4, 0x82, 0x53,
		0xC2, 0x7D, 0x67, 0xC1, 0x86, 0x66, 0x9E, 0x76, 0x44, 0xAE, 0xA1, 0x42, 0xAC, 0x9A, 0x56, 0xB1,
		0x9F, 0x5E, 0x8D, 0x8C, 0x3F, 0x8B, 0xBA, 0x47, 0x83, 0xAE, 0x5D, 0x90, 0xAB, 0x6C, 0x72, 0x91,
		0x50, 0x62, 0xBE, 0x61, 0x5C, 0xAD, 0x72, 0x77, 0xA7, 0x81, 0x61, 0x8B, 0x61, 0x4F, 0xB8, 0x73,
		0x50, 0xA2, 0x80, 0x74, 0x9C, 0x8C, 0x64, 0x80, 0x65, 0x54, 0xB1, 0x79, 0x57, 0x96, 0x85, 0x80,
		0x92, 0x8E, 0x6E, 0x78, 0x62, 0x5D, 0xAE, 0x79, 0x5F, 0x8F, 0x87, 0x8B, 0x8D, 0x8D, 0x72, 0x74,
		0x5E, 0x64, 0xAE, 0x7A, 0x65, 0x87, 0x8B, 0x8F, 0x89, 0x8C, 0x72, 0x71, 0x57, 0x7E, 0xAD, 0x71,
		0x74, 0x83, 0x8C, 0x91, 0x8C, 0x82, 0x75, 0x6D, 0x57, 0x9D, 0xA1, 0x6F, 0x7A, 0x7E, 0x8D, 0x90,
		0x8D, 0x7A, 0x7A, 0x62, 0x64, 0xAE, 0x90, 0x74, 0x79, 0x7F, 0x8E, 0x95, 0x8A, 0x76, 0x79, 0x59,
		0x7B, 0xAB, 0x84, 0x78, 0x74, 0x80, 0x92, 0x9B, 0x83, 0x77, 0x67, 0x5B, 0x91, 0x9B, 0x81, 0x78,
		0x73, 0x81, 0x99, 0x99, 0x7D, 0x71, 0x54, 0x6D, 0x96, 0x92, 0x83, 0x74, 0x72, 0x87, 0x9E, 0x8F,
		0x7C, 0x63, 0x53, 0x80, 0x95, 0x92, 0x80, 0x70, 0x76, 0x91, 0x9B, 0x8B, 0x77, 0x57, 0x63, 0x8A,
		0x95, 0x91, 0x7A, 0x6F, 0x7D, 0x98, 0x96, 0x8A, 0x6B, 0x55, 0x73, 0x8D, 0x95, 0x8B, 0x76, 0x71,
		0x87, 0x99, 0x94, 0x86, 0x60, 0x5E, 0x7A, 0x8C, 0x94, 0x86, 0x76, 0x76, 0x8C, 0x96, 0x91, 0x7D,
		0x5F, 0x66, 0x79, 0x89, 0x91, 0x87, 0x7C, 0x7B, 0x89, 0x90, 0x8E, 0x7D, 0x67, 0x68, 0x74, 0x85,
		0x8D, 0x89, 0x81, 0x7E, 0x85, 0x8B, 0x8B, 0x82, 0x73, 0x6B, 0x70, 0x7E, 0x88, 0x89, 0x84, 0x7F,
		0x82, 0x88, 0x8A, 0x86, 0x7D, 0x75, 0x73, 0x76, 0x7B, 0x82, 0x84, 0x84, 0x83, 0x85, 0x86, 0x85,
		0x82, 0x81, 0x7F, 0x79, 0x76, 0x74, 0x79, 0x80, 0x86, 0x86, 0x85, 0x83, 0x82, 0x83, 0x82, 0x82,
		0x80, 0x7A, 0x77, 0x77, 0x7A, 0x80, 0x83, 0x83, 0x82, 0x84, 0x84, 0x84, 0x82, 0x81, 0x81, 0x7F,
		0x7C, 0x79, 0x78, 0x7A, 0x7F, 0x82, 0x84, 0x84, 0x83, 0x82, 0x82, 0x82, 0x81, 0x80, 0x7F, 0x7F,
		0x7F, 0x7D, 0x7D, 0x7D, 0x7E, 0x80, 0x82, 0x82, 0x81, 0x81, 0x80, 0x80, 0x81, 0x80, 0x80, 0x80,
		0x80, 0x80, 0x80, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x80, 0x81, 0x81, 0x82, 0x82, 0x82, 0x80, 0x7E,
		0x7C, 0x7C, 0x7D, 0x7F, 0x80, 0x80, 0x81, 0x82, 0x82, 0x82, 0x81, 0x80, 0x80, 0x80, 0x7F, 0x7F,
		0x7F, 0x7F, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x7F, 0x7F, 0x7F, 0x80, 0x80, 0x80, 0x80, 0x80,
		0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x7F,
		0x7F, 0x7F, 0x80, 0x80, 0x80, 0x80, 0x81, 0x81, 0x81, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
		0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
		0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
		0x80, 0x80, 0x80, 0x80, 0x80, 0x7F, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
		0x80, 0x80, 0x80, 0x80, 0x80, 0x7F, 0x7F, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
		0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
		0x82, 0x80, 0x80, 0x81, 0x7F, 0x7F, 0x80, 0x7F, 0x81, 0x82, 0x7E, 0x81, 0x7F, 0x7F, 0x80, 0x7F,
		0x80, 0x7F, 0x80, 0x7F, 0x81, 0x80, 0x7F, 0x81, 0x7B, 0x84, 0x80, 0x79, 0x84, 0x7C, 0x80, 0x81,
		0x81, 0x8E, 0x7C, 0x78, 0x82, 0x7E, 0x80, 0x85, 0x7C, 0x79, 0x83, 0x85, 0x81, 0x80, 0x82, 0x7F,
		0x80, 0x81, 0x80, 0x81, 0x81, 0x7E, 0x7E, 0x81, 0x7F, 0x7F, 0x81, 0x80, 0x7E, 0x80, 0x80, 0x7E,
		0x82, 0x80, 0x7E, 0x81, 0x7F, 0x7E, 0x80, 0x7F, 0x7D, 0x7E, 0x7F, 0x7E, 0x83, 0x80, 0x81, 0x84,
		0x7D, 0x81, 0x86, 0x7D, 0x74, 0x75, 0x7A, 0x80, 0x84, 0x84, 0x84, 0x88, 0x8B, 0x88, 0x85, 0x7C,
		0x73, 0x6D, 0x6E, 0x7C, 0x82, 0x80, 0x88, 0x89, 0x8B, 0x97, 0x92, 0x81, 0x79, 0x71, 0x66, 0x6D,
		0x7C, 0x7D, 0x82, 0x87, 0x8C, 0x91, 0x92, 0x8E, 0x86, 0x77, 0x67, 0x64, 0x77, 0x7D, 0x7A, 0x85,
		0x8A, 0x8A, 0x94, 0x94, 0x85, 0x7D, 0x72, 0x61, 0x6E, 0x7C, 0x79, 0x7F, 0x89, 0x8B, 0x8F, 0x94,
		0x8E, 0x83, 0x78, 0x65, 0x66, 0x79, 0x7D, 0x7B, 0x85, 0x8C, 0x8C, 0x94, 0x94, 0x85, 0x7E, 0x6F,
		0x61, 0x72, 0x7E, 0x78, 0x81, 0x8B, 0x8A, 0x90, 0x97, 0x8B, 0x81, 0x7A, 0x65, 0x68, 0x7D, 0x7D,
		0x7A, 0x85, 0x8F, 0x8C, 0x92, 0x90, 0x83, 0x7F, 0x6C, 0x63, 0x79, 0x7E, 0x7A, 0x81, 0x8C, 0x8E,
		0x91, 0x93, 0x84, 0x7B, 0x6F, 0x62, 0x74, 0x83, 0x7A, 0x7D, 0x8C, 0x8E, 0x8F, 0x95, 0x89, 0x7B,
		0x70, 0x62, 0x6F, 0x82, 0x7C, 0x79, 0x8A, 0x8F, 0x8C, 0x94, 0x8D, 0x7C, 0x72, 0x62, 0x6A, 0x81,
		0x7E, 0x77, 0x87, 0x90, 0x8C, 0x92, 0x8F, 0x7F, 0x74, 0x65, 0x64, 0x7F, 0x82, 0x75, 0x83, 0x91,
		0x8D, 0x90, 0x93, 0x82, 0x73, 0x68, 0x65, 0x7B, 0x84, 0x77, 0x7F, 0x8F, 0x8D, 0x8E, 0x94, 0x84,
		0x73, 0x69, 0x66, 0x7A, 0x84, 0x7B, 0x7C, 0x8C, 0x90, 0x8C, 0x93, 0x8A, 0x76, 0x6A, 0x64, 0x76,
		0x84, 0x7C, 0x7B, 0x88, 0x8F, 0x8C, 0x92, 0x8E, 0x7A, 0x6D, 0x66, 0x72, 0x83, 0x81, 0x79, 0x84,
		0x8F, 0x8C, 0x90, 0x91, 0x7F, 0x70, 0x66, 0x6D, 0x80, 0x85, 0x7B, 0x7F, 0x8C, 0x8C, 0x8D, 0x94,
		0x85, 0x74, 0x69, 0x69, 0x7B, 0x85, 0x7E, 0x7D, 0x84, 0x89, 0x8B, 0x92, 0x8E, 0x7F, 0x76, 0x70,
		0x70, 0x78, 0x7B, 0x7B, 0x7F, 0x84, 0x87, 0x8B, 0x90, 0x8E, 0x87, 0x80, 0x74, 0x6B, 0x6A, 0x6F,
		0x79, 0x83, 0x87, 0x88, 0x8E, 0x95, 0x95, 0x8E, 0x80, 0x6F, 0x64, 0x64, 0x6B, 0x7A, 0x85, 0x88,
		0x8A, 0x91, 0x98, 0x98, 0x91, 0x80, 0x6C, 0x60, 0x5F, 0x6A, 0x7A, 0x86, 0x89, 0x8B, 0x91, 0x98,
		0x98, 0x93, 0x82, 0x6D, 0x60, 0x5F, 0x67, 0x78, 0x86, 0x89, 0x8A, 0x91, 0x96, 0x98, 0x94, 0x86,
		0x70, 0x62, 0x5E, 0x63, 0x74, 0x83, 0x8A, 0x8B, 0x8F, 0x96, 0x98, 0x96, 0x8B, 0x76, 0x65, 0x5D,
		0x61, 0x6F, 0x81, 0x89, 0x8B, 0x8F, 0x94, 0x97, 0x98, 0x90, 0x7C, 0x67, 0x5E, 0x5E, 0x68, 0x7C,
		0x88, 0x8B, 0x8E, 0x93, 0x96, 0x97, 0x93, 0x82, 0x6C, 0x5F, 0x5B, 0x63, 0x77, 0x87, 0x8B, 0x8E,
		0x92, 0x96, 0x98, 0x98, 0x88, 0x71, 0x5F, 0x5B, 0x60, 0x71, 0x84, 0x8B, 0x8C, 0x91, 0x95, 0x98,
		0x99, 0x8D, 0x76, 0x63, 0x5A, 0x5B, 0x6B, 0x80, 0x8B, 0x8C, 0x90, 0x95, 0x98, 0x9A, 0x93, 0x7D,
		0x67, 0x5B, 0x5A, 0x66, 0x7C, 0x8A, 0x8C, 0x8F, 0x94, 0x97, 0x9A, 0x96, 0x83, 0x6A, 0x5C, 0x58,
		0x60, 0x77, 0x88, 0x8B, 0x8E, 0x93, 0x97, 0x9A, 0x99, 0x8A, 0x70, 0x5E, 0x58, 0x5E, 0x71, 0x86,
		0x8C, 0x8C, 0x92, 0x97, 0x9B, 0x9C, 0x8E, 0x75, 0x5F, 0x58, 0x5C, 0x6C, 0x82, 0x8B, 0x8B, 0x90,
		0x96, 0x9B, 0x9C, 0x92, 0x7A, 0x62, 0x58, 0x5A, 0x68, 0x7E, 0x8B, 0x8B, 0x8E, 0x96, 0x9C, 0x9C,
		0x95, 0x7F, 0x65, 0x59, 0x5A, 0x65, 0x7B, 0x89, 0x8B, 0x8C, 0x95, 0x9B, 0x9D, 0x96, 0x82, 0x68,
		0x5A, 0x59, 0x62, 0x76, 0x87, 0x8B, 0x8D, 0x94, 0x9B, 0x9C, 0x97, 0x87, 0x6D, 0x5D, 0x59, 0x61,
		0x72, 0x85, 0x8B, 0x8C, 0x93, 0x9A, 0x9B, 0x97, 0x88, 0x71, 0x5D, 0x5B, 0x62, 0x70, 0x82, 0x8A,
		0x8B, 0x92, 0x99, 0x99, 0x94, 0x88, 0x74, 0x61, 0x5C, 0x62, 0x70, 0x81, 0x8C, 0x8D, 0x92, 0x99,
		0x98, 0x90, 0x87, 0x73, 0x5E, 0x5D, 0x6B, 0x7A, 0x84, 0x8A, 0x8F, 0x93, 0x96, 0x94, 0x89, 0x78,
		0x61, 0x59, 0x76, 0x84, 0x78, 0x7B, 0x8D, 0x98, 0x92, 0x8E, 0x8E, 0x81, 0x6B, 0x56, 0x6D, 0x8C,
		0x80, 0x72, 0x85, 0x97, 0x8D, 0x89, 0x92, 0x8B, 0x79, 0x65, 0x5A, 0x7D, 0x99, 0x78, 0x70, 0x8E,
		0x98, 0x87, 0x87, 0x93, 0x89, 0x72, 0x5F, 0x61, 0x8C, 0x8D, 0x6D, 0x78, 0x96, 0x92, 0x7E, 0x88,
		0x97, 0x8B, 0x6C, 0x5F, 0x5E, 0x8B, 0x8E, 0x61, 0x79, 0x9B, 0x96, 0x7F, 0x86, 0x96, 0x8A, 0x6F,
		0x61, 0x60, 0x8B, 0x99, 0x67, 0x7E, 0x9A, 0x8F, 0x83, 0x85, 0x93, 0x89, 0x70, 0x65, 0x5E, 0x7D,
		0x9C, 0x67, 0x6F, 0x9E, 0x8C, 0x85, 0x84, 0x8F, 0x98, 0x72, 0x66, 0x65, 0x70, 0x9E, 0x79, 0x61,
		0x98, 0x90, 0x85, 0x88, 0x7F, 0x96, 0x80, 0x62, 0x66, 0x61, 0x9B, 0x95, 0x57, 0x8F, 0x9F, 0x82,
		0x92, 0x80, 0x8F, 0x97, 0x67, 0x68, 0x5A, 0x7E, 0xA9, 0x57, 0x6B, 0xA8, 0x7D, 0x87, 0x87, 0x7B,
		0x9B, 0x72, 0x5E, 0x66, 0x6A, 0xB7, 0x82, 0x54, 0xAD, 0x98, 0x7F, 0x96, 0x7C, 0x95, 0x8F, 0x5D,
		0x64, 0x5A, 0x96, 0x9F, 0x40, 0x7F, 0xA9, 0x75, 0x88, 0x84, 0x82, 0x96, 0x6B, 0x5E, 0x5F, 0x83,
		0xBF, 0x67, 0x63, 0xBA, 0x8C, 0x82, 0x92, 0x7C, 0x96, 0x7F, 0x5A, 0x5F, 0x67, 0xAC, 0x87, 0x3B,
		0x9A, 0xA0, 0x6C, 0x91, 0x80, 0x83, 0x95, 0x62, 0x54, 0x67, 0x9F, 0xAF, 0x52, 0x7B, 0xBD, 0x7F,
		0x86, 0x8E, 0x7F, 0x95, 0x6E, 0x56, 0x5E, 0x81, 0xB6, 0x64, 0x4B, 0xB2, 0x8B, 0x70, 0x96, 0x7B,
		0x93, 0x87, 0x55, 0x5B, 0x76, 0xB5, 0x88, 0x43, 0xA2, 0xA8, 0x6F, 0x94, 0x7F, 0x83, 0x94, 0x58,
		0x54, 0x69, 0xAA, 0xA3, 0x44, 0x83, 0xB5, 0x74, 0x8D, 0x8C, 0x7E, 0xA1, 0x70, 0x54, 0x5C, 0x93,
		0xB6, 0x46, 0x61, 0xB7, 0x77, 0x7C, 0x90, 0x71, 0x95, 0x7D, 0x52, 0x5A, 0x80, 0xCF, 0x6F, 0x4D,
		0xC3, 0x93, 0x76, 0x9D, 0x7A, 0x92, 0x8F, 0x5A, 0x59, 0x62, 0xB7, 0x8B, 0x26, 0x9C, 0xA1, 0x63,
		0x90, 0x7E, 0x85, 0x96, 0x66, 0x5B, 0x5E, 0xA9, 0xBD, 0x3D, 0x81, 0xC2, 0x74, 0x86, 0x87, 0x82,
		0x97, 0x70, 0x5F, 0x55, 0x83, 0xC3, 0x53, 0x4C, 0xB9, 0x85, 0x71, 0x8B, 0x87, 0x93, 0x7F, 0x6A,
		0x57, 0x6A, 0xC1, 0x81, 0x3D, 0xA7, 0xA1, 0x6D, 0x84, 0x8E, 0x8B, 0x83, 0x79, 0x60, 0x54, 0xAB,
		0xAF, 0x3E, 0x7F, 0xB6, 0x75, 0x74, 0x92, 0x90, 0x7D, 0x7D, 0x71, 0x48, 0x7A, 0xC5, 0x55, 0x50,
		0xB4, 0x88, 0x66, 0x8D, 0xA5, 0x7A, 0x7F, 0x86, 0x58, 0x58, 0xC5, 0x99, 0x3A, 0xA4, 0xAB, 0x64,
		0x75, 0xA8, 0x82, 0x70, 0x8C, 0x6E, 0x40, 0x84, 0xC4, 0x42, 0x5C, 0xB9, 0x7E, 0x5C, 0x9E, 0xA4,
		0x6C, 0x87, 0x88, 0x55, 0x54, 0xCE, 0x95, 0x3A, 0xA8, 0xAD, 0x5B, 0x75, 0xB2, 0x7A, 0x70, 0x91,
		0x71, 0x34, 0x88, 0xC7, 0x3B, 0x5F, 0xBE, 0x77, 0x57, 0xA7, 0xA1, 0x67, 0x88, 0x94, 0x52, 0x4F,
		0xCE, 0x99, 0x3A, 0xA2, 0xB2, 0x57, 0x75, 0xB2, 0x7C, 0x6C, 0x93, 0x78, 0x3B, 0x75, 0xC7, 0x5E,
		0x4C, 0xB9, 0x8E, 0x52, 0x9A, 0xAC, 0x6C, 0x81, 0x98, 0x62, 0x47, 0xA7, 0xB3, 0x42, 0x79, 0xBC,
		0x67, 0x5F, 0xAC, 0x8A, 0x66, 0x8F, 0x84, 0x48, 0x5F, 0xBF, 0x86, 0x41, 0xA6, 0xA9, 0x56, 0x86,
		0xB4, 0x77, 0x79, 0x9C, 0x71, 0x44, 0x85, 0xBE, 0x59, 0x57, 0xB8, 0x7B, 0x51, 0xA0, 0x96, 0x63,
		0x8B, 0x8E, 0x54, 0x4F, 0xAA, 0xAF, 0x49, 0x86, 0xBE, 0x66, 0x71, 0xB5, 0x84, 0x72, 0x9B, 0x80,
		0x49, 0x5F, 0xBB, 0x81, 0x3A, 0xA0, 0x9C, 0x4C, 0x85, 0xA8, 0x6C, 0x79, 0x97, 0x6C, 0x47, 0x88,
		0xC8, 0x64, 0x62, 0xBF, 0x84, 0x5B, 0xA4, 0x9D, 0x6B, 0x8B, 0x91, 0x58, 0x46, 0xA4, 0xA3, 0x3F,
		0x7D, 0xAE, 0x62, 0x69, 0xAB, 0x89, 0x6C, 0x92, 0x88, 0x4A, 0x66, 0xC9, 0x86, 0x4E, 0xA6, 0xA0,
		0x5A, 0x81, 0xAC, 0x78, 0x73, 0x94, 0x71, 0x3A, 0x80, 0xBD, 0x5F, 0x5A, 0xAE, 0x89, 0x58, 0x97,
		0xAB, 0x6F, 0x80, 0x9A, 0x65, 0x49, 0x9A, 0xB4, 0x56, 0x65, 0xB1, 0x7A, 0x51, 0x9E, 0x9D, 0x64,
		0x81, 0x90, 0x5C, 0x4D, 0xA8, 0xB0, 0x4F, 0x7B, 0xBC, 0x74, 0x5F, 0xAC, 0x9C, 0x6B, 0x81, 0x8E,
		0x57, 0x4B, 0xAE, 0x9A, 0x40, 0x7D, 0xAD, 0x6D, 0x61, 0xA5, 0x9E, 0x6D, 0x7E, 0x8D, 0x59, 0x61,
		0xBC, 0x94, 0x51, 0x86, 0xAA, 0x70, 0x60, 0xA0, 0x9A, 0x6C, 0x76, 0x84, 0x52, 0x66, 0xB3, 0x8C,
		0x55, 0x85, 0xAD, 0x76, 0x6A, 0xA2, 0x9D, 0x73, 0x71, 0x7C, 0x5E, 0x6E, 0xAA, 0x83, 0x51, 0x85,
		0xA3, 0x75, 0x69, 0x98, 0xA4, 0x76, 0x6C, 0x7C, 0x63, 0x7F, 0xAD, 0x7F, 0x61, 0x87, 0x9E, 0x79,
		0x6C, 0x94, 0x9D, 0x76, 0x6A, 0x70, 0x64, 0x84, 0x9F, 0x76, 0x65, 0x8A, 0x9E, 0x7B, 0x73, 0x96,
		0x9B, 0x77, 0x69, 0x6F, 0x67, 0x8C, 0x9D, 0x77, 0x66, 0x87, 0x96, 0x7B, 0x77, 0x91, 0x98, 0x7A,
		0x6F, 0x6E, 0x6A, 0x89, 0x9C, 0x7A, 0x6E, 0x88, 0x92, 0x7F, 0x79, 0x8F, 0x91, 0x76, 0x71, 0x6C,
		0x63, 0x8B, 0x95, 0x78, 0x71, 0x86, 0x92, 0x81, 0x7E, 0x8F, 0x8E, 0x7A, 0x73, 0x6B, 0x6B, 0x8C,
		0x94, 0x7B, 0x71, 0x86, 0x8C, 0x81, 0x81, 0x8E, 0x8C, 0x79, 0x75, 0x67, 0x6B, 0x8F, 0x93, 0x7B,
		0x76, 0x86, 0x8B, 0x82, 0x82, 0x8E, 0x8A, 0x78, 0x70, 0x65, 0x6D, 0x8F, 0x92, 0x77, 0x75, 0x86,
		0x8D, 0x85, 0x82, 0x8C, 0x8C, 0x7E, 0x70, 0x5C, 0x6F, 0x9D, 0x97, 0x79, 0x6B, 0x80, 0x91, 0x8E,
		0x82, 0x83, 0x88, 0x85, 0x70, 0x55, 0x6A, 0x96, 0xA3, 0x7F, 0x6A, 0x73, 0x8E, 0x97, 0x8A, 0x7E,
		0x7D, 0x84, 0x7A, 0x5C, 0x62, 0x8F, 0x9F, 0x8D, 0x6E, 0x6D, 0x83, 0x99, 0x96, 0x83, 0x78, 0x7A,
		0x78, 0x67, 0x6A, 0x89, 0x9A, 0x8E, 0x77, 0x6B, 0x7C, 0x94, 0x99, 0x8D, 0x79, 0x74, 0x72, 0x69,
		0x70, 0x84, 0x95, 0x8E, 0x7D, 0x6F, 0x77, 0x8B, 0x97, 0x92, 0x80, 0x74, 0x72, 0x69, 0x6F, 0x81,
		0x8E, 0x91, 0x82, 0x77, 0x74, 0x82, 0x8F, 0x93, 0x89, 0x7C, 0x75, 0x6B, 0x6E, 0x7B, 0x87, 0x8D,
		0x89, 0x81, 0x79, 0x7A, 0x83, 0x8C, 0x8D, 0x84, 0x7B, 0x72, 0x70, 0x76, 0x7F, 0x85, 0x89, 0x88,
		0x82, 0x7C, 0x7B, 0x80, 0x85, 0x85, 0x81, 0x7D, 0x7C, 0x7F, 0x7F, 0x7C, 0x7C, 0x7D, 0x7F, 0x80,
		0x80, 0x80, 0x81, 0x83, 0x83, 0x83, 0x84, 0x85, 0x83, 0x7E, 0x7B, 0x79, 0x78, 0x77, 0x7A, 0x80,
		0x86, 0x86, 0x84, 0x83, 0x83, 0x84, 0x82, 0x81, 0x80, 0x7F, 0x7D, 0x7A, 0x77, 0x79, 0x7D, 0x81,
		0x82, 0x84, 0x84, 0x83, 0x83, 0x83, 0x83, 0x81, 0x81, 0x80, 0x7E, 0x7D, 0x7C, 0x7A, 0x7A, 0x7D,
		0x80, 0x83, 0x83, 0x82, 0x82, 0x82, 0x82, 0x81, 0x80, 0x7F, 0x7F, 0x80, 0x80, 0x80, 0x80, 0x7F,
		0x7E, 0x7C, 0x7B, 0x7C, 0x80, 0x82, 0x83, 0x82, 0x82, 0x82, 0x81, 0x80, 0x80, 0x81, 0x81, 0x80,
		0x7E, 0x7C, 0x79, 0x79, 0x7C, 0x80, 0x83, 0x83, 0x82, 0x82, 0x82, 0x82, 0x81, 0x7F, 0x80, 0x80,
		0x81, 0x81, 0x81, 0x7F, 0x7E, 0x7E, 0x7E, 0x7D, 0x7D, 0x7E, 0x7E, 0x80, 0x82, 0x83, 0x83, 0x83,
		0x82, 0x80, 0x7E, 0x7D, 0x7C, 0x7D, 0x7E, 0x7F, 0x80, 0x81, 0x81, 0x81, 0x81, 0x81, 0x80, 0x80,
		0x80, 0x80, 0x81, 0x81, 0x81, 0x80, 0x7F, 0x7D, 0x7D, 0x7C, 0x7B, 0x7C, 0x81, 0x84, 0x85, 0x84,
		0x82, 0x81, 0x80, 0x7F, 0x7F, 0x7F, 0x80, 0x80, 0x80, 0x7F, 0x7F, 0x7F, 0x81, 0x82, 0x82, 0x81,
		0x80, 0x7E, 0x7D, 0x7D, 0x7E, 0x80, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x80, 0x80, 0x7F,
		0x7E, 0x7E, 0x7E, 0x81, 0x83, 0x82, 0x82, 0x83, 0x82, 0x80, 0x7C, 0x7C, 0x7B, 0x7B, 0x7E, 0x82,
		0x83, 0x82, 0x80, 0x80, 0x82, 0x86, 0x87, 0x7F, 0x78, 0x73, 0x74, 0x81, 0x87, 0x83, 0x7D, 0x7C,
		0x81, 0x85, 0x84, 0x82, 0x86, 0x86, 0x7E, 0x74, 0x6A, 0x75, 0x8D, 0x8F, 0x7D, 0x73, 0x7A, 0x8A,
		0x8C, 0x83, 0x7C, 0x87, 0x89, 0x7D, 0x6E, 0x64, 0x7F, 0x9B, 0x8B, 0x6A, 0x6C, 0x89, 0x9C, 0x83,
		0x6D, 0x80, 0x9B, 0x8D, 0x6C, 0x6A, 0x70, 0x83, 0x96, 0x7F, 0x67, 0x7A, 0x91, 0x90, 0x76, 0x70,
		0x8E, 0x9C, 0x7F, 0x64, 0x7B, 0x81, 0x6F, 0x84, 0x8C, 0x77, 0x76, 0x87, 0x8D, 0x79, 0x75, 0x8A,
		0x91, 0x82, 0x71, 0x7F, 0x7F, 0x63, 0x7E, 0x9C, 0x75, 0x68, 0x8D, 0x93, 0x75, 0x76, 0x8C, 0x8A,
		0x80, 0x84, 0x82, 0x7C, 0x6D, 0x73, 0x9B, 0x88, 0x63, 0x87, 0x9A, 0x75, 0x75, 0x91, 0x84, 0x73,
		0x8B, 0x91, 0x74, 0x78, 0x7C, 0x7A, 0x92, 0x80, 0x71, 0x8B, 0x89, 0x76, 0x80, 0x89, 0x79, 0x79,
		0x8F, 0x83, 0x71, 0x88, 0x7F, 0x62, 0x8A, 0x92, 0x5C, 0x7A, 0x9C, 0x6C, 0x6D, 0x98, 0x80, 0x6A,
		0x8E, 0x8F, 0x70, 0x85, 0x91, 0x76, 0x7C, 0x8A, 0x7F, 0x73, 0x76, 0x90, 0x7C, 0x64, 0x92, 0x85,
		0x5F, 0x8D, 0x8D, 0x63, 0x86, 0x8D, 0x6A, 0x84, 0x8C, 0x70, 0x7F, 0x85, 0x6B, 0x80, 0x90, 0x6B,
		0x7B, 0x90, 0x71, 0x7A, 0x8F, 0x78, 0x79, 0x89, 0x7B, 0x7C, 0x88, 0x7E, 0x7D, 0x83, 0x7E, 0x78,
		0x7D, 0x8B, 0x7A, 0x7B, 0x90, 0x79, 0x7D, 0x91, 0x79, 0x7D, 0x91, 0x78, 0x7E, 0x8F, 0x78, 0x82,
		0x8E, 0x78, 0x80, 0x88, 0x6F, 0x81, 0x96, 0x6F, 0x7D, 0x97, 0x74, 0x7D, 0x96, 0x78, 0x7C, 0x8F,
		0x7B, 0x7C, 0x8C, 0x7E, 0x7C, 0x87, 0x7E, 0x76, 0x7E, 0x8A, 0x78, 0x73, 0x8E, 0x7A, 0x73, 0x8F,
		0x7D, 0x72, 0x8B, 0x7D, 0x76, 0x88, 0x7C, 0x7E, 0x89, 0x7B, 0x7F, 0x86, 0x6F, 0x7E, 0x93, 0x6B,
		0x7B, 0x95, 0x6D, 0x7E, 0x96, 0x70, 0x7C, 0x91, 0x73, 0x7C, 0x8C, 0x76, 0x7E, 0x8B, 0x78, 0x7E,
		0x88, 0x7A, 0x80, 0x85, 0x7A, 0x80, 0x82, 0x7C, 0x82, 0x80, 0x7E, 0x83, 0x80, 0x80, 0x83, 0x7F,
		0x7F, 0x83, 0x7E, 0x81, 0x85, 0x7E, 0x80, 0x86, 0x79, 0x78, 0x91, 0x7D, 0x74, 0x96, 0x7D, 0x75,
		0x96, 0x7E, 0x75, 0x93, 0x7F, 0x75, 0x8F, 0x80, 0x77, 0x8B, 0x81, 0x79, 0x87, 0x81, 0x7C, 0x86,
		0x7F, 0x7E, 0x85, 0x7E, 0x7F, 0x83, 0x7D, 0x7F, 0x83, 0x7E, 0x80, 0x81, 0x7F, 0x81, 0x7F, 0x7F,
		0x83, 0x7C, 0x7D, 0x83, 0x7A, 0x7E, 0x86, 0x7C, 0x7C, 0x86, 0x7D, 0x7C, 0x86, 0x7E, 0x7C, 0x84,
		0x7D, 0x7E, 0x82, 0x7D, 0x7F, 0x81, 0x7E, 0x80, 0x81, 0x7E, 0x7F, 0x81, 0x7F, 0x7F,
};
#endif

static void snd_voice(void)
{
#if ((IS_DOS && defined(__WATCOMC__)) || IS_WIN)
	if (!opt_sound)
	{
		return;
	}
#if IS_DOS
	{
		int i, n = (int) sizeof(ark_pcm);
		unsigned int t0, t1, el, wait = (unsigned int) (1193180UL / SND_RATE);
		const unsigned char *p = ark_pcm;
		unsigned char pv;

		snd_off();              /* Ningun pitido previo puede mezclarse con la voz   */
		outportb(0x43, 0xB6);   /* Timer 2 en modo 3 a frecuencia maxima (divisor 2) */
		outportb(0x42, 2);      /* La portadora no se oye: el altavoz sigue el bit 1 */
		outportb(0x42, 0);
		pv = (unsigned char) (inportb(0x61) | 0x03);  /* Se cachea el puerto         */
		outportb(0x61, pv);
		for (i = 0; i < n; i++)
		{
			if (p[i] >= 128)
			{
				pv |= 0x02;
			}
			else
			{
				pv &= 0xFD;
			}
			outportb(0x61, pv);  /* Un inportb menos por muestra                 */
			outportb(0x43, 0x00);       /* Ritmo exacto de muestra con el timer 0     */
			t0 = inportb(0x40);
			t0 |= (unsigned int) inportb(0x40) << 8;
			do
			{
				outportb(0x43, 0x00);
				t1 = inportb(0x40);
				t1 |= (unsigned int) inportb(0x40) << 8;
				el = (unsigned int) (t0 - t1);
			} while (el < wait);
		}
	}
#else
	{
		/* Cabecera RIFF de 44 bytes compuesta aqui: el PCM va pelado al ejecutable, */
		/* y el buffer es BSS, asi que tampoco engorda el fichero. Los bytes altos   */
		/* de los campos quedan a 0: rate y tamanos caben en 16 bits.                */
		static unsigned char wav[44 + sizeof(ark_pcm)];
		unsigned long dsz = (unsigned long) sizeof(ark_pcm);
		HMODULE mm;
		BOOL (WINAPI *pplay) (const char *, HMODULE, unsigned long);

		memcpy(wav, "RIFF", 4);
		wav[4] = (unsigned char) (dsz + 36);                          /* Tamano RIFF */
		wav[5] = (unsigned char) ((dsz + 36) >> 8);
		wav[6] = (unsigned char) ((dsz + 36) >> 16);
		wav[7] = (unsigned char) ((dsz + 36) >> 24);
		memcpy(wav + 8, "WAVEfmt ", 8);
		wav[16] = 16;                                       /* Tamano del bloque fmt */
		wav[20] = 1;                                                  /* Formato PCM */
		wav[22] = 1;                                                         /* Mono */
		wav[24] = (unsigned char) SND_RATE;
		wav[25] = (unsigned char) (SND_RATE >> 8);
		wav[28] = wav[24];                        /* Byte rate = rate (mono, 8 bits) */
		wav[29] = wav[25];
		wav[32] = 1;                                                  /* Block align */
		wav[34] = 8;                                             /* Bits por muestra */
		memcpy(wav + 36, "data", 4);
		wav[40] = (unsigned char) dsz;
		wav[41] = (unsigned char) (dsz >> 8);
		wav[42] = (unsigned char) (dsz >> 16);
		wav[43] = (unsigned char) (dsz >> 24);
		memcpy(wav + 44, ark_pcm, sizeof(ark_pcm));
		mm = LoadLibraryA("winmm.dll");
		if (mm != NULL)
		{
			pplay = (BOOL (WINAPI *) (const char *, HMODULE, unsigned long)) GetProcAddress(mm, "PlaySoundA");
			if (pplay != NULL)
			{
				/* SND_MEMORY|SND_ASYNC|SND_NODEFAULT: la voz suena en segundo   */
				/* plano mientras la portada sigue animada. winmm no se libera:  */
				/* el buffer es estatico y PlaySound sigue leyendolo.            */
				pplay((const char *) wav, NULL, 0x04 | 0x01 | 0x02);
			}
		}
	}
#endif
	snd_off();                  /* Garantiza el silencio al terminar la voz          */
#endif
}

/* --------------------------------------------------------------------------------- */
/* Arpegio ascendente (victoria) o descendente (derrota). Bloquea ~0.5 s: solo se    */
/* usa en pausas dramaticas (vida perdida, nivel superado).                          */
/* --------------------------------------------------------------------------------- */
static void jingle(int up)
{
	int i;

	if (!opt_sound)
	{
		return;
	}
	for (i = 0; i < 6; i++)
	{
		snd_play(up ? (300 + i * 120) : (900 - i * 130), 1);
		delay(70);
	}
	snd_off();
}

/* ================================================================================= */
/*  RECORDS PERSISTENTES (ARKANOID.DAT, top 3)                                        */
/* ================================================================================= */
static void records_default(void)
{
	strcpy(hi[0].name, "VIKING");
	hi[0].score = 5000L;
	strcpy(hi[1].name, "NIKKHO");
	hi[1].score = 3000L;
	strcpy(hi[2].name, "ROOKIE");
	hi[2].score = 1000L;
}

/* --------------------------------------------------------------------------------- */
static void records_load(void)
{
	FILE *f;
	int i;
	long sc;

	records_default();
	f = fopen(HI_FILE, "r");
	if (f == NULL)
	{
		return;
	}
	for (i = 0; i < 3; i++)
	{
		if (fscanf(f, "%15s %ld", hi[i].name, &sc) != 2)
		{
			break;
		}
		if (sc < 0)
		{
			sc = 0;
		}
		hi[i].score = sc;
	}
	fclose(f);
}

/* --------------------------------------------------------------------------------- */
static void records_save(void)
{
	FILE *f;
	int i;

	f = fopen(HI_FILE, "w");
	if (f == NULL)
	{
		return;
	}
	for (i = 0; i < 3; i++)
	{
		fprintf(f, "%s %ld\n", hi[i].name, hi[i].score);
	}
	fclose(f);
}

/* --------------------------------------------------------------------------------- */
static void records_insert(const char *name, long sc)
{
	int i, j;

	for (i = 0; i < 3; i++)
	{
		if (sc > hi[i].score)
		{
			for (j = 2; j > i; j--)
			{
				hi[j] = hi[j - 1];
			}
			strncpy(hi[i].name, name, 15);
			hi[i].name[15] = 0;
			hi[i].score = sc;
			break;
		}
	}
}

/* --------------------------------------------------------------------------------- */
static void records_show(void)
{
	int i, x1, y1;

	screen_bg(0x1F);
	textattr(0x1E);
	gotoxy(1, 1);
	cprintf(" " APP_NAME " v" APP_VERSION " ");
	clreol();
	x1 = center_x(34);
	y1 = center_y(9);
	draw_box(x1, y1, x1 + 33, y1 + 8, 0x1E, "HIGH SCORES");
	for (i = 0; i < 3; i++)
	{
		textattr(0x1F);
		cprintfxy(x1 + 2, y1 + 2 + i, "%d. %-13s", i + 1, hi[i].name);
		textattr(0x1E);
		cprintfxy(x1 + 24, y1 + 2 + i, "%8ld", hi[i].score);
	}
	textattr(0x17);
	gotoxy(1, scrh);
	cprintf(" PRESS ANY KEY ");
	clreol();
	read_key();
}

/* --------------------------------------------------------------------------------- */
/* Codigo secreto con getpass(): la entrada se oculta con asteriscos, maximo 8 chars. */
/* --------------------------------------------------------------------------------- */
static void secret_code(void)
{
	char *p;
	int x1, y1;

	screen_bg(0x1F);
	textattr(0x1E);
	gotoxy(1, 1);
	cprintf(" " APP_NAME " ");
	clreol();
	x1 = center_x(42);
	y1 = center_y(7);
	draw_box(x1, y1, x1 + 41, y1 + 6, 0x1E, "SECRET CODE");
	textattr(0x1F);
	cprintfxy(x1 + 2, y1 + 2, "A SECRET WORD FROM THE AUTHOR (8 MAX)");
	cprintfxy(x1 + 2, y1 + 4, "CODE: ");
	_setcursortype(_NORMALCURSOR);
	gotoxy(x1 + 8, y1 + 4);
	p = getpass("");
	_setcursortype(_NOCURSOR);
	if (str_ieq(p, CHEAT_CODE))
	{
		ghost = !ghost;
		textattr(0x1A);
		cprintfxy(x1 + 2, y1 + 5, ghost ? "GHOST MODE ON - BALL BOUNCES BELOW!  " : "GHOST MODE OFF. GOOD LUCK OUT THERE!  ");
		jingle(1);
	}
	else
	{
		textattr(0x4F);
		cprintfxy(x1 + 2, y1 + 5, "WRONG CODE! (HINT: THE AUTHOR'S NICK)  ");
		jingle(0);
	}
	delay(900);
}

/* ================================================================================= */
/*  PORTADA ANIMADA                                                                   */
/*                                                                                    */
/*  Titulo con fuente bitmap 5x5 dibujada celda a celda (putch + textattr), ciclando   */
/*  colores; bola rebotando; marquesina desplazada con movetext(); texto parpadeante   */
/*  con el atributo BLINK. Cualquier tecla salta al menu.                              */
/* ================================================================================= */

static const unsigned char font_ark[7][5] = {
	{ 0x0E, 0x11, 0x1F, 0x11, 0x11 },       /* A */
	{ 0x1E, 0x11, 0x1E, 0x12, 0x11 },       /* R */
	{ 0x11, 0x12, 0x1C, 0x12, 0x11 },       /* K */
	{ 0x11, 0x19, 0x15, 0x13, 0x11 },       /* N */
	{ 0x0E, 0x11, 0x11, 0x11, 0x0E },       /* O */
	{ 0x1F, 0x04, 0x04, 0x04, 0x1F },       /* I */
	{ 0x1E, 0x11, 0x11, 0x11, 0x1E }        /* D */
};

/* bit 0x10 = pixel mas a la izquierda de cada fila */
static int font_map(int ch)
{
	switch (ch)
	{
		case 'A': return 0;
		case 'R': return 1;
		case 'K': return 2;
		case 'N': return 3;
		case 'O': return 4;
		case 'I': return 5;
		case 'D': return 6;
	}
	return -1;
}

/* --------------------------------------------------------------------------------- */
static void title_draw(int tx, int ty, int bgc)
{
	const char *s = "ARKANOID";
	int i, row, px, li;

	textattr((bgc << 4) | bgc);
	for (i = 0; s[i]; i++)
	{
		li = font_map(s[i]);
		if (li < 0)
		{
			continue;
		}
		for (row = 0; row < 5; row++)
		{
			for (px = 0; px < 5; px++)
			{
				if (font_ark[li][row] & (0x10 >> px))
				{
					putchxy(tx + i * 6 + px, ty + row, ' ');
				}
			}
		}
	}
}

/* --------------------------------------------------------------------------------- */
static void intro_screen(void)
{
	static const int cyc[6] = { GREEN, CYAN, RED, MAGENTA, BROWN, LIGHTGRAY };
	static const char *marq =
		"     PUBLIC DOMAIN  *  100% CONIO.H COMPATIBLE  *  DIRECT VIDEO FOR DOS16 AND DOS32  *"
		"  CONSOLE API FOR WIN32  *  GETTEXT PUTTEXT MOVETEXT WINDOW INSLINE DELLINE UNGETCH CGETS"
		" GETPASS TEXTMODE AND MORE  *  ";
	int frame = 0, bgc, off = 0;
	int tx, ty, zx1, zx2, zy1, zy2;
	int bx, by, bdx = 1, bdy = 1;

	read_screen();
	full_window();
	screen_bg(0x1F);

	tx = center_x(47);
	ty = (scrh - 20) / 2 + 2;
	if (ty < 2)
	{
		ty = 2;
	}
	zx1 = 2;
	zx2 = scrw - 1;
	zy1 = ty + 11;
	zy2 = scrh - 4;
	bx = zx1 + 3;
	by = zy1 + 1;

	while (!kbhit())
	{
		bgc = cyc[frame % 6];

		/* Bola rebotando: primero se borra la posicion anterior.                 */
		textattr(0x1F);
		putchxy(bx, by, ' ');
		bx += bdx;
		by += bdy;
		if (bx <= zx1 || bx >= zx2)
		{
			bdx = -bdx;
			if (bx <= zx1) bx = zx1;
			if (bx >= zx2) bx = zx2;
		}
		if (by <= zy1 || by >= zy2)
		{
			bdy = -bdy;
			if (by <= zy1) by = zy1;
			if (by >= zy2) by = zy2;
		}
		putchxy(bx, by, 'O');

		/* Titulo ciclando colores + cabeceras fijas.                             */
		title_draw(tx, ty, bgc);
		textattr(0x1E);
		cprintfxy(center_x(14), ty + 7, "CONIOX EDITION");
		textattr(0x1B);
		cprintfxy(center_x(37), ty + 9, "A SHOWCASE FOR CONIOX v%d.%02d (%.11s)",
			  coniox_version / 100, coniox_version % 100, coniox_build);
		textattr(0x17);
		cprintfxy(2, 2, "GAME BUILD %.11s %.8s", __DATE__, __TIME__);

		/* Marquesina: movetext desplaza la fila 1 celda a la izquierda (solapa   */
		/* origen y destino: la libreria lo resuelve copiando al reves).          */
		textattr(0x1B);
		if (scrw > 6)
		{
			movetext(3, scrh, scrw - 2, scrh, 2, scrh);
			putchxy(scrw - 2, scrh, marq[off]);
			off++;
			if (marq[off] == 0)
			{
				off = 0;
			}
		}

		/* "PRESS ANY KEY" parpadeando con el bit BLINK (en Win32 realza fondo).  */
		textattr((frame % 2) ? (0x1E | BLINK) : 0x1E);
		cprintfxy(center_x(14), scrh - 2, "PRESS ANY KEY");
		if (frame == 0)
		{
			snd_voice();        /* Voz de arranque: suena con la portada ya dibujada */
		}

		delay(FRAME_MS * 2);
		frame++;
	}
	read_key();             /* consume la tecla (incluidas las extendidas)          */
}

/* ================================================================================= */
/*  MENU PRINCIPAL                                                                    */
/* ================================================================================= */

#define MENU_N 8

static const char *menu_items[MENU_N] = {
	"PLAY GAME",
	"CONIOX TOUR",
	"HIGH SCORES",
	"SETTINGS",
	"HELP",
	"ABOUT",
	"SECRET CODE",
	"QUIT"
};

/* --------------------------------------------------------------------------------- */
static void menu_bars(void)
{
	textattr(0x1E);
	gotoxy(1, 1);
	cprintf(" " APP_NAME " v" APP_VERSION " - MAIN MENU ");
	clreol();
	textattr(0x17);
	gotoxy(1, scrh);
	cprintf(" UP/DOWN + ENTER - ESC QUITS ");
	clreol();
}

/* --------------------------------------------------------------------------------- */
static void menu_draw(int sel)
{
	int i, bx1, by1;

	bx1 = center_x(26);
	by1 = center_y(MENU_N + 6);
	draw_box(bx1, by1, bx1 + 25, by1 + MENU_N + 3, 0x1E, "MENU");
	for (i = 0; i < MENU_N; i++)
	{
		textattr((i == sel) ? 0x30 : 0x1F);
		cprintfxy(bx1 + 2, by1 + 2 + i, " %-20s ", menu_items[i]);
	}
}

/* --------------------------------------------------------------------------------- */
/* Navegacion sin parpadeo: el fondo, las barras y el cuadro se pintan una sola      */
/* vez (al entrar, al volver de un submenu o si cambia el tamano de consola) y       */
/* cada flecha repinta solo las dos opciones que cambian de estado.                  */
/* --------------------------------------------------------------------------------- */
static int menu_loop(void)
{
	int sel = 0, c, exit_app = 0, done = 0, drawn = 0, prev = -1, w = 0, h = 0;
	int bx1, by1;

	while (!done)
	{
		read_screen();
		if (scrw < 64 || scrh < 20)
		{
			drawn = 0;
			screen_bg(0x4F);
			cprintfxy(2, 2, "CONSOLE TOO SMALL: %d x %d", scrw, scrh);
			cprintfxy(2, 3, "ARKANOID CONIOX NEEDS AT LEAST 64 x 20.");
			cprintfxy(2, 5, "RESIZE THE WINDOW (WIN32) AND PRESS A KEY.");
			read_key();
			continue;
		}
		if (!drawn || scrw != w || scrh != h)
		{
			w = scrw;
			h = scrh;
			screen_bg(0x1F);
			menu_bars();
			menu_draw(sel);
			drawn = 1;
		}
		bx1 = center_x(26);     /* Misma geometria que menu_draw()                   */
		by1 = center_y(MENU_N + 6);
		c = read_key();
		if (c == KEY_SPECIAL + KEY_UP)
		{
			prev = sel;
			sel = (sel + MENU_N - 1) % MENU_N;
		}
		else if (c == KEY_SPECIAL + KEY_DOWN)
		{
			prev = sel;
			sel = (sel + 1) % MENU_N;
		}
		else if (c == KEY_SPECIAL + KEY_HOME)
		{
			prev = sel;
			sel = 0;
		}
		else if (c == KEY_SPECIAL + KEY_END)
		{
			prev = sel;
			sel = MENU_N - 1;
		}
		else
		{
			prev = -1;
		}
		if (prev >= 0)
		{
			if (prev != sel)  /* Repintado quirurgico: solo la opcion apagada    */
			{
				textattr(0x1F);
				cprintfxy(bx1 + 2, by1 + 2 + prev, " %-20s ", menu_items[prev]);
			}
			textattr(0x30);
			cprintfxy(bx1 + 2, by1 + 2 + sel, " %-20s ", menu_items[sel]);
			snd_menu();
		}
		else if (c == '\r')
		{
			snd_select();
			switch (sel)
			{
				case 0: game_run();      break;
				case 1: demo_tour();     break;
				case 2: records_show();  break;
				case 3: settings_screen(); break;
				case 4: help_screen();   break;
				case 5: about_screen();  break;
				case 6: secret_code();   break;
				default:
					if (dialog_yn("QUIT", "REALLY QUIT ARKANOID?"))
					{
						exit_app = 1;
						done = 1;
					}
					break;
			}
			snd_off();
			drawn = 0;      /* Los submenus ensucian la pantalla: repintar       */
		}
		else if (c == KEY_ESC)
		{
			if (dialog_yn("QUIT", "REALLY QUIT ARKANOID?"))
			{
				exit_app = 1;
				done = 1;
			}
		}
	}
	return exit_app;
}

/* ================================================================================= */
/*  AJUSTES                                                                           */
/* ================================================================================= */

#define SET_N_MAX 5

/* --------------------------------------------------------------------------------- */
/* Dibuja los ajustes. row = -1 pinta el cuadro completo; row >= 0 repinta solo      */
/* esa opcion: es lo que evita el parpadeo al navegar o al cambiar un valor.         */
/* --------------------------------------------------------------------------------- */
static void settings_draw(int sel, int n, int row)
{
	static const char *labels[SET_N_MAX] = {
		"BALL SPEED", "SOUND", "TEXT MODE",
#if IS_DOS
		"DIRECT VIDEO", "BACK"
#else
		"BACK"                      /* DIRECT VIDEO solo existe en DOS */
#endif
	};
	int i, bx1, by1;

	bx1 = center_x(34);
	by1 = center_y(SET_N_MAX + 6);
	if (row < 0)
	{
		draw_box(bx1, by1, bx1 + 33, by1 + n + 3, 0x1E, "SETTINGS");
	}
	for (i = 0; i < n; i++)
	{
		if (row >= 0 && i != row)
		{
			continue;
		}
		textattr((i == sel) ? 0x30 : 0x1F);
		cprintfxy(bx1 + 2, by1 + 2 + i, " %-12s ", labels[i]);
		textattr((i == sel) ? 0x3E : 0x1E);
		switch (i)
		{
			case 0:
				cprintfxy(bx1 + 17, by1 + 2 + i, "< %d >   ", opt_speed);
				break;
			case 1:
				cprintfxy(bx1 + 17, by1 + 2 + i, "< %s >  ", opt_sound ? "ON" : "OFF");
				break;
			case 2:
				cprintfxy(bx1 + 17, by1 + 2 + i, "< %s >  ", opt_mode ? "80X50" : "80X25");
				break;
#if IS_DOS
			case 3:
				cprintfxy(bx1 + 17, by1 + 2 + i, "< %s >  ", directvideo ? "VRAM" : "BIOS");
				break;
#endif
			default:
				cprintfxy(bx1 + 17, by1 + 2 + i, "        ");
				break;
		}
	}
	if (row < 0)
	{
		textattr(0x17);
		gotoxy(1, scrh);
		cprintf(" LEFT/RIGHT CHANGES - ESC BACKS ");
		clreol();
	}
}

/* --------------------------------------------------------------------------------- */
/* Misma tecnica antiparpadeo que el menu: pantalla completa una vez y repintado     */
/* por fila despues. textmode() obliga a repintar todo al cambiar de modo.           */
/* --------------------------------------------------------------------------------- */
static void settings_screen(void)
{
	int sel = 0, c, n, done = 0, drawn = 0, prev, w = 0, h = 0;

#if IS_DOS
	n = 5;
#else
	n = 4;                      /* DIRECT VIDEO no existe en Win32                   */
#endif
	while (!done)
	{
		read_screen();
		if (!drawn || scrw != w || scrh != h)
		{
			w = scrw;
			h = scrh;
			screen_bg(0x1F);
			textattr(0x1E);
			gotoxy(1, 1);
			cprintf(" " APP_NAME " ");
			clreol();
			settings_draw(sel, n, -1);
			drawn = 1;
		}
		c = read_key();
		if (c == KEY_ESC)
		{
			done = 1;
		}
		else if (c == '\r' || c == KEY_SPECIAL + KEY_RIGHT || c == KEY_SPECIAL + KEY_LEFT)
		{
			int back = (c == KEY_SPECIAL + KEY_LEFT) ? -1 : 1;

			switch (sel)
			{
				case 0:
					opt_speed += back;
					if (opt_speed < 1) opt_speed = 9;
					if (opt_speed > 9) opt_speed = 1;
					settings_draw(sel, n, sel);
					snd_menu();
					break;
				case 1:
					opt_sound = !opt_sound;
					settings_draw(sel, n, sel);
					snd_select();
					break;
				case 2:
					opt_mode = !opt_mode;
					textmode(opt_mode ? C4350 : C80);
					read_screen();
					if (scrw < 64 || scrh < 20)
					{
						/* La consola no permite el modo: marcha atras.  */
						opt_mode = !opt_mode;
						textmode(opt_mode ? C4350 : C80);
						read_screen();
					}
					snd_select();
					drawn = 0;  /* textmode borra la pantalla: repintar  */
					break;
#if IS_DOS
				case 3:
					directvideo = directvideo ? 0 : 1;
					settings_draw(sel, n, sel);
					snd_select();
					break;
#endif
				default:
					done = 1;
					break;
			}
		}
		else if (c == KEY_SPECIAL + KEY_UP)
		{
			prev = sel;
			sel = (sel + n - 1) % n;
			settings_draw(sel, n, prev);
			settings_draw(sel, n, sel);
			snd_menu();
		}
		else if (c == KEY_SPECIAL + KEY_DOWN)
		{
			prev = sel;
			sel = (sel + 1) % n;
			settings_draw(sel, n, prev);
			settings_draw(sel, n, sel);
			snd_menu();
		}
	}
}

/* ================================================================================= */
/*  AYUDA Y ACERCA DE                                                                 */
/* ================================================================================= */
static void help_screen(void)
{
	static const char *lines[] = {
		"CONTROLS",
		" ARROWS ....... MOVE PADDLE (HOLD TO SLIDE)",
		" SPACE ........ LAUNCH THE BALL",
		" P ............ PAUSE",
		" F1 ........... THIS HELP (ALSO IN GAME)",
		" ESC .......... BACK TO MENU / QUIT GAME",
		"SCORING",
		" PLAIN BRICK .......... 10 POINTS",
		" CRACKING A HARD ONE ... 5 POINTS",
		" LEVEL CLEARED ...... 100 + 50 x LEVEL",
		"TIPS",
		" EDGE HITS BOUNCE THE BALL STEEPER.",
		" SPEED RISES EVERY LEVEL YOU CLEAR.",
		" SOLID BRICKS NEVER BREAK. GHOST MODE MAKES",
		" THE BALL BOUNCE AT THE BOTTOM (SECRET CODE)."
	};
	int i, n, x1, y1, hdr;

	n = (int) (sizeof(lines) / sizeof(lines[0]));
	read_screen();
	screen_bg(0x1F);
	textattr(0x1E);
	gotoxy(1, 1);
	cprintf(" " APP_NAME " ");
	clreol();

	x1 = center_x(52);
	y1 = center_y(n + 4);
	draw_box(x1, y1, x1 + 51, y1 + n + 3, 0x1E, "HELP");

	/* La ayuda se imprime dentro de una window() activa: lineas relativas y       */
	/* recorte automatico. highvideo()/lowvideo() resaltan secciones y notas.      */
	window(x1 + 1, y1 + 1, x1 + 50, y1 + n + 2);
	for (i = 0; i < n; i++)
	{
		hdr = (lines[i][0] != ' ');
		textattr(0x1F);
		if (hdr)
		{
			highvideo();
		}
		gotoxy(2, 2 + i);
		cputs(lines[i]);
		if (hdr)
		{
			normvideo();
		}
	}
	textattr(0x1F);
	lowvideo();
	gotoxy(2, n + 2);
	cputs("WINDOW + HIGH/LOWVIDEO + NORMVIDEO IN ACTION.");
	normvideo();
	full_window();

	textattr(0x17);
	gotoxy(1, scrh);
	cprintf(" PRESS ANY KEY ");
	clreol();
	snd_off();                  /* La ayuda tambien se abre en pleno juego (F1)     */
	read_key();
}

/* --------------------------------------------------------------------------------- */
static void about_screen(void)
{
	int x1, y1;

	read_screen();
	screen_bg(0x1F);
	textattr(0x1E);
	gotoxy(1, 1);
	cprintf(" " APP_NAME " ");
	clreol();

	x1 = center_x(56);
	y1 = center_y(14);
	draw_box(x1, y1, x1 + 55, y1 + 13, 0x1E, "ABOUT");

	textattr(0x1F);
	cprintfxy(x1 + 2, y1 + 2,  APP_NAME " v" APP_VERSION " - PUBLIC DOMAIN DEMO GAME");
	textattr(0x1B);
	cprintfxy(x1 + 2, y1 + 3,  "LIBRARY ...... CONIOX v%d.%02d BUILD %.11s",
		  coniox_version / 100, coniox_version % 100, coniox_build);
	cprintfxy(x1 + 2, y1 + 4,  "AUTHOR ....... JAVIER GUTIERREZ CHAMORRO");
	cprintfxy(x1 + 2, y1 + 5,  "                 NIKKHOKKHO.SOURCEFORGE.IO");
	cprintfxy(x1 + 2, y1 + 6,  "GAME BUILD ... %.11s %.8s", __DATE__, __TIME__);
#if defined(__WATCOMC__)
	cprintfxy(x1 + 2, y1 + 7,  "COMPILER ..... OPENWATCOM C v%d.%02d",
		  __WATCOMC__ / 100, __WATCOMC__ % 100);
#else
	cprintfxy(x1 + 2, y1 + 7,  "COMPILER ..... OTHER ANSI C COMPILER");
#endif
#if IS_DOS
	#if defined(__FLAT__)
	cprintfxy(x1 + 2, y1 + 8,  "TARGET ....... DOS32 FLAT MODEL (EXTENDER)");
	#else
	cprintfxy(x1 + 2, y1 + 8,  "TARGET ....... DOS16 REAL MODE");
	#endif
	cprintfxy(x1 + 2, y1 + 9,  "DIRECT VIDEO . %d (1 = VRAM, 0 = BIOS INT 10H)", directvideo);
#else
	cprintfxy(x1 + 2, y1 + 8,  "TARGET ....... WIN32 CONSOLE (WRITECONSOLEOUTPUT)");
	cprintfxy(x1 + 2, y1 + 9,  "CONSOLE ...... %d x %d CELLS", scrw, scrh);
#endif
	cprintfxy(x1 + 2, y1 + 10, "VIDEO MODE ... %d   NORMATTR ... 0x%02X",
		  tinfo.currmode, tinfo.normattr);
	textattr(0x1F);
	cprintfxy(x1 + 2, y1 + 11, "EVERY CONIOX FUNCTION IS EXERCISED: PLAY THE GAME,");
	cprintfxy(x1 + 2, y1 + 12, "AND DO NOT MISS THE GUIDED CONIOX TOUR!");
	textattr(0x17);
	gotoxy(1, scrh);
	cprintf(" PRESS ANY KEY ");
	clreol();
	read_key();
}

/* ================================================================================= */
/*  EL JUEGO                                                                          */
/*                                                                                    */
/*  Coordenadas: todo el campo se maneja en coordenadas "interiores" (1..fwx, 1..fwy)  */
/*  y se convierten a pantalla con ABX()/ABY(). La pala ocupa la ultima fila interior  */
/*  (paddle_row == fwy). La bola se mueve con pasos unitarios horizontales (|dx|) y    */
/*  uno vertical (dy) por tick, comprobando colision celda a celda (sin tunelado).     */
/* ================================================================================= */

static int paddle_row;

/* --------------------------------------------------------------------------------- */
static void game_geometry(void)
{
	fld_x1 = 1;
	fld_y1 = 2;
	fld_x2 = scrw - PANELW;
	fld_y2 = scrh - 1;
	fwx = fld_x2 - fld_x1 - 1;
	fwy = fld_y2 - fld_y1 - 1;
	if (fwx > GW)
	{
		fwx = GW;
		fld_x2 = fld_x1 + fwx + 1;      /* el borde abraza el area jugable      */
	}
	if (fwy > GH)
	{
		fwy = GH;
		fld_y2 = fld_y1 + fwy + 1;
	}
	paddle_row = fwy;
}

/* --------------------------------------------------------------------------------- */
/* Niveles generados por formula: 1 = normal, 2 = dos impactos ('#'), 3 = solido.      */
/* --------------------------------------------------------------------------------- */
static void init_level(void)
{
	int r, c, rows, pat;

	rows = 4 + (level - 1);
	if (rows > MAXROWS)
	{
		rows = MAXROWS;
	}
	pat = ((level - 1) % 5) + 1;
	bricks_left = 0;
	for (r = 1; r <= GH; r++)
	{
		for (c = 1; c <= GW; c++)
		{
			brick[r][c] = 0;
		}
	}
	for (r = 0; r < rows; r++)
	{
		for (c = 2; c <= fwx - 1; c++)
		{
			int t = 1;

			switch (pat)
			{
				case 1:
					t = 1;
					break;
				case 2:
					t = ((r % 2) == 0 && (c % 3) == 0) ? 2 : 1;
					break;
				case 3:
					t = (r < 2) ? 2 : ((((c / 2) % 4) == 0) ? 0 : 1);
					break;
				case 4:
					t = ((c % 5) == 0) ? 3 : (((r + c) % 3) == 0 ? 2 : 1);
					break;
				default:
					t = ((r == 0) && ((c % 6) < 2)) ? 3 : (((c + r) % 2) ? 2 : 1);
					break;
			}
			brick[2 + r][c] = (unsigned char) t;
			if (t == 1 || t == 2)
			{
				bricks_left++;
			}
		}
	}
}

/* --------------------------------------------------------------------------------- */
static void brick_draw(int ix, int iy)
{
	int v = brick[iy][ix];
	int rc = rowcolor[(iy - 2) % MAXROWS];

	if (v == 0)
	{
		textattr(0x07);
		putchxy(ABX(ix), ABY(iy), ' ');
	}
	else if (v == 1)
	{
		textattr((rc << 4) | rc);
		putchxy(ABX(ix), ABY(iy), ' ');
	}
	else if (v == 2)
	{
		textattr((rc << 4) | BLACK);
		putchxy(ABX(ix), ABY(iy), '#');
	}
	else
	{
		textattr((DARKGRAY << 4) | LIGHTGRAY);
		putchxy(ABX(ix), ABY(iy), CH_FULL);
	}
}

/* --------------------------------------------------------------------------------- */
static void bricks_draw_all(void)
{
	int ix, iy;

	for (iy = 2; iy <= MAXROWS + 1; iy++)
	{
		for (ix = 2; ix <= fwx - 1; ix++)
		{
			brick_draw(ix, iy);
		}
	}
}

/* --------------------------------------------------------------------------------- */
static void paddle_draw(void)
{
	int i, bg = ghost ? MAGENTA : CYAN;

	textattr((bg << 4) | bg);
	gotoxy(ABX(paddle_x), ABY(paddle_row));         /* Un gotoxy: putch ya avanza */
	for (i = 0; i < PW; i++)
	{
		putch(' ');
	}
}

/* --------------------------------------------------------------------------------- */
static void paddle_erase(void)
{
	int i;

	textattr(0x07);
	gotoxy(ABX(paddle_x), ABY(paddle_row));
	for (i = 0; i < PW; i++)
	{
		putch(' ');
	}
}

/* --------------------------------------------------------------------------------- */
static void ball_draw(void)
{
	textattr(ghost ? 0x0D : 0x0F);
	putchxy(ABX(ball_x), ABY(ball_y), 'O');
}

/* --------------------------------------------------------------------------------- */
/* Al borrar la bola, si esta pisando la fila de la pala (modo fantasma) repinta la   */
/* celda de la pala para no dejar agujeros.                                           */
/* --------------------------------------------------------------------------------- */
static void ball_erase(void)
{
	textattr(0x07);
	putchxy(ABX(ball_x), ABY(ball_y), ' ');
	if (ball_y == paddle_row)
	{
		paddle_draw();
	}
}

/* --------------------------------------------------------------------------------- */
/* Panel lateral: se dibuja dentro de una window() activa, con clreol() recortando    */
/* al borde de la ventana.                                                            */
/* --------------------------------------------------------------------------------- */
static void panel_draw(void)
{
	int x1 = scrw - PANELW + 2;
	int h = scrh - 4;

	full_window();
	window(x1, 3, x1 + PANELW - 3, 3 + h - 1);
	textattr(0x0B);
	gotoxy(1, 1);
	cprintf("SCORE");
	clreol();
	gotoxy(1, 2);
	textattr(0x0E);
	cprintf("%6ld", score);
	textattr(0x0B);
	if (h >= 5)
	{
		gotoxy(1, 4);
		cprintf("LIVES");
		clreol();
		gotoxy(1, 5);
		textattr(0x0E);
		for (x1 = 0; x1 < lives; x1++)
		{
			cprintf("==");
		}
		textattr(0x0B);
		clreol();
	}
	if (h >= 8)
	{
		gotoxy(1, 7);
		cprintf("LEVEL");
		clreol();
		gotoxy(1, 8);
		textattr(0x0E);
		cprintf("%6d", level);
		textattr(0x0B);
	}
	if (h >= 11)
	{
		gotoxy(1, 10);
		cprintf("RECORD");
		clreol();
		gotoxy(1, 11);
		textattr(0x0E);
		cprintf("%6ld", hi[0].score);
		textattr(0x0B);
	}
	if (h >= 14)
	{
		gotoxy(1, 13);
		cprintf("SPEED");
		clreol();
		gotoxy(1, 14);
		textattr(0x0E);
		cprintf("%6d", opt_speed);
		textattr(0x0B);
	}
	if (h >= 17)
	{
		gotoxy(1, 16);
		cprintf("SOUND");
		clreol();
		gotoxy(1, 17);
		textattr(0x0E);
		cprintf("%6s", opt_sound ? "ON" : "OFF");
		textattr(0x0B);
	}
	if (h >= 20)
	{
		gotoxy(1, 19);
		cprintf("GHOST");
		clreol();
		gotoxy(1, 20);
		textattr(0x0E);
		cprintf("%6s", ghost ? "ON" : "OFF");
		textattr(0x0B);
	}
	full_window();
}

/* --------------------------------------------------------------------------------- */
static void panel_score(void)
{
	int x1 = scrw - PANELW + 2;

	window(x1, 3, x1 + PANELW - 3, scrh - 2);
	gotoxy(1, 2);
	textattr(0x0E);
	cprintf("%6ld", score);
	textattr(0x0B);
	full_window();
}

/* --------------------------------------------------------------------------------- */
static void chrome_draw(void)
{
	textattr(0x1E);
	gotoxy(1, 1);
	cprintf(" " APP_NAME " v" APP_VERSION " ");
	clreol();
	textattr(0x1A);
	cprintfxy(scrw - 9, 1, "F1=HELP");
	textattr(0x1B);
	gotoxy(1, scrh);
	cprintf(" ESC=MENU  P=PAUSE  SPACE=LAUNCH  ARROWS=MOVE  HOME/END=JUMP ");
	clreol();
	draw_box(fld_x1, fld_y1, fld_x2, fld_y2, 0x0B, NULL);
	draw_box(fld_x2 + 1, 2, scrw, scrh - 1, 0x0F, "STATUS");
}

/* --------------------------------------------------------------------------------- */
static void game_redraw(void)
{
	full_window();
	screen_bg(0x07);
	chrome_draw();
	bricks_draw_all();
	paddle_draw();
	ball_draw();
	panel_draw();
}

/* --------------------------------------------------------------------------------- */
static void game_start(void)
{
	game_geometry();
	init_level();
	paddle_x = fwx / 2 - PW / 2 + 1;
	ball_stuck = 1;
	ball_dx = 1;
	ball_dy = -1;
	ball_x = paddle_x + PW / 2;
	ball_y = paddle_row - 1;
	ball_timer = 1;
	clear_pending = 0;
	game_redraw();
}

/* --------------------------------------------------------------------------------- */
static void move_paddle(int dx)
{
	int nx = paddle_x + dx;

	if (nx < 1)
	{
		nx = 1;
	}
	if (nx > fwx - PW + 1)
	{
		nx = fwx - PW + 1;
	}
	if (nx == paddle_x)
	{
		return;
	}
	paddle_erase();
	paddle_x = nx;
	paddle_draw();
	if (ball_stuck)
	{
		ball_erase();
		ball_x = paddle_x + PW / 2;
		ball_draw();
	}
}

/* --------------------------------------------------------------------------------- */
/* Angulo de salida segun el punto de impacto: centro = suave, bordes = agudo (dx 2).  */
/* --------------------------------------------------------------------------------- */
static int paddle_dx_for(int hitx)
{
	int t = hitx - (paddle_x + PW / 2);
	int s = (t < 0) ? -1 : 1;
	int m = (t < 0) ? -t : t;

	if (m == 0)
	{
		return (ball_dx != 0) ? ball_dx : 1;
	}
	if (m >= 3)
	{
		return s * 2;
	}
	return s;
}

/* --------------------------------------------------------------------------------- */
static void hit_brick(int ix, int iy)
{
	int v = brick[iy][ix];

	if (v == 3)
	{
		snd_solid();
		return;
	}
	if (v == 2)
	{
		brick[iy][ix] = 1;
		score += 5L;
		snd_crack();
	}
	else
	{
		brick[iy][ix] = 0;
		score += 10L;
		bricks_left--;
		snd_break();
	}
	brick_draw(ix, iy);
	panel_score();
	if (bricks_left <= 0)
	{
		clear_pending = 1;
	}
}

/* --------------------------------------------------------------------------------- */
/* Un paso unitario de bola. Devuelve 1 = sigue, 0 = perdida, 2 = nivel superado.      */
/* --------------------------------------------------------------------------------- */
static int move_one(int mdx, int mdy)
{
	int nx, ny;

	nx = ball_x + mdx;
	ny = ball_y + mdy;

	if (mdx != 0)
	{
		if (nx < 1)
		{
			ball_dx = -ball_dx;
			nx = 1;
			snd_wall();
		}
		else if (nx > fwx)
		{
			ball_dx = -ball_dx;
			nx = fwx;
			snd_wall();
		}
		else if (brick[ball_y][nx] != 0)
		{
			hit_brick(nx, ball_y);
			ball_dx = -ball_dx;
			nx = ball_x;
			if (clear_pending)
			{
				return 2;
			}
		}
		ball_x = nx;
		return 1;
	}

	/* Movimiento vertical                                                           */
	if (ny < 1)
	{
		ball_dy = 1;
		ny = 1;
		snd_wall();
	}
	else if (ny > fwy)
	{
		if (ghost)
		{
			ball_dy = -1;
			ny = fwy;
			snd_wall();
		}
		else
		{
			ball_x = nx;
			ball_y = ny;
			return 0;
		}
	}
	else if (ny == paddle_row && ball_dy > 0 && nx >= paddle_x && nx < paddle_x + PW)
	{
		ball_dy = -1;
		ball_dx = paddle_dx_for(nx);
		snd_paddle();
		return 1;                       /* la bola no entra en la fila de la pala */
	}
	else if (brick[ny][nx] != 0)
	{
		hit_brick(nx, ny);
		ball_dy = -ball_dy;
		ny = ball_y;               /* La bola no entra en el ladrillo                  */
		if (clear_pending)
		{
			return 2;
		}
	}
	ball_y = ny;
	return 1;
}

/* --------------------------------------------------------------------------------- */
static int ball_step(void)
{
	int i, n, r = 1;

	ball_erase();
	n = (ball_dx < 0) ? -ball_dx : ball_dx;
	for (i = 0; i < n && r == 1; i++)
	{
		r = move_one((ball_dx < 0) ? -1 : 1, 0);
	}
	if (r == 1)
	{
		r = move_one(0, ball_dy);
	}
	ball_draw();
	return r;
}

/* --------------------------------------------------------------------------------- */
/* Fotogramas entre pasos de bola: DOS tiene ticks de 55 ms, Win32 frames de 20 ms.    */
/* La velocidad efectiva sube con el nivel.                                            */
/* --------------------------------------------------------------------------------- */
static int ball_interval(void)
{
	int spd = opt_speed + (level - 1) / 2;
	int f;

	if (spd > 9)
	{
		spd = 9;
	}
#if IS_DOS
	f = (12 - spd) / 2;
	if (f < 1)
	{
		f = 1;
	}
#else
	f = 12 - spd;
	if (f < 2)
	{
		f = 2;
	}
#endif
	return f;
}

/* --------------------------------------------------------------------------------- */
static void center_msg(const char *s, int attr)
{
	int ix = (fwx - (int) strlen(s)) / 2 + 1;
	int iy = paddle_row - 5;
	int i;

	if (ix < 1)
	{
		ix = 1;
	}
	textattr(attr);
	cputsxy(ABX(ix), ABY(iy), s);
	delay(700);
	textattr(0x07);
	for (i = 0; s[i]; i++)
	{
		putchxy(ABX(ix + i), ABY(iy), ' ');
	}
}

/* --------------------------------------------------------------------------------- */
/* Pausa con gettext/puttext: el campo queda intacto al reanudar.                      */
/* --------------------------------------------------------------------------------- */
static void pause_overlay(void)
{
	int x1, y1, x2, y2;

	x1 = center_x(26);
	x2 = x1 + 25;
	y1 = center_y(5);
	y2 = y1 + 4;
	if (!gettext(x1, y1, x2, y2, dlgbuf))
	{
		return;
	}
	draw_box(x1, y1, x2, y2, 0x1F, "PAUSE");
	textattr(0x1B);
	cprintfxy(x1 + 2, y1 + 2, "PAUSED - PRESS ANY KEY");
	snd_off();                  /* Ningun pitido activo debe sonar en la pausa      */
	read_key();
	puttext(x1, y1, x2, y2, dlgbuf);
}

/* --------------------------------------------------------------------------------- */
static int lose_ball(void)
{
	ball_erase();
	/* La bola muere sobre la linea de borde: reparala.                              */
	textattr(0x0B);
	putchxy(ABX(ball_x), fld_y2, B_HZ);
	jingle(0);
	lives--;
	panel_draw();
	if (lives <= 0)
	{
		game_over();
		return 0;
	}
	center_msg("BALL LOST!", 0x0C);
	ball_stuck = 1;
	ball_dx = 1;
	ball_dy = -1;
	ball_x = paddle_x + PW / 2;
	ball_y = paddle_row - 1;
	ball_draw();
	return 1;
}

/* --------------------------------------------------------------------------------- */
/* Game over con alta de record: nombre pedido con cgets() y cursor visible.           */
/* --------------------------------------------------------------------------------- */
static void game_over(void)
{
	char namebuf[16];
	char *pname;
	int x1, y1, x2, y2, i;

	x1 = center_x(40);
	x2 = x1 + 39;
	y1 = center_y(11);
	y2 = y1 + 10;
	if (!gettext(x1, y1, x2, y2, dlgbuf))
	{
		return;
	}
	draw_box(x1, y1, x2, y2, 0x4E, "GAME OVER");
	textattr(0x4F);
	cprintfxy(x1 + 3, y1 + 2, "FINAL SCORE: %8ld", score);
	cprintfxy(x1 + 3, y1 + 3, "LEVEL REACHED: %6d", level);
	if (score > hi[2].score)
	{
		textattr(0x4E | BLINK);
		cprintfxy(x1 + 3, y1 + 5, "NEW HIGH SCORE!");
		textattr(0x4E);
		cprintfxy(x1 + 3, y1 + 6, "ENTER YOUR NAME (12 MAX):");
		cprintfxy(x1 + 3, y1 + 7, "[            ]");
		_setcursortype(_NORMALCURSOR);
		namebuf[0] = 13;                /* cgets: maximo 12 caracteres            */
		gotoxy(x1 + 4, y1 + 7);
		pname = cgets(namebuf);
		_setcursortype(_NOCURSOR);
		for (i = 0; pname[i]; i++)
		{
			pname[i] = (char) toupper((unsigned char) pname[i]);
		}
		if (!pname[0])
		{
			strcpy(pname, "PLAYER");
		}
		records_insert(pname, score);
		records_save();
	}
	else
	{
		textattr(0x4E);
		cprintfxy(x1 + 3, y1 + 5, "BEST: %s %8ld", hi[0].name, hi[0].score);
	}
	textattr(0x4B);
	cprintfxy(x1 + 3, y1 + 9, "PRESS ANY KEY");
	read_key();
	puttext(x1, y1, x2, y2, dlgbuf);
}

/* --------------------------------------------------------------------------------- */
static void level_clear(void)
{
	long bonus = 100L + 50L * (long) level;
	int x1, y1, x2, y2;

	score += bonus;
	panel_draw();
	jingle(1);
	x1 = center_x(30);
	x2 = x1 + 29;
	y1 = center_y(7);
	y2 = y1 + 6;
	if (gettext(x1, y1, x2, y2, dlgbuf))
	{
		draw_box(x1, y1, x2, y2, 0x1A, "WELL DONE");
		textattr(0x1A);
		cprintfxy(x1 + 3, y1 + 2, "LEVEL %d CLEARED!", level);
		textattr(0x1E);
		cprintfxy(x1 + 3, y1 + 3, "BONUS: %ld", bonus);
		textattr(0x1B);
		cprintfxy(x1 + 3, y1 + 5, "GET READY...");
		delay(1400);
		puttext(x1, y1, x2, y2, dlgbuf);
	}
	level++;
	init_level();
	paddle_x = fwx / 2 - PW / 2 + 1;
	ball_stuck = 1;
	ball_dx = 1;
	ball_dy = -1;
	ball_x = paddle_x + PW / 2;
	ball_y = paddle_row - 1;
	ball_timer = 1;
	clear_pending = 0;
	game_redraw();
}

/* --------------------------------------------------------------------------------- */
static void game_run(void)
{
	int c, running = 1, r, oldw, oldh;

	level = 1;
	score = 0;
	lives = LIVES0;
	game_start();
	while (running)
	{
		while (kbhit())
		{
			c = read_key();
			if (c == KEY_SPECIAL + KEY_LEFT)
			{
				move_paddle(-2);
			}
			else if (c == KEY_SPECIAL + KEY_RIGHT)
			{
				move_paddle(2);
			}
			else if (c == KEY_SPECIAL + KEY_HOME)
			{
				move_paddle(-fwx);
			}
			else if (c == KEY_SPECIAL + KEY_END)
			{
				move_paddle(fwx);
			}
			else if (c == ' ')
			{
				if (ball_stuck)
				{
					ball_stuck = 0;
					ball_dy = -1;
					ball_dx = (rand() % 2) ? -1 : 1;
					snd_paddle();
				}
			}
			else if (c == 'p' || c == 'P')
			{
				pause_overlay();
			}
			else if (c == KEY_SPECIAL + KEY_F1)
			{
				/* help_screen() refresca scrw/scrh: guarda el tamano previo    */
				/* para detectar un redimension de consola ocurrido dentro.     */
				oldw = scrw;
				oldh = scrh;
				help_screen();
				read_screen();
				if (scrw != oldw || scrh != oldh)
				{
					running = 0;            /* Geometria vieja: vuelta al menu       */
				}
				else
				{
					game_redraw();
				}
			}
			else if (c == KEY_ESC)
			{
				if (dialog_yn("QUIT GAME", "ABANDON THIS GAME?"))
				{
					running = 0;
				}
			}
		}
		if (!running)
		{
			break;
		}
		oldw = scrw;
		oldh = scrh;
		read_screen();
		if (scrw != oldw || scrh != oldh)
		{
			break;                  /* redimension: vuelta al menu             */
		}
		snd_tick();
		if (!ball_stuck && --ball_timer <= 0)
		{
			ball_timer = ball_interval();
			r = ball_step();
			if (r == 0)
			{
				if (!lose_ball())
				{
					running = 0;
				}
			}
			else if (r == 2)
			{
				level_clear();
			}
		}
		delay(FRAME_MS);
	}
	snd_off();
}

/* ================================================================================= */
/*  CONIOX TOUR: demostracion guiada de la libreria, paso a paso.                     */
/* ================================================================================= */

/* --------------------------------------------------------------------------------- */
static int demo_header(int n, const char *title)
{
	read_screen();
	screen_bg(0x1F);
	textattr(0x1E);
	gotoxy(1, 1);
	cprintf(" CONIOX TOUR - STEP %d/13 ", n);
	clreol();
	cprintfxy(center_x((int) strlen(title)), 3, title);
	return 1;
}

/* --------------------------------------------------------------------------------- */
static int demo_wait(void)
{
	int c;

	textattr(0x9E);
	cprintfxy(center_x(31), scrh - 1, "PRESS ANY KEY (ESC = END TOUR)");
	c = read_key();
	return (c == KEY_ESC) ? 0 : 1;
}

/* --------------------------------------------------------------------------------- */
static int demo_s01(void)
{
	demo_header(1, "LIBRARY INFO (GETTEXTINFO)");
	read_screen();
	textattr(0x1F);
	cprintfxy(6, 6,  "CONIOX VERSION .... v%d.%02d", coniox_version / 100, coniox_version % 100);
	cprintfxy(6, 7,  "LIBRARY BUILD ..... %.11s", coniox_build);
	cprintfxy(6, 8,  "GAME BUILD ........ %.11s %.8s", __DATE__, __TIME__);
#if defined(__WATCOMC__)
	cprintfxy(6, 9,  "COMPILER .......... OPENWATCOM C v%d.%02d", __WATCOMC__ / 100, __WATCOMC__ % 100);
#else
	cprintfxy(6, 9,  "COMPILER .......... OTHER ANSI C COMPILER");
#endif
#if IS_DOS
	#if defined(__FLAT__)
	cprintfxy(6, 10, "TARGET ............ DOS32 FLAT MODEL");
	#else
	cprintfxy(6, 10, "TARGET ............ DOS16 REAL MODE");
	#endif
#else
	cprintfxy(6, 10, "TARGET ............ WIN32 CONSOLE");
#endif
	cprintfxy(6, 12, "SCREENWIDTH ....... %d", tinfo.screenwidth);
	cprintfxy(6, 13, "SCREENHEIGHT ...... %d", tinfo.screenheight);
	cprintfxy(6, 14, "CURRMODE .......... %d", tinfo.currmode);
	cprintfxy(6, 15, "NORMATTR .......... 0x%02X", tinfo.normattr);
	cprintfxy(6, 16, "CURSOR ............ %d,%d", tinfo.curx, tinfo.cury);
#if IS_DOS
	cprintfxy(6, 17, "DIRECTVIDEO ....... %d  (1 = VRAM, 0 = BIOS)", directvideo);
#endif
	return demo_wait();
}

/* --------------------------------------------------------------------------------- */
static int demo_s02(void)
{
	int bg, fg, x0;

	demo_header(2, "COLOR PALETTE (TEXTCOLOR + TEXTBACKGROUND)");
	if (scrw < 66)
	{
		textattr(0x1F);
		cprintfxy(4, 8, "NOT ENOUGH COLUMNS TO SHOW THE FULL PALETTE.");
		return demo_wait();
	}
	x0 = (scrw - 62) / 2;
	for (bg = 0; bg < 8; bg++)
	{
		for (fg = 0; fg < 16; fg++)
		{
			textattr((bg << 4) | fg);
			cprintfxy(x0 + fg * 4, 6 + bg * 2, "%X%X", fg, fg);
		}
	}
	textattr(0x1F);
	cprintfxy(x0, 22, "16 FOREGROUNDS x 8 BACKGROUNDS VIA TEXTATTR().");
	textbackground(RED);
	textcolor(WHITE | BLINK);
	cprintfxy(x0, 23, "TEXTCOLOR + TEXTBACKGROUND + BLINK (DOS BLINKS IT)");
	textattr(0x1F);
	return demo_wait();
}

/* --------------------------------------------------------------------------------- */
static int demo_s03(void)
{
	demo_header(3, "LOWVIDEO / NORMVIDEO / HIGHVIDEO");
	textattr(0x0F);
	lowvideo();
	cprintfxy(10, 8, "LOWVIDEO() CLEARS THE INTENSITY BIT");
	textattr(0x07);
	normvideo();
	cprintfxy(10, 10, "NORMVIDEO() RESTORES THE STARTUP ATTRIBUTE");
	textattr(0x07);
	highvideo();
	cprintfxy(10, 12, "HIGHVIDEO() SETS THE INTENSITY BIT");
	normvideo();
	textattr(0x1F);
	return demo_wait();
}

/* --------------------------------------------------------------------------------- */
static int demo_s04(void)
{
	demo_header(4, "GOTOXY / WHEREX / WHEREY");
	textattr(0x1F);
	gotoxy(12, 8);
	cprintf("I AM AT X=%d Y=%d", wherex(), wherey());
	gotoxy(40, 12);
	cprintf("NOW AT X=%d Y=%d", wherex(), wherey());
	cprintfxy(12, 15, "COORDINATES ARE RELATIVE TO THE ACTIVE WINDOW.");
	textattr(0x1F);
	return demo_wait();
}

/* --------------------------------------------------------------------------------- */
static int demo_s05(void)
{
	static const char *w = "PUTCH";
	int i, c;

	demo_header(5, "PUTCH / CPUTS / CPRINTF / GETCHE");
	textattr(0x1F);
	for (i = 0; w[i]; i++)
	{
		putchxy(10 + i * 2, 6, w[i]);
	}
	cputsxy(10, 8, "CPUTS WRITES STRINGS WITHOUT NEWLINE.");
	cprintfxy(10, 10, "CPRINTF FORMATS: %d %s %c 0x%02X [%5d]", 42, "STRING", 'C', 0xAB, 7);
	cprintfxy(10, 13, "GETCHE ECHOES - PRESS ONE KEY: ");
	c = getche();
	if (c == 0)
	{
		c = getch();
		textattr(0x1B);
		cprintf("  (EXTENDED KEY, SCAN CODE %d)", c);
	}
	else
	{
		textattr(0x1B);
		cprintf("  (ASCII CODE %d)", c);
	}
	textattr(0x1F);
	return demo_wait();
}

/* --------------------------------------------------------------------------------- */
static int demo_s06(void)
{
	demo_header(6, "WINDOW() + CLREOL + SCROLLING");
	draw_box(9, 6, 41, 16, 0x1B, "ACTIVE WINDOW");
	window(10, 7, 40, 15);
	textattr(0x1F);
	gotoxy(1, 1);
	cputs("ALL OUTPUT IS CLIPPED AND WRAPPED INSIDE THE WINDOW. THIS TEXT IS LONG ENOUGH TO "
	      "OVERFLOW THE 31 COLUMNS AND TO SCROLL UP INSIDE THE BOX. ");
	gotoxy(1, 8);
	textattr(0x1E);
	cprintf("CLREOL");
	clreol();
	textattr(0x1F);
	window(1, 1, scrw, scrh);
	return demo_wait();
}

/* --------------------------------------------------------------------------------- */
static int demo_s07(void)
{
	int i, j, c2;

	demo_header(7, "INSLINE() / DELLINE()");
	for (i = 0; i < 6; i++)
	{
		c2 = rowcolor[i % MAXROWS];
		textattr((c2 << 4) | c2);
		gotoxy(2, 7 + i);
		cprintf(" LINE %d ", i + 1);
		for (j = 10; j <= scrw - 2; j++)
		{
			putch(' ');
		}
	}
	textattr(0x07);
	gotoxy(2, 9);
	insline();
	textattr(0x1F);
	cprintfxy(2, 15, "INSLINE() OPENED A GAP AT LINE 9.");
	if (!demo_wait())
	{
		return 0;
	}
	demo_header(7, "INSLINE() / DELLINE()");
	for (i = 0; i < 6; i++)
	{
		c2 = rowcolor[i % MAXROWS];
		textattr((c2 << 4) | c2);
		gotoxy(2, 7 + i);
		cprintf(" LINE %d ", i + 1);
		for (j = 10; j <= scrw - 2; j++)
		{
			putch(' ');
		}
	}
	textattr(0x07);
	gotoxy(2, 8);
	delline();
	delline();
	textattr(0x1F);
	cprintfxy(2, 15, "DELLINE() REMOVED LINES 8 AND 9.");
	return demo_wait();
}

/* --------------------------------------------------------------------------------- */
static int demo_s08(void)
{
	static unsigned short patbuf[96];
	int x, y;

	demo_header(8, "GETTEXT / PUTTEXT / MOVETEXT");
	for (y = 0; y < 6; y++)
	{
		for (x = 0; x < 16; x++)
		{
			textattr(((x + y) % 2) ? 0x1B : 0x4E);
			putchxy(12 + x, 7 + y, ' ');
		}
	}
	textattr(0x1F);
	cprintfxy(12, 14, "ORIGINAL PATTERN.");
	gettext(12, 7, 27, 12, patbuf);
	for (y = 0; y < 6; y++)
	{
		for (x = 0; x < 16; x++)
		{
			textattr(0x4F);
			putchxy(12 + x, 7 + y, 'X');
		}
	}
	cprintfxy(12, 14, "SCRIBBLED OVER IT...       ");
	if (!demo_wait())
	{
		return 0;
	}
	demo_header(8, "GETTEXT / PUTTEXT / MOVETEXT");
	puttext(12, 7, 27, 12, patbuf);
	textattr(0x1F);
	cprintfxy(12, 14, "PUTTEXT() RESTORED IT!     ");
	if (!demo_wait())
	{
		return 0;
	}
	demo_header(8, "GETTEXT / PUTTEXT / MOVETEXT");
	movetext(12, 7, 27, 12, 40, 9);
	textattr(0x1F);
	cprintfxy(12, 14, "MOVETEXT() CLONED IT.      ");
	return demo_wait();
}

/* --------------------------------------------------------------------------------- */
static int demo_s09(void)
{
	int r;
	demo_header(9, "_SETCURSORTYPE()");
	textattr(0x1F);
	putchxy(14, 8, 'X');
	gotoxy(16, 8);
	_setcursortype(_NORMALCURSOR);
	cprintfxy(8, 11, "_NORMALCURSOR: THIN UNDERSCORE (PARKED BY THE X)");
	if (!demo_wait())
	{
		_setcursortype(_NOCURSOR);
		return 0;
	}
	demo_header(9, "_SETCURSORTYPE()");
	textattr(0x1F);
	putchxy(14, 8, 'X');
	gotoxy(16, 8);
	_setcursortype(_SOLIDCURSOR);
	cprintfxy(8, 11, "_SOLIDCURSOR: FULL BLOCK CURSOR                ");
	if (!demo_wait())
	{
		_setcursortype(_NOCURSOR);
		return 0;
	}
	demo_header(9, "_SETCURSORTYPE()");
	textattr(0x1F);
	putchxy(14, 8, 'X');
	gotoxy(16, 8);
	_setcursortype(_NOCURSOR);
	cprintfxy(8, 11, "_NOCURSOR: HIDDEN (THE GAME USES THIS)         ");
	r = demo_wait();
	_setcursortype(_NOCURSOR);
	return r;
}

/* --------------------------------------------------------------------------------- */
static int demo_s10(void)
{
	int c, c2, n = 0;

	demo_header(10, "KBHIT() / UNGETCH()");
	textattr(0x1F);
	cprintfxy(8, 8, "POLLING KBHIT() WITHOUT BLOCKING - PRESS A PRINTABLE KEY");
	while (!kbhit())
	{
		n++;
		cprintfxy(8, 10, "%6d POLLS SO FAR", n);
		delay(FRAME_MS);
	}
	do
	{
		c = read_key();
	} while ((c & KEY_SPECIAL) || c < 32 || c > 126);
	cprintfxy(8, 10, "KEY '%c' DETECTED AFTER %d POLLS.", c, n);
	ungetch(c);
	cprintfxy(8, 12, "UNGETCH('%c') PUSHED IT BACK INTO THE QUEUE...", c);
	c2 = getch();
	cprintfxy(8, 14, "GETCH() READ IT AGAIN: '%c' - QUEUE LOOP WORKS.", c2);
	return demo_wait();
}

/* --------------------------------------------------------------------------------- */
static int demo_s11(void)
{
	static char namebuf[16];
	char *pname;
	int num = 0;
	char *p;

	demo_header(11, "CGETS / CSCANF / GETPASS");
	_setcursortype(_NORMALCURSOR);
	textattr(0x1F);
	cprintfxy(6, 7, "CGETS  - YOUR NAME (12 MAX): ");
	gotoxy(36, 7);
	namebuf[0] = 13;
	pname = cgets(namebuf);
	cprintfxy(6, 9, "CSCANF - A NUMBER: ");
	gotoxy(25, 9);
	cscanf("%d", &num);
	cprintfxy(6, 11, "GETPASS - A PASSWORD (8 MAX, ECHOES *): ");
	gotoxy(46, 11);
	p = getpass("");
	_setcursortype(_NOCURSOR);
	textattr(0x1F);
	cprintfxy(6, 14, "HELLO %s!", (pname[0]) ? pname : "ANONYMOUS");
	cprintfxy(6, 15, "YOU TYPED %d AND THE PASSWORD '%s'.", num, p);
	return demo_wait();
}

/* --------------------------------------------------------------------------------- */
static int demo_s12(void)
{
	long c1, c2, ms;
	int i;

	demo_header(12, "DELAY() TIMING");
	textattr(0x1F);
	c1 = (long) clock();
	delay(500);
	c2 = (long) clock();
	ms = (long) (((double) (c2 - c1)) * 1000.0 / (double) CLOCKS_PER_SEC);
	cprintfxy(8, 8, "DELAY(500) MEASURED %ld MS WITH CLOCK().", ms);
	cprintfxy(8, 10, "ON DOS THE BIOS TICK IS 55 MS (COARSE BUT SURE).");
	cprintfxy(8, 11, "ON WIN32 CONIOX USES Sleep() (FINE).");
	cprintfxy(8, 13, "ANIMATED DOTS:");
	for (i = 0; i < 5; i++)
	{
		putchxy(23 + i * 2, 13, '.');
		delay(150);
	}
	return demo_wait();
}

/* --------------------------------------------------------------------------------- */
static int demo_s13(void)
{
	demo_header(13, "DOS SPECIFICS: DIRECTVIDEO / _WSCROLL");
#if IS_DOS
	textattr(0x1F);
	cprintfxy(6, 7, "PRINTING WITH DIRECTVIDEO=1 (STRAIGHT TO VIDEO RAM).");
	directvideo = 0;
	cprintfxy(6, 9, "PRINTING WITH DIRECTVIDEO=0 (VIA BIOS INT 10H). SLOWER!");
	directvideo = 1;
	_wscroll = 0;
	textattr(0x1E);
	gotoxy(6, 11);
	cprintf("_WSCROLL=0: THIS LINE OVERFLOWS THE EDGE AND STAYS PUT.........................");
	_wscroll = 1;
	textattr(0x1F);
	cprintfxy(6, 13, "DIRECTVIDEO RESTORED TO 1.");
	return demo_wait();
#else
	textattr(0x1F);
	cprintfxy(6, 7, "DIRECTVIDEO AND _WSCROLL ARE DOS SPECIFICS.");
	cprintfxy(6, 9, "ON WIN32, CONIOX EMULATES CONIO WITH THE CONSOLE API:");
	cprintfxy(6, 10, "WRITECONSOLEOUTPUT, SCROLLCONSOLESCREENBUFFER, ...");
	cprintfxy(6, 12, "SAME CALLS, DIFFERENT ENGINE. 100% SOURCE COMPATIBLE.");
	return demo_wait();
#endif
}

/* --------------------------------------------------------------------------------- */
static void demo_tour(void)
{
	static int (* const demo_tab[13])(void) = {
		demo_s01, demo_s02, demo_s03, demo_s04, demo_s05, demo_s06, demo_s07,
		demo_s08, demo_s09, demo_s10, demo_s11, demo_s12, demo_s13
	};
	int i;

	for (i = 0; i < 13; i++)
	{
		if (!demo_tab[i]())
		{
			break;
		}
	}
	snd_off();
}

/* ================================================================================= */
/*  MAIN                                                                              */
/* ================================================================================= */
int main(void)
{
	int startmode;

	coniox_init(APP_NAME);          /* En Win32 ademas fija el titulo de consola      */
#if IS_WIN
	SetConsoleOutputCP(437);        /* CP 437: los marcos de caja se dibujan bien     */
#endif
	inittextinfo();
	read_screen();
	startmode = tinfo.currmode;
	records_load();
	srand((unsigned) time(NULL));
	_setcursortype(_NOCURSOR);
	intro_screen();
	menu_loop();
	snd_off();
#if IS_DOS
	directvideo = 1;                /* Por si el usuario lo dejo en modo BIOS         */
#endif
	read_screen();
	if (tinfo.currmode != startmode)
	{
		textmode(startmode);    /* Restaura el modo de texto original             */
		read_screen();
	}
	normvideo();
	_setcursortype(_NORMALCURSOR);
	full_window();
	clrscr();
	gotoxy(1, 1);
	cputs("THANK YOU FOR PLAYING " APP_NAME " v" APP_VERSION "!\r\n");
	cputs("POWERED BY CONIOX, THE PUBLIC DOMAIN CONIO COMPATIBLE LIBRARY.\r\n");
	return 0;
}