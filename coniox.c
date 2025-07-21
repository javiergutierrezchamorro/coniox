/* ----------------------------------------------------------------------------------------------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <malloc.h>
#include <stddef.h>
#include "coniox.h"



/* ----------------------------------------------------------------------------------------------------------------- */
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
	wchar_t coniox_ansi2unicode(char ch);
	char coniox_unicode2ansi(wchar_t ch);
	void coniox_putwchxyattr(int x, int y, wchar_t ch, int attr);
#endif


#if (__BORLANDC__)
	#define coniox_inline
#else
	#define coniox_inline inline
#endif


coniox_inline void coniox_putchxyattr(int x, int y, int ch, int attr);
void coniox_putchxyattrwh(int x, int y, int ch, int attr, int w, int h);



/* ----------------------------------------------------------------------------------------------------------------- */
struct text_info ti; /*= { 1, 1, 80, 25, 15, 15, 0, 25, 80, 1, 1 } */;
int directvideo = 1;
int _wscroll = 1;
int coniox_setcursortype = _NORMALCURSOR;



/* ----------------------------------------------------------------------------------------------------------------- */
/* INTERNAL (Private)*/
/* ----------------------------------------------------------------------------------------------------------------- */
int coniox_vsscanf(const char *buffer, const char *format, va_list argPtr)
{
	void *a[20] = {NULL};
	size_t count = 0;
	const char *p;
	char c;

	p = format;
	while (1)
	{
		c = *(p++);
		if ( c == 0 )
		{
			break;
		}
		if (c == '%' && ( p[0] != '*' && p[0] != '%' ))
		{
			a[count++] = va_arg(argPtr, void *);
		}
	}
	return(sscanf_s(buffer, format, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8], a[9], a[10], a[11], a[12], a[13], a[14], a[15], a[16], a[17], a[18], a[19]));
}


/* ----------------------------------------------------------------------------------------------------------------- */
coniox_inline int coniox_inwindow(int x, int y)
{
	return (!(x<ti.winleft || y<ti.wintop || x>ti.winright || y>ti.winbottom));
}




/* ----------------------------------------------------------------------------------------------------------------- */
/* PORTABLE FUNCTIONS */
/* ----------------------------------------------------------------------------------------------------------------- */
void clreol(void)
{
	coniox_init(NULL);
	coniox_putchxyattrwh(ti.winleft + ti.curx - 1, ti.wintop + ti.cury - 1, ' ', ti.attribute, ti.winright - ti.winleft + 2 - ti.curx, 1);
}


/* ----------------------------------------------------------------------------------------------------------------- */
void gettextinfo(struct text_info *__r)
{
	coniox_init(NULL);
	memcpy(__r, &ti, sizeof(ti));
}


/* ----------------------------------------------------------------------------------------------------------------- */
void highvideo(void)
{
	ti.attribute |= 0x08;
}


/* ----------------------------------------------------------------------------------------------------------------- */
void lowvideo(void)
{
	ti.attribute &= ~0x08;
}



/* ----------------------------------------------------------------------------------------------------------------- */
void normvideo(void)
{
	ti.attribute = ti.normattr;
}



/* ----------------------------------------------------------------------------------------------------------------- */
void textbackground(int __newcolor)
{
	ti.attribute = (ti.attribute & 0x8F) | ((__newcolor << 4) & 0x7f);
}



/* ----------------------------------------------------------------------------------------------------------------- */
void textcolor(int __newcolor)
{
	ti.attribute = (ti.attribute & 0x70) | (__newcolor & 0x8F);
}


/* ----------------------------------------------------------------------------------------------------------------- */
void window(int __left, int __top, int __right, int __bottom)
{
	coniox_init(NULL);

	if ((__left < 1) || (__top < 1) || (__right > ti.screenwidth) || (__bottom > ti.screenheight))
	{
		return;
	}
	ti.winleft = (short) __left;
	ti.winright = (short) __right;
	ti.wintop = (short) __top;
	ti.winbottom = (short) __bottom;
	gotoxy(1, 1);
}


/* ----------------------------------------------------------------------------------------------------------------- */
int putch(int __c)
{
	int oldx, oldy;
	
	coniox_init(NULL);
	switch (__c)
	{
		case '\r':
			gotoxy(1, ti.cury);
			break;
		case '\n':
			if (ti.cury < ti.winbottom - ti.wintop + 1)
			{
				gotoxy(ti.curx, ti.cury + 1);
			}
			else
			{
				oldx = ti.curx;
				oldy = ti.cury;
				gotoxy(1, 1);
				delline();
				gotoxy(oldx, oldy);
			}
			break;
		case '\b':
			if (ti.curx > 1)
			{
				gotoxy(ti.curx - 1, ti.cury);
			}
			break;
		default:
			coniox_putchxyattr(ti.winleft + ti.curx - 1, ti.wintop + ti.cury - 1, __c, ti.attribute);
			if (ti.curx + 1 > ti.winright - ti.winleft + 1)
			{
				if (_wscroll)
				{
					if (ti.cury < ti.winbottom - ti.wintop + 1)
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
					gotoxy(1, ti.cury);
				}
			}
			else
			{
				gotoxy(ti.curx + 1, ti.cury);
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
				case '\b': /* backspace */
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
int cprintf (const char *__format, ...)
{
	int r;
	va_list ap;
	char buffer[PRINTFBUF_SIZE];

	va_start(ap, __format);
	#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
		r = vsnprintf(buffer, PRINTFBUF_SIZE, __format, ap);
	#else
		r = vsprintf(buffer, __format, ap);
	#endif
	va_end(ap);
	cputs(buffer);
	return(r);
}


/* ----------------------------------------------------------------------------------------------------------------- */
int cscanf(const char *__format, ...)
{
	int r;
	char buffer[PRINTFBUF_SIZE + 3];

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
	while (ch == 0)
	{
		getch();
		ch = getch();
	}
	putch(ch);
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
				case '\b': /* backspace */
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
int cputs(const char *__str)
{
	int k = 0;

	while (*__str)
	{
		putch(*__str);
		++__str;
		++k;
	}
	return(k);
}


/* ----------------------------------------------------------------------------------------------------------------- */
int coniox_vswscanf(const wchar_t* buffer, const wchar_t* format, va_list argPtr)
{
	void* a[20] = { NULL };
	size_t count = 0;
	const wchar_t* p;


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
			a[count++] = va_arg(argPtr, void*);
		}
	}
	return(swscanf_s(buffer, format, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8], a[9], a[10], a[11], a[12], a[13], a[14], a[15], a[16], a[17], a[18], a[19]));
}

/* ----------------------------------------------------------------------------------------------------------------- */
wchar_t* cgetws(wchar_t* __str)
{
	wchar_t* str;
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
				case L'\b': /* backspace */
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
int cputws(const wchar_t* __str)
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
	
	coniox_init(NULL);
	switch (__c)
	{
		case L'\r':
			gotoxy(1, ti.cury);
			break;
		case L'\n':
			if (ti.cury < ti.winbottom - ti.wintop + 1)
			{
				gotoxy(ti.curx, ti.cury + 1);
			}
			else
			{
				oldx = ti.curx;
				oldy = ti.cury;
				gotoxy(1, 1);
				delline();
				gotoxy(oldx, oldy);
			}
			break;
		case L'\b':
			if (ti.curx > 1)
			{
				gotoxy(ti.curx - 1, ti.cury);
			}
			break;
		default:
			coniox_putwchxyattr(ti.winleft + ti.curx - 1, ti.wintop + ti.cury - 1, __c, ti.attribute);
			if (ti.curx + 1 > ti.winright - ti.winleft + 1)
			{
				if (_wscroll)
				{
					if (ti.cury < ti.winbottom - ti.wintop + 1)
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
					gotoxy(1, ti.cury);
				}
			}
			else
			{
				gotoxy(ti.curx + 1, ti.cury);
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
int cwprintf(const wchar_t* __format, ...)
{
	int r;
	wchar_t buffer[PRINTFBUF_SIZE];
	va_list ap;

	va_start(ap, __format);
	r = vswprintf(buffer, PRINTFBUF_SIZE, __format, ap);
	va_end(ap);
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
	GetConsoleMode(coniox_console_output, &m);
	SetConsoleMode(coniox_console_output, m | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT | ENABLE_ECHO_INPUT);

	if (title)
	{
		SetConsoleTitle(title);
	}
}




/* ----------------------------------------------------------------------------------------------------------------- */
inline void coniox_putchxyattr(int x, int y, int ch, int attr)
{
	CHAR_INFO ci;
	SMALL_RECT r;
	COORD c = {0, 0}, s = {1, 1};


	if (!coniox_inwindow(x, y))
	{
		return;
	}
	r.Left = (short) x - 1;
	r.Top = (short) y - 1;
	r.Right = (short) x - 1;
	r.Bottom = (short) y - 1;
	#if UNICODE
			ci.Char.UnicodeChar = (wchar_t) ch;
	#else
			ci.Char.AsciiChar = (char) ch;
	#endif
	ci.Attributes = (short) attr;
	WriteConsoleOutput(coniox_console_output, &ci, s, c, &r);
}




/* ----------------------------------------------------------------------------------------------------------------- */
void coniox_putwchxyattr(int x, int y, wchar_t ch, int attr)
{
	CHAR_INFO ci;
	SMALL_RECT r;
	COORD c = { 0, 0 }, s = { 1, 1 };


	if (!coniox_inwindow(x, y))
	{
		return;
	}
	r.Left = (short) (x - 1);
	r.Top = (short) (y - 1);
	r.Right = (short) (x - 1);
	r.Bottom = (short) (y - 1);
	#if UNICODE
		ci.Char.UnicodeChar = ch;
	#else
		ci.Char.AsciiChar = (char) ch;
	#endif
	ci.Attributes = (WORD) attr;
	WriteConsoleOutput(coniox_console_output, &ci, s, c, &r);
}




/* ----------------------------------------------------------------------------------------------------------------- */
void coniox_putchxyattrwh(int x, int y, int ch, int attr, int w, int h)
{
	int i;
	COORD c;


	if (x < ti.winleft)
	{
		w -= ti.winleft - x;
	}
	if (y < ti.wintop)
	{
		h -= ti.wintop - y;
	}
	if (x + w - 1 > ti.winright)
	{
		w = ti.winright - x + 1;
	}
	if (y + h - 1 > ti.winbottom)
	{
		h = ti.winbottom - y + 1;
	}

	if (w <= 0 || h <= 0)
	{
		return;
	}

	for (i = 0; i < h; ++i)
	{
		DWORD written;
		c.X = (short) (x - 1);
		c.Y = (short) (y - 1 + i);
		FillConsoleOutputAttribute(coniox_console_output, (WORD) attr, w, c, &written);
		FillConsoleOutputCharacter(coniox_console_output, (char) ch, w, c, &written);
	}
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

	ansistr[0] = ( char ) ch;
	unicodestr[0] = ch;
	unicodestr[1] = 0;
	WideCharToMultiByte( CP_ACP, 0, unicodestr, 1, ansistr, 1, NULL, NULL );
	return ansistr[0];
}



/* ----------------------------------------------------------------------------------------------------------------- */
int getch(void)
{
	int car;
	DWORD leidos, modo;

	GetConsoleMode(coniox_console_input, &modo);
	SetConsoleMode(coniox_console_input, modo & !ENABLE_ECHO_INPUT & !ENABLE_PROCESSED_INPUT);
	ReadConsole(coniox_console_input, &car, 1, &leidos, NULL);
	SetConsoleMode(coniox_console_input, modo);
	return(car & 0xFF);
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
	CHAR_INFO *ci;
	short *buf;
	COORD s, c = { 0,0 };


	coniox_init(NULL);
	if (__right < __left || __bottom < __top)
	{
		return(0);
	}

	r.Left = (short) __left - 1;
	r.Top = (short) __top - 1;
	r.Right = (short) __right - 1;
	r.Bottom = (short) __bottom - 1;
	s.X = (short) (__right - __left + 1);
	s.Y = (short) (__bottom - __top + 1);
	ci = (CHAR_INFO *) malloc(s.X * s.Y * sizeof(CHAR_INFO));
	if (!ci)
	{
		return(0);
	}
	buf = (short *) __destin;
	if (ReadConsoleOutput(coniox_console_output, ci, s, c, &r ))
	{
		for (i = 0; i < s.X * s.Y; i++)
		{
			#if UNICODE
				//buf[i].letter = (char) ci[i].Char.UnicodeChar;
				buf[i] = (ci[i].Char.UnicodeChar & 0xFF) + ((ci[i].Attributes & 0xFF) << 8);
			#else
				//buf[i].letter = ci[i].Char.AsciiChar;
				buf[i] = (ci[i].Char.AsciiChar & 0xFF) + ((ci[i].Attributes & 0xFF) << 8);
			#endif
		}
	}
	free(ci);
	return(1);
}


/* ----------------------------------------------------------------------------------------------------------------- */
int movetext(int __left, int __top, int __right, int __bottom, int __destleft, int __desttop)
{
	COORD c;
	SMALL_RECT r;
	CHAR_INFO ci;

	coniox_init(NULL);

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
	CHAR_INFO *buffer;
	//char_info *ci;
	short *ci;
	COORD s, c = { 0,0 };

	coniox_init(NULL);
	if ( __right < __left || __bottom < __top)
	{
		return(0);
	}

	r.Left = (short) __left - 1;
	r.Top = (short) __top - 1;
	r.Right = (short) __right - 1;
	r.Bottom = (short) __bottom - 1;
	s.X = (short) (__right - __left + 1);
	s.Y = (short) (__bottom - __top + 1);

	buffer = malloc(s.X * s.Y * sizeof(CHAR_INFO));
	if (!buffer)
	{
		return(0);
	}
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
	free(buffer);
	return(1);
}



/* ----------------------------------------------------------------------------------------------------------------- */
int putwtext(int __left, int __top, int __right, int __bottom, const wchar_info *__source)
{
	int i;
	SMALL_RECT r;
	CHAR_INFO* buffer;
	//char_info *ci;
	short* ci;
	COORD s, c = { 0,0 };

	coniox_init(NULL);
	if (__right < __left || __bottom < __top)
	{
		return(0);
	}

	r.Left = (short)__left - 1;
	r.Top = (short)__top - 1;
	r.Right = (short)__right - 1;
	r.Bottom = (short)__bottom - 1;
	s.X = (short)(__right - __left + 1);
	s.Y = (short)(__bottom - __top + 1);

	buffer = malloc(s.X * s.Y * sizeof(CHAR_INFO));
	if (!buffer)
	{
		return(0);
	}
	ci = (short*)__source;
	for (i = 0; i < s.X * s.Y; i++)
	{
		#if UNICODE
			buffer[i].Char.UnicodeChar = (wchar_t) ci[i] & 0xFFFF;
		#else
			buffer[i].Char.AsciiChar = (unsigned char)ci[i] & 0xFF;
		#endif
		buffer[i].Attributes = ci[i] >> 8;
	}
	WriteConsoleOutput(coniox_console_output, buffer, s, c, &r);
	free(buffer);
	return(1);
}


/* ----------------------------------------------------------------------------------------------------------------- */
int getwtext(int __left, int __top, int __right, int __bottom, wchar_info *__destin)
{
	int i;
	SMALL_RECT r;
	CHAR_INFO* buffer;
	COORD s, c = { 0,0 };

	coniox_init(NULL);
	if (__right < __left || __bottom < __top)
	{
		return(0);
	}
	r.Left = (short) (__left - 1);
	r.Top = (short) (__top - 1);
	r.Right = (short) (__right - 1);
	r.Bottom = (short) (__bottom - 1);
	s.X = (short) (__right - __left + 1);
	s.Y = (short) (__bottom - __top + 1);
	buffer = malloc(s.X * s.Y * sizeof(CHAR_INFO));
	if (!buffer)
	{
		return(0);
	}
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
	free(buffer);
	return(0);
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
				ti.winleft = info.srWindow.Left;
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
}



/* ----------------------------------------------------------------------------------------------------------------- */
int kbhit(void)
{
	DWORD nevents = 0;
	INPUT_RECORD ir;


	coniox_init(NULL);

	while (1)
	{
		GetNumberOfConsoleInputEvents(coniox_console_input, &nevents);
		if ( nevents == 0)
		{
			return(0);
		}
		PeekConsoleInput(coniox_console_input, &ir, 1, &nevents);
		if (ir.EventType == KEY_EVENT && ir.Event.KeyEvent.bKeyDown)
		{
			return(1);
		}
		ReadConsoleInput(coniox_console_input, &ir, 1, &nevents);
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
	if (WriteConsoleInput(coniox_console_input, ir, 2, &written))
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
	if (WriteConsoleInput(coniox_console_input, ir, 2, &written))
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
#include <dos.h>
#include <bios.h>

int coniox_basecrt = 0x3D4;

#if defined(__WATCOMC__)
	#include <i86.h>
	unsigned int outp( int port, int value );
#endif

#if defined(__DJGPP__)
	#include <sys/nearptr.h>
	#define _fmemmove	memmove
	#define fmemcpy		memcpy
	#define MK_FP(seg,off) ((((seg)<<4)|(off)) + __djgpp_conventional_base
#endif


#ifndef MK_FP
	#define MK_FP(seg,off) (((seg)<<4)|(off))
#endif

#define coniox_far __far


/* ----------------------------------------------------------------------------------------------------------------- */
#if ((defined(__FLAT__)) || (defined(__DJGPP__)))
	#define coniox_far __far
	unsigned short *coniox_vram;
	unsigned short *coniox_currentoffset;
	#define coniox_int86 int386
	#define coniox_int86x int386x
	#if defined(__WATCOMC__)
		unsigned short *coniox_offset(unsigned int piX, unsigned int piY); 
		#pragma aux coniox_offset =										    \
			"			 .386													   "\
			"			 movzx eax, byte ptr ti + 16				"\
			"			 imul eax, edi									    "\
			"			 add eax, esi									   "\
			"			 shl eax, 1												 "\
			"			 add eax, coniox_vram					   "\
			parm nomemory [ESI][EDI]												   \
			modify exact nomemory [EAX]																	 \
			value [EAX];
	#else
		#define coniox_offset(piX, piY) (coniox_vram + ((ti.screenwidth * (piY)) + (piX)))
	#endif
#else
	#define coniox_far __far
	unsigned short coniox_far *coniox_vram;
	unsigned short coniox_far *coniox_currentoffset;
	#define coniox_int86 int86
	#define coniox_int86x int86x
	#if defined(__WATCOMKKC__)
		unsigned short coniox_far *coniox_offset(unsigned int piX, unsigned int piY);
		#pragma aux coniox_offset =										    \
				"			 .8086													    "\
				"			 xor ah, ah												 "\
				"			 mov al, byte ptr ti + 16					    "\
				"			 mul di														 "\
				"			 add ax, si										 "\
				"			 shl ax, 1												    "\
			    "			 mov dx, seg coniox_vram"      /* cargar segmento de coniox_vram en dx */ \
			    "			 add ax, word ptr coniox_vram" /* sumar offset base de coniox_vram */ \
			    parm nomemory [SI][DI]          \
			    modify exact nomemory [DX AX]            \
			    value [DX AX];			
	#else
		#define coniox_offset(piX, piY) (coniox_vram + ((ti.screenwidth * (piY)) + (piX)))
	#endif
#endif


/* ----------------------------------------------------------------------------------------------------------------- */
#if defined(__WATCOMC__)
	void coniox_far *coniox_fmemsetw (void coniox_far *m, short val, size_t count);
	#if (defined(__FLAT__))
		#pragma aux coniox_fmemsetw =										\
				"			      .386												      "\
				"			      movzx eax, ax										  "\
				"			      mov edx, eax										  "\
				"			      shl eax, 16										  "\
				"			      or eax, edx  										  "\
				"			      shr ecx, 1										     "\
				"				  cld											"\
				"			      rep stosd										       "\
				"			      test ecx, 1										     "\
				"			      jz short no_odd										"\
				"			      stosw										       "\
				"no_odd:																"\
				parm [EDI][AX][ECX]													     \
				modify exact [EDI EDX ECX EAX];
	#else
		#pragma aux coniox_fmemsetw =										\
				"			 .8086													    "\
				"			 cld														"\
				"			 rep stosw												   "\
				parm [ES DI][AX][CX]													   \
				modify exact [DI CX];
	#endif
#else
	void coniox_far *coniox_fmemsetw (void coniox_far *m, short val, size_t count)
	{
		short coniox_far* buf;

		buf = m;
		while (count--)
		{
			*buf++ = val;
		}
		return(m);
	}
#endif


#if !defined(__TURBOC__)
	unsigned int inp (int port);
	unsigned int inpw (int port);
	unsigned long inpd (int port);
	unsigned int outp (int port, int value);
	unsigned long outpd (int port, unsigned long value);
	unsigned int outpw (int port, unsigned int value);

	#define outportb		outp
	#define outport			outpw
	#define outportd		outpd
	#define inportb			inp
	#define inport			inpw
	#define inportd			inpd
#endif


#define peekb(s,o)			(*((unsigned char coniox_far *) MK_FP((s),(o))))
#define peekw(s,o)			(*((unsigned short coniox_far *) MK_FP((s),(o))))
#define peekl(s,o)			(*((unsigned long coniox_far *) MK_FP((s),(o))))
#define pokeb(s,o,x)		(*((unsigned char coniox_far *) MK_FP((s),(o))) = (unsigned char)(x))
#define pokew(s,o,x)		(*((unsigned short coniox_far *) MK_FP((s),(o))) = (unsigned short)(x))
#define pokel(s,o,x)		(*((unsigned long coniox_far *) MK_FP((s),(o))) = (unsigned long)(x))


void coniox_idle(void);
int coniox_movetext_nonoverlap(int __left, int __top, int __right, int __bottom, int __destleft, int __desttop);
int coniox_get_is_emulator (void);
void coniox_blink(unsigned int blink);
coniox_inline void coniox_putchattrcursor(int ch, int attr);
int coniox_is_emulator = 0;



/* ----------------------------------------------------------------------------------------------------------------- */
void coniox_init(const void* title)
{
	unsigned int cursor;
	union REGS r;

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
	if (ti.currmode == C4350)
	{
	}
	else if (directvideo)
	{
		ti.currmode = peekb(0x40, 0x49);
	}
	else
	{
		r.h.ah = 0xF;
		coniox_int86(0x10, &r, &r);
		ti.currmode = r.h.al;
	}

	#if (defined(__FLAT__))
		if ((ti.currmode == BW40) || (ti.currmode == BW80) || (ti.currmode == MONO))
		{
			coniox_vram = (unsigned short *) 0x000B0000UL;
		}
		else
		{
			coniox_vram = (unsigned short *) 0x000B8000UL;
		}
	#elif (defined(__DJGPP__))
		if ((ti.currmode == BW40) || (ti.currmode == BW80) || (ti.currmode == MONO))
		{
			coniox_vram = (unsigned short *) 0x000B0000UL +  __djgpp_conventional_base;
		}
		else
		{
			coniox_vram = (unsigned short *) 0x000B8000UL +  __djgpp_conventional_base;
		}
	#else
		if ((ti.currmode == BW40) || (ti.currmode == BW80) || (ti.currmode == MONO))
		{
			coniox_vram = MK_FP(0xB000, 0);
		}
		else
		{
			coniox_vram = MK_FP(0xB800, 0);

		}
	#endif

	/* Base for 6845 CRT controller */
	coniox_basecrt = peekw(0x40, 0x63);     /* 0x3D4 */

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
	ti.attribute = 7;
	ti.winleft = 1;
	ti.wintop = 1;
	ti.winright = ti.screenwidth;
	ti.winbottom = ti.screenheight;
	
	//Debug
	printf("Width: %d\n", ti.screenwidth);
	#if ((defined(__FLAT__)) || (defined(__DJGPP__)))
		printf("Pointer: %p\n", (void *) coniox_offset(80, 25));
	#else
		printf("Pointer: %Fp\n", (void far *) coniox_offset(80, 25));
	#endif
	
	
	//Get if under emulator
	coniox_is_emulator = coniox_get_is_emulator();
	
	//Activate blink
	coniox_blink(1);

	//exit(0);
}



/* ----------------------------------------------------------------------------------------------------------------- */
int cputs(const char *__str)
{
	char c;
	int k = 0;
	int oldx, oldy;
	unsigned int winwidth  = ti.winright  - ti.winleft  + 1;
	unsigned int winheight = ti.winbottom - ti.wintop   + 1;
	
	
	coniox_init(NULL);
	coniox_currentoffset = coniox_offset(ti.winleft + ti.curx - 2, ti.wintop + ti.cury - 2);

	while (c = *__str)
	{
		switch (c)
		{
			case '\r':
				coniox_currentoffset-= (ti.curx - 1);
				ti.curx = 1;
				break;
			case '\n':
				if (ti.cury < winheight)
				{
					coniox_currentoffset += ti.screenwidth;
					ti.cury++;
				}
				else
				{
					oldx = ti.curx;
					oldy = ti.cury;
					ti.curx = 1;
					ti.cury = 1;
					delline();
					ti.curx = oldx;
					ti.cury = oldy;
				}
				break;
			case '\b':
				if (ti.curx > 1)
				{
					coniox_currentoffset--;
					ti.curx--;
				}
				break;
			default:
				/* Inlined coniox_putchattrcursor(c, ti.attribute); */
				if (directvideo)
				{
					*coniox_currentoffset = (ti.attribute << 8) | (c & 0xFF);
				}
				else
				{
					union REGS r;
					r.h.ah = 2;
					r.h.bh = 0;
					r.h.dh = ti.cury - 1;
					r.h.dl = ti.curx - 1;
					coniox_int86(0x10, &r, &r);

					r.h.ah = 0x9;
					r.h.al = c;
					r.h.bh = 0;
					r.h.bl = ti.attribute;
					#if defined(__WATCOMC__)
						r.w.cx = 1;
					#else
						r.x.cx = 1;
					#endif
					coniox_int86(0x10, &r, &r);
				}
				if (ti.curx + 1 > winwidth)
				{
					if (_wscroll)
					{
						if (ti.cury < winheight)
						{
							coniox_currentoffset += ti.screenwidth - (ti.curx - 1);
							ti.curx = 1;
							ti.cury++;
						}
						else
						{
							coniox_currentoffset -= (ti.curx - 1);
							oldy = ti.cury;
							ti.curx = 1;
							ti.cury = 1;
							delline();
							ti.cury = oldy;
						}
					}
					else
					{
						coniox_currentoffset -= (ti.curx - 1);
						ti.curx = 1;
					}
				}
				else
				{
					coniox_currentoffset++;
					ti.curx++;
				}
		}
		++__str;
		++k;
	}
	gotoxy(ti.curx, ti.cury);
	return(k);
}


/* ----------------------------------------------------------------------------------------------------------------- */
coniox_inline void coniox_putchattrcursor(int ch, int attr)
{
	if (directvideo)
	{
		*coniox_currentoffset = (attr << 8) | (ch & 0xFF);
	}
	else
	{
		union REGS r;
		r.h.ah = 2;
		r.h.bh = 0;
		r.h.dh = ti.cury - 1;
		r.h.dl = ti.curx - 1;
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
	int y1;
	unsigned short v;
	union REGS r;


	if (directvideo)
	{
		v = (attr << 8) | (ch & 0xFF);
		p = (unsigned short coniox_far*) coniox_offset(x - 1, y - 1);
		for (y1 = y; y1 < y + h; y1++)
		{
			coniox_fmemsetw(p, v, w);
			p += ti.screenwidth;
		}
	}
	else
	{
		for (y1 = y; y1 < y + h; y1++)
		{
			r.h.ah = 2;
			r.h.bh = 0;
			r.h.dh = y1 - 1;
			r.h.dl = x - 1;
			coniox_int86(0x10, &r, &r);

			r.h.ah = 0x9;
			r.h.al = ch;
			r.h.bh = 0;
			r.h.bl = attr;
			#if defined(__WATCOMC__)
				r.w.cx = w;
			#else
				r.x.cx = w;
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
	coniox_int86(0x28, &r, &r);

	/*
		Advanced Power Management v1.0+
		call when the system is idle and should be suspended until the next
		system event or interrupt
		should not be called from within a hardware interrupt handler to avoid
		reentrance problems
		if an interrupt causes the system to resume normal processing, the
		interrupt may or may not have been handled when the BIOS returns
		from this call; thus, the caller should allow interrupts on return
		interrupt handlers may not retain control if the BIOS allows
		interrupts while in idle mode even if they are able to determine
		that they were called from idle mode
		the caller should issue this call continuously in a loop until it needs
		to perform some processing of its own
	*/
	#if defined(__WATCOMC__)
		r.w.ax = 0x5305;
	#else
		r.x.ax = 0x5305;
	#endif
	coniox_int86(0x15, &r, &r);

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
    // En DOSBox, inp(0x04F4) t�picamente devuelve 0xFF
    return (result == 0xFF);
}
    

/* ----------------------------------------------------------------------------------------------------------------- */
void coniox_blink(unsigned int blink)
{
	if (directvideo)
	{
		unsigned char v;
		outportb(0x3DA, 0);
		outportb(0x3C0, 0x10);
		v = inportb(0x3C1);
		if (blink)
		{
			v &= ~0x08; // bit 3 = 0. blink ON
		}
		else
		{
			v |= 0x08;  // bit 3 = 1. blink OFF (fondo extendido)
		}
		outportb(0x3DA, 0);
		outportb(0x3C0, 0x10);
		outportb(0x3C0, v);
	}
	else
	{
		union REGS r;
		#if defined(__WATCOMC__)
			r.w.ax = 0x1003;		// Subfunci�n: set blink/intensity bit
		#else
			r.x.ax = 0x1003;
		#endif
		r.h.bh = blink ? 0 : 1;
		coniox_int86(0x10, &r, &r);
	}
}



/* ----------------------------------------------------------------------------------------------------------------- */
void delay (unsigned int ms)
{
	unsigned long lTicks;

	if (directvideo)
	{
		lTicks = peekl(0, 0x46C);
		while (peekl(0, 0x46C) < (lTicks + (unsigned long) ms * 182 / 10000))
		{
			coniox_idle();
		}
	}
	else
	{
		union REGS r;
		unsigned long microsec = ms * 1000UL;
		
		#if defined(__WATCOMC__)
			r.w.ax = 0x8600;                    // funci�n 0x86 en AH, AL=0
			r.w.cx = microsec & 0xFFFF;        // parte baja de microsegundos
			r.w.dx = (microsec >> 16) & 0xFFFF; // parte alta de microsegundos
		#else
			r.x.ax = 0x8600;                    // funci�n 0x86 en AH, AL=0
			r.x.cx = microsec & 0xFFFF;        // parte baja de microsegundos
			r.x.dx = (microsec >> 16) & 0xFFFF; // parte alta de microsegundos
		 #endif
		coniox_int86(0x15, &r, &r);            // llamada a BIOS
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
	int cursor_start, cursor_end;
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
		outportb(coniox_basecrt, 0x0A);
		outportb(coniox_basecrt + 1, cursor_start);
		outportb(coniox_basecrt, 0x0B);
		outportb(coniox_basecrt + 1, cursor_end);
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

	coniox_init(NULL);
	if (!coniox_inwindow(ti.winleft + __x - 1, ti.wintop + __y - 1))
	{
		return;
	}
	ti.curx = __x;
	ti.cury = __y;

	/* Only move cursor if visible */
	if (coniox_setcursortype != _NOCURSOR)
	{
		if (directvideo)
		{
			cursor = ((ti.wintop + ti.cury - 2) * ti.screenwidth) + (ti.winleft + ti.curx - 2);
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
			r.h.dh = ti.wintop + ti.cury - 2;
			r.h.dl = ti.winleft + ti.curx - 2;
			coniox_int86(0x10, &r, &r);
		}
	}
}



/* ----------------------------------------------------------------------------------------------------------------- */
/*Borra la l�nea donde se encuentre el cursor y mueve todas las l�neas inferiores a una
l�nea anterior. La funci�n delline funciona en la ventana de texto activa.*/
void delline(void)
{
	coniox_init(NULL);
	//Seems to be faster using BIOS under emulators
	if ((directvideo) && (!coniox_is_emulator))
	{
		coniox_movetext_nonoverlap(ti.winleft, ti.cury + ti.wintop, ti.winright, ti.winbottom, ti.winleft, ti.cury + ti.wintop - 1);
		coniox_putchxyattrwh(ti.winleft, ti.winbottom, ' ', ti.attribute, ti.winright - ti.winleft + 1, 1);
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
		coniox_int86(0x10, &r, &r);
	}
}


/* ----------------------------------------------------------------------------------------------------------------- */
int gettext(int __left, int __top, int __right, int __bottom, void* __destin)
{
	unsigned short coniox_far* p;
	int y1;
	unsigned int width = (__right - __left + 1) << 1;
	

	coniox_init(NULL);
	if (__right < __left || __bottom < __top)
	{
		return(0);
	}
	
	p = (unsigned short coniox_far*) coniox_offset(__left - 1, __top - 1);
	for (y1 = __top; y1 <= __bottom; y1++)
	{
		_fmemcpy((unsigned short coniox_far *) __destin, p, width);
		p += ti.screenwidth;
		__destin = (void *) ((unsigned short *)__destin + (width >> 1));
	}
	return(1);
}


/* ----------------------------------------------------------------------------------------------------------------- */
int puttext(int __left, int __top, int __right, int __bottom, void* __source)
{
	unsigned short coniox_far* p;
	int y1;
	unsigned int width = (__right - __left + 1) << 1;

	coniox_init(NULL);
	if (__right < __left || __bottom < __top)
	{
		return(0);
	}
	
	p = (unsigned short coniox_far*) coniox_offset(__left - 1, __top - 1);
	for (y1 = __top; y1 <= __bottom; y1++)
	{
		_fmemcpy(p, (unsigned short coniox_far *) __source, width);
		p += ti.screenwidth;
		__source = (void *) ((unsigned short *)__source + (width >> 1));
	}
	return(1);
}



/* ----------------------------------------------------------------------------------------------------------------- */
int movetext(int __left, int __top, int __right, int __bottom, int __destleft, int __desttop)
{
	unsigned short coniox_far* p;
	unsigned short coniox_far* __destin;
	int y1;
	unsigned int width = (__right - __left + 1) << 1;

	coniox_init(NULL);
	p = (unsigned short coniox_far*) coniox_offset(__left - 1, __top - 1);
	__destin = (unsigned short coniox_far*) coniox_offset(__destleft - 1, __top + __desttop - __top - 1);
	for (y1 = __top; y1 <= __bottom; y1++)
	{
		_fmemmove((unsigned short coniox_far *) __destin, p, width);
		p += ti.screenwidth;
		__destin += ti.screenwidth;
	}
	return(1);
}


/* ----------------------------------------------------------------------------------------------------------------- */
int coniox_movetext_nonoverlap(int __left, int __top, int __right, int __bottom, int __destleft, int __desttop)
{
	unsigned short coniox_far* p;
	unsigned short coniox_far* __destin;
	int y1;
	unsigned int width = (__right - __left + 1) << 1;

	coniox_init(NULL);
	p = (unsigned short coniox_far*) coniox_offset(__left - 1, __top - 1);
	__destin = (unsigned short coniox_far*) coniox_offset(__destleft - 1, __top + __desttop - __top - 1);
	for (y1 = __top; y1 <= __bottom; y1++)
	{
		_fmemcpy((unsigned short coniox_far *) __destin, p, width);
		p += ti.screenwidth;
		__destin += ti.screenwidth;
	}
	return(1);
}



/* ----------------------------------------------------------------------------------------------------------------- */
void clrscr(void)
{
	gotoxy(1, 1);
	if (directvideo)
	{
		coniox_putchxyattrwh(ti.winleft, ti.wintop, ' ', ti.attribute, ti.winright - ti.winleft + 1, ti.winbottom - ti.wintop + 1);
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
		r.h.ch = ti.cury + ti.wintop - 2;       /* Upper row number, */
		r.h.dl = ti.winright - 1;		      /*  Right column number */
		r.h.dh = ti.winbottom - 1;		    /* Lower row number */
		coniox_int86(0x10, &r, &r);
	}
}


/* ----------------------------------------------------------------------------------------------------------------- */
wchar_t getwch(void)
{
	coniox_init(NULL);
	coniox_idle();
	/* Todo: Implement */
	return(0);
}

int getch_last_extended_key = 0;
/* ----------------------------------------------------------------------------------------------------------------- */
int kbhit(void)
{
	if (getch_last_extended_key)
	{
		return(getch_last_extended_key);
	}
	coniox_idle();

	if ((directvideo) && (!coniox_is_emulator))
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
			r.h.ah = 1;
			coniox_int86(0x16, &r, &r);
			return((r.x.flags & 0x40) == 0 ? 1 : 0);
		#endif
	}
}


/* ----------------------------------------------------------------------------------------------------------------- */
int getch(void)
{
	if ((directvideo) && (!coniox_is_emulator))
	{
		unsigned short tail, scancode;
		unsigned short far* keyboard_buffer = (unsigned short far*)MK_FP(0x40, 0x1E);

		if (getch_last_extended_key)
		{
			int ch = getch_last_extended_key;
			getch_last_extended_key = 0;
			return ch;
		}

		while (!kbhit())
		{
		}

		tail = peekw(0x40, 0x1C);
		scancode = keyboard_buffer[tail >> 1];
		tail = (tail + 2) & 0x1E;
		pokew(0x40, 0x1C, tail);

		if ((scancode & 0xFF) == 0)
		{
			getch_last_extended_key = scancode >> 8;
			return 0;
		}

		return scancode & 0xFF;
	}
	else
	{
		union REGS r;

		if (getch_last_extended_key)
		{
			int ch = getch_last_extended_key;
			getch_last_extended_key = 0;
			return ch;
		}

		r.h.ah = 0;  // INT 16h AH=0 - Wait for keypress
		coniox_int86(0x16, &r, &r);

		if (r.h.al == 0)
		{
			// Tecla extendida: guardar el scan code
			getch_last_extended_key = r.h.ah;
			return 0;
		}
		else
		{
			// Tecla normal
			return r.h.al;
		}
	}
}

/* ----------------------------------------------------------------------------------------------------------------- */
int ungetch(int __ch)
{
	if ((directvideo) && (!coniox_is_emulator))
	{
		unsigned short head, tail, new_tail;
	    unsigned short far* keyboard_buffer = (unsigned short far*)MK_FP(0x40, 0x1E);
	    unsigned short scancode;

	    head = peekw(0x40, 0x1A);
	    tail = peekw(0x40, 0x1C);

	    // Verificar si el buffer no est� lleno
	    new_tail = tail - 2;
	    if ((short) new_tail < 0)
	    {
	    	new_tail = 30;
    	}
	    if (new_tail == head)
	    {
	        // Buffer lleno, no se puede insertar m�s
	        return EOF;
	    }

	    // Insertar el car�cter con scancode 0 (sin scan code)
	    scancode = (0 << 8) | (__ch & 0xFF);
	    keyboard_buffer[new_tail >> 1] = scancode;

	    // Actualizar tail
	    pokew(0x40, 0x1C, new_tail);
	    return __ch;
	}
	else
	{
		union REGS r;
		r.h.ah = 0x05; // ungetch BIOS
		#if defined(__WATCOMC__)		
			r.w.cx = (__ch & 0xFF);
		#else
			r.x.cx = (__ch & 0xFF);
		#endif
		
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
