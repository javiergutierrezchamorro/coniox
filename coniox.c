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


void coniox_putchattrcursor(int ch, int attr);
void coniox_putchxyattr(int x, int y, int ch, int attr);
void coniox_putchxyattrwh(int x, int y, int ch, int attr, int w, int h);


/* ----------------------------------------------------------------------------------------------------------------- */
struct text_info ti; /*= { 1, 1, 80, 25, 15, 15, 0, 25, 80, 1, 1 } */;
int directvideo = 1;
int _wscroll = 1;



/* ----------------------------------------------------------------------------------------------------------------- */
/* INTERNAL (Private)*/
/* ----------------------------------------------------------------------------------------------------------------- */
int coniox_vsscanf(const char  *buffer, const char  *format, va_list argPtr)
{
		void *a[20] = {NULL};
		size_t count = 0;
		const char *p;


		p = format;
		while (1)
		{
				char c = *(p++);
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
int coniox_inwindow(int x, int y)
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
		//gotoxy(ti.curx, ti.cury);
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
		textattr(ti.attribute | 0x08);
}



/* ----------------------------------------------------------------------------------------------------------------- */
void lowvideo(void)
{
		textattr(ti.attribute & 0xF7);
}



/* ----------------------------------------------------------------------------------------------------------------- */
void normvideo(void)
{
		textattr(ti.normattr);
}



/* ----------------------------------------------------------------------------------------------------------------- */
void textbackground(int __newcolor)
{
		textattr(( ti.attribute & 0xFF0F) | (( __newcolor & 0xF ) << 4 ));
}



/* ----------------------------------------------------------------------------------------------------------------- */
void textcolor(int __newcolor)
{
		textattr(( ti.attribute & 0xFFF0) | (__newcolor & 0xF ));
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


		if (__str == NULL)
		{
				return(NULL);
		}
		str = __str + 2;
		maxlen = (int) (( unsigned char) __str[0] ) - 1;
		coniox_init(NULL);
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
		char buffer[PRINTFBUF_SIZE];
		int r;
		va_list ap;


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
		char buffer[PRINTFBUF_SIZE + 3];
		int r;


		va_list ap;
		buffer[0] = PRINTFBUF_SIZE;
		cgets(buffer);
		va_start(ap, __format);
		r = coniox_vsscanf(buffer + 2, __format, ap);
		va_end( ap );
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
		char str[PASS_MAX + 1];
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


		if (__str == NULL)
		{
				return(NULL);
		}

		str = __str + 2;
		maxlen = (int)(__str[0]) - 1;
		coniox_init(NULL);
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
					gotoxy(ti.curx, ti.cury + 1);
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
			if (ti.curx > 1) gotoxy(ti.curx - 1, ti.cury);
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
		wchar_t buffer[PRINTFBUF_SIZE + 3];
		int r;
		va_list ap;


		buffer[0] = PRINTFBUF_SIZE;
		cgetws(buffer);
		va_start(ap, __format);
		r = coniox_vswscanf(buffer + 2, __format, ap);
		va_end(ap);
		return r;
}


/* ----------------------------------------------------------------------------------------------------------------- */
int cwprintf(const wchar_t* __format, ...)
{
		wchar_t buffer[PRINTFBUF_SIZE];
		va_list ap;
		int r;


		va_start(ap, __format);
		r = vswprintf(buffer, PRINTFBUF_SIZE, __format, ap);
		va_end(ap);
		cputws(buffer);
		return r;
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
	ti.normattr = info.wAttributes;
	ti.winright = info.dwSize.X;
	ti.winbottom = info.dwSize.Y;
	ti.screenwidth = info.dwSize.X;
	ti.screenheight = info.dwSize.Y;
	ti.curx = info.dwCursorPosition.X + 1;
	ti.cury = info.dwCursorPosition.Y + 1;
	ti.attribute = info.wAttributes;
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
void coniox_putchxyattr(int x, int y, int ch, int attr)
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
		r.Left = (SHORT) (x - 1);
		r.Top = (SHORT) (y - 1);
		r.Right = (SHORT) (x - 1);
		r.Bottom = (SHORT) (y - 1);
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
		COORD c;
		int i;


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
				c.X = (SHORT) (x - 1);
				c.Y = (SHORT) (y - 1 + i);
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
				getwch();
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
		MultiByteToWideChar( CP_ACP, 0, ansistr, 1, unicodestr, 1 );
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
		return(car);
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
		c.X = ti.winleft + ti.curx - 2;
		c.Y = ti.wintop + ti.cury - 2;
		SetConsoleCursorPosition(coniox_console_output, c);
}


/* ----------------------------------------------------------------------------------------------------------------- */
void clrscr(void)
{
	coniox_init(NULL);

	coniox_putchxyattrwh(ti.winleft, ti.wintop, ' ', ti.attribute, ti.winright - ti.winleft + 1, ti.winbottom - ti.wintop + 1);
	gotoxy(1, 1);
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
		CHAR_INFO *ci;
		char_info *buf;
		COORD s, c = { 0,0 };


		if (__right < __left || __bottom < __top)
		{
				return(0);
		}

		coniox_init(NULL);
		r.Left = (SHORT) (__left - 1);
		r.Top = (SHORT) (__top - 1);
		r.Right = (SHORT) (__right - 1);
		r.Bottom = (SHORT) (__bottom - 1);
		s.X = (SHORT) (__right - __left + 1);
		s.Y = (SHORT) (__bottom - __top + 1);
		ci = (CHAR_INFO *) malloc(s.X * s.Y * sizeof(CHAR_INFO));
		if (!ci)
		{
				return(0);
		}
		buf = (char_info *) __destin;
		if (ReadConsoleOutput(coniox_console_output, ci, s, c, &r ))
		{
				for (i = 0; i < s.X * s.Y; i++)
				{
						#if UNICODE
								buf[i].letter = (char) ci[i].Char.UnicodeChar;
						#else
								buf[i].letter = ci[i].Char.AsciiChar;
						#endif
						buf[i].attr = (unsigned char) ci[i].Attributes;
				}
		}
		free(ci);
		return(0);
}


/* ----------------------------------------------------------------------------------------------------------------- */
int movetext(int __left, int __top, int __right, int __bottom, int __destleft, int __desttop)
{
/*
		CHAR_INFO * buffer;
		SMALL_RECT r;
		COORD c = { 0,0 }, s;


		coniox_init(NULL);
		s.X = (SHORT) __right - __left + 1;
		s.Y = (SHORT) __bottom - __top + 1;
		if ( s.X <= 0 || s.Y <= 0 )
		{
				return(0);
		}
		buffer = malloc(s.X * s.Y * sizeof( CHAR_INFO ));
		if (!buffer)
		{
				return(0);
		}
		r.Left = (SHORT) __left - 1;
		r.Top = (SHORT) __top - 1;
		r.Right = (SHORT) __right - 1;
		r.Bottom = (SHORT) __bottom - 1;
		if (ReadConsoleOutput(coniox_console_output, buffer, s, c, &r))
		{
				WriteConsoleOutput(coniox_console_output, buffer, s, c, &r);
		}
		free(buffer);
		return(0);
*/


		COORD c;
		SMALL_RECT r;
		CHAR_INFO ci;

		coniox_init(NULL);

		r.Left = (SHORT) (ti.winleft + __left -1);
		r.Top = (SHORT) (ti.wintop + __top-1);
		r.Right = (SHORT) (ti.winleft + __right-1);
		r.Bottom = (SHORT) (ti.wintop + __bottom-1);
		c.X = (SHORT) (ti.winleft + __destleft-1);
		c.Y = (SHORT) (ti.wintop + __desttop-1);
		#if UNICODE
				ci.Char.UnicodeChar = L' ';
		#else
			ci.Char.AsciiChar = ' ';
		#endif
		ci.Attributes = ti.attribute;
		ScrollConsoleScreenBuffer(coniox_console_output, &r, NULL, c, &ci);
		return(0);
}


/* ----------------------------------------------------------------------------------------------------------------- */
int puttext(int __left, int __top, int __right, int __bottom, void *__source)
{
		int i;
		SMALL_RECT r;
		CHAR_INFO *buffer;
		char_info *ci;
		COORD s, c = { 0,0 };


		if ( __right < __left || __bottom < __top)
		{
				return(0);
		}

		coniox_init(NULL);

		r.Left = (SHORT) (__left - 1);
		r.Top = (SHORT) (__top - 1);
		r.Right = (SHORT) (__right - 1);
		r.Bottom = (SHORT) (__bottom - 1);
		s.X = (SHORT) (__right - __left + 1);
		s.Y = (SHORT) (__bottom - __top + 1);

		buffer = malloc(s.X * s.Y * sizeof(CHAR_INFO));
		if (!buffer)
		{
				return(0);
		}
		ci = (char_info *) __source;
		for (i = 0; i < s.X * s.Y; i++)
		{
				#if UNICODE
						buffer[i].Char.UnicodeChar = (wchar_t) ci[i].letter;
				#else
						buffer[i].Char.AsciiChar = (unsigned char) ci[i].letter;
				#endif
				buffer[i].Attributes = ci[i].attr;
		}
		WriteConsoleOutput(coniox_console_output, buffer, s, c, &r);
		free(buffer);
		return(0);
}



/* ----------------------------------------------------------------------------------------------------------------- */
int putwtext(int __left, int __top, int __right, int __bottom, const wchar_info *__source)
{
		int i;
		SMALL_RECT r;
		CHAR_INFO* buffer;
		COORD s, c = { 0,0 };


		if ((__right < __left) || (__bottom < __top))
		{
				return(0);
		}
		coniox_init(NULL);
		r.Left = (SHORT) (__left - 1);
		r.Top = (SHORT)( __top - 1);
		r.Right = (SHORT) (__right - 1);
		r.Bottom = (SHORT) (__bottom - 1);
		s.X = (SHORT) (__right - __left + 1);
		s.Y = (SHORT) (__bottom - __top + 1);
		buffer = malloc(s.X * s.Y * sizeof(CHAR_INFO));
		if (!buffer)
		{
				return(0);
		}
		for ( i = 0; i < s.X * s.Y; i++ )
		{
				#if UNICODE
						buffer[i].Char.UnicodeChar = __source[i].letter;
				#else
						buffer[i].Char.AsciiChar = (char) __source[i].letter;
				#endif
				buffer[i].Attributes = __source[i].attr;
		}
		WriteConsoleOutput(coniox_console_output, buffer, s, c, &r );
		free(buffer);
		return(0);
}


/* ----------------------------------------------------------------------------------------------------------------- */
int getwtext(int __left, int __top, int __right, int __bottom, wchar_info *__destin)
{
		int i;
		SMALL_RECT r;
		CHAR_INFO* buffer;
		COORD s, c = { 0,0 };


		if (__right < __left || __bottom < __top)
		{
				return(0);
		}
		coniox_init(NULL);
		r.Left = (SHORT) (__left - 1);
		r.Top = (SHORT) (__top - 1);
		r.Right = (SHORT) (__right - 1);
		r.Bottom = (SHORT) (__bottom - 1);
		s.X = (SHORT) (__right - __left + 1);
		s.Y = (SHORT) (__bottom - __top + 1);
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
						break;
		}

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
				ti.currmode = (unsigned short) __newmode;
		}
		#endif
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
						break;
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
		coniox_init(NULL);
		Sleep(ms);
}

#endif //  ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)





/* ----------------------------------------------------------------------------------------------------------------- */
/* DOS ONLY (Non portable) */
/* ----------------------------------------------------------------------------------------------------------------- */
#if ((__DOS__) || (__MSDOS__))
#include <dos.h>
#include <bios.h>

int coniox_setcursortype = _NORMALCURSOR;
int coniox_basecrt = 0x3D4;

#if !defined(__TURBOC__)
		#include "conio.h"
		#define outportb				outp
		#define outport		   outpw
		#define inportb		   inp
		#define inport		      inpw
#endif

#if defined(__WATCOMC__)
		#include <i86.h>
		#if (!defined(__FLAT__))
			//#define MK_FP((s), (o))		((s<<16) | o)
		#endif
#endif

#define peekb(s,o)			    (*((unsigned char  __far *)MK_FP((s),(o))))
#define peekw(s,o)			    (*((unsigned short  __far *)MK_FP((s),(o))))
#define peekl(s,o)			    (*((unsigned long __far *)MK_FP((s),(o))))
#define pokeb(s,o,x)		(*((unsigned char __far *)MK_FP((s),(o))) = (unsigned char)(x))
#define pokew(s,o,x)		(*((unsigned short  __far *)MK_FP((s),(o))) = (unsigned short)(x))
#define pokel(s,o,x)		(*((unsigned long __far *)MK_FP((s),(o))) = (unsigned long)(x))

/* ----------------------------------------------------------------------------------------------------------------- */
#if (defined(__FLAT__))
		unsigned short *coniox_vram;
		unsigned short *coniox_currentoffset;
		#define coniox_far
		#define coniox_int86 int386
		#if defined(__WATCOMC__)
				unsigned short *coniox_offset(unsigned int piX, unsigned int piY);
				#pragma aux coniox_offset =										    \
						"			 .386													   "\
						"			 movzx eax, byte ptr ti + 16				"\
						"			 imul eax, edi									    "\
						"			 add eax, esi									   "\
						"			 shl eax, 1												 "\
						"			 add eax, coniox_vram					   "\
						parm [ESI][EDI]												   \
						modify exact []																	 \
						value [EAX];
		#else
				#define coniox_offset(piX, piY) (coniox_vram + ((ti.screenwidth * (piY)) + (piX)))
		#endif
#else
		unsigned short far *coniox_vram;
		unsigned short far *coniox_currentoffset;
		#define coniox_far far
		#define coniox_int86 int86
		#if defined(__WATCOMCToDo__)
				unsigned short far *coniox_offset(unsigned int piX, unsigned int piY);
				#pragma aux coniox_offset =										    \
						"			 .8086													    "\
						"			 xor ah, ah												 "\
						"			 mov al, byte ptr ti + 16					    "\
						"			 mul di														 "\
						"			 add ax, si										 "\
						"			 shl ax, 1												    "\
						"			 mov dx, word ptr coniox_vram	   "\
						parm [SI][DI]													 \
						modify exact []																	 \
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
					"			 .386												      "\
					"			      push ax										  "\
					"			      push ax										  "\
					"			      pop eax										  "\
					"			      push ecx										 "\
					"			      shr ecx, 1										     "\
					"			      rep stosd										       "\
					"			      pop ecx										   "\
					"			      and ecx, 1										     "\
					"			      rep stosw										       "\
					parm [EDI][AX][ECX]													     \
					modify exact []																	 \
					value [EDI];
		#else
			#pragma aux coniox_fmemsetw =										\
					"			 .8086													    "\
					"			 rep stosw												   "\
					parm [ES DI][AX][CX]													   \
					modify exact []																	 \
					value [ES DI];
		#endif
#else
	//Todo: Use inline asm
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


void coniox_idle(void);


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

		//Base for 6845 CRT controller
		coniox_basecrt = peekw(0x40, 0x63);     //0x3D4

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
			ti.screenwidth = peekb(0x40, 0x4A);
			if (ti.currmode == C4350)
			{
				ti.screenheight = 50;
			}
			else
			{
				ti.screenheight = peekb(0x40, 0x84) + 1;
			}
			//ti.attribute = peekb(0x40, 0x66);
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
				ti.screenheight = 25;   //Todo: Fill rows and attribute */
			}
		}
		ti.attribute = 7;
		ti.winleft = 1;
		ti.wintop = 1;
		ti.winright = ti.screenwidth;
		ti.winbottom = ti.screenheight;
}



/* ----------------------------------------------------------------------------------------------------------------- */
int cputs(const char *__str)
{
	char c;
	int k = 0;
	int oldx, oldy;
	
	coniox_init(NULL);
	coniox_currentoffset = (unsigned short coniox_far *) coniox_offset(ti.winleft + ti.curx - 2, ti.wintop + ti.cury - 2);

	while (c = *__str)
	{
		switch (c)
		{
			case '\r':
				coniox_currentoffset-= (ti.curx - 1);
				ti.curx = 1;
				break;
			case '\n':
				if (ti.cury < ti.winbottom - ti.wintop + 1)
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
				coniox_putchattrcursor(c, ti.attribute);
				if (ti.curx + 1 > ti.winright - ti.winleft + 1)
				{
					if (_wscroll)
					{
						if (ti.cury < ti.winbottom - ti.wintop + 1)
						{
							coniox_currentoffset = coniox_currentoffset + ti.screenwidth - (ti.curx - 1);
							ti.curx = 1;
							ti.cury++;
						}
						else
						{
							coniox_currentoffset-= (ti.curx - 1);
							oldy = ti.cury;
							ti.curx = 1;
							ti.cury = 1;
							delline();
							ti.cury = oldy;
						}
					}
					else
					{
						coniox_currentoffset-= (ti.curx - 1);
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
void coniox_putchattrcursor(int ch, int attr)
{
		union REGS r;

		if (directvideo)
		{
			*coniox_currentoffset = (attr << 8) | ch;
		}
		else
		{
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
void coniox_putchxyattr(int x, int y, int ch, int attr)
{
		unsigned short coniox_far *p;
		union REGS r;

		if (directvideo)
		{
			p = (unsigned short coniox_far *) coniox_offset(x - 1, y - 1);
			*p = (attr << 8) | ch;
		}
		else
		{
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
		v = (attr << 8) | ch;
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
void delay (unsigned int ms)
{
	unsigned long lTicks;

	coniox_init(NULL);
	lTicks = peekl(0, 0x46C);
	while (peekl(0, 0x46C) < (lTicks + (unsigned long) ms * 182 / 10000))
	{
		coniox_idle();
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
			//For 4350 additionally we need to load 8x8 EGA/VGA font
			#if defined(__WATCOMC__)
				r.w.ax = 0x1112;
				r.w.bx = 0;
			#else
				r.x.ax = 0x1112;
				r.x.bx = 0;
			#endif
			coniox_int86(0x10, &r, &r);

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
	_setcursortype(_NORMALCURSOR);
}


/* ----------------------------------------------------------------------------------------------------------------- */
void _setcursortype(int __cur_t)
{
	union REGS r;

	coniox_init(NULL);
	coniox_setcursortype = __cur_t;
	if (directvideo)
	{
		switch (__cur_t)
		{
			case _NOCURSOR:
					outportb(coniox_basecrt, 0x0A);
					outportb(coniox_basecrt + 1, 0x20);
					break;
			case _SOLIDCURSOR:
					outportb(coniox_basecrt, 0x0A);
					outportb(coniox_basecrt + 1, (inportb(0x3D5) & 0xC0) | 0);
					outportb(coniox_basecrt, 0x0B);
					outportb(coniox_basecrt + 1, (inportb(0x3D5) & 0xE0) | 15);
					break;
			case _NORMALCURSOR:
			default:
					outportb(coniox_basecrt, 0x0A);
					outportb(coniox_basecrt + 1, (inportb(0x3D5) & 0xC0) | 6);
					outportb(coniox_basecrt, 0x0B);
					outportb(coniox_basecrt + 1, (inportb(0x3D5) & 0xE0) | 7);
					break;
		}
	}
	else
	{
		switch (__cur_t)
		{
			case _NOCURSOR:
					r.h.ah = 0x1;
					#if defined(__WATCOMC__)
						r.w.cx = 0x2607;
					#else
						r.x.cx = 0x2607;
					#endif
					coniox_int86(0x10, &r, &r);
					break;
			case _SOLIDCURSOR:
					r.h.ah = 0x1;
					#if defined(__WATCOMC__)
						r.w.cx = 0x07;
					#else
						r.x.cx = 0x07;
					#endif
					coniox_int86(0x10, &r, &r);
					break;
			case _NORMALCURSOR:
					r.h.ah = 0x1;
					#if defined(__WATCOMC__)
						r.w.cx = 0x607;
					#else
						r.x.cx = 0x607;
					#endif
					coniox_int86(0x10, &r, &r);
					break;
		}
	}
}


/* ----------------------------------------------------------------------------------------------------------------- */
void textattr(int __newattr)
{
	coniox_init(NULL);
	ti.attribute = (unsigned short) __newattr;
}


/* ----------------------------------------------------------------------------------------------------------------- */
void gotoxy(int __x, int __y)
{
	union REGS r;
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
			r.h.ah = 2;
			r.h.bh = 0;
			r.h.dh = ti.wintop + ti.cury - 2;
			r.h.dl = ti.winleft + ti.curx - 2;
			coniox_int86(0x10, &r, &r);
		}
	}
}



/* ----------------------------------------------------------------------------------------------------------------- */
/*Borra la línea donde se encuentre el cursor y mueve todas las líneas inferiores a una
línea anterior. La función delline funciona en la ventana de texto activa.*/
void delline(void)
{
	union REGS r;
	coniox_init(NULL);

	if (directvideo)
	{
		movetext(ti.winleft, ti.cury + ti.wintop, ti.winright, ti.winbottom, ti.winleft, ti.cury + ti.wintop - 1);
		coniox_putchxyattrwh(ti.winleft, ti.winbottom, ' ', ti.attribute, ti.winright - ti.winleft + 1, 1);
	}
	else
	{
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
	//gotoxy(ti.curx, ti.cury);

	//__scroll(UP, _video.windowx1, _wherey(),_video.windowx2, _video.windowy2, 1);
}


/* ----------------------------------------------------------------------------------------------------------------- */
int gettext(int __left, int __top, int __right, int __bottom, void* __destin)
{
	unsigned short coniox_far* p;
	int y1;

	coniox_init(NULL);

	p = (unsigned short coniox_far*) coniox_offset(__left - 1, __top - 1);
	for (y1 = __top; y1 <= __bottom; y1++)
	{
		_fmemcpy((unsigned short coniox_far *) __destin, p, (__right - __left + 1) << 1);
		p += ti.screenwidth;
		(unsigned short *) __destin += (__right - __left + 1);
	}
	return(1);
}


/* ----------------------------------------------------------------------------------------------------------------- */
int puttext(int __left, int __top, int __right, int __bottom, void* __source)
{
	unsigned short coniox_far* p;
	int y1;

	coniox_init(NULL);

	p = (unsigned short coniox_far*) coniox_offset(__left - 1, __top - 1);
	for (y1 = __top; y1 <= __bottom; y1++)
	{
		_fmemcpy(p, (unsigned short coniox_far *) __source, (__right - __left + 1) << 1);
		p += ti.screenwidth;
		(unsigned short *) __source += (__right - __left + 1);
	}
	return(1);
}



/* ----------------------------------------------------------------------------------------------------------------- */
int movetext(int __left, int __top, int __right, int __bottom, int __destleft, int __desttop)
{
	unsigned short coniox_far* p;
	unsigned short coniox_far* __destin;
	int y1;

	coniox_init(NULL);

	p = (unsigned short coniox_far*) coniox_offset(__left - 1, __top - 1);
	__destin = (unsigned short coniox_far*) coniox_offset(__destleft - 1, __top + __desttop - __top - 1);
	for (y1 = __top; y1 <= __bottom; y1++)
	{
		_fmemmove((unsigned short coniox_far *) __destin, p, (__right - __left + 1) << 1);
		p += ti.screenwidth;
		__destin += ti.screenwidth;
	}
	return(1);
}


/* ----------------------------------------------------------------------------------------------------------------- */
void clrscr(void)
{
	union REGS r;

	coniox_init(NULL);

	if (directvideo)
	{
		coniox_putchxyattrwh(ti.winleft, ti.wintop, ' ', ti.attribute, ti.winright - ti.winleft + 1, ti.winbottom - ti.wintop + 1);
		gotoxy(1, 1);
	}
	else
	{
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
		//Todo
		return(0);
}


/* ----------------------------------------------------------------------------------------------------------------- */
int kbhit(void)
{
		union REGS r;


		coniox_init(NULL);
		coniox_idle();
		r.h.ah = 1;
		coniox_int86(0x16, &r, &r);
		#if defined(__WATCOMC__)
			return(r.w.ax);
		#else
			return(r.x.ax);
		#endif
}


/* ----------------------------------------------------------------------------------------------------------------- */
int getch(void)
{
		union REGS r;

		coniox_init(NULL);
		coniox_idle();
		r.h.ah = 0;
		coniox_int86(0x16, &r, &r);
		return((int) r.h.al);
}

#endif
