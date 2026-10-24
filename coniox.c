/* ----------------------------------------------------------------------------------------------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stddef.h>
#include "coniox.h"



/* ----------------------------------------------------------------------------------------------------------------- */
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
	#define CONIOX_MAX_CHARINFO	(160 * 100)
	wchar_t coniox_ansi2unicode(char ch);
	char coniox_unicode2ansi(wchar_t ch);
	void coniox_putwchxyattr(int x, int y, wchar_t ch, int attr);
#endif


#if defined(__WATCOMC__)
	#define coniox_inline __inline
#elif defined(__BORLANDC__)
	#define coniox_inline
#else
	#define coniox_inline inline
#endif

coniox_inline void coniox_putchxyattr(int x, int y, int ch, int attr);
void coniox_putchxyattrwh(int x, int y, int ch, int attr, int w, int h);
void coniox_idle(void);
int coniox_get_is_emulator (void);
void coniox_blink(unsigned int blink);
void coniox_enhanced_palette(unsigned int pal);
int coniox_is_emulator = 0;



/* ----------------------------------------------------------------------------------------------------------------- */
struct text_info ti; /*= { 1, 1, 80, 25, 15, 15, 0, 25, 80, 1, 1 } */
int directvideo = 1;
int _wscroll = 1;
int coniox_setcursortype = _NORMALCURSOR;
int coniox_cursor_dirty = 0;	/* B: 1 = el cursor fisico esta por detras de ti (curx, cury) */



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
				if (count >= 39)	/* a[40]: par buffer+tamano, ultimo indice 39 */
				{
					break;
				}
			#else
				if (count >= 20)	/* sscanf se llama con 20 argumentos */
				{
					break;
				}
			#endif
			if (*p == 's' || *p == 'c' || *p == '[')
			{
				a[count++] = va_arg(argPtr, void *);		/* buffer */
				#ifdef _MSC_VER
					a[count++] = va_arg(argPtr, size_t *);	/* tama�o: solo sscanf_s */
				#endif
			}
			else
			{
				a[count++] = va_arg(argPtr, void *);
			}
		}
	}
	#ifdef _MSC_VER
		return(sscanf_s(buffer, format,	a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8], a[9], a[10], a[11], a[12], a[13], a[14], a[15], a[16], a[17], a[18], a[19], 	a[20], a[21], a[22], a[23], a[24], a[25], a[26], a[27], a[28], a[29], a[30], a[31], a[32], a[33], a[34], a[35], a[36], a[37], a[38], a[39]));
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
    ti.normattr = 7;
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
			ti.curx = 1;
			gotoxy(1, ti.cury);
			break;
		case '\n':
			if (ti.cury < winheight)
			{
				ti.cury++;
				gotoxy(ti.curx, ti.cury);
			}
			else if (_wscroll)
			{
				/* LF: keep the column, like Borland's putch. */
				oldx = ti.curx;
				oldy = ti.cury;
				gotoxy(1, 1);
				delline();
				gotoxy(oldx, oldy);
			}
			/* _wscroll = 0: LF on the last line does not scroll. */
			break;
		case '\b':
			if (ti.curx > 1)
			{
				ti.curx--;
				gotoxy(ti.curx, ti.cury);
			}
			break;
		default:
			coniox_putchxyattr(ti.winleft + ti.curx - 1, ti.wintop + ti.cury - 1, __c, ti.attribute);
			if (ti.curx < winwidth)
			{
				ti.curx++;
				coniox_cursor_dirty = 1;	/* B: el cursor se sincroniza al bloquear o al cerrar la rafaga */
			}
			else
			{
				if (_wscroll)
				{
					if (ti.cury < winheight)
					{
						gotoxy(1, ti.cury + 1);
					}
					else
					{
						oldy = ti.cury;
						gotoxy(1, 1);
						delline();
						gotoxy(1, oldy);
					}
				}
				else
				{
					/* _wscroll = 0: stay on the last column, further output overwrites the edge cell. */
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
			if (count >= 20)	/* swscanf_s se llama con 20 argumentos */
			{
				break;
			}
			a[count++] = va_arg(argPtr, void*);
		}
	}
	return(swscanf_s(buffer, format, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8], a[9], a[10], a[11], a[12], a[13], a[14], a[15], a[16], a[17], a[18], a[19]));
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
int cputws(const wchar_t *__str)
{
	int k = 0;

	while (*__str)
	{
		putwch(*__str);
		++__str;
		++k;
	}
	return k;
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
			gotoxy(1, ti.cury);
			break;
		case L'\n':
			if (ti.cury < winheight)
			{
				gotoxy(ti.curx, ti.cury + 1);
			}
			else if (_wscroll)
			{
				/* LF: keep the column, like Borland putwch. */
				oldx = ti.curx;
				oldy = ti.cury;
				gotoxy(1, 1);
				delline();
				gotoxy(oldx, oldy);
			}
			/* _wscroll = 0: LF on the last line does not scroll. */
			break;
		case L'\b':
			if (ti.curx > 1)
			{
				gotoxy(ti.curx - 1, ti.cury);
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
						gotoxy(1, ti.cury + 1);
					}
					else
					{
						oldy = ti.cury;
						gotoxy(1, 1);
						delline();
						gotoxy(1, oldy);
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
				coniox_cursor_dirty = 1;	/* B: el cursor se sincroniza al bloquear o al cerrar la rafaga */
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
int cputs(const char *__str)
{
	const char *orig = __str;
	const char *run;
	unsigned int i;
	unsigned int runlen, remaining;
	unsigned int winwidth, winheight;
	int oldx, oldy;
	COORD c = { 0, 0 };
	static CHAR_INFO ci[CONIOX_MAX_CHARINFO];	/* estatica: 64 KB no caben en la pila (igual que en putchxyattrwh) */

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
		/* Carriage return: volver al inicio de linea. */
		if (*__str == '\r')
		{
			ti.curx = 1;
			gotoxy(1, ti.cury);
			++__str;
			continue;
		}

		/* Line feed: avanzar o desplazar la ventana. */
		if (*__str == '\n')
		{
			if (ti.cury < (int) winheight)
			{
				ti.cury++;
				gotoxy(ti.curx, ti.cury);
			}
			else if (_wscroll)
			{
				/* LF: keep the column, like Borland cputs. */
				oldx = ti.curx;
				oldy = ti.cury;
				gotoxy(1, 1);
				delline();
				gotoxy(oldx, oldy);
			}
			/* _wscroll = 0: LF on the last line does not scroll. */
			++__str;
			continue;
		}

		/* Backspace: retroceder sin borrar el caracter. */
		if (*__str == '\b')
		{
			if (ti.curx > 1)
			{
				ti.curx--;
				gotoxy(ti.curx, ti.cury);
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
			ti.curx = (int) winwidth;	/* la ventana se encogio: evitar underflow en remaining */
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
				ti.curx = (int) winwidth;	/* _wscroll = 0: el siguiente run sobrescribe la celda del borde */
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


	r.Left = (short) x - 1;
	r.Top = (short) y - 1;
	r.Right = (short) x - 1;
	r.Bottom = (short) y - 1;
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
	static CHAR_INFO ci[CONIOX_MAX_CHARINFO];
	
	if (w <= 0 || h <= 0)
	{
		return;
	}
	
	total = w * h;
	if (total > CONIOX_MAX_CHARINFO)
	{
		return;
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
	int car;
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
	coniox_cursor_dirty = 0;	/* gotoxy deja el cursor fisico al dia (o oculto) */
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
	gotoxy(ti.curx, ti.cury);
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
	gotoxy(ti.curx, ti.cury);
}



/* ----------------------------------------------------------------------------------------------------------------- */
int gettext(int __left, int __top, int __right, int __bottom, void *__destin)
{
	int i;
	SMALL_RECT r;
	short *buf;
	COORD s, c = { 0,0 };
	static CHAR_INFO ci[CONIOX_MAX_CHARINFO];


	coniox_init(NULL);
	/* Validacion estilo Borland: rectangulo dentro de la pantalla absoluta. */
	if (__left < 1 || __top < 1 || __right > ti.screenwidth || __bottom > ti.screenheight || __right < __left || __bottom < __top || (__right - __left + 1) * (__bottom - __top + 1) > CONIOX_MAX_CHARINFO)
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
	if (ReadConsoleOutput(coniox_console_output, ci, s, c, &r ))
	{
		for (i = 0; i < s.X * s.Y; i++)
		{
			#if UNICODE
				buf[i] = (ci[i].Char.UnicodeChar & 0xFF) + ((ci[i].Attributes & 0xFF) << 8);
			#else
				buf[i] = (ci[i].Char.AsciiChar & 0xFF) + ((ci[i].Attributes & 0xFF) << 8);
			#endif
		}
	}
	return(1);
}


/* ----------------------------------------------------------------------------------------------------------------- */
int movetext(int __left, int __top, int __right, int __bottom, int __destleft, int __desttop)
{
	COORD c;
	SMALL_RECT r;
	CHAR_INFO ci;

	coniox_init(NULL);

	/* Validacion estilo Borland: origen y destino dentro de la pantalla. */
	if (__left < 1 || __top < 1 || __right < __left || __bottom < __top || __right > ti.screenwidth || __bottom > ti.screenheight)
	{
		return(0);
	}
	if (__destleft < 1 || __desttop < 1 || __destleft + (__right - __left) > ti.screenwidth || __desttop + (__bottom - __top) > ti.screenheight)
	{
		return(0);
	}

	r.Left = (short) __left - 1;
	r.Top = (short) __top - 1;
	r.Right = (short)__right - 1;
	r.Bottom = (short) __bottom - 1;
	c.X = (short) __destleft - 1;
	c.Y = (short) __desttop - 1;
	#if UNICODE
		ci.Char.UnicodeChar = L' ';
	#else
		ci.Char.AsciiChar = ' ';
	#endif
	ci.Attributes = ti.attribute;
	ScrollConsoleScreenBuffer(coniox_console_output, &r, NULL, c, &ci);
	return(1);
}


/* ----------------------------------------------------------------------------------------------------------------- */
int puttext(int __left, int __top, int __right, int __bottom, void *__source)
{
	int i;
	SMALL_RECT r;
	short *ci;
	COORD s, c = { 0,0 };
	static CHAR_INFO buffer[CONIOX_MAX_CHARINFO];


	coniox_init(NULL);
	/* Validacion estilo Borland: rectangulo dentro de la pantalla absoluta. */
	if (__left < 1 || __top < 1 || __right > ti.screenwidth || __bottom > ti.screenheight || __right < __left || __bottom < __top || (__right - __left + 1) * (__bottom - __top + 1) > CONIOX_MAX_CHARINFO)
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
	return(1);
}



/* ----------------------------------------------------------------------------------------------------------------- */
int putwtext(int __left, int __top, int __right, int __bottom, const wchar_info *__source)
{
	int i;
	SMALL_RECT r;
	COORD s, c = { 0,0 };
	static CHAR_INFO buffer[CONIOX_MAX_CHARINFO];


	coniox_init(NULL);
	/* Validacion estilo Borland: rectangulo dentro de la pantalla absoluta. */
	if (__left < 1 || __top < 1 || __right > ti.screenwidth || __bottom > ti.screenheight || __right < __left || __bottom < __top || (__right - __left + 1) * (__bottom - __top + 1) > CONIOX_MAX_CHARINFO)
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
	return(1);
}


/* ----------------------------------------------------------------------------------------------------------------- */
int getwtext(int __left, int __top, int __right, int __bottom, wchar_info *__destin)
{
	int i;
	SMALL_RECT r;
	COORD s, c = { 0,0 };
	static CHAR_INFO buffer[CONIOX_MAX_CHARINFO];


	coniox_init(NULL);
	/* Validacion estilo Borland: rectangulo dentro de la pantalla absoluta. */
	if (__left < 1 || __top < 1 || __right > ti.screenwidth || __bottom > ti.screenheight || __right < __left || __bottom < __top || (__right - __left + 1) * (__bottom - __top + 1) > CONIOX_MAX_CHARINFO)
	{
		return(0);
	}
	r.Left = (short) (__left - 1);
	r.Top = (short) (__top - 1);
	r.Right = (short) (__right - 1);
	r.Bottom = (short) (__bottom - 1);
	s.X = (short) (__right - __left + 1);
	s.Y = (short) (__bottom - __top + 1);

	if (ReadConsoleOutput(coniox_console_output, buffer, s, c, &r ))
	{
		for (i = 0; i < s.X * s.Y; i++)
		{
			#if UNICODE
				__destin[i].letter = buffer[i].Char.UnicodeChar;
			#else
				__destin[i].letter = (char) buffer[i].Char.AsciiChar;
			#endif
			__destin[i].attr = buffer[i].Attributes;
		}
	}
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
		switch (__newmode)
		{
			case C40X14:
			case BW40X14:
				info.dwSize.X = 40;
				info.dwSize.Y = 14;
				break;
			case C40X21:
			case BW40X21:
				info.dwSize.X = 40;
				info.dwSize.Y = 21;
				break;
			case C40X28:
			case BW40X28:
				info.dwSize.X = 40;
				info.dwSize.Y = 28;
				break;
			case C40X43:
			case BW40X43:
				info.dwSize.X = 40;
				info.dwSize.Y = 43;
				break;
			case C40X50:
			case BW40X50:
				info.dwSize.X = 40;
				info.dwSize.Y = 50;
				break;
			case C40X60:
			case BW40X60:
				info.dwSize.X = 40;
				info.dwSize.Y = 60;
				break;
			case MONO14:
			case C80X14:
			case BW80X14:
				info.dwSize.X = 80;
				info.dwSize.Y = 14;
				break;
			case MONO21:
			case C80X21:
			case BW80X21:
				info.dwSize.X = 80;
				info.dwSize.Y = 21;
				break;
			case MONO28:
			case C80X28:
			case BW80X28:
				info.dwSize.X = 80;
				info.dwSize.Y = 28;
				break;
			case MONO43:
			case C80X43:
			case BW80X43:
				info.dwSize.X = 80;
				info.dwSize.Y = 43;
				break;
			case MONO50:
			case C80X50:
			case BW80X50:
				info.dwSize.X = 80;
				info.dwSize.Y = 50;
				break;
			case MONO60:
			case C80X60:
			case BW80X60:
				info.dwSize.X = 80;
				info.dwSize.Y = 60;
				break;
			case C4350:
				info.dwSize.X = 80;
				info.dwSize.Y = 50;
				break;
			case BW40:
			case C40:
				info.dwSize.X = 40;
				info.dwSize.Y = 25;
				break;
			case BW80:
			case C80:
			case _ORIGMODE:
			case MONO:
			default:
				info.dwSize.X = 80;
				info.dwSize.Y = 25;
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
	Sleep(ms);
}

#endif /* ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32) */





/* ----------------------------------------------------------------------------------------------------------------- */
/* DOS ONLY (Non portable) */
/* ----------------------------------------------------------------------------------------------------------------- */
#if ((__DOS__) || (__MSDOS__))
int coniox_basecrt = 0x3D4;

/* ----------------------------------------------------------------------------------------------------------------- */
#if ((defined(__FLAT__)) || (defined(__DJGPP__)))
	unsigned short *coniox_vram;
	#define coniox_int86 int386
	#define coniox_int86x int386x
	#define coniox_offset(piX, piY) (coniox_vram + ((ti.screenwidth * (piY)) + (piX)))
#else
	unsigned short coniox_far *coniox_vram;
	#define coniox_int86 int86
	#define coniox_int86x int86x
	#define coniox_offset(piX, piY) (coniox_vram + ((ti.screenwidth * (piY)) + (piX)))
#endif


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
        #pragma aux coniox_fmemsetw =					\
            "		movzx	edx, ax					"	\
            "		shl		eax, 16					"	\
            "		or		eax, edx				"	\
            "		shr		ecx, 1					"	\
            "		cld								"	\
            "		rep		stosd					"	\
            "		jnc		short no_odd				"	\
            "		stosw							"	\
            "no_odd:									"	\
            parm [EDI][AX][ECX]							\
            modify exact [EDI EDX ECX EAX];
        #pragma aux coniox_putrun =					\
            "		mov		edx, ebx				"	\
            "		and		edx, 0FF00h				"	\
            "		mov		ebx, edx				"	\
            "		shl		ebx, 16					"	\
            "		or		ebx, edx				"	\
            "		test	ecx, 1					"	\
            "		jz		_preven					"	\
            "		movzx	eax, byte ptr [esi]		"	\
            "		inc		esi						"	\
            "		or		eax, edx				"	\
            "		stosw							"	\
            "		dec		ecx						"	\
            "_preven:									"	\
            "		shr		ecx, 1					"	\
            "		jecxz	_prdone					"	\
            "_prpairs:									"	\
            "		movzx	eax, byte ptr [esi]		"	\
            "		movzx	edx, byte ptr [esi+1]	"	\
            "		add		esi, 2					"	\
            "		shl		edx, 16					"	\
            "		or		eax, edx				"	\
            "		or		eax, ebx				"	\
            "		stosd							"	\
            "		loop	_prpairs				"	\
            "_prdone:									"	\
            parm [EDI][ESI][EBX][ECX]					\
            modify exact [EDI ESI ECX EDX EAX EBX];
        #pragma aux coniox_fmemcpy =					\
            "		mov		ecx, ebx				"	\
            "		shr		ecx, 2					"	\
            "		cld								"	\
            "		rep		movsd					"	\
            "		mov		ecx, ebx				"	\
            "		and		ecx, 3					"	\
            "		rep		movsb					"	\
            parm [EDI][ESI][EBX]						\
            modify exact [ECX EDI ESI];
    #else
        #pragma aux coniox_fmemsetw =					\
            "		cld								"	\
            "		rep		stosw					"	\
            parm [ES DI][AX][CX]						\
            modify exact [DI CX];
        #pragma aux coniox_putrun =					\
            "		cld								"	\
            "_prnext:									"	\
            "		lodsb							"	\
            "		mov		ah, bh					"	\
            "		stosw							"	\
            "		loop	_prnext				"	\
            parm [ES DI][SI][BX][CX]					\
            modify exact [DI SI CX AX];
        #pragma aux coniox_fmemcpy =					\
            "		push	ds						"	\
            "		mov		ds, dx					"	\
            "		cld								"	\
            "		shr		cx, 1					"	\
            "		rep		movsw					"	\
            "		jnc		short _fcpy_done			"	\
            "		movsb							"	\
            "_fcpy_done:								"	\
            "		pop		ds						"	\
            parm [ES DI][DX SI][CX]					\
            modify exact [CX SI DI];
    #endif
#else
    #define coniox_fmemcpy	_fmemcpy
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

	(void) title;					/* W303: suppress unused parameter warning */

	/* Check if already initialized */
	if (ti.normattr == 7)
	{
		return;
	}
	ti.normattr = 7;
	
	
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
	//coniox_basecrt = peekw(0x40, 0x63);	 /* 0x3D4 */

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
	ti.attribute = 7;
	ti.winleft = 1;
	ti.wintop = 1;
	ti.winright = ti.screenwidth;
	ti.winbottom = ti.screenheight;
	
	
	//Get if under emulator
	coniox_is_emulator = coniox_get_is_emulator();
	
	//Activate blink
	coniox_blink(1);
	coniox_enhanced_palette(1);
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
		coniox_currentoffset = coniox_offset(ti.winleft + ti.curx - 2, ti.wintop + ti.cury - 2);
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
			ti.curx = (int)winwidth;	/* la ventana se encogio: evitar underflow en remaining */
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
				}
				else
				{
					ti.curx = 1;
					ti.cury = 1;
					delline();
					ti.cury = (int)winheight;
				}
			}
			else
			{
				ti.curx = (int)winwidth;    /* _wscroll = 0: stay on the last column, the next run overwrites the edge cell */
			}
			if (directvideo)
			{
				coniox_currentoffset = coniox_offset(ti.winleft + ti.curx - 2, ti.wintop + ti.cury - 2);
			}
		}
		__str += runlen;
	}
	gotoxy(ti.curx, ti.cury);
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
		else if (w == 1)
		{
			p = (unsigned short coniox_far*) coniox_offset(x - 1, y - 1);
			for (y1 = y; y1 < y + h; y1++)
			{
				*p = v;
				p += ti.screenwidth;
			}
			return;
		}
		p = (unsigned short coniox_far*) coniox_offset(x - 1, y - 1);
		ylim = y + h;
		sw = ti.screenwidth;
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
        {0x00, 0x00, 0x00},	/*  0: Negro */
        {0x00, 0x00, 0x24},	/*  1: Azul (m�s oscuro que el VGA por defecto) */
        {0x00, 0x24, 0x04},	/*  2: Verde (m�s oscuro, tinte azulado) */
        {0x00, 0x22, 0x28},	/*  3: Cian (m�s oscuro, menos verde) */
        {0x24, 0x00, 0x00},	/*  4: Rojo (m�s oscuro) */
        {0x28, 0x08, 0x2A},	/*  5: Magenta (algo desaturado) */
        {0x30, 0x30, 0x08},	/*  6: Marr�n (oliva/amarillo oscuro, desv�a del VGA est�ndar) */
        {0x2B, 0x2B, 0x2B},	/*  7: Gris claro (pr�cticamente id�ntico al default) */
        {0x00, 0x00, 0x1D},	/*  8: Gris oscuro (se muestra azul marino, desv�a del VGA est�ndar) */
        {0x04, 0x08, 0x3C},	/*  9: Azul brillante (m�s oscuro y saturado) */
        {0x00, 0x28, 0x0E},	/* 10: Verde brillante (tirando a esmeralda) */
        {0x00, 0x1F, 0x3F},	/* 11: Cian claro (m�s profundo, sin el rojo lavado del default) */
        {0x28, 0x04, 0x10},	/* 12: Rojo claro (tinte burdeos) */
        {0x2C, 0x08, 0x3F},	/* 13: Magenta claro (tirando a violeta) */
        {0x30, 0x2A, 0x10},	/* 14: Amarillo (apagado, algo anaranjado) */
        {0x30, 0x30, 0x3F}	/* 15: Blanco (apagado, tinte fr�o azulado) */
    };
    static const unsigned char coniox_default[16][3] =
    {
        {0x00, 0x00, 0x00},	/*  0: Negro */
        {0x00, 0x00, 0x2A},	/*  1: Azul */
        {0x00, 0x2A, 0x00},	/*  2: Verde */
        {0x00, 0x2A, 0x2A},	/*  3: Cian */
        {0x2A, 0x00, 0x00},	/*  4: Rojo */
        {0x2A, 0x00, 0x2A},	/*  5: Magenta */
        {0x2A, 0x2A, 0x00},	/*  6: Marron */
        {0x2A, 0x2A, 0x2A},	/*  7: Gris claro */
        {0x00, 0x00, 0x15},	/*  8: Gris oscuro */
        {0x00, 0x00, 0x3F},	/*  9: Azul brillante */
        {0x00, 0x2A, 0x15},	/* 10: Verde brillante */
        {0x00, 0x2A, 0x3F},	/* 11: Cian claro */
        {0x2A, 0x00, 0x15},	/* 12: Rojo claro */
        {0x2A, 0x00, 0x3F},	/* 13: Magenta claro */
        {0x2A, 0x2A, 0x15},	/* 14: Amarillo */
        {0x2A, 0x2A, 0x3F}	/* 15: Blanco */
    };

    if (ti.currmode == MONO)
    {
        return;
    }

    /* pal = 0: paleta por defecto del VGA. pal != 0: paleta mejorada de CONIOX. */
    coniox_rgb = pal ? coniox_palette : coniox_default;

    if (directvideo)
    {
        outportb(0x3C8, 0x00);		/* DAC: escribir desde el registro 0 */
        for (i = 0; i < 16; i++)
        {
            outportb(0x3C9, coniox_rgb[i][0]);	/* R */
            outportb(0x3C9, coniox_rgb[i][1]);	/* G */
            outportb(0x3C9, coniox_rgb[i][2]);	/* B */
        }
    }
    else
    {
        union REGS r;
        for (i = 0; i < 16; i++)
        {
            #if defined(__WATCOMC__)
                r.w.ax = 0x1010;	/* Subfunci�n: establecer registro DAC */
                r.w.bx = i;		/* BX = �ndice de color (BH=0) */
            #else
                r.x.ax = 0x1010;
                r.x.bx = i;
            #endif
            r.h.dh = coniox_rgb[i][0];	/* R */
            r.h.ch = coniox_rgb[i][1];	/* G */
            r.h.cl = coniox_rgb[i][2];	/* B */
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
		unsigned int status = coniox_basecrt + 6;	/* 3DAh color, 3BAh mono */

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
		outportb(0x3C0, 0x20);		/* flip-flop en �ndice, v�deo encendido */
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

    lWait = (((unsigned long) ms * 91UL) + 4999UL) / 5000UL;        /* ms -> ticks a 18.2 Hz, redondeo arriba */

    if (directvideo)
    {
        lTicks = peekl(0, 0x46C);
        /* Resta modular: sobrevive al reinicio del contador a medianoche */
        while (lWait != 0 && (peekl(0, 0x46C) - lTicks) < lWait)
        {
            coniox_idle();
        }
    }
    else
    {
        union REGS r;
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
            coniox_idle();
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
			pokeb(0x40, 0x87, peekb(0x40, 0x87) | 1);	/* Disable cursor size emultation */
			pokew(0x40, 0x4C, 8000);	/* Correct video buffer size */
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
		ti.normattr = 0;
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
	}
	
	
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
		switch (__cur_t)
		{
			case _NOCURSOR:
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
	coniox_cursor_dirty = 0;	/* gotoxy deja el cursor fisico al dia (o oculto) */

	/* Only move cursor if visible */
	if (coniox_setcursortype != _NOCURSOR)
	{
		if (directvideo)
		{
			scrx = ti.winleft + __x - 2;
			scry = ti.wintop  + __y - 2;
			pokew(0x40, 0x50, (scry << 8) | (scrx & 0xFF));	/* Put cursor position in BIOS */
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
		movetext(ti.winleft, ti.cury + ti.wintop, ti.winright, ti.winbottom, ti.winleft, ti.cury + ti.wintop - 1);
		coniox_fmemsetw((unsigned short coniox_far *) coniox_offset(ti.winleft - 1, ti.winbottom - 1), (short)(unsigned short)((ti.attribute << 8) | ' '), (size_t)(ti.winright - ti.winleft + 1));
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
		r.h.cl = ti.winleft - 1;			/*  Left column number */
		r.h.ch = ti.cury + ti.wintop - 2;   /* Upper row number, */
		r.h.dl = ti.winright - 1;		   /*  Right column number */
		r.h.dh = ti.winbottom - 1;		  /* Lower row number */
		coniox_int86(0x10, &r, &r);			/* Scroll up */
	}
}


/* ----------------------------------------------------------------------------------------------------------------- */
void insline(void)
{
	coniox_init(NULL);
	//Seems to be faster using BIOS under emulators
	if ((directvideo) && (!coniox_is_emulator))
	{
		movetext(ti.winleft, ti.cury + ti.wintop - 1, ti.winright, ti.winbottom - 1, ti.winleft, ti.cury + ti.wintop);
		coniox_fmemsetw((unsigned short coniox_far *) coniox_offset(ti.winleft - 1, ti.cury + ti.wintop - 2), (short)(unsigned short)((ti.attribute << 8) | ' '), (size_t)(ti.winright - ti.winleft + 1));
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
		r.h.cl = ti.winleft - 1;			/*  Left column number */
		r.h.ch = ti.cury + ti.wintop - 2;   /* Upper row number, */
		r.h.dl = ti.winright - 1;		   /*  Right column number */
		r.h.dh = ti.winbottom - 1;		  /* Lower row number */
		coniox_int86(0x10, &r, &r);			/* Scroll down */
	}
}



/* ----------------------------------------------------------------------------------------------------------------- */
int gettext(int __left, int __top, int __right, int __bottom,
            void coniox_far *__destin)
{
	unsigned short coniox_far *p;
	unsigned short coniox_far *dst;
	unsigned int width;
	int rows;
	int y;

	size_t row_bytes;
	size_t total_bytes;

	coniox_init(NULL);

	/* Validar destino y rect�ngulo, con coordenadas absolutas 1-based. */
	if (__destin == NULL || __left < 1 || __top < 1 || __right < __left || __bottom < __top || __right > ti.screenwidth || __bottom > ti.screenheight)
	{
		return 0;
	}

	width = (unsigned int)(__right - __left + 1);
	rows = __bottom - __top + 1;

	row_bytes = (size_t)width * sizeof(unsigned short);
	total_bytes = row_bytes * (size_t)rows;

	p = (unsigned short coniox_far *) coniox_offset(__left - 1, __top - 1);

	dst = (unsigned short coniox_far *)__destin;

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

			p += ti.screenwidth;
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
	int rows;
	int y;

	size_t row_bytes;
	size_t total_bytes;

	coniox_init(NULL);

	/* Validar origen y rect�ngulo, con coordenadas absolutas 1-based. */
	if (__source == NULL || __left < 1 || __top < 1 || __right < __left || __bottom < __top || __right > ti.screenwidth || __bottom > ti.screenheight)
	{
		return 0;
	}

	width = (unsigned int)(__right - __left + 1);
	rows = __bottom - __top + 1;

	row_bytes = (size_t)width * sizeof(unsigned short);
	total_bytes = row_bytes * (size_t)rows;

	p = (unsigned short coniox_far *) coniox_offset(__left - 1, __top - 1);

	src = (unsigned short coniox_far *)__source;

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

			p += ti.screenwidth;
			src += width;
		}
	}

	return 1;
}

/* ----------------------------------------------------------------------------------------------------------------- */
int movetext(int __left, int __top, int __right, int __bottom, int __destleft, int __desttop)
{
	unsigned short coniox_far* src;
	unsigned short coniox_far* dst;
	unsigned int width;
	unsigned int row_bytes;
	int rows;
	int y1;
	int step;


	coniox_init(NULL);
	
	if (__left == __destleft && __top == __desttop)
	{
		return 1;
	}

	/* Coordinates are absolute screen coordinates. Reject empty/out-of-range rectangles. */
	if (__left < 1 || __top < 1 || __right < __left || __bottom < __top ||
	    __right > ti.screenwidth || __bottom > ti.screenheight)
	{
		return 0;
	}

	width = (unsigned int)(__right - __left + 1);
	rows = __bottom - __top + 1;
	if (__destleft < 1 || __desttop < 1 ||
	    __destleft + (int)width - 1 > ti.screenwidth ||
	    __desttop + rows - 1 > ti.screenheight)
	{
		return 0;
	}
	row_bytes = width * (unsigned int)sizeof(unsigned short);

	/* A full-width move is contiguous in text memory; memmove handles overlap. */
	if (__left == 1 && __right == ti.screenwidth && __destleft == 1)
	{
		src = (unsigned short coniox_far*)(void coniox_far*) coniox_offset(0, __top - 1);
		dst = (unsigned short coniox_far*)(void coniox_far*) coniox_offset(0, __desttop - 1);
		_fmemmove(dst, src, (unsigned long)row_bytes * (unsigned long)rows);
		return 1;
	}

	/* Copy bottom-up when moving down, otherwise top-down: one loop, two directions. */
	if (__desttop > __top)
	{
		y1 = rows - 1;
		step = -1;
	}
	else
	{
		y1 = 0;
		step = 1;
	}

	for (; y1 >= 0 && y1 < rows; y1 += step)
	{
		src = (unsigned short coniox_far*)(void coniox_far*) coniox_offset(__left - 1, __top + y1 - 1);
		dst = (unsigned short coniox_far*)(void coniox_far*) coniox_offset(__destleft - 1, __desttop + y1 - 1);

		if (__destleft > __left && __destleft <= __right)
		{
			_fmemmove(dst, src, row_bytes);
		}
		else
		{
			coniox_fmemcpy(dst, src, row_bytes);
		}
	}
	return 1;
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
			coniox_putchxyattrwh(ti.winleft, ti.wintop, ' ', ti.attribute, ti.winright - ti.winleft + 1, ti.winbottom - ti.wintop + 1);
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
		r.h.cl = ti.winleft - 1;			/*  Left column number */
		r.h.ch = ti.wintop - 1;		/* Upper row number */
		r.h.dl = ti.winright - 1;			  /*  Right column number */
		r.h.dh = ti.winbottom - 1;			/* Lower row number */
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
		return(peekw(0x40, 0x1A) != peekw(0x40, 0x1C));
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
		unsigned short start = peekw(0x40, 0x80);
		unsigned short end   = peekw(0x40, 0x82);
		unsigned short head, scancode;
	
		while (!kbhit())
		{
			coniox_idle();
		}
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
			return(EOF);	/* buffer lleno */
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
