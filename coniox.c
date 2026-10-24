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
	void coniox_putwchxyattr(int x, int y, wchar_t ch, int attr);
	#include <wchar.h>
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
void coniox_readnormattr(void);



/* ----------------------------------------------------------------------------------------------------------------- */
struct text_info ti;
int directvideo = 1;
int _wscroll = 1;
int coniox_setcursortype = _NORMALCURSOR;
int coniox_cursor_dirty = 0;	/* 1 = physical cursor lags behind ti.curx/ti.cury */

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
#define coniox_offset(piX, piY) (coniox_vram + ((ti.screenwidth * (piY)) + (piX)))
static unsigned short coniox_far *coniox_cell;

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
	#define coniox_fmemcpy      _fmemmove
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
#endif



/* ----------------------------------------------------------------------------------------------------------------- */
/* INTERNAL (Private)*/
/* ----------------------------------------------------------------------------------------------------------------- */
int coniox_vsscanf(const char *buffer, const char *format, va_list argPtr)
{
	void *a[40] = {NULL};	// Room for up to 20 arguments plus possible sizes
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
			// Skip optional width modifiers
			while (*p >= '0' && *p <= '9')
			{
				p++;
			}
			// Skip optional length modifiers (l, h, z, j, t, L)
			while (*p == 'l' || *p == 'h' || *p == 'z' || *p == 'j' || *p == 't' || *p == 'L')
			{
				p++;
			}
			// A trailing % consumes no argument
			if (!*p)
			{
				break;
			}
			// Specifiers that take an extra size argument
			#ifdef _MSC_VER
				if (count >= 39)	/* a[40]: buffer+size pair, last index 39 */
				{
					break;
				}
			#else
				if (count >= 20)	/* sscanf is called with 20 arguments */
				{
					break;
				}
			#endif
			if (*p == 's' || *p == 'c' || *p == '[')
			{
				a[count++] = va_arg(argPtr, void *);            /* buffer */
				#ifdef _MSC_VER
					a[count++] = va_arg(argPtr, size_t *);	/* size: sscanf_s only */
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
	int aw;


	coniox_init(NULL);
	aw = ti.winright - ti.winleft - ti.curx + 2;	/* cells remaining to the end of the line */
	#if ((__DOS__) || (__MSDOS__))
		if (directvideo)
		{
			/* coniox_cell already points at the cursor cell: one direct fill, no offset computation. */
			coniox_fmemsetw(coniox_cell, (short) ((ti.attribute << 8) | ' '), (size_t) aw);
			return;
		}
	#endif
	coniox_putchxyattrwh(ti.winleft + ti.curx - 1, ti.wintop + ti.cury - 1, ' ', ti.attribute, aw, 1);
}


/* ----------------------------------------------------------------------------------------------------------------- */
void inittextinfo(void)
{
	coniox_init(NULL);
	coniox_readnormattr();	/* re-read the real startup attribute, not a fixed 7 */
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
	/* Deferred physical cursor sync: putch()/putwch() only set coniox_cursor_dirty; the wait points (kbhit/getch) and gotoxy() do the real sync. */
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
			coniox_cell -= ti.curx - 1;
#endif
			ti.curx = 1;
			coniox_cursor_dirty = 1;
			break;
		case '\n':
			if (ti.cury < winheight)
			{
#if ((__DOS__) || (__MSDOS__))
				coniox_cell += ti.screenwidth;
#endif
				ti.cury++;
				coniox_cursor_dirty = 1;
			}
			else if (_wscroll)
			{
				/* LF: keep the column, like Borland's putch (delline() never moves the physical cursor). */
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
		case '\b':
			if (ti.curx > 1)
			{
#if ((__DOS__) || (__MSDOS__))
				--coniox_cell;
#endif
				ti.curx--;
				coniox_cursor_dirty = 1;
			}
			break;
		default:
#if ((__DOS__) || (__MSDOS__))
			if (directvideo)
			{
				/* Fast path: write straight into the cached coniox_cell (the same cell coniox_putchxyattr() would compute). */
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
				++coniox_cell;
#endif
				ti.curx++;
				coniox_cursor_dirty = 1;
			}
			else
			{
				if (_wscroll)
				{
					if (ti.cury < winheight)
					{
#if ((__DOS__) || (__MSDOS__))
						coniox_cell += ti.screenwidth - winwidth + 1;
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
						coniox_cell -= winwidth - 1;
#endif
						coniox_cursor_dirty = 1;
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

	while (ch != '\r' && ch != EOF)
	{
		ch = getch();
		if (ch == 0)
		{
			getch();
		}
		else if (ch != '\r' && ch != EOF)
		{
			switch (ch)
			{
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

	#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32) || defined(__DJGPP__)
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
	return((int) strlen(buffer));
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
	cgets((char *) buffer);
	va_start(ap, __format);
	r = coniox_vsscanf((const char *) buffer + 2, __format, ap);
	va_end(ap);
	return r;
}


/* ----------------------------------------------------------------------------------------------------------------- */
int getche(void)
{
	int ch;

	ch = getch();
	if (ch != 0 && ch != EOF)
	{
		putch(ch);
		coniox_flush_cursor();
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

	coniox_init(NULL);
	cputs(__prompt);
	x = ti.curx;
	y = ti.cury;

	while (ch != '\r' && ch != EOF)
	{
		ch = getch();
		if (ch == 0)
		{
			getch();
		}
		else if (ch != '\r' && ch != EOF)
		{
			switch (ch)
			{
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
int coniox_vswscanf(const wchar_t* buffer, const wchar_t* format, va_list argPtr)
{
	void *a[20] = { NULL };
	size_t count = 0;
	const wchar_t *p;
	wchar_t c;

	p = format;
	while ((c = *p++) != 0)
	{
		if (c == L'%' && p[0] != L'*' && p[0] != L'%')
		{
			if (!p[0])
			{
				break;
			}
			if (count >= 20)	/* plain swscanf: one argument per specifier */
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

	while (ch != L'\r' && ch != (wchar_t) EOF)
	{
		ch = getwch();
		if (ch == 0)
		{
			getwch();
		}
		else if (ch != L'\r' && ch != (wchar_t) EOF)
		{
			switch (ch)
			{
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
			coniox_cursor_dirty = 1;
			break;
		case L'\n':
			if (ti.cury < winheight)
			{
				ti.cury++;
				coniox_cursor_dirty = 1;
			}
			else if (_wscroll)
			{
				/* LF: keep the column, like Borland putwch (delline() never moves the physical cursor). */
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
				coniox_cursor_dirty = 1;
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
	return((int) wcslen(buffer));
}
#endif



/* ----------------------------------------------------------------------------------------------------------------- */
/* WINDOWS ONLY (Non portable) */
/* ----------------------------------------------------------------------------------------------------------------- */
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
#include <windows.h>
#include <Wincon.h>
HANDLE coniox_console_output = NULL;
HANDLE coniox_console_input = NULL;

static CHAR_INFO coniox_charinfo[CONIOX_MAX_CHARINFO];


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
			/* Clear the handles so later calls can retry (e.g. after a later AttachConsole); leaving them set would make the library look initialized with a zeroed ti forever. */
			coniox_console_output = NULL;
			coniox_console_input = NULL;
			return;
		}
		FreeConsole();
		AllocConsole();
		/* The new console has different standard handles: fetch them again. */
		coniox_console_output = GetStdHandle(STD_OUTPUT_HANDLE);
		coniox_console_input = GetStdHandle(STD_INPUT_HANDLE);
	}
	ti.normattr = (unsigned short) info.wAttributes;
	ti.winleft = (unsigned short) info.srWindow.Left + 1;
	ti.wintop = (unsigned short) info.srWindow.Top + 1;
	ti.winright = (unsigned short) info.srWindow.Right + 1;
	ti.winbottom = (unsigned short) info.srWindow.Bottom + 1;
	ti.screenwidth = (unsigned short) info.dwSize.X;
	ti.screenheight = (unsigned short) info.dwSize.Y;
	ti.curx = (unsigned short) info.dwCursorPosition.X + 1;
	ti.cury = (unsigned short) info.dwCursorPosition.Y + 1;
	ti.attribute = (unsigned short) info.wAttributes;
	ti.currmode = (unsigned short) ((ti.screenwidth <= 40) ? C40 : C80);
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
		SetConsoleTitleA((LPCSTR) title);
	}
}

/* ----------------------------------------------------------------------------------------------------------------- */
void coniox_readnormattr(void)
{
	CHAR_INFO ci;
	COORD c = { 0, 0 }, s = { 1, 1 };
	SMALL_RECT r;

	coniox_init(NULL);
	/* Read the real startup attribute for ti.normattr from the cell under the cursor, like the DOS version: GetConsoleScreenBufferInfo returns the attribute textattr() left, not the startup one. If the read fails (redirected output) the coniox_init() value is kept. */
	r.Left = (short) (ti.winleft + ti.curx - 2);
	r.Top = (short) (ti.wintop + ti.cury - 2);
	r.Right = r.Left;
	r.Bottom = r.Top;
	if (ReadConsoleOutput(coniox_console_output, &ci, s, c, &r))
	{
		ti.normattr = (unsigned short) ci.Attributes;
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
	CHAR_INFO *ci = coniox_charinfo;

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
		/* Carriage return: back to start of line. WriteConsoleOutput neither reads nor moves the console cursor; the closing gotoxy() syncs it. */
		if (*__str == '\r')
		{
			ti.curx = 1;
			++__str;
			continue;
		}

		/* Line feed: advance or scroll the window. */
		if (*__str == '\n')
		{
			if (ti.cury < (int) winheight)
			{
				ti.cury++;
			}
			else if (_wscroll)
			{
				/* LF: keep the column, like Borland cputs (delline() only reads ti). */
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

		/* Backspace: move back without erasing. */
		if (*__str == '\b')
		{
			if (ti.curx > 1)
			{
				ti.curx--;
			}
			++__str;
			continue;
		}

		/* Group printable characters up to the next control, the window edge or the batch size: one WriteConsoleOutputA per run. */
		run = __str;
		runlen = 0;
		if (ti.curx > (int) winwidth)
		{
			ti.curx = (int) winwidth;
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

		/* A run never crosses the window edge: the wrap is decided once. */
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
				ti.curx = (int) winwidth;	/* _wscroll = 0: the next run overwrites the edge cell */
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
	CHAR_INFO *ci = coniox_charinfo;

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
		/* Carriage return: back to start of line (deferred sync). */
		if (*__str == L'\r')
		{
			ti.curx = 1;
			++__str;
			continue;
		}

		/* Line feed: advance or scroll the window. */
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

		/* Backspace: move back without erasing. */
		if (*__str == L'\b')
		{
			if (ti.curx > 1)
			{
				ti.curx--;
			}
			++__str;
			continue;
		}

		/* Unicode twin of cputs(): one WriteConsoleOutputW per run. */
		run = __str;
		runlen = 0;
		if (ti.curx > (int) winwidth)
		{
			ti.curx = (int) winwidth;
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

		/* A run never crosses the window edge: the wrap is decided once. */
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
				ti.curx = (int) winwidth;	/* _wscroll = 0: the next run overwrites the edge cell */
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
	CHAR_INFO ci;
	SMALL_RECT r;
	COORD c = {0, 0}, s = {1, 1};


	r.Left = (short) (x - 1);
	r.Top = (short) (y - 1);
	r.Right = (short) (x - 1);
	r.Bottom = (short) (y - 1);
	ci.Char.AsciiChar = (char) ch;
	ci.Attributes = (WORD) attr;
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
	ci = (total <= CONIOX_MAX_CHARINFO) ? coniox_charinfo : (CHAR_INFO *) malloc((size_t) total * sizeof(CHAR_INFO));
	if (ci == NULL)
	{
		return;
	}

	#ifdef UNICODE
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
	if (ci != coniox_charinfo)
	{
		free(ci);
	}
}



/* ----------------------------------------------------------------------------------------------------------------- */
wchar_t getwch(void)
{
	int ch = getch();

	return((ch == EOF) ? (wchar_t) EOF : coniox_ansi2unicode((char) ch));
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
	if (ch != (wchar_t) EOF)
	{
		putwch(ch);
		coniox_flush_cursor();
	}
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
int getch(void)
{
	char ch;	/* ReadConsoleA writes 1 byte */
	DWORD read, mode = 0;
	int r;

	coniox_init(NULL);	/* getch() may be the first conio call: ensure handles */
	coniox_flush_cursor();
	if (GetConsoleMode(coniox_console_input, &mode))
	{
		SetConsoleMode(coniox_console_input, mode & ~ENABLE_ECHO_INPUT & ~ENABLE_PROCESSED_INPUT & ~ENABLE_LINE_INPUT);
	}
	r = (ReadConsoleA(coniox_console_input, &ch, 1, &read, NULL) && read != 0) ? (int) (unsigned char) ch : EOF;
	if (mode)
	{
		SetConsoleMode(coniox_console_input, mode);	/* restore only a mode that was really read */
	}
	return(r);
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
	coniox_cursor_dirty = 0;	/* gotoxy leaves the physical cursor up to date (or hidden) */
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
	ti.attribute = (unsigned short) (__newattr & 0xFF);
	SetConsoleTextAttribute(coniox_console_output, ti.attribute);
}


/* ----------------------------------------------------------------------------------------------------------------- */
void delline(void)
{
	COORD c;
	SMALL_RECT r;
	CHAR_INFO ci;


	coniox_init(NULL);
	/* Last window line (or a one-line window): the scroll rectangle is empty and ScrollConsoleScreenBuffer would fail, so clear the line instead. */
	if (ti.cury >= ti.winbottom - ti.wintop + 1)
	{
		coniox_putchxyattrwh(ti.winleft, ti.wintop + ti.cury - 1, ' ', ti.attribute, ti.winright - ti.winleft + 1, 1);
		return;
	}
	c.X = ti.winleft - 1;
	c.Y = ti.wintop - 1 + ti.cury - 1;
	r.Left = ti.winleft - 1;
	r.Top = ti.wintop - 1 + ti.cury;
	r.Right = ti.winright - 1;
	r.Bottom = ti.winbottom - 1;
	ci.Attributes = ti.attribute;
	#ifdef UNICODE
		ci.Char.UnicodeChar = L' ';
	#else
		ci.Char.AsciiChar = ' ';
	#endif
	ScrollConsoleScreenBuffer(coniox_console_output, &r, NULL, c, &ci);
	/* ScrollConsoleScreenBuffer does not move the console cursor; the sync happens via coniox_flush_cursor(). */
}


/* ----------------------------------------------------------------------------------------------------------------- */
void insline(void)
{
	COORD c;
	SMALL_RECT r;
	CHAR_INFO ci;


	coniox_init(NULL);
	/* Last window line (or a one-line window): the scroll rectangle is empty and ScrollConsoleScreenBuffer would fail, so clear the line instead. */
	if (ti.cury >= ti.winbottom - ti.wintop + 1)
	{
		coniox_putchxyattrwh(ti.winleft, ti.wintop + ti.cury - 1, ' ', ti.attribute, ti.winright - ti.winleft + 1, 1);
		return;
	}
	c.X = ti.winleft - 1;
	c.Y = ti.wintop - 1 + ti.cury;
	r.Left = ti.winleft - 1;
	r.Top = ti.wintop - 1 + ti.cury - 1;
	r.Right = ti.winright - 1;
	r.Bottom = ti.winbottom - 2;
	ci.Attributes = ti.attribute;
	#ifdef UNICODE
		ci.Char.UnicodeChar = L' ';
	#else
		ci.Char.AsciiChar = ' ';
	#endif
	ScrollConsoleScreenBuffer(coniox_console_output, &r, NULL, c, &ci);
	/* ScrollConsoleScreenBuffer does not move the console cursor; the sync happens via coniox_flush_cursor(). */
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
	/* Borland-style validation: rectangle inside the absolute screen. */
	if (__destin == NULL || __left < 1 || __top < 1 || __right > ti.screenwidth || __bottom > ti.screenheight || __right < __left || __bottom < __top)
	{
		return(0);
	}
	total = (__right - __left + 1) * (__bottom - __top + 1);
	ci = (total <= CONIOX_MAX_CHARINFO) ? coniox_charinfo : (CHAR_INFO *) malloc((size_t) total * sizeof(CHAR_INFO));
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
		if (ci != coniox_charinfo)
		{
			free(ci);
		}
		return(0);
	}
	for (i = 0; i < s.X * s.Y; i++)
	{
		#ifdef UNICODE
			buf[i] = (ci[i].Char.UnicodeChar & 0xFF) + ((ci[i].Attributes & 0xFF) << 8);
		#else
			buf[i] = (ci[i].Char.AsciiChar & 0xFF) + ((ci[i].Attributes & 0xFF) << 8);
		#endif
	}
	if (ci != coniox_charinfo)
	{
		free(ci);
	}
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

	/* Borland-style validation: source and destination inside the screen. */
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
	scratch = (total <= CONIOX_MAX_CHARINFO) ? coniox_charinfo : (CHAR_INFO *) malloc((size_t) total * sizeof(CHAR_INFO));
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
		if (scratch != coniox_charinfo)
		{
			free(scratch);
		}
		return(0);
	}
	r.Left = (short) __destleft - 1;
	r.Top = (short) __desttop - 1;
	r.Right = (short) (__destleft + w - 2);
	r.Bottom = (short) (__desttop + h - 2);
	WriteConsoleOutput(coniox_console_output, scratch, s, c, &r);
	if (scratch != coniox_charinfo)
	{
		free(scratch);
	}
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
	/* Borland-style validation: rectangle inside the absolute screen. */
	if (__source == NULL || __left < 1 || __top < 1 || __right > ti.screenwidth || __bottom > ti.screenheight || __right < __left || __bottom < __top)
	{
		return(0);
	}
	total = (__right - __left + 1) * (__bottom - __top + 1);
	buffer = (total <= CONIOX_MAX_CHARINFO) ? coniox_charinfo : (CHAR_INFO *) malloc((size_t) total * sizeof(CHAR_INFO));
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
		#ifdef UNICODE
			buffer[i].Char.UnicodeChar = (wchar_t) (ci[i] & 0xFF);
		#else
			buffer[i].Char.AsciiChar = (unsigned char) ci[i];
		#endif
		buffer[i].Attributes = (ci[i] >> 8) & 0xFF;
	}
	WriteConsoleOutput(coniox_console_output, buffer, s, c, &r);
	if (buffer != coniox_charinfo)
	{
		free(buffer);
	}
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
	/* Borland-style validation: rectangle inside the absolute screen. */
	if (__source == NULL || __left < 1 || __top < 1 || __right > ti.screenwidth || __bottom > ti.screenheight || __right < __left || __bottom < __top)
	{
		return(0);
	}
	total = (__right - __left + 1) * (__bottom - __top + 1);
	buffer = (total <= CONIOX_MAX_CHARINFO) ? coniox_charinfo : (CHAR_INFO *) malloc((size_t) total * sizeof(CHAR_INFO));
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
		#ifdef UNICODE
			buffer[i].Char.UnicodeChar = __source[i].letter;
		#else
			buffer[i].Char.AsciiChar = (char) __source[i].letter;
		#endif
		buffer[i].Attributes = __source[i].attr;
	}
	WriteConsoleOutput(coniox_console_output, buffer, s, c, &r);
	if (buffer != coniox_charinfo)
	{
		free(buffer);
	}
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
	/* Borland-style validation: rectangle inside the absolute screen. */
	if (__destin == NULL || __left < 1 || __top < 1 || __right > ti.screenwidth || __bottom > ti.screenheight || __right < __left || __bottom < __top)
	{
		return(0);
	}
	total = (__right - __left + 1) * (__bottom - __top + 1);
	buffer = (total <= CONIOX_MAX_CHARINFO) ? coniox_charinfo : (CHAR_INFO *) malloc((size_t) total * sizeof(CHAR_INFO));
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
		if (buffer != coniox_charinfo)
		{
			free(buffer);
		}
		return(0);
	}
	for (i = 0; i < s.X * s.Y; i++)
	{
		#ifdef UNICODE
			__destin[i].letter = buffer[i].Char.UnicodeChar;
		#else
			__destin[i].letter = (unsigned char) buffer[i].Char.AsciiChar;
		#endif
		__destin[i].attr = buffer[i].Attributes;
	}
	if (buffer != coniox_charinfo)
	{
		free(buffer);
	}
	return(1);
}


/* ----------------------------------------------------------------------------------------------------------------- */
void textmode(int __newmode)
{
	#if defined(__ZTC__)
		CONSOLE_SCREEN_BUFFER_INFO info;
	#else
		CONSOLE_SCREEN_BUFFER_INFOEX info;
	#endif


	coniox_init(NULL);

	#if defined(__ZTC__)
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
		/* Mode table: first match wins; any unlisted mode falls back to 80x25. */
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
				info.dwSize.X = 80;
				info.dwSize.Y = 25;
			}
		}
		//DigitalMars crashes if used
		#if (!defined(__ZTC__))
			if (SetConsoleScreenBufferInfoEx(coniox_console_output, &info))
			{
				SMALL_RECT r;

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

	/* gotoxy() does not move the physical cursor while hidden (_NOCURSOR); reposition it when re-enabled. */
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
		if (ir.EventType == KEY_EVENT && ir.Event.KeyEvent.bKeyDown && ir.Event.KeyEvent.uChar.AsciiChar != 0)
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
	ir[1] = ir[0];
	ir[1].Event.KeyEvent.bKeyDown = FALSE;
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
	ir[1] = ir[0];
	ir[1].Event.KeyEvent.bKeyDown = FALSE;
	if (WriteConsoleInput(coniox_console_input, ir, 2, &written) && written == 2)
	{
		return(__ch);
	}
	return((wchar_t) EOF);
}



/* ----------------------------------------------------------------------------------------------------------------- */
void delay(unsigned int ms)
{
	coniox_flush_cursor();
	Sleep(ms);
}

#endif /* ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32) */





/* ----------------------------------------------------------------------------------------------------------------- */
/* DOS ONLY (Non portable) */
/* ----------------------------------------------------------------------------------------------------------------- */
#if ((__DOS__) || (__MSDOS__))

void coniox_idle(void);
int coniox_get_is_emulator(void);
void coniox_blink(unsigned int blink);
void coniox_enhanced_palette(unsigned int pal);
int coniox_is_emulator = 0;

/* ----------------------------------------------------------------------------------------------------------------- */
void coniox_init(const void* title)
{
	unsigned int cursor;
	union REGS r;

	(void) title;                                   /* W303: suppress unused parameter warning */

	/* Already initialized? screenwidth is 0 only before the first coniox_init(); textmode() resets it to 0 to force reinitialization. */
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
			coniox_vram = (unsigned short *) (0x000B0000UL + __djgpp_conventional_base);
		}
		else
		{
			coniox_vram = (unsigned short *) (0x000B8000UL + __djgpp_conventional_base);
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
	/* Initial position of the putch()/cputs() cell cache: the just-read (curx, cury) cell; gotoxy() keeps it up to date. */
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

	coniox_init(NULL);
	/* Real screen attribute for ti.normattr, read at the logical cursor cell: the coniox_cell cache with directvideo=1, INT 10h AH=08h otherwise (the deferred cursor is flushed first). */
	if (directvideo)
	{
		ti.normattr = (unsigned short)((*coniox_cell >> 8) & 0xFF);
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
	unsigned int runlen, remaining;
	unsigned int winwidth, winheight;
	int oldx, oldy;
	unsigned short attrword;

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
		/* The putch()/gotoxy() cell cache already points at the (curx, cury) cell: the entry coniox_offset() is saved and re-synced on exit. */
		coniox_currentoffset = coniox_cell;
	}

	while (*__str)
	{
		/* Carriage return: back to start of line. */
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

		/* Line feed: advance or scroll the window. */
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

		/* Backspace: move back without erasing. */
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

		run = __str;
		runlen = 0;
		if (ti.curx > (int)winwidth)
		{
			ti.curx = (int)winwidth;
			if (directvideo)
			{
				/* Re-sync the offset with the clamp so the run paints inside the window. */
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
			/* Fast path: the whole run straight to VRAM. */
			coniox_putrun(coniox_currentoffset, run, attrword, runlen);
			coniox_currentoffset += runlen;
		}
		else
		{
			/* BIOS path: AH=02h positions, AH=09h writes. */
			unsigned int i;
			union REGS r;

			for (i = 0; i < runlen; i++)
			{
				r.h.ah = 0x02;
				r.h.bh = 0;
				r.h.dh = (unsigned char)(ti.wintop + ti.cury - 2);
				r.h.dl = (unsigned char)(ti.winleft + ti.curx - 2 + i);
				coniox_int86(0x10, &r, &r);

				/* Restore the registers AH=09h requires. */
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

		/* A run never crosses the window edge: the wrap is decided once. */
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
	/* Deferred physical cursor sync, same pattern as putch(): the real cursor is programmed at the next wait point via coniox_flush_cursor(). */
	coniox_cursor_dirty = 1;
	/* Leave the cell cache pointing at (curx, cury) for the next putch()/cputs(). */
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
	int y1, ylim;

	if (w <= 0 || h <= 0)
	{
		return;
	}

	if (directvideo)
	{
		unsigned short coniox_far *p;
		unsigned short v;
		unsigned int sw;

		v = (unsigned short)((attr << 8) | (ch & 0xFF));
		// fast-path 1x1 avoids fmemsetw + loop
		if ((w == 1) && (h == 1))
		{
			*(unsigned short coniox_far *) coniox_offset(x - 1, y - 1) = v;
			return;
		}
		sw = ti.screenwidth;
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
			/* Full-screen-width block: VRAM rows are contiguous, one fill covers all rows. */
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
		union REGS r;

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
int coniox_get_is_emulator(void)
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
		{0x00, 0x00, 0x00},
		{0x00, 0x00, 0x24},
		{0x00, 0x24, 0x04},
		{0x00, 0x22, 0x28},
		{0x24, 0x00, 0x00},
		{0x28, 0x08, 0x2A},
		{0x30, 0x30, 0x08},
		{0x2B, 0x2B, 0x2B},
		{0x00, 0x00, 0x1D},
		{0x04, 0x08, 0x3C},
		{0x00, 0x28, 0x0E},
		{0x00, 0x1F, 0x3F},
		{0x28, 0x04, 0x10},
		{0x2C, 0x08, 0x3F},
		{0x30, 0x2A, 0x10},
		{0x30, 0x30, 0x3F}
	};
	static const unsigned char coniox_default[16][3] =
	{
		{0x00, 0x00, 0x00},
		{0x00, 0x00, 0x2A},
		{0x00, 0x2A, 0x00},
		{0x00, 0x2A, 0x2A},
		{0x2A, 0x00, 0x00},
		{0x2A, 0x00, 0x2A},
		{0x2A, 0x2A, 0x00},
		{0x2A, 0x2A, 0x2A},
		{0x00, 0x00, 0x15},
		{0x00, 0x00, 0x3F},
		{0x00, 0x2A, 0x15},
		{0x00, 0x2A, 0x3F},
		{0x2A, 0x00, 0x15},
		{0x2A, 0x00, 0x3F},
		{0x2A, 0x2A, 0x15},
		{0x2A, 0x2A, 0x3F}
	};

	if (ti.currmode == MONO)
	{
		return;
	}

	/* pal = 0: default VGA palette; pal != 0: CONIOX enhanced palette. */
	coniox_rgb = pal ? coniox_palette : coniox_default;

	if (directvideo)
	{
		outportb(0x3C8, 0x00);	/* DAC: start writing at register 0 */
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
				r.w.ax = 0x1010;	/* Set DAC register */
				r.w.bx = i;	/* BX = color index (BH=0) */
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
		outportb(0x3C0, 0x20);	/* flip-flop to index, video on */
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
	unsigned long lTicks;
	unsigned long lWait, lNow;

	coniox_flush_cursor();

	lWait = (((unsigned long) ms * 91UL) + 4999UL) / 5000UL;	/* ms -> ticks at 18.2 Hz, rounded up */

	if (directvideo)
	{
		unsigned long lIdleTick;

		lTicks = peekl(0x40, 0x6C);	/* BDA 0040:006C tick counter */
		lIdleTick = lTicks - 1UL;	/* forces a coniox_idle() on the first lap */
		/* Modular subtraction: survives the midnight counter reset. */
		while (lWait != 0)
		{
			lNow = peekl(0x40, 0x6C);

			if ((lNow - lTicks) >= lWait)
			{
				break;
			}
			/* coniox_idle() only when the tick changes (18.2 Hz): the spin used to raise 3 software INTs per lap. */
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

		r.h.ah = 0;	/* INT 1Ah AH=00h: read ticks */
		coniox_int86(0x1A, &r, &r);
		#if defined(__WATCOMC__)
			lTicks = ((unsigned long) r.w.cx << 16) | r.w.dx;
		#else
			lTicks = ((unsigned long) r.x.cx << 16) | r.x.dx;
		#endif
		while (lWait)
		{
			r.h.ah = 0;
			coniox_int86(0x1A, &r, &r);
			#if defined(__WATCOMC__)
				lNow = ((unsigned long) r.w.cx << 16) | r.w.dx;
			#else
				lNow = ((unsigned long) r.x.cx << 16) | r.x.dx;
			#endif
			if ((lNow - lTicks) >= lWait)
			{
				break;
			}
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

	coniox_init(NULL);
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
			pokeb(0x40, 0x87, peekb(0x40, 0x87) | 1);       /* Disable cursor size emulation */
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
		ti.screenwidth = 0;	/* force coniox_init() to reinitialize */
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
		switch (__cur_t)
		{
			case _NOCURSOR:
				cursor_start = 0x20;	/* start > end: cursor hidden */
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
		/* BIOS branch: constants for INT 10h AH=01h (sized over the 8-scanline grid the interrupt assumes). */
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
		/* While the cursor was hidden gotoxy() skipped the BIOS position update: reposition it when re-enabled. */
		if (__cur_t != _NOCURSOR)
		{
			gotoxy(ti.curx, ti.cury);
		}
	}

}



/* ----------------------------------------------------------------------------------------------------------------- */
void textattr(int __newattr)
{
	coniox_init(NULL);
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
	coniox_cell = coniox_offset(ti.winleft + ti.curx - 2, ti.wintop + ti.cury - 2);
	coniox_cursor_dirty = 0;	/* gotoxy leaves the physical cursor up to date (or hidden) */

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
		unsigned short coniox_far *srcrow;
		unsigned short coniox_far *dstrow;
		size_t rowbytes;
		int y, sw, yfirst, ylast, width;

		sw = ti.screenwidth;
		yfirst = ti.cury + ti.wintop;
		ylast = ti.winbottom;
		width = ti.winright - ti.winleft + 1;
		rowbytes = (size_t) width * sizeof(unsigned short);

		srcrow = coniox_offset(ti.winleft - 1, yfirst - 1);
		dstrow = srcrow - sw;
		if ((width == sw) && (yfirst <= ylast))
		{
			/* Full window width: VRAM rows contiguous and copying upwards (dst < src), so the whole block is one forward memcpy. */
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
		/* Mirror of delline(): the block moves down, so copy bottom-up to read each row before it is overwritten. */
		unsigned short coniox_far *srcrow;
		unsigned short coniox_far *dstrow;
		size_t rowbytes;
		int y, sw, ytop, width;

		sw = ti.screenwidth;
		ytop = ti.cury + ti.wintop - 1;
		width = ti.winright - ti.winleft + 1;
		rowbytes = (size_t) width * sizeof(unsigned short);

		srcrow = coniox_offset(ti.winleft - 1, ti.winbottom - 2);
		dstrow = srcrow + sw;
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
int gettext(int __left, int __top, int __right, int __bottom, void coniox_far *__destin)
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

	/* Validate destination and rectangle, absolute 1-based coordinates. */
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

	sw = ti.screenwidth;

	/* Full screen width: all rows contiguous. */
	if (__left == 1 && __right == ti.screenwidth)
	{
		coniox_fmemcpy(dst, p, total_bytes);
	}
	else
	{
		/* Copy row by row honoring the real VRAM stride. */
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

	/* Validate source and rectangle, absolute 1-based coordinates. */
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

	sw = ti.screenwidth;

	/* Full screen width: all rows contiguous. */
	if (__left == 1 && __right == ti.screenwidth)
	{
		coniox_fmemcpy(p, src, total_bytes);
	}
	else
	{
		/* Copy row by row without touching adjacent cells. */
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
	unsigned short *buffer;
	size_t cells;
	int ok;


	coniox_init(NULL);

	/* Coordinates are absolute screen coordinates. Reject empty/out-of-range rectangles. */
	if (__left < 1 || __top < 1 || __right < __left || __bottom < __top || __right > ti.screenwidth || __bottom > ti.screenheight)
	{
		return 0;
	}
	if (__destleft < 1 || __desttop < 1 || __destleft + (__right - __left) > ti.screenwidth || __desttop + (__bottom - __top) > ti.screenheight)
	{
		return 0;
	}
	if (__left == __destleft && __top == __desttop)
	{
		return 1;
	}
	if (directvideo)
	{
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
			/* Partial overlap within one row (0 < dx < w): the only case requiring backwards cell-by-cell copying. */
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
		else if (dy > 0 && dy < h && (dx == 0 || dx >= w || dx <= -w))
		{
			/* Partial vertical overlap (destination below), no horizontal overlap: copy bottom-up, like insline(), to read each source row before it is overwritten. */
			src += (h - 1) * sw;
			dst += (h - 1) * sw;
			for (row = 0; row < h; row++)
			{
				coniox_fmemcpy(dst, src, (size_t) w * sizeof(unsigned short));
				src -= sw;
				dst -= sw;
			}
		}
		else if (dy > 0 && dy < h)
		{
			/* Diagonal overlap (destination below and shifted sideways): copy bottom-up and right-to-left, so writes always trail reads. */
			src += (h - 1) * sw + w - 1;
			dst += (h - 1) * sw + w - 1;
			for (row = 0; row < h; row++)
			{
				for (i = w - 1; i >= 0; i--)
				{
					dst[i] = src[i];
				}
				src -= sw;
				dst -= sw;
			}
		}
		else
		{
			/* All remaining cases: copying forward and top-down, reads always stay ahead of writes. */
			if (w == sw)
			{
				/* Full width: contiguous rows, single block. */
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

	cells = (size_t)(__right - __left + 1) * (size_t)(__bottom - __top + 1);
	buffer = (unsigned short *) malloc(cells * sizeof(unsigned short));
	if (buffer == NULL)
	{
		return 0;
	}

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
			/* Full-width block: VRAM rows are contiguous, so a single fill covers all rows. gotoxy() just left coniox_cell at the first cell. */
			coniox_fmemsetw(coniox_cell, (short)((ti.attribute << 8) | ' '), (size_t)ti.screenwidth * (ti.winbottom - ti.wintop + 1));
		}
		else
		{
			unsigned short coniox_far *p;
			unsigned short v;
			int width, height, y, sw;

			v = (unsigned short)((ti.attribute << 8) | ' ');
			width = ti.winright - ti.winleft + 1;
			height = ti.winbottom - ti.wintop + 1;
			sw = ti.screenwidth;
			p = coniox_cell;
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
	return((wchar_t) getch());
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

	coniox_flush_cursor();

	if (directvideo)
	{
		/* One far dword read: head in the low word (0x1A), tail in the high word (0x1C); they differ iff a key is pending. */
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

	coniox_flush_cursor();

	if (directvideo)
	{
		unsigned short start, end, head, scancode;
		unsigned long lIdleTick = peekl(0x40, 0x6C) - 1UL;	/* forces idle on the first lap */

		while (!kbhit())
		{
			unsigned long lNowTick = peekl(0x40, 0x6C);

			/* coniox_idle() only when the BDA tick changes (18.2 Hz). */
			if (lNowTick != lIdleTick)
			{
				lIdleTick = lNowTick;
				coniox_idle();
			}
		}
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
			return(EOF);	/* buffer full */
		}
		pokew(0x40, tail, (__ch & 0xFF));
		pokew(0x40, 0x1C, new_tail);
		_enable();
		return __ch;
	}
	else
	{
		union REGS r;

		/* INT 16h AH=05h takes the key in CX ((scan << 8) | ASCII); AL is output only (0 = success). A zero scan code makes getch() read back exactly __ch. */
		memset(&r, 0, sizeof(r));
		r.h.ah = 0x05;
		#if defined(__WATCOMC__)
			r.w.cx = (unsigned short) (__ch & 0xFF);
		#else
			r.x.cx = (unsigned short) (__ch & 0xFF);
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
#endif /* ((__DOS__) || (__MSDOS__)) */

