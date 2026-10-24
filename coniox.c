/* ----------------------------------------------------------------------------------------------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stddef.h>
#include "coniox.h"



/* ----------------------------------------------------------------------------------------------------------------- */
#define CONIOX_MAX_CHARINFO     (160 * 100)
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
	wchar_t coniox_ansi2unicode(char ch);
	char coniox_unicode2ansi(wchar_t ch);
	void coniox_putwchxyattr(int x, int y, wchar_t ch, int attr);
	#include <wchar.h>
	/* W-1b: prototipos wide. coniox.h solo los declara con UNICODE definido,
	   pero las definiciones de abajo se compilan siempre en Windows; sin
	   estos prototipos quedan implicitas (C4013 en C89, error en C99+). */
	wchar_t *cgetws(wchar_t *__str);
	int cputws(const wchar_t *__str);
	int cwscanf(const wchar_t *__format, ...);
	int cwprintf(const wchar_t *__format, ...);
	wchar_t getwche(void);
	wchar_t ungetwch(wchar_t __ch);
#endif


#if defined(__WATCOMC__)
	#define coniox_inline __inline
#elif defined(__BORLANDC__)
	#if (__BORLANDC__ >= 0x450)
		#define coniox_inline static __inline
	#else
		#define coniox_inline static
	#endif
#else
	#define coniox_inline inline
#endif

coniox_inline void coniox_putchxyattr(int x, int y, int ch, int attr);
void coniox_putchxyattrwh(int x, int y, int ch, int attr, int w, int h);
void coniox_idle(void);
int coniox_get_is_emulator (void);
void coniox_blink(unsigned int blink);
void coniox_enhanced_palette(unsigned int pal);
void coniox_readnormattr(void);
int coniox_is_emulator = 0;



/* ----------------------------------------------------------------------------------------------------------------- */
struct text_info ti; /*= { 1, 1, 80, 25, 15, 15, 0, 25, 80, 1, 1 } */
int directvideo = 1;
int _wscroll = 1;
int coniox_setcursortype = _NORMALCURSOR;
int coniox_cursor_dirty = 0;    /* B: 1 = el cursor fisico esta por detras de ti (curx, cury) */

#if ((__DOS__) || (__MSDOS__))
int coniox_basecrt = 0x3D4;

#if ((defined(__FLAT__)) || (defined(__DJGPP__)))
	unsigned short *coniox_vram;
	#define coniox_int86 int386
	#define coniox_int86x int386x
#else
	unsigned short coniox_far *coniox_vram;
	#define coniox_int86 int86
	#define coniox_int86x int86x
#endif
/* C: coniox_vram/coniox_int86/coniox_offset se adelantan para que putch()
   pueda escribir en VRAM sin llamada a coniox_putchxyattr() por caracter.
   coniox_offset() toma coordenadas 0-based. */
#define coniox_offset(piX, piY) (coniox_vram + ((ti.screenwidth * (piY)) + (piX)))

/* D: celda VRAM cacheada de la posicion logica (curx, cury). putch() escribe
   por aqui sin recalcular coniox_offset() por caracter (un imul de 16 bits,
   ~70-150 ciclos en 8086/286) ni recargar ti.* tras cada store far (el
   compilador debe asumir que el store a VRAM puede aliasear ti). Invariante:
   coniox_cell == coniox_offset(ti.winleft + ti.curx - 2, ti.wintop + ti.cury - 2).
   La establece coniox_init(), la re-sincroniza gotoxy() (window() y textmode()
   llegan via gotoxy) y la mantienen putch()/cputs() con aritmetica pura;
   coniox_flush_cursor() vuelve a pasar por gotoxy() y la repara en cada
   punto de espera. */
static unsigned short coniox_far *coniox_cell;
#endif



/* ----------------------------------------------------------------------------------------------------------------- */
/* INTERNAL (Private)*/
/* ----------------------------------------------------------------------------------------------------------------- */
int coniox_vsscanf(const char *buffer, const char *format, va_list argPtr)
{
	void *a[40] = {NULL}; // Espacio para hasta 20 argumentos + posibles tama�os
	size_t count = 0;
	const char *p;
	char c;

	if (!buffer || !format)
	{
		return(0);
	}

	p = format;
	while ((c = *p++) != 0)
	{
		if (c == '%')
		{
			if (*p == '%')
			{
				p++;
				continue;
			}
			if (*p == '*')
			{
				p++;
				continue;
			}
			// Saltar modificadores de ancho (opcional)
			while (*p >= '0' && *p <= '9')
			{
				p++;
			}
			// Detectar especificadores que requieren tama�o adicional
			#ifdef _MSC_VER
				if (count >= 39)        /* a[40]: par buffer+tamano, ultimo indice 39 */
				{
					break;
				}
			#else
				if (count >= 20)        /* sscanf se llama con 20 argumentos */
				{
					break;
				}
			#endif
			if (*p == 's' || *p == 'c' || *p == '[')
			{
				a[count++] = va_arg(argPtr, void *);            /* buffer */
				#ifdef _MSC_VER
					a[count++] = va_arg(argPtr, size_t *);  /* tama�o: solo sscanf_s */
				#endif
			}
			else
			{
				a[count++] = va_arg(argPtr, void *);
			}
		}
	}
	#ifdef _MSC_VER
		return(sscanf_s(buffer, format, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8], a[9], a[10], a[11], a[12], a[13], a[14], a[15], a[16], a[17], a[18], a[19],       a[20], a[21], a[22], a[23], a[24], a[25], a[26], a[27], a[28], a[29], a[30], a[31], a[32], a[33], a[34], a[35], a[36], a[37], a[38], a[39]));
	#else
		return(sscanf(buffer, format, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8], a[9], a[10], a[11], a[12], a[13], a[14], a[15], a[16], a[17], a[18], a[19]));
	#endif
}


/* ----------------------------------------------------------------------------------------------------------------- */
coniox_inline int coniox_inwindow(int x, int y)
{
	return ((unsigned int)(x - ti.winleft) <= (unsigned int)(ti.winright  - ti.winleft)) && ((unsigned int)(y - ti.wintop)  <= (unsigned int)(ti.winbottom - ti.wintop));
}



/* ----------------------------------------------------------------------------------------------------------------- */
/* PORTABLE FUNCTIONS */
/* ----------------------------------------------------------------------------------------------------------------- */
void clreol(void)
{
	int ax, ay, aw;


	coniox_init(NULL);
	ax = ti.winleft + ti.curx - 1;
	ay = ti.wintop  + ti.cury - 1;
	aw = ti.winright - ax + 1;          /* chars remaining to end of line */
	coniox_putchxyattrwh(ax, ay, ' ', ti.attribute, aw, 1);
}


/* ----------------------------------------------------------------------------------------------------------------- */
void inittextinfo(void)
{
	coniox_init(NULL);
	coniox_readnormattr();  /* E: atributo de arranque real de la consola, no un 7 fijo */
	textattr(ti.normattr);
}


/* ----------------------------------------------------------------------------------------------------------------- */
void gettextinfo(struct text_info *__r)
{
	coniox_init(NULL);
	*__r = ti;
}


/* ----------------------------------------------------------------------------------------------------------------- */
void highvideo(void)
{
	coniox_init(NULL);
	ti.attribute |= 0x08;
}


/* ----------------------------------------------------------------------------------------------------------------- */
void lowvideo(void)
{
	coniox_init(NULL);
	ti.attribute &= ~0x08;
}



/* ----------------------------------------------------------------------------------------------------------------- */
void normvideo(void)
{
	coniox_init(NULL);
	ti.attribute = ti.normattr;
}



/* ----------------------------------------------------------------------------------------------------------------- */
void textbackground(int __newcolor)
{
	coniox_init(NULL);
	ti.attribute = (unsigned short)((ti.attribute & 0x8F) | ((__newcolor & 0x07) << 4));
}



/* ----------------------------------------------------------------------------------------------------------------- */
void textcolor(int __newcolor)
{
	coniox_init(NULL);
	ti.attribute = (unsigned short)((ti.attribute & 0x70) | (__newcolor & 0x8F));
}


/* ----------------------------------------------------------------------------------------------------------------- */
void window(int __left, int __top, int __right, int __bottom)
{
	coniox_init(NULL);

	if (__left < 1 || __top < 1 || __right > ti.screenwidth  || __bottom > ti.screenheight || __left > __right          || __top > __bottom)
	{
		return;
	}
	ti.winleft   = (short) __left;
	ti.winright  = (short) __right;
	ti.wintop    = (short) __top;
	ti.winbottom = (short) __bottom;
	gotoxy(1, 1);
}


/* ----------------------------------------------------------------------------------------------------------------- */
coniox_inline void coniox_flush_cursor(void)
{
	/*
		B: sincronizado diferido del cursor fisico. putch()/putwch() solo
		marcan coniox_cursor_dirty al avanzar; los puntos de espera
		(kbhit/getch) y gotoxy() hacen el sync real. Coste nulo si esta
		limpia: una comparacion.
	*/
	if ((!coniox_cursor_dirty) || (coniox_setcursortype == _NOCURSOR))
	{
		return;
	}
	coniox_cursor_dirty = 0;
	gotoxy(ti.curx, ti.cury);
}


/* ----------------------------------------------------------------------------------------------------------------- */
int putch(int __c)
{
	int oldx, oldy;
	int winwidth, winheight;

	coniox_init(NULL);

	winwidth  = ti.winright  - ti.winleft + 1;
	winheight = ti.winbottom - ti.wintop  + 1;

	switch (__c)
	{
		case '\r':
#if ((__DOS__) || (__MSDOS__))
			coniox_cell -= ti.curx - 1;     /* D: a la columna 1 de la ventana, misma fila */
#endif
			ti.curx = 1;
			coniox_cursor_dirty = 1;        /* C: sin gotoxy; el sync llega en el proximo punto de espera */
			break;
		case '\n':
			if (ti.cury < winheight)
			{
#if ((__DOS__) || (__MSDOS__))
				coniox_cell += ti.screenwidth;  /* D: misma columna, fila siguiente */
#endif
				ti.cury++;
				coniox_cursor_dirty = 1;
			}
			else if (_wscroll)
			{
				/* LF: keep the column, like Borland's putch. delline() no toca el
				   cursor fisico en ninguna rama, asi que el baile solo toca ti
				   y deja la marca de sync diferido. */
				oldx = ti.curx;
				oldy = ti.cury;
				ti.curx = 1;
				ti.cury = 1;
				delline();
				ti.curx = oldx;
				ti.cury = oldy;
				/* D: delline() corre bajo las mismas direcciones y (curx, cury)
				   vuelve a su sitio: la celda cacheada no cambia. */
				coniox_cursor_dirty = 1;
			}
			/* _wscroll = 0: LF on the last line does not scroll. */
			break;
		case '\b':
			if (ti.curx > 1)
			{
#if ((__DOS__) || (__MSDOS__))
				--coniox_cell;                  /* D: una celda a la izquierda */
#endif
				ti.curx--;
				coniox_cursor_dirty = 1;
			}
			break;
		default:
#if ((__DOS__) || (__MSDOS__))
			if (directvideo)
			{
				/* C: ruta rapida sin llamada. MISMA celda que coniox_putchxyattr():
				   coniox_offset() toma coordenadas 0-based, luego restan 2. La
				   ronda 7 pasaba -1 y cada caracter salia +1 fila/+1 columna.
				   D: la celda ya vive cacheada en coniox_cell: ni coniox_offset()
				   (imul de 16 bits por caracter) ni recargas de ti.* tras el
				   store far; el avance es ++coniox_cell mas abajo. */
				*coniox_cell = (unsigned short) ((ti.attribute << 8) | (__c & 0xFF));
			}
			else
			{
				coniox_putchxyattr(ti.winleft + ti.curx - 1, ti.wintop + ti.cury - 1, __c, ti.attribute);
			}
#else
			coniox_putchxyattr(ti.winleft + ti.curx - 1, ti.wintop + ti.cury - 1, __c, ti.attribute);
#endif
			if (ti.curx < winwidth)
			{
#if ((__DOS__) || (__MSDOS__))
				++coniox_cell;                  /* D: celda siguiente, sin recalcular el offset */
#endif
				ti.curx++;
				coniox_cursor_dirty = 1;        /* B: el cursor se sincroniza al bloquear o al cerrar la rafaga */
			}
			else
			{
				if (_wscroll)
				{
					if (ti.cury < winheight)
					{
#if ((__DOS__) || (__MSDOS__))
						coniox_cell += ti.screenwidth - winwidth + 1;   /* D: (1, cury+1) es aritmetica pura */
#endif
						ti.curx = 1;
						ti.cury++;
						coniox_cursor_dirty = 1;
					}
					else
					{
						oldy = ti.cury;
						ti.curx = 1;
						ti.cury = 1;
						delline();
						ti.cury = oldy;
#if ((__DOS__) || (__MSDOS__))
						coniox_cell -= winwidth - 1;    /* D: de la col. winwidth a la 1, misma fila: el scroll corre bajo las mismas direcciones */
#endif
						coniox_cursor_dirty = 1;
					}
				}
				else
				{
					/* _wscroll = 0: stay on the last column, further output overwrites the edge cell. */
					/* D: coniox_cell tampoco avanza: la proxima escritura pisa la misma celda. */
					coniox_cursor_dirty = 1;
				}
			}
	}
	return(__c & 0xFF);
}


/* ----------------------------------------------------------------------------------------------------------------- */
char *cgets(char *__str)
{
	char *str;
	int maxlen;
	int length = 0;
	int ch = 0;
	int x, y;

	coniox_init(NULL);

	if (__str == NULL)
	{
		return(NULL);
	}
	str = __str + 2;
	maxlen = (int) ((unsigned char) __str[0] ) - 1;
	x = ti.curx;
	y = ti.cury;

	while (ch != '\r')
	{
		ch = getch();
		if (ch == 0)
		{
			getch();
		}
		else
		{
			switch (ch)
			{
				case '\r':
					break;
				case '\b':
					if (length > 0)
					{
						coniox_putchxyattr(ti.winleft + x - 1 + --length, ti.wintop + y - 1, ' ', ti.attribute);
					}
					gotoxy(x + length, y);
					break;
				default:
					if (length < maxlen)
					{
						putch(ch);
						str[length++] = (char) ch;
					}
			}
		}
	}
	__str[1] = (char) length;
	str[length] = 0;
	return str;
}



/* ----------------------------------------------------------------------------------------------------------------- */
int cvprintf(const char *__format, va_list __arglist)
{
	int r;
	char buffer[PRINTFBUF_SIZE];

	if (!__format || !__format[0])
	{
		return(0);
	}

	#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
		r = vsnprintf(buffer, PRINTFBUF_SIZE, __format, __arglist);
	#elif defined(__WATCOMC__)
		r = _vsnprintf(buffer, PRINTFBUF_SIZE, __format, __arglist);
	#else
		r = vsprintf(buffer, __format, __arglist);
	#endif
	if (r < 0)
	{
		buffer[0] = 0;
	}
	else if ((size_t) r >= sizeof(buffer))
	{
		buffer[sizeof(buffer) - 1] = 0;
	}
	cputs(buffer);
	return(r);
}


/* ----------------------------------------------------------------------------------------------------------------- */
int cprintf(const char *__format, ...)
{
	int r;
	va_list ap;

	va_start(ap, __format);
	r = cvprintf(__format, ap);
	va_end(ap);
	return(r);
}

/* ----------------------------------------------------------------------------------------------------------------- */
int cscanf(const char *__format, ...)
{
	int r;
	unsigned char buffer[PRINTFBUF_SIZE + 3];

	va_list ap;
	buffer[0] = PRINTFBUF_SIZE;
	cgets(buffer);
	va_start(ap, __format);
	r = coniox_vsscanf(buffer + 2, __format, ap);
	va_end(ap);
	return r;
}


/* ----------------------------------------------------------------------------------------------------------------- */
int getche(void)
{
	int ch;

	ch = getch();
	if (ch != 0)
	{
		putch(ch);
		coniox_flush_cursor();  /* C: el eco deja el cursor fisico al dia */
	}
	return(ch);
}


/* ----------------------------------------------------------------------------------------------------------------- */
char *getpass(const char *__prompt)
{
	static char str[PASS_MAX + 1];
	int length = 0;
	int ch = 0;
	int x, y;

	cputs(__prompt);
	x = ti.curx;
	y = ti.cury;

	while (ch != '\r')
	{
		ch = getch();
		if (ch == 0)
		{
			getch();
		}
		else
		{
			switch (ch)
			{
				case '\r':
					break;
				case '\b':
					if (length > 0)
					{
						coniox_putchxyattr(ti.winleft + x - 1 + --length, ti.wintop + y - 1, ' ', ti.attribute);
					}
					gotoxy(x + length, y);
					break;
				default:
					if (length < PASS_MAX)
					{
						putch('*');
						str[length++] = (char) ch;
					}
			}
		}
	}
	str[length] = 0;
	return(str);
}


/* ----------------------------------------------------------------------------------------------------------------- */
int wherex(void)
{
	coniox_init(NULL);
	return(ti.curx);
}



/* ----------------------------------------------------------------------------------------------------------------- */
int wherey(void)
{
	coniox_init(NULL);
	return(ti.cury);
}


/* ----------------------------------------------------------------------------------------------------------------- */
/* WINDOWS ONLY (Portable) */
/* ----------------------------------------------------------------------------------------------------------------- */
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)


/* ----------------------------------------------------------------------------------------------------------------- */
int cputsportable(const char *__str)
{
	const char *orig = __str;


	while (*__str)
	{
		putch(*__str++);
	}
        
	coniox_flush_cursor();  /* C: un sync por cadena, no por caracter */
	return (int)(__str - orig);
}


/* ----------------------------------------------------------------------------------------------------------------- */
int coniox_vswscanf(const wchar_t* buffer, const wchar_t* format, va_list argPtr)
{
	void *a[20] = { NULL };
	size_t count = 0;
	const wchar_t *p;


	p = format;
	while (1)
	{
		wchar_t c = *(p++);
		if (c == 0)
		{
			break;
		}
		if (c == L'%' && (p[0] != L'*' && p[0] != L'%'))
		{
			if (count >= 20)        /* swscanf (clasico): 20 argumentos, uno por especificador; swscanf_s exigiria
						ademas un tamano por %s/%c/[% que el llamador de cwscanf no pasa */
			{
				break;
			}
			a[count++] = va_arg(argPtr, void*);
		}
	}
	return(swscanf(buffer, format, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8], a[9], a[10], a[11], a[12], a[13], a[14], a[15], a[16], a[17], a[18], a[19]));
}


/* ----------------------------------------------------------------------------------------------------------------- */
wchar_t *cgetws(wchar_t *__str)
{
	wchar_t *str;
	int maxlen;
	int length = 0;
	wchar_t ch = 0;
	int x, y;

	coniox_init(NULL);
	if (__str == NULL)
	{
		return(NULL);
	}

	str = __str + 2;
	maxlen = (int)(__str[0]) - 1;
	x = ti.curx;
	y = ti.cury;

	while (ch != L'\r')
	{
		ch = getwch();
		if (ch == 0)
		{
			getwch();
		}
		else
		{
			switch (ch)
			{
				case L'\r':
					break;
				case L'\b':
					if (length > 0)
					{
						coniox_putwchxyattr(ti.winleft + x - 1 + --length, ti.wintop + y - 1, ' ', ti.attribute);
					}
					gotoxy(x + length, y);
					break;
				default:
					if (length < maxlen)
					{
						putwch(ch);
						str[length++] = ch;
					}
			}
		}
	}
	__str[1] = (wchar_t)length;
	str[length] = 0;
	return str;
}





/* ----------------------------------------------------------------------------------------------------------------- */
wchar_t putwch(wchar_t __c)
{
	int oldx, oldy;
	int winwidth, winheight;


	coniox_init(NULL);

	winwidth = ti.winright - ti.winleft + 1;
	winheight = ti.winbottom - ti.wintop + 1;

	switch (__c)
	{
		case L'\r':
			ti.curx = 1;
			coniox_cursor_dirty = 1;        /* C: sin gotoxy; el sync llega en el proximo punto de espera */
			break;
		case L'\n':
			if (ti.cury < winheight)
			{
				ti.cury++;
				coniox_cursor_dirty = 1;
			}
			else if (_wscroll)
			{
				/* LF: keep the column, like Borland putwch. delline() no toca el
				   cursor fisico: el baile solo toca ti y marca el sync diferido. */
				oldx = ti.curx;
				oldy = ti.cury;
				ti.curx = 1;
				ti.cury = 1;
				delline();
				ti.curx = oldx;
				ti.cury = oldy;
				coniox_cursor_dirty = 1;
			}
			/* _wscroll = 0: LF on the last line does not scroll. */
			break;
		case L'\b':
			if (ti.curx > 1)
			{
				ti.curx--;
				coniox_cursor_dirty = 1;
			}
			break;
		default:
			coniox_putwchxyattr(ti.winleft + ti.curx - 1, ti.wintop + ti.cury - 1, __c, ti.attribute);
			if (ti.curx >= winwidth)
			{
				if (_wscroll)
				{
					if (ti.cury < winheight)
					{
						ti.curx = 1;
						ti.cury++;
						coniox_cursor_dirty = 1;
					}
					else
					{
						oldy = ti.cury;
						ti.curx = 1;
						ti.cury = 1;
						delline();
						ti.cury = oldy;
						coniox_cursor_dirty = 1;
					}
				}
				else
				{
					/* _wscroll = 0: stay on the last column, further output overwrites the edge cell. */
					coniox_cursor_dirty = 1;
				}
			}
			else
			{
				ti.curx++;
				coniox_cursor_dirty = 1;        /* B: el cursor se sincroniza al bloquear o al cerrar la rafaga */
			}
	}
	return(__c);
}


/* ----------------------------------------------------------------------------------------------------------------- */
int cwscanf(const wchar_t* __format, ...)
{
	int r;
	va_list ap;
	wchar_t buffer[PRINTFBUF_SIZE + 3];

	buffer[0] = PRINTFBUF_SIZE;
	cgetws(buffer);
	va_start(ap, __format);
	r = coniox_vswscanf(buffer + 2, __format, ap);
	va_end(ap);
	return(r);
}


/* ----------------------------------------------------------------------------------------------------------------- */
int cwprintf(const wchar_t *__format, ...)
{
	int r;
	wchar_t buffer[PRINTFBUF_SIZE];
	va_list ap;

	if (!__format || !__format[0])
	{
		return(0);
	}

	va_start(ap, __format);
	r = vswprintf(buffer, PRINTFBUF_SIZE, __format, ap);
	va_end(ap);
	if (r < 0)
	{
		buffer[0] = 0;
	}
	else if ((size_t) r >= sizeof(buffer) / sizeof(buffer[0]))
	{
		buffer[sizeof(buffer) / sizeof(buffer[0]) - 1] = 0;
	}
	cputws(buffer);
	return(r);
}
#endif



/* ----------------------------------------------------------------------------------------------------------------- */
/* WINDOWS ONLY (Non portable) */
/* ----------------------------------------------------------------------------------------------------------------- */
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
#include <windows.h>
#include <Wincon.h>
#include <wchar.h>
HANDLE coniox_console_output = NULL;
HANDLE coniox_console_input = NULL;

static CHAR_INFO coniox_charinfo[CONIOX_MAX_CHARINFO];
static CHAR_INFO *coniox_charinfo_scratch(int total)
{
	if (total <= CONIOX_MAX_CHARINFO)
	{
		return coniox_charinfo;
	}
	return (CHAR_INFO *) malloc((size_t) total * sizeof(CHAR_INFO));
}

static void coniox_free_charinfo(CHAR_INFO *p)
{
	if (p != coniox_charinfo)
	{
		free(p);
	}
}


/* ----------------------------------------------------------------------------------------------------------------- */
void coniox_init(const void *title)
{
	int i;
	DWORD m;
	UINT acp;
	CONSOLE_SCREEN_BUFFER_INFO info;

	/* Check if already initialized */
	if (coniox_console_output)
	{
		return;
	}
	coniox_console_output = GetStdHandle(STD_OUTPUT_HANDLE);
	coniox_console_input = GetStdHandle(STD_INPUT_HANDLE);

	for (i = 0; i < 2; ++i)
	{
		if (GetConsoleScreenBufferInfo(coniox_console_output, &info))
		{
			break;
		}
		if (i)
		{
			/* W-3: no dejar el handle asignado; si no, las siguientes
			   llamadas creerian la libreria ya inicializada con ti a
			   ceros y handles invalidos para siempre. NULL permite
			   reintentar (p. ej. tras un AttachConsole posterior). */
			coniox_console_output = NULL;
			coniox_console_input = NULL;
			return;
		}
		FreeConsole();
		AllocConsole();
		/* La consola nueva tiene otros handles estandar: re-obtenerlos. */
		coniox_console_output = GetStdHandle(STD_OUTPUT_HANDLE);
		coniox_console_input = GetStdHandle(STD_INPUT_HANDLE);
	}
	ti.normattr = (unsigned short) info.wAttributes;
	ti.winleft = (unsigned short) info.srWindow.Left + 1;
	ti.wintop = (unsigned short)info.srWindow.Top + 1;
	ti.winright = (unsigned short) info.srWindow.Right + 1;
	ti.winbottom = (unsigned short) info.srWindow.Bottom + 1;
	ti.screenwidth = (unsigned short) info.dwSize.X;
	ti.screenheight = (unsigned short) info.dwSize.Y;
	ti.curx = (unsigned short) info.dwCursorPosition.X + 1;
	ti.cury = (unsigned short) info.dwCursorPosition.Y + 1;
	ti.attribute = (unsigned short) info.wAttributes;
	acp = GetACP();
	if (GetConsoleOutputCP() != acp )
	{
		SetConsoleOutputCP(acp);
		SetConsoleCP(acp);
	}
	if (GetConsoleMode(coniox_console_input, &m))
	{
		SetConsoleMode(coniox_console_input, m | ENABLE_WINDOW_INPUT);
	}

	if (title)
	{
		SetConsoleTitle(title);
	}
}

/* ----------------------------------------------------------------------------------------------------------------- */
void coniox_readnormattr(void)
{
	CONSOLE_SCREEN_BUFFER_INFO info;

	/*
		C: relee el atributo por defecto de la consola para ti.normattr: es el de
		arranque del programa, no el que textattr() deja en ti.attribute. Si la
		lectura falla (salida redirigida) se conserva el valor de coniox_init().
	*/
	if (GetConsoleScreenBufferInfo(coniox_console_output, &info))
	{
		ti.normattr = (unsigned short) info.wAttributes;
	}
}



/* ----------------------------------------------------------------------------------------------------------------- */
int cputs(const char *__str)
{
	const char *orig = __str;
	const char *run;
	unsigned int i;
	unsigned int runlen, remaining;
	unsigned int winwidth, winheight;
	int oldx, oldy;
	COORD c = { 0, 0 };
	CHAR_INFO *ci = coniox_charinfo;        /* W-S4: scratch compartido unico (antes: 64 KB estaticos por llamada) */

	if (__str == NULL)
	{
		return(0);
	}

	coniox_init(NULL);

	winwidth  = (unsigned int)(ti.winright - ti.winleft + 1);
	winheight = (unsigned int)(ti.winbottom - ti.wintop + 1);

	if (winwidth == 0 || winheight == 0)
	{
		return(0);
	}

	while (*__str)
	{
		/* Carriage return: volver al inicio de linea. El cursor fisico no
		   se toca aqui: WriteConsoleOutput no lee ni mueve el cursor de
		   consola; el gotoxy() de cierre lo sincroniza al final. */
		if (*__str == '\r')
		{
			ti.curx = 1;
			++__str;
			continue;
		}

		/* Line feed: avanzar o desplazar la ventana. */
		if (*__str == '\n')
		{
			if (ti.cury < (int) winheight)
			{
				ti.cury++;
			}
			else if (_wscroll)
			{
				/* LF: keep the column, like Borland cputs. delline()
				   borra la linea 1 de la ventana y solo lee ti; el
				   sync fisico llega con el gotoxy() de cierre. */
				oldx = ti.curx;
				oldy = ti.cury;
				ti.curx = 1;
				ti.cury = 1;
				delline();
				ti.curx = oldx;
				ti.cury = oldy;
			}
			/* _wscroll = 0: LF on the last line does not scroll. */
			++__str;
			continue;
		}

		/* Backspace: retroceder sin borrar el caracter (sync diferido). */
		if (*__str == '\b')
		{
			if (ti.curx > 1)
			{
				ti.curx--;
			}
			++__str;
			continue;
		}

		/*
		 * Agrupar caracteres imprimibles hasta el siguiente control, el borde
		 * de la ventana o el tamanio del lote: un solo WriteConsoleOutputA por run.
		 * Los controles ya se filtraron arriba y remaining >= 1, asi que runlen
		 * siempre es >= 1 (no hay riesgo de bucle infinito).
		 */
		run = __str;
		runlen = 0;
		if (ti.curx > (int) winwidth)
		{
			ti.curx = (int) winwidth;       /* la ventana se encogio: evitar underflow en remaining */
		}
		remaining = winwidth - (unsigned int) ti.curx + 1;
		if (remaining > CONIOX_MAX_CHARINFO)
		{
			remaining = CONIOX_MAX_CHARINFO;
		}

		while (runlen < remaining &&
		       run[runlen] != '\0' &&
		       run[runlen] != '\r' &&
		       run[runlen] != '\n' &&
		       run[runlen] != '\b')
		{
			runlen++;
		}

		for (i = 0; i < runlen; i++)
		{
			ci[i].Char.AsciiChar = run[i];
			ci[i].Attributes = ti.attribute;
		}

		{
			SMALL_RECT r;
			COORD s;

			r.Left   = (short) (ti.winleft + ti.curx - 2);
			r.Top    = (short) (ti.wintop  + ti.cury - 2);
			r.Right  = (short) (r.Left + runlen - 1);
			r.Bottom = r.Top;
			s.X = (short) runlen;
			s.Y = 1;
			WriteConsoleOutputA(coniox_console_output, ci, s, c, &r);
		}
		ti.curx += runlen;

		/* El run nunca cruza el borde de la ventana: el wrap se decide una sola vez. */
		if (ti.curx > (int) winwidth)
		{
			if (_wscroll)
			{
				if (ti.cury < (int) winheight)
				{
					ti.curx = 1;
					ti.cury++;
				}
				else
				{
					ti.curx = 1;
					ti.cury = 1;
					delline();
					ti.cury = (int) winheight;
				}
			}
			else
			{
				ti.curx = (int) winwidth;       /* _wscroll = 0: el siguiente run sobrescribe la celda del borde */
			}
		}
		__str += runlen;
	}
	gotoxy(ti.curx, ti.cury);
	return (int)(__str - orig);
}



/* ----------------------------------------------------------------------------------------------------------------- */
int cputws(const wchar_t *__str)
{
	const wchar_t *orig = __str;
	const wchar_t *run;
	unsigned int i;
	unsigned int runlen, remaining;
	unsigned int winwidth, winheight;
	int oldx, oldy;
	COORD c = { 0, 0 };
	CHAR_INFO *ci = coniox_charinfo;        /* W-S4: scratch compartido unico (antes: 64 KB estaticos por llamada) */

	if (__str == NULL)
	{
		return(0);
	}

	coniox_init(NULL);

	winwidth  = (unsigned int)(ti.winright - ti.winleft + 1);
	winheight = (unsigned int)(ti.winbottom - ti.wintop + 1);

	if (winwidth == 0 || winheight == 0)
	{
		return(0);
	}

	while (*__str)
	{
		/* Carriage return: volver al inicio de linea (sync diferido). */
		if (*__str == L'\r')
		{
			ti.curx = 1;
			++__str;
			continue;
		}

		/* Line feed: avanzar o desplazar la ventana. */
		if (*__str == L'\n')
		{
			if (ti.cury < (int) winheight)
			{
				ti.cury++;
			}
			else if (_wscroll)
			{
				/* LF: keep the column, like Borland cputws. */
				oldx = ti.curx;
				oldy = ti.cury;
				ti.curx = 1;
				ti.cury = 1;
				delline();
				ti.curx = oldx;
				ti.cury = oldy;
			}
			/* _wscroll = 0: LF on the last line does not scroll. */
			++__str;
			continue;
		}

		/* Backspace: retroceder sin borrar el caracter (sync diferido). */
		if (*__str == L'\b')
		{
			if (ti.curx > 1)
			{
				ti.curx--;
			}
			++__str;
			continue;
		}

		/*
		 * Gemelo Unicode de cputs(): agrupar imprimibles hasta el siguiente
		 * control o el borde de la ventana: un solo WriteConsoleOutputW por
		 * run (antes: un syscall por caracter via putwch()).
		 */
		run = __str;
		runlen = 0;
		if (ti.curx > (int) winwidth)
		{
			ti.curx = (int) winwidth;       /* la ventana se encogio: evitar underflow en remaining */
		}
		remaining = winwidth - (unsigned int) ti.curx + 1;
		if (remaining > CONIOX_MAX_CHARINFO)
		{
			remaining = CONIOX_MAX_CHARINFO;
		}

		while (runlen < remaining &&
		       run[runlen] != L'\0' &&
		       run[runlen] != L'\r' &&
		       run[runlen] != L'\n' &&
		       run[runlen] != L'\b')
		{
			runlen++;
		}

		for (i = 0; i < runlen; i++)
		{
			ci[i].Char.UnicodeChar = run[i];
			ci[i].Attributes = ti.attribute;
		}

		{
			SMALL_RECT r;
			COORD s;

			r.Left   = (short) (ti.winleft + ti.curx - 2);
			r.Top    = (short) (ti.wintop  + ti.cury - 2);
			r.Right  = (short) (r.Left + runlen - 1);
			r.Bottom = r.Top;
			s.X = (short) runlen;
			s.Y = 1;
			WriteConsoleOutputW(coniox_console_output, ci, s, c, &r);
		}
		ti.curx += runlen;

		/* El run nunca cruza el borde de la ventana: el wrap se decide una sola vez. */
		if (ti.curx > (int) winwidth)
		{
			if (_wscroll)
			{
				if (ti.cury < (int) winheight)
				{
					ti.curx = 1;
					ti.cury++;
				}
				else
				{
					ti.curx = 1;
					ti.cury = 1;
					delline();
					ti.cury = (int) winheight;
				}
			}
			else
			{
				ti.curx = (int) winwidth;       /* _wscroll = 0: el siguiente run sobrescribe la celda del borde */
			}
		}
		__str += runlen;
	}
	gotoxy(ti.curx, ti.cury);
	return (int)(__str - orig);
}



/* ----------------------------------------------------------------------------------------------------------------- */
inline void coniox_putchxyattr(int x, int y, int ch, int attr)
{
	CHAR_INFO ci;
	SMALL_RECT r;
	COORD c = {0, 0}, s = {1, 1};


	r.Left = (short) (x - 1);
	r.Top = (short) (y - 1);
	r.Right = (short) (x - 1);
	r.Bottom = (short) (y - 1);
	ci.Char.AsciiChar = (char) ch;
	ci.Attributes = (short) attr;
	WriteConsoleOutputA(coniox_console_output, &ci, s, c, &r);
}




/* ----------------------------------------------------------------------------------------------------------------- */
void coniox_putwchxyattr(int x, int y, wchar_t ch, int attr)
{
	CHAR_INFO ci;
	SMALL_RECT r;
	COORD c = { 0, 0 }, s = { 1, 1 };


	r.Left = (short) (x - 1);
	r.Top = (short) (y - 1);
	r.Right = (short) (x - 1);
	r.Bottom = (short) (y - 1);
	ci.Char.UnicodeChar = ch;
	ci.Attributes = (WORD) attr;
	WriteConsoleOutputW(coniox_console_output, &ci, s, c, &r);
}


/* ----------------------------------------------------------------------------------------------------------------- */
void coniox_putchxyattrwh(int x, int y, int ch, int attr, int w, int h)
{
	int i;
	int total;
	SMALL_RECT r;
	COORD s, c;
	CHAR_INFO *ci;
        
	if (w <= 0 || h <= 0)
	{
		return;
	}
        
	total = w * h;
	ci = coniox_charinfo_scratch(total);
	if (ci == NULL)
	{
		return;         /* E: sin memoria para un rectangulo tan grande */
	}

	#if UNICODE
		ci[0].Char.UnicodeChar = (wchar_t) ch;
	#else
		ci[0].Char.AsciiChar = (char) ch;
	#endif
	ci[0].Attributes = (WORD) attr;

	for (i = 1; i < total; i <<= 1)
	{
		int copy = i;
		if (copy > total - i)
		{
			copy = total - i;
		}
		memcpy(&ci[i], &ci[0], copy * sizeof(CHAR_INFO));
	}

	r.Left = (short) x - 1;
	r.Top = (short) y - 1;
	r.Right = (short) (x + w - 2);
	r.Bottom = (short) (y + h - 2);

	s.X = (short) w;
	s.Y = (short) h;
	c.X = 0;
	c.Y = 0;

	WriteConsoleOutput(coniox_console_output, ci, s, c, &r);
	coniox_free_charinfo(ci);
}



/* ----------------------------------------------------------------------------------------------------------------- */
wchar_t getwch(void)
{
	return(coniox_ansi2unicode((char) getch()));
}


/* ----------------------------------------------------------------------------------------------------------------- */
wchar_t getwche(void)
{
	wchar_t ch;

	ch = getwch();
	while (ch == 0)
	{
		ch = getwch();
	}
	putwch(ch);
	coniox_flush_cursor();  /* C: el eco deja el cursor fisico al dia */
	return(ch);
}


/* ----------------------------------------------------------------------------------------------------------------- */
wchar_t coniox_ansi2unicode(char ch)
{
	char ansistr[2];
	wchar_t unicodestr[2];

	ansistr[0] = ch;
	ansistr[1] = 0;
	unicodestr[0] = 0;
	MultiByteToWideChar(CP_ACP, 0, ansistr, 1, unicodestr, 1);
	return(unicodestr[0]);
}



/* ----------------------------------------------------------------------------------------------------------------- */
char coniox_unicode2ansi(wchar_t ch)
{
	char ansistr[2];
	wchar_t unicodestr[2];

	ansistr[0] = (char) ch;
	unicodestr[0] = ch;
	unicodestr[1] = 0;
	WideCharToMultiByte(CP_ACP, 0, unicodestr, 1, ansistr, 1, NULL, NULL);
	return ansistr[0];
}



/* ----------------------------------------------------------------------------------------------------------------- */
int getch(void)
{
	char car;       /* W-S3: ReadConsoleA escribe 1 byte; char en vez de int con bytes altos sin inicializar */
	DWORD leidos, modo = 0;

	coniox_init(NULL); /* getch() puede ser la primera llamada conio: asegurar los handles */
	coniox_flush_cursor();  /* B: el cursor fisico al dia antes de bloquearse */
	if (GetConsoleMode(coniox_console_input, &modo))
	{
		SetConsoleMode(coniox_console_input, modo & ~ENABLE_ECHO_INPUT & ~ENABLE_PROCESSED_INPUT & ~ENABLE_LINE_INPUT);
	}
	if (!ReadConsoleA(coniox_console_input, &car, 1, &leidos, NULL) || leidos == 0)
	{
		SetConsoleMode(coniox_console_input, modo);
		return EOF;
	}
	SetConsoleMode(coniox_console_input, modo);
	return((int) ((unsigned char) car));
}



/* ----------------------------------------------------------------------------------------------------------------- */
void gotoxy(int __x, int __y)
{
	COORD c;

	coniox_init(NULL);
	if (!coniox_inwindow(ti.winleft + __x - 1, ti.wintop + __y - 1))
	{
		return;
	}
	ti.curx = (short) __x;
	ti.cury = (short) __y;
	coniox_cursor_dirty = 0;        /* gotoxy deja el cursor fisico al dia (o oculto) */
	if (coniox_setcursortype != _NOCURSOR)
	{
		c.X = ti.winleft + ti.curx - 2;
		c.Y = ti.wintop + ti.cury - 2;
		SetConsoleCursorPosition(coniox_console_output, c);
	}
}


/* ----------------------------------------------------------------------------------------------------------------- */
void clrscr(void)
{
	gotoxy(1, 1);
	coniox_putchxyattrwh(ti.winleft, ti.wintop, ' ', ti.attribute, ti.winright - ti.winleft + 1, ti.winbottom - ti.wintop + 1);
}



/* ----------------------------------------------------------------------------------------------------------------- */
void textattr(int __newattr)
{
	coniox_init(NULL);
	ti.attribute = (unsigned short) __newattr;
	SetConsoleTextAttribute(coniox_console_output, (unsigned short) ti.attribute);
}


/* ----------------------------------------------------------------------------------------------------------------- */
void delline(void)
{
	COORD c;
	SMALL_RECT r;
	CHAR_INFO ci;


	coniox_init(NULL);
	c.X = ti.winleft - 1;
	c.Y = ti.wintop - 1 + ti.cury - 1;
	r.Left = ti.winleft - 1;
	r.Top = ti.wintop - 1 + ti.cury;
	r.Right = ti.winright - 1;
	r.Bottom = ti.winbottom - 1;
	ci.Attributes = ti.attribute;
	#if UNICODE
		ci.Char.UnicodeChar = L' ';
	#else
		ci.Char.AsciiChar = ' ';
	#endif
	ScrollConsoleScreenBuffer(coniox_console_output, &r, NULL, c, &ci);
	/* C: el scroll no mueve el cursor de consola; el gotoxy() de cierre sobraba
	   y, durante el baile de scroll de putch()/putwch() (ti apuntando a (1,1)),
	   llevaba el cursor fisico a la esquina. El sync llega con el flush. */
}


/* ----------------------------------------------------------------------------------------------------------------- */
void insline(void)
{
	COORD c;
	SMALL_RECT r;
	CHAR_INFO ci;


	coniox_init(NULL);
	c.X = ti.winleft - 1;
	c.Y = ti.wintop - 1 + ti.cury;
	r.Left = ti.winleft - 1;
	r.Top = ti.wintop - 1 + ti.cury - 1;
	r.Right = ti.winright - 1;
	r.Bottom = ti.winbottom - 2;
	ci.Attributes = ti.attribute;
	#if UNICODE
		ci.Char.UnicodeChar = L' ';
	#else
		ci.Char.AsciiChar = ' ';
	#endif
	ScrollConsoleScreenBuffer(coniox_console_output, &r, NULL, c, &ci );
	/* C: el scroll no mueve el cursor de consola; el gotoxy() de cierre sobraba
	   y, durante el baile de scroll de putch()/putwch() (ti apuntando a (1,1)),
	   llevaba el cursor fisico a la esquina. El sync llega con el flush. */
}



/* ----------------------------------------------------------------------------------------------------------------- */
int gettext(int __left, int __top, int __right, int __bottom, void *__destin)
{
	int i;
	int total;
	SMALL_RECT r;
	short *buf;
	COORD s, c = { 0,0 };
	CHAR_INFO *ci;


	coniox_init(NULL);
	/* Validacion estilo Borland: rectangulo dentro de la pantalla absoluta. */
	if (__left < 1 || __top < 1 || __right > ti.screenwidth || __bottom > ti.screenheight || __right < __left || __bottom < __top)
	{
		return(0);
	}
	total = (__right - __left + 1) * (__bottom - __top + 1);
	ci = coniox_charinfo_scratch(total);
	if (ci == NULL)
	{
		return(0);
	}



	r.Left = (short) __left - 1;
	r.Top = (short) __top - 1;
	r.Right = (short) __right - 1;
	r.Bottom = (short) __bottom - 1;
	s.X = (short) (__right - __left + 1);
	s.Y = (short) (__bottom - __top + 1);
	buf = (short *) __destin;
	if (!ReadConsoleOutput(coniox_console_output, ci, s, c, &r ))
	{
		coniox_free_charinfo(ci);
		return(0);      /* W-2: sin la lectura no hay datos validos que devolver */
	}
	for (i = 0; i < s.X * s.Y; i++)
	{
		#if UNICODE
			buf[i] = (ci[i].Char.UnicodeChar & 0xFF) + ((ci[i].Attributes & 0xFF) << 8);
		#else
			buf[i] = (ci[i].Char.AsciiChar & 0xFF) + ((ci[i].Attributes & 0xFF) << 8);
		#endif
	}
	coniox_free_charinfo(ci);
	return(1);
}


/* ----------------------------------------------------------------------------------------------------------------- */
int movetext(int __left, int __top, int __right, int __bottom, int __destleft, int __desttop)
{
	COORD s, c = { 0, 0 };
	SMALL_RECT r;
	CHAR_INFO *scratch;
	int w, h, total;

	coniox_init(NULL);

	/* Validacion estilo Borland: origen y destino dentro de la pantalla. */
	if (__left < 1 || __top < 1 || __right < __left || __bottom < __top ||
		__right > ti.screenwidth || __bottom > ti.screenheight)
	{
		return(0);
	}
	if (__destleft < 1 || __desttop < 1 ||
		__destleft + (__right - __left) > ti.screenwidth ||
		__desttop + (__bottom - __top) > ti.screenheight)
	{
		return(0);
	}

	w = __right - __left + 1;
	h = __bottom - __top + 1;
	total = w * h;
	scratch = coniox_charinfo_scratch(total);
	if (scratch == NULL)
	{
		return(0);
	}

	r.Left = (short) __left - 1;
	r.Top = (short) __top - 1;
	r.Right = (short) __right - 1;
	r.Bottom = (short) __bottom - 1;
	s.X = (short) w;
	s.Y = (short) h;
	if (!ReadConsoleOutput(coniox_console_output, scratch, s, c, &r))
	{
		coniox_free_charinfo(scratch);
		return(0);
	}
	r.Left = (short) __destleft - 1;
	r.Top = (short) __desttop - 1;
	r.Right = (short) (__destleft + w - 2);
	r.Bottom = (short) (__desttop + h - 2);
	WriteConsoleOutput(coniox_console_output, scratch, s, c, &r);
	coniox_free_charinfo(scratch);
	return(1);
}


/* ----------------------------------------------------------------------------------------------------------------- */
int puttext(int __left, int __top, int __right, int __bottom, void *__source)
{
	int i;
	int total;
	SMALL_RECT r;
	short *ci;
	COORD s, c = { 0,0 };
	CHAR_INFO *buffer;


	coniox_init(NULL);
	/* Validacion estilo Borland: rectangulo dentro de la pantalla absoluta. */
	if (__left < 1 || __top < 1 || __right > ti.screenwidth || __bottom > ti.screenheight || __right < __left || __bottom < __top)
	{
		return(0);
	}
	total = (__right - __left + 1) * (__bottom - __top + 1);
	buffer = coniox_charinfo_scratch(total);
	if (buffer == NULL)
	{
		return(0);
	}

	r.Left = (short) __left - 1;
	r.Top = (short) __top - 1;
	r.Right = (short) __right - 1;
	r.Bottom = (short) __bottom - 1;
	s.X = (short) (__right - __left + 1);
	s.Y = (short) (__bottom - __top + 1);

	ci = (short *) __source;
	for (i = 0; i < s.X * s.Y; i++)
	{
		#if UNICODE
			buffer[i].Char.UnicodeChar = (wchar_t) (ci[i] & 0xFF);
		#else
			buffer[i].Char.AsciiChar = (unsigned char) ci[i] & 0xFF;
		#endif
		buffer[i].Attributes = (ci[i] >> 8) & 0xFF;
	}
	WriteConsoleOutput(coniox_console_output, buffer, s, c, &r);
	coniox_free_charinfo(buffer);
	return(1);
}



/* ----------------------------------------------------------------------------------------------------------------- */
int putwtext(int __left, int __top, int __right, int __bottom, const wchar_info *__source)
{
	int i;
	int total;
	SMALL_RECT r;
	COORD s, c = { 0,0 };
	CHAR_INFO *buffer;


	coniox_init(NULL);
	/* Validacion estilo Borland: rectangulo dentro de la pantalla absoluta. */
	if (__left < 1 || __top < 1 || __right > ti.screenwidth || __bottom > ti.screenheight || __right < __left || __bottom < __top)
	{
		return(0);
	}
	total = (__right - __left + 1) * (__bottom - __top + 1);
	buffer = coniox_charinfo_scratch(total);
	if (buffer == NULL)
	{
		return(0);
	}

	r.Left = (short)__left - 1;
	r.Top = (short)__top - 1;
	r.Right = (short)__right - 1;
	r.Bottom = (short)__bottom - 1;
	s.X = (short)(__right - __left + 1);
	s.Y = (short)(__bottom - __top + 1);

	for (i = 0; i < s.X * s.Y; i++)
	{
		#if UNICODE
			buffer[i].Char.UnicodeChar = __source[i].letter;
		#else
			buffer[i].Char.AsciiChar = (char) __source[i].letter;
		#endif
		buffer[i].Attributes = __source[i].attr;
	}
	WriteConsoleOutput(coniox_console_output, buffer, s, c, &r);
	coniox_free_charinfo(buffer);
	return(1);
}


/* ----------------------------------------------------------------------------------------------------------------- */
int getwtext(int __left, int __top, int __right, int __bottom, wchar_info *__destin)
{
	int i;
	int total;
	SMALL_RECT r;
	COORD s, c = { 0,0 };
	CHAR_INFO *buffer;


	coniox_init(NULL);
	/* Validacion estilo Borland: rectangulo dentro de la pantalla absoluta. */
	if (__left < 1 || __top < 1 || __right > ti.screenwidth || __bottom > ti.screenheight || __right < __left || __bottom < __top)
	{
		return(0);
	}
	total = (__right - __left + 1) * (__bottom - __top + 1);
	buffer = coniox_charinfo_scratch(total);
	if (buffer == NULL)
	{
		return(0);
	}
	r.Left = (short) (__left - 1);
	r.Top = (short) (__top - 1);
	r.Right = (short) (__right - 1);
	r.Bottom = (short) (__bottom - 1);
	s.X = (short) (__right - __left + 1);
	s.Y = (short) (__bottom - __top + 1);

	if (!ReadConsoleOutput(coniox_console_output, buffer, s, c, &r ))
	{
		coniox_free_charinfo(buffer);
		return(0);      /* W-2: sin la lectura no hay datos validos que devolver */
	}
	for (i = 0; i < s.X * s.Y; i++)
	{
		#if UNICODE
			__destin[i].letter = buffer[i].Char.UnicodeChar;
		#else
			__destin[i].letter = (char) buffer[i].Char.AsciiChar;
		#endif
		__destin[i].attr = buffer[i].Attributes;
	}
	coniox_free_charinfo(buffer);
	return(1);
}


/* ----------------------------------------------------------------------------------------------------------------- */
void textmode(int __newmode)
{
	#if __ZTC__
		CONSOLE_SCREEN_BUFFER_INFO info;
	#else
		CONSOLE_SCREEN_BUFFER_INFOEX info;
	#endif
	SMALL_RECT r;


	coniox_init(NULL);

	#if __ZTC__
		GetConsoleScreenBufferInfo(coniox_console_output, &info);
	#else
		info.cbSize = sizeof(info);
		GetConsoleScreenBufferInfoEx(coniox_console_output, &info);
	#endif

	if (__newmode == LASTMODE)
	{
		__newmode = ti.currmode;
	}
	if (__newmode != ti.currmode)
	{
		/*
		 * W-S2: la tabla sustituye al switch de ~80 lineas; mismo orden,
		 * gana la primera coincidencia. Cualquier modo no listado (el
		 * default del switch anterior) cae a 80x25.
		 */
		{
			static const struct { int mode; short w, h; } coniox_modes[] =
			{
				{ C40X14, 40, 14 }, { BW40X14, 40, 14 },
				{ C40X21, 40, 21 }, { BW40X21, 40, 21 },
				{ C40X28, 40, 28 }, { BW40X28, 40, 28 },
				{ C40X43, 40, 43 }, { BW40X43, 40, 43 },
				{ C40X50, 40, 50 }, { BW40X50, 40, 50 },
				{ C40X60, 40, 60 }, { BW40X60, 40, 60 },
				{ MONO14, 80, 14 }, { C80X14, 80, 14 }, { BW80X14, 80, 14 },
				{ MONO21, 80, 21 }, { C80X21, 80, 21 }, { BW80X21, 80, 21 },
				{ MONO28, 80, 28 }, { C80X28, 80, 28 }, { BW80X28, 80, 28 },
				{ MONO43, 80, 43 }, { C80X43, 80, 43 }, { BW80X43, 80, 43 },
				{ MONO50, 80, 50 }, { C80X50, 80, 50 }, { BW80X50, 80, 50 },
				{ MONO60, 80, 60 }, { C80X60, 80, 60 }, { BW80X60, 80, 60 },
				{ C4350,  80, 50 },
				{ BW40,   40, 25 }, { C40,    40, 25 },
				{ BW80,   80, 25 }, { C80,    80, 25 },
				{ _ORIGMODE, 80, 25 }, { MONO, 80, 25 }
			};
			unsigned int mi;

			for (mi = 0; mi < sizeof(coniox_modes) / sizeof(coniox_modes[0]); mi++)
			{
				if (coniox_modes[mi].mode == __newmode)
				{
					info.dwSize.X = coniox_modes[mi].w;
					info.dwSize.Y = coniox_modes[mi].h;
					break;
				}
			}
			if (mi == sizeof(coniox_modes) / sizeof(coniox_modes[0]))
			{
				/* Modo no catalogado: el default del switch anterior. */
				info.dwSize.X = 80;
				info.dwSize.Y = 25;
			}
		}
		//DigitalMars crashes if used
		#if (!__ZTC__)
			if (SetConsoleScreenBufferInfoEx(coniox_console_output, &info))
			{
				r.Left = 0;
				r.Top = 0;
				r.Right = info.dwSize.X - 1;
				r.Bottom = info.dwSize.Y - 1;
				SetConsoleWindowInfo(coniox_console_output, TRUE, &r);
				ti.normattr = info.wAttributes;
				ti.winleft = 1;
				ti.wintop = 1;
				ti.winright = info.dwSize.X;
				ti.winbottom = info.dwSize.Y;
				ti.screenwidth = info.dwSize.X;
				ti.screenheight = info.dwSize.Y;
				ti.curx = info.dwCursorPosition.X + 1;
				ti.cury = info.dwCursorPosition.Y + 1;
				ti.attribute = info.wAttributes;
				ti.currmode = (unsigned short)__newmode;
			}
		#endif
	}
	clrscr();
	_setcursortype(_NORMALCURSOR);
}


/* ----------------------------------------------------------------------------------------------------------------- */
void _setcursortype(int __cur_t)
{
	CONSOLE_CURSOR_INFO cci;

	coniox_init(NULL);
	coniox_setcursortype = __cur_t;
	switch(__cur_t)
	{
		case _NOCURSOR:
			cci.dwSize = 100;
			cci.bVisible = FALSE;
			break;
		case _SOLIDCURSOR:
			cci.dwSize = 100;
			cci.bVisible = TRUE;
			break;
		case _NORMALCURSOR:
		default:
			cci.dwSize = 10;
			cci.bVisible = TRUE;
	}
	SetConsoleCursorInfo(coniox_console_output, &cci);

	/* Gemelo del bug DOS: gotoxy() no mueve el cursor fisico mientras esta
	   oculto (_NOCURSOR), asi que al reactivarlo hay que reposicionarlo. */
	if (__cur_t != _NOCURSOR)
	{
		gotoxy(ti.curx, ti.cury);
	}
}



/* ----------------------------------------------------------------------------------------------------------------- */
int kbhit(void)
{
	DWORD nevents = 0;
	INPUT_RECORD ir;


	coniox_init(NULL);
	coniox_flush_cursor();

	while (1)
	{
		if (!GetNumberOfConsoleInputEvents(coniox_console_input, &nevents) || nevents == 0)
		{
			return(0);
		}
		if (!PeekConsoleInput(coniox_console_input, &ir, 1, &nevents) || nevents == 0)
		{
			return(0);
		}
		if (ir.EventType == KEY_EVENT && ir.Event.KeyEvent.bKeyDown)
		{
			return(1);
		}
		if (!ReadConsoleInput(coniox_console_input, &ir, 1, &nevents) || nevents == 0)
		{
			return(0);
		}
		if (ir.EventType == WINDOW_BUFFER_SIZE_EVENT)
		{
			ti.screenwidth = ir.Event.WindowBufferSizeEvent.dwSize.X;
			ti.screenheight = ir.Event.WindowBufferSizeEvent.dwSize.Y;
			if (ti.winright > ti.screenwidth)
			{
				ti.winright = ti.screenwidth;
			}
			if (ti.winbottom > ti.screenheight)
			{
				ti.winbottom = ti.screenheight;
			}
			if (ti.winleft > ti.winright)
			{
				ti.winleft = ti.winright;
			}
			if (ti.wintop > ti.winbottom)
			{
				ti.wintop = ti.winbottom;
			}
			if (!coniox_inwindow(ti.winleft + ti.curx - 1, ti.wintop + ti.cury - 1 ))
			{
				gotoxy(1, 1);
			}
		}
	}
}


/* ----------------------------------------------------------------------------------------------------------------- */
int ungetch(int __ch)
{
	INPUT_RECORD ir[2];
	DWORD written;


	coniox_init(NULL);
	ir[0].EventType = KEY_EVENT;
	ir[0].Event.KeyEvent.bKeyDown = TRUE;
	ir[0].Event.KeyEvent.wRepeatCount = 1;
	ir[0].Event.KeyEvent.wVirtualKeyCode = 0;
	ir[0].Event.KeyEvent.wVirtualScanCode = 0;
	ir[0].Event.KeyEvent.uChar.AsciiChar = (CHAR) __ch;
	ir[0].Event.KeyEvent.dwControlKeyState = 0;
	ir[1].EventType = KEY_EVENT;
	ir[1].Event.KeyEvent.bKeyDown = FALSE;
	ir[1].Event.KeyEvent.wRepeatCount = 1;
	ir[1].Event.KeyEvent.wVirtualKeyCode = 0;
	ir[1].Event.KeyEvent.wVirtualScanCode = 0;
	ir[1].Event.KeyEvent.uChar.AsciiChar = (CHAR) __ch;
	ir[1].Event.KeyEvent.dwControlKeyState = 0;
	if (WriteConsoleInput(coniox_console_input, ir, 2, &written) && written == 2)
	{
		return(__ch);
	}
	return(EOF);
}


/* ----------------------------------------------------------------------------------------------------------------- */
wchar_t ungetwch(wchar_t __ch)
{
	INPUT_RECORD ir[2];
	DWORD written;

	coniox_init(NULL);
	ir[0].EventType = KEY_EVENT;
	ir[0].Event.KeyEvent.bKeyDown = TRUE;
	ir[0].Event.KeyEvent.wRepeatCount = 1;
	ir[0].Event.KeyEvent.wVirtualKeyCode = 0;
	ir[0].Event.KeyEvent.wVirtualScanCode = 0;
	ir[0].Event.KeyEvent.uChar.UnicodeChar = __ch;
	ir[0].Event.KeyEvent.dwControlKeyState = 0;
	ir[1].EventType = KEY_EVENT;
	ir[1].Event.KeyEvent.bKeyDown = FALSE;
	ir[1].Event.KeyEvent.wRepeatCount = 1;
	ir[1].Event.KeyEvent.wVirtualKeyCode = 0;
	ir[1].Event.KeyEvent.wVirtualScanCode = 0;
	ir[1].Event.KeyEvent.uChar.UnicodeChar = __ch;
	ir[1].Event.KeyEvent.dwControlKeyState = 0;
	if (WriteConsoleInput(coniox_console_input, ir, 2, &written) && written == 2)
	{
		return(__ch);
	}
	return((wchar_t) EOF);
}



/* ----------------------------------------------------------------------------------------------------------------- */
void delay(unsigned int ms)
{
	coniox_flush_cursor();  /* C: el cursor fisico al dia durante la espera */
	Sleep(ms);
}

#endif /* ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32) */





/* ----------------------------------------------------------------------------------------------------------------- */
/* DOS ONLY (Non portable) */
/* ----------------------------------------------------------------------------------------------------------------- */
#if ((__DOS__) || (__MSDOS__))


/* ----------------------------------------------------------------------------------------------------------------- */
#if defined(__WATCOMC__)
	void coniox_fmemsetw(void coniox_far *m, short val, size_t count);
	void coniox_fmemcpy(void coniox_far *dst, const void coniox_far *src, size_t count);
	#if (defined(__FLAT__))
		void coniox_putrun(unsigned short *piDst, const char *piSrc, unsigned int piAttr, size_t piCount);
	#else
		void coniox_putrun(unsigned short coniox_far *piDst, const char *piSrc, unsigned int piAttr, size_t piCount);
	#endif
	#if (defined(__FLAT__))
		#pragma aux coniox_fmemsetw =                                   \
			"           movzx   edx, ax                                 "       \
			"           shl             eax, 16                                 "       \
			"           or              eax, edx                                "       \
			"           shr             ecx, 1                                  "       \
			"           cld                                                             "       \
			"           rep             stosd                                   "       \
			"           jnc             short no_odd                            "       \
			"           stosw                                                   "       \
			"no_odd:                                                                    "       \
			parm [EDI][AX][ECX]                                                 \
			modify exact [EDI EDX ECX EAX];
		#pragma aux coniox_putrun =                                     \
			"           mov             edx, ebx                                "       \
			"           and             edx, 0FF00h                             "       \
			"           mov             ebx, edx                                "       \
			"           shl             ebx, 16                                 "       \
			"           or              ebx, edx                                "       \
			"           test    ecx, 1                                  "       \
			"           jz              _preven                                 "       \
			"           movzx   eax, byte ptr [esi]             "       \
			"           inc             esi                                             "       \
			"           or              eax, edx                                "       \
			"           stosw                                                   "       \
			"           dec             ecx                                             "       \
			"_preven:                                                                   "       \
			"           shr             ecx, 1                                  "       \
			"           jecxz   _prdone                                 "       \
			"_prpairs:                                                                  "       \
			"           movzx   eax, byte ptr [esi]             "       \
			"           movzx   edx, byte ptr [esi+1]   "       \
			"           add             esi, 2                                  "       \
			"           shl             edx, 16                                 "       \
			"           or              eax, edx                                "       \
			"           or              eax, ebx                                "       \
			"           stosd                                                   "       \
			"           loop    _prpairs                                "       \
			"_prdone:                                                                   "       \
			parm [EDI][ESI][EBX][ECX]                                   \
			modify exact [EDI ESI ECX EDX EAX EBX];
		#pragma aux coniox_fmemcpy =                                    \
			"           mov             ecx, ebx                                "       \
			"           shr             ecx, 2                                  "       \
			"           cld                                                             "       \
			"           rep             movsd                                   "       \
			"           mov             ecx, ebx                                "       \
			"           and             ecx, 3                                  "       \
			"           rep             movsb                                   "       \
			parm [EDI][ESI][EBX]                                                \
			modify exact [ECX EDI ESI];
	#else
		#pragma aux coniox_fmemsetw =                                   \
			"           cld                                                             "       \
			"           rep             stosw                                   "       \
			parm [ES DI][AX][CX]                                                \
			modify exact [DI CX];
		#pragma aux coniox_putrun =                                     \
			"           cld                                                             "       \
			"_prnext:                                                                   "       \
			"           lodsb                                                   "       \
			"           mov             ah, bh                                  "       \
			"           stosw                                                   "       \
			"           loop    _prnext                         "       \
			parm [ES DI][SI][BX][CX]                                    \
			modify exact [DI SI CX AX];
		#pragma aux coniox_fmemcpy =                                    \
			"           push    ds                                              "       \
			"           mov             ds, dx                                  "       \
			"           cld                                                             "       \
			"           shr             cx, 1                                   "       \
			"           rep             movsw                                   "       \
			"           jnc             short _fcpy_done                        "       \
			"           movsb                                                   "       \
			"_fcpy_done:                                                                "       \
			"           pop             ds                                              "       \
			parm [ES DI][DX SI][CX]                                     \
			modify exact [CX SI DI];
	#endif
#else
	#define coniox_fmemcpy      _fmemcpy
	void coniox_fmemsetw(void coniox_far *m, short val, size_t count)
	{
		short coniox_far *buf = (short coniox_far *) m;

		while (count >= 4)
		{
			buf[0] = val;
			buf[1] = val;
			buf[2] = val;
			buf[3] = val;
			buf += 4;
			count -= 4;
		}
		while (count--)
		{
			*buf++ = val;
		}
	}

	void coniox_putrun(unsigned short coniox_far *piDst, const char coniox_far *piSrc, unsigned int piAttr, unsigned int piCount)
	{
		unsigned short coniox_far *d;
		unsigned short cellword;

		d = piDst;
		cellword = (unsigned short) (piAttr & 0xFF00);
		while (piCount >= 4)
		{
			d[0] = cellword | (unsigned char) piSrc[0];
			d[1] = cellword | (unsigned char) piSrc[1];
			d[2] = cellword | (unsigned char) piSrc[2];
			d[3] = cellword | (unsigned char) piSrc[3];
			d += 4;
			piSrc += 4;
			piCount -= 4;
		}
		while (piCount--)
		{
			*d++ = cellword | (unsigned char) *piSrc++;
		}
	}
#endif

/* ----------------------------------------------------------------------------------------------------------------- */
void coniox_init(const void* title)
{
	unsigned int cursor;
	union REGS r;

	(void) title;                                   /* W303: suppress unused parameter warning */

	/* Check if already initialized */
	/* C: guardia sin helper y sin flag escondido: screenwidth solo es 0 antes de
	   la primera coniox_init(), y textmode() lo repone a 0 para reinicializar. */
	if (ti.screenwidth != 0)
	{
		return;
	}
        
        
	#if (defined(__DJGPP__))
		__djgpp_nearptr_enable();
	#endif

	/* Get current video mode */
	if (ti.currmode != C4350)
	{
		if (directvideo)
		{
			ti.currmode = peekb(0x40, 0x49);
		}
		else
		{
			r.h.ah = 0xF;
			coniox_int86(0x10, &r, &r);
			ti.currmode = r.h.al;
		}
	}

	#if (defined(__FLAT__))
		if (ti.currmode == MONO)
		{
			coniox_vram = (unsigned short *) 0x000B0000UL;
		}
		else
		{
			coniox_vram = (unsigned short *) 0x000B8000UL;
		}
	#elif (defined(__DJGPP__))
		if (ti.currmode == MONO)
		{
			coniox_vram = (unsigned short *) 0x000B0000UL +  __djgpp_conventional_base;
		}
		else
		{
			coniox_vram = (unsigned short *) 0x000B8000UL +  __djgpp_conventional_base;
		}
	#else
		if (ti.currmode == MONO)
		{
			coniox_vram = MK_FP(0xB000, 0);
		}
		else
		{
			coniox_vram = MK_FP(0xB800, 0);

		}
	#endif

	/* Base for 6845 CRT controller: keep the default 0x3D4 even if the adapter
	   is monochrome (BDA 0040:0063 = 0x3B4). Programming the cursor through
	   3B4h leaves it broken on emulators and some VGA boards. */
	//coniox_basecrt = peekw(0x40, 0x63);    /* 0x3D4 */

	/* Get cursor position */
	if (directvideo)
	{
		cursor = peekw(0x40, 0x50);
		ti.curx = (cursor & 0xFF) + 1;
		ti.cury = ((cursor >> 8) & 0xFF) + 1;
	}
	else
	{
		r.h.ah = 0x3;
		r.h.bh = 0;
		coniox_int86(0x10, &r, &r);
		ti.curx = r.h.dl + 1;
		ti.cury = r.h.dh + 1;
	}

	/* Get screen mode */
	if (directvideo)
	{
		ti.screenwidth = peekw(0x40, 0x4A);
		if (ti.currmode == C4350)
		{
			ti.screenheight = 50;
		}
		else
		{
			ti.screenheight = peekb(0x40, 0x84) + 1;
		}
	}
	else
	{
		r.h.ah = 0xF;
		coniox_int86(0x10, &r, &r);
		ti.screenwidth = r.h.ah;
		if (ti.currmode == C4350)
		{
			ti.screenheight = 50;
		}
		else
		{
			ti.screenheight = 25;   /* Todo: Fill rows */
		}
	}
	/* BDA 0040:0084 is only meaningful on EGA/VGA-class systems.
	   Reject implausible heights (including the pre-EGA value 1). */
	if (ti.screenheight < 2 || ti.screenheight > 128)
	{
		ti.screenheight = 25;
	}
	ti.normattr = 7;        
	ti.attribute = 7;
	ti.winleft = 1;
	ti.wintop = 1;
	ti.winright = ti.screenwidth;
	ti.winbottom = ti.screenheight;
	/* D: posicion inicial de la cache de celda de putch()/cputs(): la celda
	   (curx, cury) recien leida. textmode() re-pasa por aqui al reponer el
	   modo; gotoxy() la mantiene a partir de entonces. */
	coniox_cell = coniox_offset(ti.winleft + ti.curx - 2, ti.wintop + ti.cury - 2);
        
        
	//Get if under emulator
	coniox_is_emulator = coniox_get_is_emulator();
        
	//Activate blink
	coniox_blink(1);
	coniox_enhanced_palette(1);
}


/* ----------------------------------------------------------------------------------------------------------------- */
void coniox_readnormattr(void)
{
	union REGS r;

	/*
		C: atributo REAL de la pantalla para ti.normattr, leyendo la celda del
		cursor logico: VRAM con directvideo=1, INT 10h AH=08h con directvideo=0
		(sincronizando antes el cursor fisico diferido). Es el mismo mecanismo
		que el conio clasico usa para obtener normattr al arrancar.
	*/
	if (directvideo)
	{
		ti.normattr = (unsigned short)((*coniox_offset(ti.winleft + ti.curx - 2, ti.wintop + ti.cury - 2) >> 8) & 0xFF);
	}
	else
	{
		coniox_flush_cursor();
		r.h.ah = 0x8;
		r.h.bh = 0;
		coniox_int86(0x10, &r, &r);
		ti.normattr = r.h.ah;
	}
}



/* ----------------------------------------------------------------------------------------------------------------- */
int cputs(const char *__str)
{
	const char *orig = __str;
	const char *run;
	unsigned int i;
	unsigned int runlen, remaining;
	unsigned int winwidth, winheight;
	int oldx, oldy;
	unsigned short attrword;
	union REGS r;

	#if ((defined(__FLAT__)) || (defined(__DJGPP__)))
		unsigned short *coniox_currentoffset;
	#else
		unsigned short coniox_far *coniox_currentoffset;
	#endif

	if (__str == NULL)
	{
		return 0;
	}

	coniox_init(NULL);

	winwidth  = (unsigned int)(ti.winright - ti.winleft + 1);
	winheight = (unsigned int)(ti.winbottom - ti.wintop + 1);

	if (winwidth == 0 || winheight == 0)
	{
		return 0;
	}

	attrword = (unsigned short)(ti.attribute << 8);

	if (directvideo)
	{
		/* D: la cache de putch()/gotoxy() ya apunta a la celda (curx, cury):
		   se ahorra el coniox_offset() de entrada; al salir se re-sincroniza. */
		coniox_currentoffset = coniox_cell;
	}

	while (*__str)
	{
		/* Carriage return: regresar al inicio de la l?nea. */
		if (*__str == '\r')
		{
			if (directvideo)
			{
				coniox_currentoffset -= ti.curx - 1;
			}
			ti.curx = 1;
			++__str;
			continue;
		}

		/* Line feed: avanzar o desplazar la ventana. */
		if (*__str == '\n')
		{
			if (ti.cury < (int)winheight)
			{
				if (directvideo)
				{
					coniox_currentoffset += ti.screenwidth;
				}
				ti.cury++;
			}
			else if (_wscroll)
			{
				/* LF: keep the column, like Borland cputs. */
				oldx = ti.curx;
				oldy = ti.cury;
				ti.curx = 1;
				ti.cury = 1;
				delline();
				ti.curx = oldx;
				ti.cury = oldy;
				if (directvideo)
				{
					coniox_currentoffset = coniox_offset(ti.winleft + ti.curx - 2, ti.wintop + ti.cury - 2);
				}
			}
			/* _wscroll = 0: LF on the last line does not scroll. */
			++__str;
			continue;
		}

		/* Backspace: retroceder sin borrar el car?cter. */
		if (*__str == '\b')
		{
			if (ti.curx > 1)
			{
				if (directvideo)
				{
					--coniox_currentoffset;
				}
				ti.curx--;
			}
			++__str;
			continue;
		}

		/*
		 * Agrupar caracteres imprimibles hasta el siguiente control
		 * o hasta el final de la l?nea de la ventana.
		 */
		run = __str;
		runlen = 0;
		if (ti.curx > (int)winwidth)
		{
			ti.curx = (int)winwidth;        /* la ventana se encogio: evitar underflow en remaining */
			if (directvideo)
			{
				/* D: el offset seguia apuntando a la posicion real del cursor, ya
				   fuera de la ventana; re-sincronizarlo con el clamp para que el
				   run pinte dentro y el wrap aritmetico de abajo parta del sitio
				   correcto (la rama Windows siempre escribio en la celda clampada). */
				coniox_currentoffset = coniox_offset(ti.winleft + ti.curx - 2, ti.wintop + ti.cury - 2);
			}
		}
		remaining = winwidth - (unsigned int)ti.curx + 1;

		while (runlen < remaining &&
			   run[runlen] != '\0' &&
			   run[runlen] != '\r' &&
			   run[runlen] != '\n' &&
			   run[runlen] != '\b')
		{
			runlen++;
		}

		if (directvideo)
		{
			/* Ruta r?pida: el run completo directo a VRAM. */
			coniox_putrun(coniox_currentoffset, run, attrword, runlen);
			coniox_currentoffset += runlen;
		}
		else
		{
			/* Ruta BIOS: AH=02h posiciona, AH=09h escribe. */
			for (i = 0; i < runlen; i++)
			{
				r.h.ah = 0x02;
				r.h.bh = 0;
				r.h.dh = (unsigned char)(ti.wintop + ti.cury - 2);
				r.h.dl = (unsigned char)(ti.winleft + ti.curx - 2 + i);
				coniox_int86(0x10, &r, &r);

				/* Restaurar los registros que AH=09h exige. */
				r.h.ah = 0x09;
				r.h.al = (unsigned char) run[i];
				r.h.bh = 0;
				r.h.bl = (unsigned char) ti.attribute;
				#if defined(__WATCOMC__)
					r.w.cx = 1;
				#else
					r.x.cx = 1;
				#endif
				coniox_int86(0x10, &r, &r);
			}
		}
		ti.curx += runlen;

		/* El run nunca cruza el borde de la ventana: el wrap se decide una sola vez. */
		if (ti.curx > (int)winwidth)
		{
			if (_wscroll)
			{
				if (ti.cury < (int)winheight)
				{
					ti.curx = 1;
					ti.cury++;
					if (directvideo)
					{
						/* D: el offset acaba en la columna winwidth+1 de la fila actual
						   (uno pasado el ultimo store); la nueva celda (1, cury+1) es
						   aritmetica pura, sin repetir el imul de coniox_offset(). */
						coniox_currentoffset += ti.screenwidth - winwidth;
					}
				}
				else
				{
					ti.curx = 1;
					ti.cury = 1;
					delline();
					ti.cury = (int)winheight;
					if (directvideo)
					{
						coniox_currentoffset = coniox_offset(ti.winleft + ti.curx - 2, ti.wintop + ti.cury - 2);
					}
				}
			}
			else
			{
				ti.curx = (int)winwidth;    /* _wscroll = 0: stay on the last column, the next run overwrites the edge cell */
				if (directvideo)
				{
					coniox_currentoffset = coniox_offset(ti.winleft + ti.curx - 2, ti.wintop + ti.cury - 2);
				}
			}
		}
		__str += runlen;
	}
	/* C: sync diferido del cursor fisico, mismo patron que putch(): ti ya
	   lleva la posicion logica y el cursor real se programa en el proximo
	   punto de espera (kbhit/getch/getche/delay) via coniox_flush_cursor().
	   Antes cada cputs()/cprintf() pagaba 1 pokew a la BDA + 4 outportb al
	   CRTC al terminar: I/O de puerto lenta incluso en un 386. */
	coniox_cursor_dirty = 1;
	/* D: deja la cache de celda apuntando a (curx, cury) para el proximo
	   putch()/cputs(): en la ruta directa el propio offset ya coincide; en
	   la BIOS se recalcula una vez por llamada, no por caracter. */
	if (directvideo)
	{
		coniox_cell = coniox_currentoffset;
	}
	else
	{
		coniox_cell = coniox_offset(ti.winleft + ti.curx - 2, ti.wintop + ti.cury - 2);
	}
	return (int)(__str - orig);
}



/* ----------------------------------------------------------------------------------------------------------------- */
coniox_inline void coniox_putchxyattr(int x, int y, int ch, int attr)
{
	if (directvideo)
	{
		*(unsigned short coniox_far *) coniox_offset(x - 1, y - 1) = (attr << 8) | (ch & 0xFF);
	}
	else
	{
		union REGS r;
		r.h.ah = 2;
		r.h.bh = 0;
		r.h.dh = y - 1;
		r.h.dl = x - 1;
		coniox_int86(0x10, &r, &r);

		r.h.ah = 0x9;
		r.h.al = ch;
		r.h.bh = 0;
		r.h.bl = attr;
		#if defined(__WATCOMC__)
			r.w.cx = 1;
		#else
			r.x.cx = 1;
		#endif
		coniox_int86(0x10, &r, &r);
	}
}


/* ----------------------------------------------------------------------------------------------------------------- */
void coniox_putchxyattrwh(int x, int y, int ch, int attr, int w, int h)
{
	unsigned short coniox_far* p;
	int y1, ylim;
	unsigned short v;
	unsigned int sw;
	union REGS r;

	if (w <= 0 || h <= 0)
	{
		return;
	}

	if (directvideo)
	{
		v = (unsigned short)((attr << 8) | (ch & 0xFF));
		// fast-path 1x1 -> putcell evita fmemsetw + loop
		if ((w == 1) && (h == 1))
		{
			*(unsigned short coniox_far *) coniox_offset(x - 1, y - 1) = v;
			return;
		}
		sw = ti.screenwidth;    /* D: hoisting: tambien el bucle w == 1 usa el stride */
		if (w == 1)
		{
			p = (unsigned short coniox_far*) coniox_offset(x - 1, y - 1);
			for (y1 = y; y1 < y + h; y1++)
			{
				*p = v;
				p += sw;
			}
			return;
		}
		p = (unsigned short coniox_far*) coniox_offset(x - 1, y - 1);
		ylim = y + h;
		if ((int) sw == w)
		{
			/* Bloque a lo ancho completo de la pantalla: las filas son
			   contiguas en VRAM y un solo fill cubre las h filas, el
			   mismo caso que clrscr() ya trata aparte. clreol() sobre
			   ventana completa pasa por aqui. */
			coniox_fmemsetw(p, v, (size_t) sw * (size_t) h);
			return;
		}
		for (y1 = y; y1 < ylim; y1++)
		{
			coniox_fmemsetw(p, v, w);
			p += sw;
		}
	}
	else
	{
		ylim = y + h;
		for (y1 = y; y1 < ylim; y1++)
		{
			r.h.ah = 0x02;
			r.h.bh = 0;
			r.h.dh = (unsigned char)(y1 - 1);
			r.h.dl = (unsigned char)(x - 1);
			coniox_int86(0x10, &r, &r);

			/* Restore all inputs for AH=09h after the cursor-position call. */
			r.h.ah = 0x09;
			r.h.al = (unsigned char)ch;
			r.h.bh = 0;
			r.h.bl = (unsigned char)attr;
			#if defined(__WATCOMC__)
				r.w.cx = (unsigned short)w;
			#else
				r.x.cx = (unsigned short)w;
			#endif
			coniox_int86(0x10, &r, &r);
		}
	}
}



/* ----------------------------------------------------------------------------------------------------------------- */
void coniox_idle(void)
{
	union REGS r;

	/*
		This interrupt was undocumented prior to DOS 5.0, but it has been
		supported without change since DOS 2.0.  It lets programs such as the
		DOS Print command carry out background tasks while the system awaits
		keyboard input.
		Programs that are idle (polling for input) can issue INT 28H or call
		INT 2FH 1680H to free up a timeslice so that background tasks can operate.
		DOS itself calls INT 28H during fns 01H through 0cH (DOS I/O services)
		while it is awaiting user response.
	*/
	memset(&r, 0, sizeof(r));
	coniox_int86(0x28, &r, &r);
        
        
	/*
		Programs can use this function in idle loops to enhance performance
		under multitaskers; this call is supported by MS Windows 3+, DOS 5+,
		DPMI 1.0+, and in OS/2 2.0+ for multitasking DOS applications
		does not block the program; it just gives up the remainder of the time
		slice
		should not be used by Windows-specific programs
		when called very often without intermediate screen output under
		MS Windows 3.x, the VM will go into an idle-state and will not
		receive the next slice before 8 seconds have elapsed. This time can
		be changed in SYSTEM.INI through "IdleVMWakeUpTime=<seconds>".
		Setting it to zero results in a long wait.
		this function has no effect under OS/2 2.10-4.0 if the DOS box has an
		"Idle Sensitivity" setting of 100
	*/
	#if defined(__WATCOMC__)
		r.w.ax = 0x1680;
	#else
		r.x.ax = 0x1680;
	#endif
	coniox_int86(0x2F, &r, &r);
        

	/*
		Under DESQview, if the process issuing this call has hooked INT 08h,
		the current time-slice is set to expire at the next clock tick rather
		than immediately
	*/
	#if defined(__WATCOMC__)
		r.w.ax = 0x1000;
	#else
		r.x.ax = 0x1000;
	#endif
	coniox_int86(0x15, &r, &r);

}


/* ----------------------------------------------------------------------------------------------------------------- */
int coniox_get_is_emulator (void)
{
	unsigned char result;

	outportb(0x04F4, 0x00);
	result = inportb(0x04F4);
	return (result == 0xFF);
}
        

/* ----------------------------------------------------------------------------------------------------------------- */
void coniox_enhanced_palette(unsigned int pal)
{
	unsigned int i;
	const unsigned char (*coniox_rgb)[3];
	static const unsigned char coniox_palette[16][3] =
	{
		{0x00, 0x00, 0x00},     /*  0: Negro */
		{0x00, 0x00, 0x24},     /*  1: Azul (m�s oscuro que el VGA por defecto) */
		{0x00, 0x24, 0x04},     /*  2: Verde (m�s oscuro, tinte azulado) */
		{0x00, 0x22, 0x28},     /*  3: Cian (m�s oscuro, menos verde) */
		{0x24, 0x00, 0x00},     /*  4: Rojo (m�s oscuro) */
		{0x28, 0x08, 0x2A},     /*  5: Magenta (algo desaturado) */
		{0x30, 0x30, 0x08},     /*  6: Marr�n (oliva/amarillo oscuro, desv�a del VGA est�ndar) */
		{0x2B, 0x2B, 0x2B},     /*  7: Gris claro (pr�cticamente id�ntico al default) */
		{0x00, 0x00, 0x1D},     /*  8: Gris oscuro (se muestra azul marino, desv�a del VGA est�ndar) */
		{0x04, 0x08, 0x3C},     /*  9: Azul brillante (m�s oscuro y saturado) */
		{0x00, 0x28, 0x0E},     /* 10: Verde brillante (tirando a esmeralda) */
		{0x00, 0x1F, 0x3F},     /* 11: Cian claro (m�s profundo, sin el rojo lavado del default) */
		{0x28, 0x04, 0x10},     /* 12: Rojo claro (tinte burdeos) */
		{0x2C, 0x08, 0x3F},     /* 13: Magenta claro (tirando a violeta) */
		{0x30, 0x2A, 0x10},     /* 14: Amarillo (apagado, algo anaranjado) */
		{0x30, 0x30, 0x3F}      /* 15: Blanco (apagado, tinte fr�o azulado) */
	};
	static const unsigned char coniox_default[16][3] =
	{
		{0x00, 0x00, 0x00},     /*  0: Negro */
		{0x00, 0x00, 0x2A},     /*  1: Azul */
		{0x00, 0x2A, 0x00},     /*  2: Verde */
		{0x00, 0x2A, 0x2A},     /*  3: Cian */
		{0x2A, 0x00, 0x00},     /*  4: Rojo */
		{0x2A, 0x00, 0x2A},     /*  5: Magenta */
		{0x2A, 0x2A, 0x00},     /*  6: Marron */
		{0x2A, 0x2A, 0x2A},     /*  7: Gris claro */
		{0x00, 0x00, 0x15},     /*  8: Gris oscuro */
		{0x00, 0x00, 0x3F},     /*  9: Azul brillante */
		{0x00, 0x2A, 0x15},     /* 10: Verde brillante */
		{0x00, 0x2A, 0x3F},     /* 11: Cian claro */
		{0x2A, 0x00, 0x15},     /* 12: Rojo claro */
		{0x2A, 0x00, 0x3F},     /* 13: Magenta claro */
		{0x2A, 0x2A, 0x15},     /* 14: Amarillo */
		{0x2A, 0x2A, 0x3F}      /* 15: Blanco */
	};

	if (ti.currmode == MONO)
	{
		return;
	}

	/* pal = 0: paleta por defecto del VGA. pal != 0: paleta mejorada de CONIOX. */
	coniox_rgb = pal ? coniox_palette : coniox_default;

	if (directvideo)
	{
		outportb(0x3C8, 0x00);          /* DAC: escribir desde el registro 0 */
		for (i = 0; i < 16; i++)
		{
			outportb(0x3C9, coniox_rgb[i][0]);  /* R */
			outportb(0x3C9, coniox_rgb[i][1]);  /* G */
			outportb(0x3C9, coniox_rgb[i][2]);  /* B */
		}
	}
	else
	{
		union REGS r;
		for (i = 0; i < 16; i++)
		{
			#if defined(__WATCOMC__)
				r.w.ax = 0x1010;        /* Subfunci�n: establecer registro DAC */
				r.w.bx = i;             /* BX = �ndice de color (BH=0) */
			#else
				r.x.ax = 0x1010;
				r.x.bx = i;
			#endif
			r.h.dh = coniox_rgb[i][0];  /* R */
			r.h.ch = coniox_rgb[i][1];  /* G */
			r.h.cl = coniox_rgb[i][2];  /* B */
			coniox_int86(0x10, &r, &r);
		}
	}
}

/* ----------------------------------------------------------------------------------------------------------------- */
void coniox_blink(unsigned int blink)
{
	if ((directvideo) && (ti.currmode != MONO))
	{
		unsigned char v;
		unsigned int status = coniox_basecrt + 6;       /* 3DAh color, 3BAh mono */

		_disable();
		inportb(status);
		outportb(0x3C0, 0x10 | 0x20);
		v = inportb(0x3C1);
		if (blink)
		{
			v |= 0x08;
		}
		else
		{
			v &= ~0x08;
		}
		outportb(0x3C0, v);
		outportb(0x3C0, 0x20);          /* flip-flop en �ndice, v�deo encendido */
		_enable();
	}
	else
	{
		union REGS r;
		#if defined(__WATCOMC__)
			r.w.ax = 0x1003;
			r.w.bx = blink ? 1 : 0;
		#else
			r.x.ax = 0x1003;
			r.x.bx = blink ? 1 : 0;
		#endif
		coniox_int86(0x10, &r, &r);
	}
}


/* ----------------------------------------------------------------------------------------------------------------- */
void delay (unsigned int ms)
{
	unsigned long lTicks = 0;
	unsigned long lWait, lNow;

	coniox_flush_cursor();      /* C: el cursor fisico al dia durante la espera */

	lWait = (((unsigned long) ms * 91UL) + 4999UL) / 5000UL;        /* ms -> ticks a 18.2 Hz, redondeo arriba */

	if (directvideo)
	{
		unsigned long lIdleTick;

		lTicks = peekl(0x40, 0x6C);     /* BDA 0040:006C, contador de ticks */
		lIdleTick = lTicks - 1UL;   /* fuerza un coniox_idle() en la 1a vuelta */
		/* Resta modular: sobrevive al reinicio del contador a medianoche */
		while (lWait != 0)
		{
			lNow = peekl(0x40, 0x6C);

			if ((lNow - lTicks) >= lWait)
			{
				break;
			}
			/* coniox_idle() solo al cambiar el tick (18,2 Hz): lanzaba 3
			   INTs software (28h, 2Fh/1680h, 15h/1000h) en cada vuelta del
			   spin; el propio comentario de coniox_idle() avisa de que
			   llamarlo muy seguido penaliza bajo multitaskers. */
			if (lNow != lIdleTick)
			{
				lIdleTick = lNow;
				coniox_idle();
			}
		}
	}
	else
	{
		union REGS r;
		unsigned long lIdleTick = 0xFFFFFFFFUL;

		while (lWait)
		{
			r.h.ah = 0;                                 /* INT 1Ah AH=00h: leer ticks */
			coniox_int86(0x1A, &r, &r);
			#if defined(__WATCOMC__)
				lNow = ((unsigned long) r.w.cx << 16) | r.w.dx;
			#else
				lNow = ((unsigned long) r.x.cx << 16) | r.x.dx;
			#endif
			if (lTicks == 0)                            /* primera vuelta: fijar referencia */
			{
				lTicks = lNow;
			}
			if ((lNow - lTicks) >= lWait)
			{
				break;
			}
			/* idle solo al cambiar el tick: entre tick y tick basta con
			   el propio INT 1Ah del sondeo (antes: 3 INTs mas por vuelta). */
			if (lNow != lIdleTick)
			{
				lIdleTick = lNow;
				coniox_idle();
			}
		}
	}
}


/* ----------------------------------------------------------------------------------------------------------------- */
void textmode(int __newmode)
{
	union REGS r;

	if (__newmode == LASTMODE)
	{
		__newmode = ti.currmode;
	}
	if (__newmode != ti.currmode)
	{
		if (__newmode == C4350)
		{
			#if defined(__WATCOMC__)
				r.w.ax = C80;
			#else
				r.x.ax = C80;
			#endif
			coniox_int86(0x10, &r, &r);
			/* For 4350 additionally we need to load 8x8 EGA/VGA font */
			#if defined(__WATCOMC__)
				r.w.ax = 0x1112;
				r.w.bx = 0;
			#else
				r.x.ax = 0x1112;
				r.x.bx = 0;
			#endif
			coniox_int86(0x10, &r, &r);
			/* For 4350 select alternate print screen routine */
			#if defined(__WATCOMC__)
				r.w.ax = 0x1200;
			#else
				r.x.ax = 0x1200;
			#endif
			r.h.bl = 0x20;
			coniox_int86(0x10, &r, &r);
			pokeb(0x40, 0x87, peekb(0x40, 0x87) | 1);       /* Disable cursor size emultation */
			pokew(0x40, 0x4C, 8000);        /* Correct video buffer size */
		}
		else
		{
			#if defined(__WATCOMC__)
				r.w.ax = __newmode;
			#else
				r.x.ax = __newmode;
			#endif
			coniox_int86(0x10, &r, &r);
		}
		ti.currmode = (short int) __newmode;
		/* Reinit */
		ti.screenwidth = 0;     /* C: coniox_init() ahora se guarda en screenwidth, no en normattr */
		/* D: coniox_init() re-lee dimensiones y cursor y reposiciona coniox_cell;
		   el window() de abajo vuelve a sincronizarla via gotoxy(1, 1). */
		coniox_init(NULL);
	}
	window(1, 1, ti.screenwidth, ti.screenheight);
	textattr(ti.normattr);
	clrscr();
	_setcursortype(_NORMALCURSOR);
}


/* ----------------------------------------------------------------------------------------------------------------- */
void _setcursortype(int __cur_t)
{
	int cursor_start, cursor_end, cellheight;
        
        
	coniox_init(NULL);
	coniox_setcursortype = __cur_t;
	if (directvideo)
	{
		/* Writing CRTC 0x0A/0x0B directly bypasses BIOS cursor emulation: scale
		   the shape to the actual cell height from BDA 0040:0085 (h), instead
		   of the 8 scan line based constants expected by INT 10h AH=01h. */
		cellheight = peekb(0x40, 0x85);
		if (cellheight < 2 || cellheight > 32)
		{
			cellheight = 16;
		}
		/* Un solo switch: el de constantes BIOS era trabajo muerto aqui (la
		   tabla de abajo lo reescribia entero); solo _NOCURSOR tomaba de ahi
		   el 0x20/0, que ahora se fija directamente. */
		switch (__cur_t)
		{
			case _NOCURSOR:
				cursor_start = 0x20;    /* start > end: cursor oculto */
				cursor_end = 0;
				break;
			case _SOLIDCURSOR:
				cursor_start = 0;
				cursor_end = cellheight - 1;
				break;
			case _NORMALCURSOR:
			default:
				cursor_start = cellheight - 2;
				cursor_end = cellheight - 1;
				break;
		}
		outportb(coniox_basecrt, 0x0A);
		outportb(coniox_basecrt + 1, cursor_start);
		outportb(coniox_basecrt, 0x0B);
		outportb(coniox_basecrt + 1, cursor_end);

		/* Re-program the cursor address: while the cursor was hidden gotoxy()
		   skipped the CRTC update, so 0x0E/0x0F can hold a stale position.
		   gotoxy() syncs both BDA 0040:0050 and CRTC 0x0E/0x0F. */
		if (__cur_t != _NOCURSOR)
		{
			gotoxy(ti.curx, ti.cury);
		}
	}
	else
	{
		union REGS r;
		/* Rama BIOS: constantes para INT 10h AH=01h (emulacion de tamano sobre
		   la rejilla de 8 scanlines que asume la interrupcion). */
		switch (__cur_t)
		{
			case _NOCURSOR:
				cursor_start = 0x20;
				cursor_end = 0;
				break;
			case _SOLIDCURSOR:
				if (ti.screenheight == 43)
				{
					cursor_start = 0;
					cursor_end = 8;
				}
				else
				{
					cursor_start = 0;
					cursor_end = ti.currmode == MONO ? 12 : 7;
				}
				break;
			case _NORMALCURSOR:
			default:
				if (ti.currmode == MONO)
				{
					cursor_start = 11;
					cursor_end = 12;
				}
				else
				{
					cursor_start = 6;
					cursor_end = ti.screenheight == 43 ? 0 : 7;
				}
				break;
		}
		r.h.ah = 0x1;
		r.h.ch = cursor_start;
		r.h.cl = cursor_end;
		coniox_int86(0x10, &r, &r);
	}

}



/* ----------------------------------------------------------------------------------------------------------------- */
void textattr(int __newattr)
{
	ti.attribute = (unsigned short) (__newattr & 0xFF);
}


/* ----------------------------------------------------------------------------------------------------------------- */
void gotoxy(int __x, int __y)
{
	int cursor;
	unsigned int scrx;
	unsigned int scry;

	coniox_init(NULL);
	if (!coniox_inwindow(ti.winleft + __x - 1, ti.wintop + __y - 1))
	{
		return;
	}
	ti.curx = __x;
	ti.cury = __y;
	/* D: re-sincroniza la cache de celda de putch()/cputs() con la posicion
	   recien validada; window() y textmode() llegan aqui via gotoxy(1, 1). */
	coniox_cell = coniox_offset(ti.winleft + ti.curx - 2, ti.wintop + ti.cury - 2);
	coniox_cursor_dirty = 0;        /* gotoxy deja el cursor fisico al dia (o oculto) */

	/* Only move cursor if visible */
	if (coniox_setcursortype != _NOCURSOR)
	{
		if (directvideo)
		{
			scrx = ti.winleft + __x - 2;
			scry = ti.wintop  + __y - 2;
			pokew(0x40, 0x50, (scry << 8) | (scrx & 0xFF)); /* Put cursor position in BIOS */
			cursor = scry * ti.screenwidth + scrx;
			outportb(coniox_basecrt, 0x0F);
			outportb(coniox_basecrt + 1, cursor & 0xFF);
			outportb(coniox_basecrt, 0x0E);
			outportb(coniox_basecrt + 1, (cursor >> 8) & 0xFF);
		}
		else
		{
			union REGS r;
			r.h.ah = 2;
			r.h.bh = 0;
			r.h.dh = (unsigned char)(ti.wintop  + __y - 2);
			r.h.dl = (unsigned char)(ti.winleft + __x - 2);
			coniox_int86(0x10, &r, &r);
		}
	}
}



/* ----------------------------------------------------------------------------------------------------------------- */
void delline(void)
{
	coniox_init(NULL);
	//Seems to be faster using BIOS under emulators
	if ((directvideo) && (!coniox_is_emulator))
	{
		/* Copia directa VRAM->VRAM fila a fila y de arriba abajo: al subir
		   lineas cada fila fuente se lee justo antes de que la iteracion
		   siguiente la pise. Sustituye al viajecito movetext()->gettext()->
		   buffer estatico->puttext(): la mitad de trafico de memoria (el
		   cuello de botella real en VRAM de 16 bits), cero chequeos de
		   rectangulo por llamada y sin el techo de CONIOX_MAX_CHARINFO. */
		unsigned short coniox_far *srcrow;
		unsigned short coniox_far *dstrow;
		size_t rowbytes;
		int y, sw, yfirst, ylast, width;

		/* D: hoisting: el bound y el stride se recargaban de ti en cada
		   iteracion (la llamada con store far obliga a asumir aliasing). */
		sw = ti.screenwidth;
		yfirst = ti.cury + ti.wintop;
		ylast = ti.winbottom;
		width = ti.winright - ti.winleft + 1;
		rowbytes = (size_t) width * sizeof(unsigned short);

		srcrow = coniox_offset(ti.winleft - 1, yfirst - 1);
		dstrow = srcrow - sw;
		if ((width == sw) && (yfirst <= ylast))
		{
			/* D: ventana a ancho completo: filas contiguas en VRAM y copia
			   hacia arriba (dst < src), asi que todo el bloque es un solo
			   memcpy hacia adelante en vez de N llamadas. El scroll de
			   cputs() a pantalla completa y el LF de la ultima fila pasan
			   por aqui; el guard cubre un cury raro fuera de ventana. */
			coniox_fmemcpy(dstrow, srcrow, rowbytes * (size_t)(ylast - yfirst + 1));
		}
		else
		{
			for (y = yfirst; y <= ylast; y++)
			{
				coniox_fmemcpy(dstrow, srcrow, rowbytes);
				srcrow += sw;
				dstrow += sw;
			}
		}
		coniox_fmemsetw((unsigned short coniox_far *) coniox_offset(ti.winleft - 1, ti.winbottom - 1), (short)(unsigned short)((ti.attribute << 8) | ' '), (size_t) width);
	}
	else
	{
		union REGS r;
		#if defined(__WATCOMC__)
			r.w.ax = 0x0601;
		#else
			r.x.ax = 0x0601;
		#endif
		r.h.bh = ti.attribute;
		r.h.cl = ti.winleft - 1;                        /*  Left column number */
		r.h.ch = ti.cury + ti.wintop - 2;   /* Upper row number, */
		r.h.dl = ti.winright - 1;                  /*  Right column number */
		r.h.dh = ti.winbottom - 1;                /* Lower row number */
		coniox_int86(0x10, &r, &r);                     /* Scroll up */
	}
}


/* ----------------------------------------------------------------------------------------------------------------- */
void insline(void)
{
	coniox_init(NULL);
	//Seems to be faster using BIOS under emulators
	if ((directvideo) && (!coniox_is_emulator))
	{
		/* Espejo de delline(): aqui el bloque baja una linea, asi que la
		   copia va de abajo arriba para leer cada fila antes de que la
		   pise la anterior. */
		unsigned short coniox_far *srcrow;
		unsigned short coniox_far *dstrow;
		size_t rowbytes;
		int y, sw, ytop, width;

		/* D: hoisting: el stride y el bound se recargaban de ti en cada
		   iteracion por las llamadas intercaladas. */
		sw = ti.screenwidth;
		ytop = ti.cury + ti.wintop - 1;
		width = ti.winright - ti.winleft + 1;
		rowbytes = (size_t) width * sizeof(unsigned short);

		srcrow = coniox_offset(ti.winleft - 1, ti.winbottom - 2);
		dstrow = srcrow + sw;
		/* D: sin bloque unico tipo delline(): aqui dst > src (el bloque baja)
		   y coniox_fmemcpy solo copia hacia adelante, pisaria filas aun sin
		   leer; se mantiene fila a fila, de abajo arriba. */
		for (y = ti.winbottom - 1; y >= ytop; y--)
		{
			coniox_fmemcpy(dstrow, srcrow, rowbytes);
			srcrow -= sw;
			dstrow -= sw;
		}
		coniox_fmemsetw((unsigned short coniox_far *) coniox_offset(ti.winleft - 1, ti.cury + ti.wintop - 2), (short)(unsigned short)((ti.attribute << 8) | ' '), (size_t) width);
	}
	else
	{
		union REGS r;
		#if defined(__WATCOMC__)
			r.w.ax = 0x0701;
		#else
			r.x.ax = 0x0701;
		#endif
		r.h.bh = ti.attribute;
		r.h.cl = ti.winleft - 1;                        /*  Left column number */
		r.h.ch = ti.cury + ti.wintop - 2;   /* Upper row number, */
		r.h.dl = ti.winright - 1;                  /*  Right column number */
		r.h.dh = ti.winbottom - 1;                /* Lower row number */
		coniox_int86(0x10, &r, &r);                     /* Scroll down */
	}
}



/* ----------------------------------------------------------------------------------------------------------------- */
int gettext(int __left, int __top, int __right, int __bottom,
	    void coniox_far *__destin)
{
	unsigned short coniox_far *p;
	unsigned short coniox_far *dst;
	unsigned int width;
	unsigned int sw;
	int rows;
	int y;

	size_t row_bytes;
	size_t total_bytes;

	coniox_init(NULL);

	/* Validar destino y rect�ngulo, con coordenadas absolutas 1-based. */
	if (__destin == NULL ||
	    __left < 1 ||
	    __top < 1 ||
	    __right < __left ||
	    __bottom < __top ||
	    __right > ti.screenwidth ||
	    __bottom > ti.screenheight)
	{
		return 0;
	}

	width = (unsigned int)(__right - __left + 1);
	rows = __bottom - __top + 1;

	row_bytes = (size_t)width * sizeof(unsigned short);
	total_bytes = row_bytes * (size_t)rows;

	p = (unsigned short coniox_far *) coniox_offset(__left - 1, __top - 1);

	dst = (unsigned short coniox_far *)__destin;

	/* D: stride a un local: ti.screenwidth se recargaba del global tras cada
	   llamada del bucle (el compilador debe asumir aliasing con el store far). */
	sw = ti.screenwidth;

	/* Pantalla completa a lo ancho: todas las filas son contiguas. */
	if (__left == 1 && __right == ti.screenwidth)
	{
		coniox_fmemcpy(dst, p, total_bytes);
	}
	else
	{
		/* Copia cada fila respetando el stride real de VRAM. */
		for (y = 0; y < rows; y++)
		{
			coniox_fmemcpy(dst, p, row_bytes);

			p += sw;
			dst += width;
		}
	}

	return 1;
}

/* ----------------------------------------------------------------------------------------------------------------- */
int puttext(int __left, int __top, int __right, int __bottom,
	    void coniox_far *__source)
{
	unsigned short coniox_far *p;
	unsigned short coniox_far *src;
	unsigned int width;
	unsigned int sw;
	int rows;
	int y;

	size_t row_bytes;
	size_t total_bytes;

	coniox_init(NULL);

	/* Validar origen y rect�ngulo, con coordenadas absolutas 1-based. */
	if (__source == NULL ||
	    __left < 1 ||
	    __top < 1 ||
	    __right < __left ||
	    __bottom < __top ||
	    __right > ti.screenwidth ||
	    __bottom > ti.screenheight)
	{
		return 0;
	}

	width = (unsigned int)(__right - __left + 1);
	rows = __bottom - __top + 1;

	row_bytes = (size_t)width * sizeof(unsigned short);
	total_bytes = row_bytes * (size_t)rows;

	p = (unsigned short coniox_far *) coniox_offset(__left - 1, __top - 1);

	src = (unsigned short coniox_far *)__source;

	/* D: stride a un local: ti.screenwidth se recargaba del global tras cada
	   llamada del bucle (el compilador debe asumir aliasing con el store far). */
	sw = ti.screenwidth;

	/* Pantalla completa a lo ancho: todas las filas son contiguas. */
	if (__left == 1 && __right == ti.screenwidth)
	{
		coniox_fmemcpy(p, src, total_bytes);
	}
	else
	{
		/* Copiar por filas sin sobrescribir las zonas adyacentes. */
		for (y = 0; y < rows; y++)
		{
			coniox_fmemcpy(p, src, row_bytes);

			p += sw;
			src += width;
		}
	}

	return 1;
}

/* ----------------------------------------------------------------------------------------------------------------- */
int movetext(int __left, int __top, int __right, int __bottom, int __destleft, int __desttop)
{
	unsigned short *buffer;         /* un malloc por llamada (antes: 32 KB estaticos) */
	size_t cells;
	int ok;


	coniox_init(NULL);

	if (__left == __destleft && __top == __desttop)
	{
		return 1;
	}

	/* Coordinates are absolute screen coordinates. Reject empty/out-of-range rectangles. */
	if (__left < 1 || __top < 1 || __right < __left || __bottom < __top || __right > ti.screenwidth || __bottom > ti.screenheight)
	{
		return 0;
	}
	if (__destleft < 1 || __desttop < 1 || __destleft + (__right - __left) > ti.screenwidth || __desttop + (__bottom - __top) > ti.screenheight)
	{
		return 0;
	}
	if (directvideo)
	{
		/* D: copia directa VRAM->VRAM in-place: se evita el snapshot
		   malloc + gettext + puttext (el doble de trafico por VRAM, que es
		   el cuello de botella real, y en CGA ademas duele leer) y la
		   reserva de heap. La direccion de copia sigue al solape, el mismo
		   patron que delline()/insline(); la ruta BIOS sigue mas abajo. */
		unsigned short coniox_far *src;
		unsigned short coniox_far *dst;
		int w, h, sw, dx, dy, row, i;

		w = __right - __left + 1;
		h = __bottom - __top + 1;
		sw = ti.screenwidth;
		dx = __destleft - __left;
		dy = __desttop - __top;
		src = coniox_offset(__left - 1, __top - 1);
		dst = coniox_offset(__destleft - 1, __desttop - 1);

		if (dy == 0 && dx > 0 && dx < w)
		{
			/* E: solape parcial dentro de la misma fila (0 < dx < w): el unico
			   caso que exige celda a celda hacia atras. Con dx >= w las
			   filas no se pisan y vale la ruta general de abajo, con
			   coniox_fmemcpy por fila, igual que hacia la izquierda. */
			for (row = 0; row < h; row++)
			{
				for (i = w - 1; i >= 0; i--)
				{
					dst[i] = src[i];
				}
				src += sw;
				dst += sw;
			}
		}
		else if (dy > 0 && dy < h)
		{
			/* Solape vertical parcial (destino por debajo): fila a fila de abajo
			   arriba, como insline(), para leer cada fila fuente antes de
			   que la pise la copia. Dentro de la fila el sentido da igual:
			   origen y destino no comparten fila. */
			src += (h - 1) * sw;
			dst += (h - 1) * sw;
			for (row = 0; row < h; row++)
			{
				coniox_fmemcpy(dst, src, (size_t) w * sizeof(unsigned short));
				src -= sw;
				dst -= sw;
			}
		}
		else
		{
			/* E: todos los demas casos (destino por arriba, |dy| >= h, o dy == 0
			   con dx <= 0 o dx >= w): copiando hacia adelante y de arriba
			   abajo la lectura va siempre por delante de la escritura,
			   el mismo argumento que hacia la izquierda. */
			if (w == sw)
			{
				/* Ancho completo: filas contiguas, un solo bloque. */
				coniox_fmemcpy(dst, src, (size_t) w * (size_t) h * sizeof(unsigned short));
			}
			else
			{
				for (row = 0; row < h; row++)
				{
					coniox_fmemcpy(dst, src, (size_t) w * sizeof(unsigned short));
					src += sw;
					dst += sw;
				}
			}
		}
		return 1;
	}

	/* C: buffer con un solo malloc en vez del estatico de CONIOX_MAX_CHARINFO
	   celdas: desaparece el techo fijo del rectangulo y los 32 KB de BSS
	   solo los paga quien llama de verdad a movetext(). */
	cells = (size_t)(__right - __left + 1) * (size_t)(__bottom - __top + 1);
	buffer = (unsigned short *) malloc(cells * sizeof(unsigned short));
	if (buffer == NULL)
	{
		return 0;
	}

	/* C: copia sin borrar via gettext+puttext, como el DOS clasico. El snapshot
	   intermedio resuelve el solape origen/destino y el rectangulo origen queda
	   intacto. D: queda solo para la ruta BIOS (directvideo == 0). E: la rama
	   Windows hace lo propio con ReadConsoleOutput+WriteConsoleOutput, asi que
	   el origen se preserva en todas las plataformas. */
	if (!gettext(__left, __top, __right, __bottom, buffer))
	{
		free(buffer);
		return 0;
	}
	ok = puttext(__destleft, __desttop, __destleft + (__right - __left), __desttop + (__bottom - __top), buffer);
	free(buffer);
	return ok;
}



/* ----------------------------------------------------------------------------------------------------------------- */
void clrscr(void)
{
	gotoxy(1, 1);
	if (directvideo)
	{
		if (ti.winright - ti.winleft + 1 == ti.screenwidth)
		{
			/* Full-width block: VRAM rows are contiguous, so a single fill covers all rows */
			coniox_fmemsetw((unsigned short coniox_far*) coniox_offset(ti.winleft - 1, ti.wintop - 1), (short)((ti.attribute << 8) | ' '), (size_t)ti.screenwidth * (ti.winbottom - ti.wintop + 1));
		}
		else
		{
			/* Relleno directo por filas: antes desviaba por coniox_putchxyattrwh(),
			   que reevalua los casos 1x1/columna/ancho completo (ya descartados
			   aqui), recalcula la celda y anade la llamada. */
			unsigned short coniox_far *p;
			unsigned short v;
			int width, height, y, sw;

			v = (unsigned short)((ti.attribute << 8) | ' ');
			width = ti.winright - ti.winleft + 1;
			height = ti.winbottom - ti.wintop + 1;
			sw = ti.screenwidth;    /* D: stride a un local, fuera del bucle */
			p = (unsigned short coniox_far *) coniox_offset(ti.winleft - 1, ti.wintop - 1);
			for (y = 0; y < height; y++)
			{
				coniox_fmemsetw(p, (short) v, (size_t) width);
				p += sw;
			}
		}
	}
	else
	{
		union REGS r;
		#if defined(__WATCOMC__)
			r.w.ax = 0x0600;
		#else
			r.x.ax = 0x0600;
		#endif
		r.h.bh = ti.attribute;
		r.h.cl = ti.winleft - 1;                        /*  Left column number */
		r.h.ch = ti.wintop - 1;         /* Upper row number */
		r.h.dl = ti.winright - 1;                         /*  Right column number */
		r.h.dh = ti.winbottom - 1;                      /* Lower row number */
		coniox_int86(0x10, &r, &r);
	}
}


/* ----------------------------------------------------------------------------------------------------------------- */
#if defined(UNICODE)
wchar_t getwch(void)
{
	coniox_init(NULL);
	coniox_idle();
	/* Todo: Implement */
	return(0);
}
#endif

int getch_last_extended_key = 0;
/* ----------------------------------------------------------------------------------------------------------------- */
int kbhit(void)
{
	if (getch_last_extended_key)
	{
		return(getch_last_extended_key);
	}

	coniox_flush_cursor();  /* B: el cursor fisico al dia antes de esperar teclado */

	if (directvideo)
	{
		/* Una sola lectura doble de la BDA: head en la palabra baja (0x1A) y
		   tail en la alta (0x1C). En sondeo continuo ahorra construir el
		   puntero far dos veces por llamada; comparar ambas palabras equivale
		   a comparar head != tail (la cola solo coincide consigo misma vacia). */
		{
			unsigned long q = peekl(0x40, 0x1A);

			return ((unsigned short) q != (unsigned short) (q >> 16));
		}
	}
	else
	{
		#if defined(__WATCOMC__)
			union REGPACK r;
			memset(&r, 0, sizeof(union REGPACK));
			r.h.ah = 1;
			intr(0x16, &r);
			return((r.w.flags & 0x40) == 0 ? 1 : 0);
		#else
			union REGS r;
			memset(&r, 0, sizeof(union REGS));
			r.h.ah = 1;
			coniox_int86(0x16, &r, &r);
			return((r.x.flags & 0x40) == 0 ? 1 : 0);
		#endif
	}
}


/* ----------------------------------------------------------------------------------------------------------------- */
int getch(void)
{
	if (getch_last_extended_key)
	{
		int ch = getch_last_extended_key;
		getch_last_extended_key = 0;
		return ch;
	}

	coniox_flush_cursor();  /* B: el cursor fisico al dia antes de bloquearse */

	if (directvideo)
	{
		unsigned short start, end, head, scancode;
		unsigned long lIdleTick = peekl(0x40, 0x6C) - 1UL;    /* fuerza idle en la 1a vuelta */
        
		while (!kbhit())
		{
			unsigned long lNowTick = peekl(0x40, 0x6C);

			/* coniox_idle() solo al cambiar el tick de la BDA (18,2 Hz).
			   El spin anterior lanzaba 3 INTs software por vuelta: millones
			   de INTs por segundo mientras se espera una tecla. */
			if (lNowTick != lIdleTick)
			{
				lIdleTick = lNowTick;
				coniox_idle();
			}
		}
		/* Los limites del bufer solo hacen falta al consumir la tecla: leerlos
		   aqui y no antes del bucle ahorra 2 lecturas far en cada getch() que
		   se bloquea esperando (el caso normal). */
		start = peekw(0x40, 0x80);
		end   = peekw(0x40, 0x82);
		if (start == 0 || end <= start) { start = 0x1E; end = 0x3E; }
		_disable();
		head = peekw(0x40, 0x1A);
		scancode = peekw(0x40, head);
		head += 2;
		if (head >= end)
		{
			head = start;
		}
		pokew(0x40, 0x1A, head);
		_enable();

		if ((scancode & 0xFF) == 0 || (scancode & 0xFF) == 0xE0)
		{
			getch_last_extended_key = scancode >> 8;
			return 0;
		}
		else
		{
			return scancode & 0xFF;
		}
	}
	else
	{
		union REGS r;
		r.h.ah = 0;
		coniox_int86(0x16, &r, &r);
		if ((r.h.al == 0) || (r.h.al == 0xE0))
		{
			getch_last_extended_key = r.h.ah;
			return 0;
		}
		else
		{
			return r.h.al;
		}
	}
}

/* ----------------------------------------------------------------------------------------------------------------- */
int ungetch(int __ch)
{
	if (directvideo)
	{
		unsigned short start, end, head, tail, new_tail;

		start = peekw(0x40, 0x80);
		end   = peekw(0x40, 0x82);
		if (start == 0 || end <= start)
		{
			start = 0x1E;
			end = 0x3E;
		}

		/* Windows WriteConsoleInput() appends to the input queue, so consecutive
		   ungetch() calls are read back FIFO. Prepending at the head reverses
		   them, so coniodemo finish() would type "uoy knahT" instead of
		   "Thank you". Append at the BIOS keyboard buffer tail to match the
		   Windows behaviour. */
		_disable();
		head = peekw(0x40, 0x1A);
		tail = peekw(0x40, 0x1C);
		new_tail = tail + 2;
		if (new_tail >= end)
		{
			new_tail = start;
		}
		if (new_tail == head)
		{
			_enable();
			return(EOF);    /* buffer lleno */
		}
		pokew(0x40, tail, (__ch & 0xFF));
		pokew(0x40, 0x1C, new_tail);
		_enable();
		return __ch;
	}
	else
	{
		union REGS r;

		/* INT 16h AH=05h stores the word (scan << 8) | ASCII in the keyboard
		   buffer: AL = ASCII, CX = scan code. Set AL = __ch and CX = 0 so the
		   word read back by getch() is exactly __ch. Leaving AL untouched stored
		   garbage, and CX = __ch turned it into an extended-key word. */
		memset(&r, 0, sizeof(r));
		r.h.ah = 0x05;
		r.h.al = (unsigned char) (__ch & 0xFF);
		coniox_int86(0x16, &r, &r);

		if (r.h.al == 0)
		{
			return __ch;
		}
		else
		{
			return EOF;
		}
	}
}
#endif
