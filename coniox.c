/* ----------------------------------------------------------------------------------------------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stddef.h>
#include "coniox.h"


/* ----------------------------------------------------------------------------------------------------------------- */
#define CONIOX_MAX_CHARINFO     (160 * 100)

#if defined(__WATCOMC__)
	#define coniox_inline __inline
#elif (defined(__TURBOC__) && (__BORLANDC__ >= 0x450))
	#define coniox_inline __inline
#elif defined(__TURBOC__)
	#define coniox_inline	/* Borland/Turbo C before 4.5 has no inline keyword in C mode */
#else
	#define coniox_inline inline
#endif

#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
	#include <wchar.h>
	static coniox_inline wchar_t coniox_ansi2unicode(char ch);
	static coniox_inline void coniox_putwchxyattr(int x, int y, wchar_t ch, int attr);
	wchar_t *cgetws(wchar_t *__str);
	int cputws(const wchar_t *__str);
	int cwscanf(const wchar_t *__format, ...);
	int cwprintf(const wchar_t *__format, ...);
	wchar_t getwche(void);
	wchar_t ungetwch(wchar_t __ch);
#endif

static coniox_inline void coniox_putchxyattr(int x, int y, int ch, int attr);
static void coniox_putchxyattrwh(int x, int y, int ch, int attr, int w, int h);
static coniox_inline int coniox_inwindow(int x, int y);
static coniox_inline void coniox_readnormattr(void);
static coniox_inline void coniox_flush_cursor(void);


/* ----------------------------------------------------------------------------------------------------------------- */
static struct text_info ti;

/* Inline "already initialized?" test: ti.screenwidth stays 0 until coniox_init() succeeds (textmode() also zeroes it to force a reinit).
   Saves a call and return on every public function, which matters most in the tiny ones (textcolor, wherex, putch, gotoxy). */
#define coniox_ready()	do { if (ti.screenwidth == 0) { coniox_init(NULL); } } while (0)

/* Window size cache: putch()/cputs() need it on every call. Kept out of text_info, whose layout is public; coniox_winsize() must follow every change of ti.winleft/winright/wintop/winbottom. */
static int coniox_winwidth;
static int coniox_winheight;
#define coniox_winsize()	(coniox_winwidth = ti.winright - ti.winleft + 1, coniox_winheight = ti.winbottom - ti.wintop + 1)
int directvideo = 1;
int _wscroll = 1;

static int coniox_setcursortype = _NORMALCURSOR;
static int coniox_cursor_dirty = 0;	/* 1 = physical cursor lags behind ti.curx/ti.cury */
static int getch_last_extended_key = 0;	/* scan code pending after getch() returned 0 for an extended key */

#if ((__DOS__) || (__MSDOS__))
#define coniox_basecrt 0x3D4

/* coniox_far is empty in FLAT and DJGPP: the same declaration is a near pointer there and a far one in real mode. */
typedef unsigned short coniox_far *coniox_cellptr;
static coniox_cellptr coniox_vram;
static coniox_cellptr coniox_cell;	/* cell under the logical cursor, kept by putch(), cputs() and gotoxy() */
#if ((defined(__FLAT__)) && (!defined(__DJGPP__)))
	#define coniox_int86 int386
#else
	#define coniox_int86 int86
#endif
#define coniox_offset(piX, piY) (coniox_vram + ((ti.screenwidth * (piY)) + (piX)))

/* --- Real-mode segment helpers ----------------------------------------
   The DOSBox VESA detection needs an INT 10h call with ES:DI pointing at a conventional-memory
   (low 1MB) buffer that the real-mode VBE BIOS can read and write. Real-mode builds already have
   every far pointer in that range; protected-mode builds (DJGPP, Watcom DOS32) must allocate a
   dedicated DOS-memory block.

   coniox_int86x is the segment-aware twin of coniox_int86: int86x (real-mode, DJGPP/DPMI) or
   int386x (Watcom FLAT). coniox_w() selects the WORDREGS member of union REGS, which is .x in
   real-mode Watcom / Borland / DJGPP and .w in 32-bit Watcom (its .x is DWORDREGS there).

   coniox_dpmi_alloc_dos / coniox_dpmi_free_dos go straight to DPMI 0x0100 / 0x0101 instead of
   wrapping _dos_allocmem/_dos_freemem: the libc wrapper changed signature between Open Watcom
   1.9 (2 args: size, &seg) and 2.0 (3 args: size, &seg, &sel), while the raw DPMI service is the
   same in every version and in DJGPP. */
#if (defined(__DJGPP__))
	#include <dpmi.h>
#endif
#if ((defined(__FLAT__)) && (!defined(__DJGPP__)))
	#define coniox_int86x int386x
#else
	#define coniox_int86x int86x
#endif
#if (defined(__WATCOMC__))
	#define coniox_w(regs) (regs).w	/* OW: WORDREGS exposed as .w in 16- and 32-bit builds */
#else
	#define coniox_w(regs) (regs).x	/* Borland/Turbo C, DJGPP: WORDREGS exposed as .x */
#endif

/* ----------------------------------------------------------------------------------------------------------------- */
#if defined(__WATCOMC__)
	static coniox_inline void coniox_fmemsetw(void coniox_far *m, short val, size_t count);
	static coniox_inline void coniox_fmemcpy(void coniox_far *dst, const void coniox_far *src, size_t count);
	static coniox_inline void coniox_fmemout(unsigned int port, const void coniox_far *src, size_t count);
	static coniox_inline void coniox_fmemcpyr(void coniox_far *dst, const void coniox_far *src, size_t count);
	static coniox_inline unsigned int coniox_putrun(coniox_cellptr piDst, const char coniox_far *piSrc, unsigned int piAttr, size_t piCount);
	#if (defined(__FLAT__))
		/* parm [AX] only guarantees the low word: zero-extend it before building the duplicated cell word.
		   386+: a leading STOSW brings EDI to a dword boundary (cells are words, so only bit 1 can differ), so every STOSD of the REP is aligned. */
		#pragma aux coniox_fmemsetw = \
			"movzx eax, ax" \
			"mov edx, eax" \
			"shl eax, 16" \
			"or eax, edx" \
			"cld" \
			"test edi, 2" \
			"jz short _fsal" \
			"jecxz _fsdone" \
			"stosw" \
			"dec ecx" \
			"_fsal:" \
			"mov edx, ecx" \
			"shr ecx, 1" \
			"rep stosd" \
			"test dl, 1" \
			"jz short _fsdone" \
			"stosw" \
			"_fsdone:" \
			parm [EDI][AX][ECX] \
			modify exact [EDI EDX ECX EAX];
		/* Scans and writes the run in one pass; returns the number of cells written (it stops at the first NUL, BS, LF or CR, or after ECX chars).
		   386+: with a dword aligned destination it moves 4 chars per pass: one dword load, a SWAR test for any byte < 14 (such a quad goes char by char), two STOSD.
		   The dword load is skipped when it would cross a 4K page, because the string may end inside the quad and the next page may not be mapped. */
		#pragma aux coniox_putrun = \
			"and ebx, 0FF00h" \
			"cld" \
			"push ecx" \
			"mov edx, ebx" \
			"shl ebx, 16" \
			"or ebx, edx" \
			"test edi, 2" \
			"jnz short _prsingle" \
			"_prqcheck:" \
			"cmp ecx, 4" \
			"jb short _prsingle" \
			"_prquad:" \
			"mov eax, esi" \
			"and eax, 0FFFh" \
			"cmp eax, 0FFCh" \
			"ja short _prsingle" \
			"mov edx, [esi]" \
			"mov eax, edx" \
			"sub eax, 0E0E0E0Eh" \
			"not edx" \
			"and eax, edx" \
			"not edx" \
			"and eax, 80808080h" \
			"jnz short _prsingle" \
			"add esi, 4" \
			"movzx eax, dh" \
			"shl eax, 16" \
			"mov al, dl" \
			"or eax, ebx" \
			"stosd" \
			"shr edx, 16" \
			"movzx eax, dh" \
			"shl eax, 16" \
			"mov al, dl" \
			"or eax, ebx" \
			"stosd" \
			"sub ecx, 4" \
			"cmp ecx, 4" \
			"jae short _prquad" \
			"_prsingle:" \
			"test ecx, ecx" \
			"jz short _prdone" \
			"movzx eax, byte ptr [esi]" \
			"cmp al, 13" \
			"jbe short _prctl" \
			"_prput:" \
			"inc esi" \
			"or eax, ebx" \
			"stosw" \
			"dec ecx" \
			"test edi, 2" \
			"jnz short _prsingle" \
			"jmp short _prqcheck" \
			"_prctl:" \
			"je short _prdone" \
			"cmp al, 10" \
			"je short _prdone" \
			"cmp al, 8" \
			"je short _prdone" \
			"test al, al" \
			"jnz short _prput" \
			"_prdone:" \
			"pop eax" \
			"sub eax, ecx" \
			parm [EDI][ESI][EBX][ECX] \
			value [EAX] \
			modify exact [EDI ESI ECX EDX EAX EBX];
		/* 386+: a leading MOVSW aligns the destination to a dword when it is only word aligned (partial-width windows, user buffers). */
		#pragma aux coniox_fmemcpy = \
			"cld" \
			"mov ecx, ebx" \
			"test edi, 2" \
			"jz short _fcal" \
			"cmp ecx, 2" \
			"jb short _fcal" \
			"movsw" \
			"sub ecx, 2" \
			"_fcal:" \
			"push ecx" \
			"shr ecx, 2" \
			"rep movsd" \
			"pop ecx" \
			"and ecx, 3" \
			"rep movsb" \
			parm [EDI][ESI][EBX] \
			modify exact [ECX EDI ESI];
		/* Backwards copy (STD) for overlapping moves toward higher addresses: dst > src, count in bytes (even, whole cells).
		   Dwords from the top down, then the one leftover word at the bottom; CLD on exit, as the ABI expects. */
		#pragma aux coniox_fmemcpyr = \
			"mov edx, ecx" \
			"lea esi, [esi+ecx-4]" \
			"lea edi, [edi+ecx-4]" \
			"shr ecx, 2" \
			"std" \
			"rep movsd" \
			"test dl, 2" \
			"jz short _fcrdone" \
			"add esi, 2" \
			"add edi, 2" \
			"movsw" \
			"_fcrdone:" \
			"cld" \
			parm [EDI][ESI][ECX] \
			modify exact [ECX EDX ESI EDI];
		/* port in DX (low word of EDX), source in ESI, byte count in ECX: one REP OUTSB to a fixed I/O port (DAC data, etc.). */
		#pragma aux coniox_fmemout = \
			"cld" \
			"rep outsb" \
			parm [EDX][ESI][ECX] \
			modify exact [ESI ECX];
	#else
		#pragma aux coniox_fmemsetw = \
			"cld" \
			"rep stosw" \
			parm [ES DI][AX][CX] \
			modify exact [DI CX];
		/* Scans and writes the run in one pass; returns the number of cells written (it stops at the first NUL, BS, LF or CR, or after CX chars). */
		#pragma aux coniox_putrun = \
			"push ds" \
			"mov ds, dx" \
			"cld" \
			"push cx" \
			"jcxz short _prdone" \
			"_prnext:" \
			"lodsb" \
			"cmp al, 13" \
			"jbe short _prctl" \
			"_prput:" \
			"mov ah, bh" \
			"stosw" \
			"loop _prnext" \
			"jmp short _prdone" \
			"_prctl:" \
			"je short _prdone" \
			"cmp al, 10" \
			"je short _prdone" \
			"cmp al, 8" \
			"je short _prdone" \
			"or al, al" \
			"jnz short _prput" \
			"_prdone:" \
			"pop ax" \
			"sub ax, cx" \
			"pop ds" \
			parm [ES DI][DX SI][BX][CX] \
			value [AX] \
			modify exact [DI SI CX AX];
		#pragma aux coniox_fmemcpy = \
			"push ds" \
			"mov ds, dx" \
			"cld" \
			"shr cx, 1" \
			"rep movsw" \
			"jnc short _fcpy_done" \
			"movsb" \
			"_fcpy_done:" \
			"pop ds" \
			parm [ES DI][DX SI][CX] \
			modify exact [CX SI DI];
		/* Backwards copy (STD) for overlapping moves toward higher addresses: dst > src, count in bytes (even, whole cells). Pure 8086. */
		#pragma aux coniox_fmemcpyr = \
			"push ds" \
			"mov ds, dx" \
			"std" \
			"mov ax, cx" \
			"shr cx, 1" \
			"add si, ax" \
			"sub si, 2" \
			"add di, ax" \
			"sub di, 2" \
			"rep movsw" \
			"cld" \
			"pop ds" \
			parm [ES DI][DX SI][CX] \
			modify exact [AX CX SI DI];
		/* port in DX, far source in AX:SI, byte count in CX.
		   Pure 8086 loop (LODSB/OUT/LOOP): REP OUTSB is 186+ and rejected when the
		   compiler CPU setting is 8086 (default DOS16 without -3/-4). */
		#pragma aux coniox_fmemout = \
			"push ds" \
			"mov ds, ax" \
			"cld" \
			"jcxz short _fmodone" \
			"_fmonext:" \
			"lodsb" \
			"out dx, al" \
			"loop _fmonext" \
			"_fmodone:" \
			"pop ds" \
			parm [DX][AX SI][CX] \
			modify exact [SI CX AX];
	#endif
#else
	#define coniox_fmemcpy      _fmemmove

	/* Backwards copy for overlapping moves toward higher addresses: dst > src, count in bytes (even, whole cells). */
	static coniox_inline void coniox_fmemcpyr(void coniox_far *dst, const void coniox_far *src, size_t count)
	{
		unsigned short coniox_far *d = (unsigned short coniox_far *) dst + (count >> 1);
		const unsigned short coniox_far *s = (const unsigned short coniox_far *) src + (count >> 1);

		while (count >= 2)
		{
			*--d = *--s;
			count -= 2;
		}
	}
	static coniox_inline void coniox_fmemsetw(void coniox_far *m, short val, size_t count)
	{
		short coniox_far *buf = (short coniox_far *) m;

		/* Unroll by 4; most clears (clreol, clrscr lines, delline bottom) are long enough to benefit. */
		while (count >= 4)
		{
			buf[0] = val;
			buf[1] = val;
			buf[2] = val;
			buf[3] = val;
			buf += 4;
			count -= 4;
		}
		if (count & 2)
		{
			buf[0] = val;
			buf[1] = val;
			buf += 2;
		}
		if (count & 1)
		{
			*buf = val;
		}
	}

	/* Byte stream to a fixed I/O port (DAC data, ATTR, etc.). */
	static coniox_inline void coniox_fmemout(unsigned int port, const void coniox_far *src, size_t count)
	{
		const unsigned char coniox_far *p = (const unsigned char coniox_far *) src;

		while (count--)
		{
			outportb(port, *p++);
		}
	}

	/* Scans and writes the run in one pass; returns the number of cells written (it stops at the first NUL, BS, LF or CR, or after piCount chars).
	   Pointer form instead of indexing: better code generation on older compilers (Turbo C, early Watcom without pragma). */
	static coniox_inline unsigned int coniox_putrun(coniox_cellptr piDst, const char coniox_far *piSrc, unsigned int piAttr, unsigned int piCount)
	{
		unsigned short cellword = (unsigned short) (piAttr & 0xFF00);
		unsigned short coniox_far *dst = piDst;
		const char coniox_far *src = piSrc;
		unsigned int n = 0;

		while (n < piCount)
		{
			unsigned char ch = (unsigned char) *src;
			/* Common case first: printable > 13 costs one compare. */
			if (ch > '\r')
			{
				*dst++ = cellword | ch;
				src++;
				n++;
				continue;
			}
			if (ch == 0 || ch == '\r' || ch == '\n' || ch == '\b')
			{
				break;
			}
			*dst++ = cellword | ch;
			src++;
			n++;
		}
		return n;
	}

#endif
#endif


/* ----------------------------------------------------------------------------------------------------------------- */
/* INTERNAL (Private)*/
/* ----------------------------------------------------------------------------------------------------------------- */
static int coniox_vsscanf(const char *buffer, const char *format, va_list argPtr)
{
	#ifdef _MSC_VER
		void *a[40] = {NULL};	/* Room for up to 20 arguments plus possible sizes */
	#else
		void *a[20] = {NULL};	/* sscanf is called with 20 arguments */
	#endif
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
			if (*p == '%' || *p == '*')
			{
				p++;
				continue;
			}
			/* Skip optional width modifiers */
			while (*p >= '0' && *p <= '9')
			{
				p++;
			}
			/* Skip optional length modifiers (l, h, z, j, t, L) */
			while (*p == 'l' || *p == 'h' || *p == 'z' || *p == 'j' || *p == 't' || *p == 'L')
			{
				p++;
			}
			/* A trailing % consumes no argument */
			if (!*p)
			{
				break;
			}
			#ifdef _MSC_VER
				if (count >= 39)	/* a[40]: buffer+size pair, last index 39 */
				{
					break;
				}
				a[count++] = va_arg(argPtr, void *);
				if (*p == 's' || *p == 'c' || *p == '[')
				{
					a[count++] = (void *) va_arg(argPtr, size_t);	/* sscanf_s takes the buffer size by value */
				}
			#else
				if (count >= 20)	/* sscanf is called with 20 arguments */
				{
					break;
				}
				a[count++] = va_arg(argPtr, void *);
			#endif
		}
	}
	#ifdef _MSC_VER
		return(sscanf_s(buffer, format, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8], a[9], a[10], a[11], a[12], a[13], a[14], a[15], a[16], a[17], a[18], a[19], a[20], a[21], a[22], a[23], a[24], a[25], a[26], a[27], a[28], a[29], a[30], a[31], a[32], a[33], a[34], a[35], a[36], a[37], a[38], a[39]));
	#else
		return(sscanf(buffer, format, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8], a[9], a[10], a[11], a[12], a[13], a[14], a[15], a[16], a[17], a[18], a[19]));
	#endif
}


/* ----------------------------------------------------------------------------------------------------------------- */
static coniox_inline int coniox_inwindow(int x, int y)
{
	return ((unsigned int)(x - ti.winleft) <= (unsigned int)(ti.winright  - ti.winleft)) && ((unsigned int)(y - ti.wintop)  <= (unsigned int)(ti.winbottom - ti.wintop));
}


/* ----------------------------------------------------------------------------------------------------------------- */
/* PORTABLE FUNCTIONS */
/* ----------------------------------------------------------------------------------------------------------------- */
void clreol(void)
{
	int aw;


	coniox_ready();
	aw = ti.winright - ti.winleft - ti.curx + 2;	/* cells remaining to the end of the line */
	#if ((__DOS__) || (__MSDOS__))
		if ((directvideo) && (aw > 0))
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
	coniox_readnormattr();	/* re-read the real startup attribute, not a fixed 7 */
	textattr(ti.normattr);
}


/* ----------------------------------------------------------------------------------------------------------------- */
void gettextinfo(struct text_info *__r)
{
	coniox_ready();
	if (__r != NULL)
	{
		*__r = ti;
	}
}


/* ----------------------------------------------------------------------------------------------------------------- */
void highvideo(void)
{
	coniox_ready();
	ti.attribute |= 0x08;
}


/* ----------------------------------------------------------------------------------------------------------------- */
void lowvideo(void)
{
	coniox_ready();
	ti.attribute &= ~0x08;
}


/* ----------------------------------------------------------------------------------------------------------------- */
void normvideo(void)
{
	coniox_ready();
	ti.attribute = ti.normattr;
}


/* ----------------------------------------------------------------------------------------------------------------- */
void textbackground(int __newcolor)
{
	coniox_ready();
	ti.attribute = (unsigned short)((ti.attribute & 0x8F) | ((__newcolor & 0x07) << 4));
}


/* ----------------------------------------------------------------------------------------------------------------- */
void textcolor(int __newcolor)
{
	coniox_ready();
	ti.attribute = (unsigned short)((ti.attribute & 0x70) | (__newcolor & 0x8F));
}


/* ----------------------------------------------------------------------------------------------------------------- */
void window(int __left, int __top, int __right, int __bottom)
{
	coniox_ready();

	if (__left < 1 || __top < 1 || __right > ti.screenwidth || __bottom > ti.screenheight || __left > __right || __top > __bottom)
	{
		return;
	}
	ti.winleft   = (unsigned short) __left;
	ti.winright  = (unsigned short) __right;
	ti.wintop    = (unsigned short) __top;
	ti.winbottom = (unsigned short) __bottom;
	coniox_winsize();
	gotoxy(1, 1);
}


/* ----------------------------------------------------------------------------------------------------------------- */
/* Cursor movement shared by putch(), putwch(), cputs() and cputws(). The DOS callers use the return values to keep their cell pointer in step. */
#if ((__DOS__) || (__MSDOS__))
	#define coniox_celladd(piCell, piDelta)	((piCell) += (piDelta))
#else
	#define coniox_celladd(piCell, piDelta)	((void) (piDelta))
#endif

/* Scrolls the window one line up. delline() works on the cursor row and never moves the cursor, so aim it at the first window row and put it back. */
static coniox_inline void coniox_scrollup(void)
{
	unsigned short oldx = ti.curx;
	unsigned short oldy = ti.cury;

	ti.curx = 1;
	ti.cury = 1;
	delline();
	ti.curx = oldx;
	ti.cury = oldy;
}

/* CR: returns the columns moved back. */
static coniox_inline int coniox_cr(void)
{
	int moved = ti.curx - 1;

	ti.curx = 1;
	return moved;
}

/* BS: returns 1 when the cursor moved back. */
static coniox_inline int coniox_bs(void)
{
	if (ti.curx > 1)
	{
		ti.curx--;
		return 1;
	}
	return 0;
}

/* LF: keeps the column. Returns 1 when the cursor moved down a row, 0 when it scrolled or stayed (_wscroll = 0 on the last line). */
static coniox_inline int coniox_lf(void)
{
	if (ti.cury < coniox_winheight)
	{
		ti.cury++;
		return 1;
	}
	if (_wscroll)
	{
		coniox_scrollup();
	}
	return 0;
}

/* Char just written on the last column: returns 0 when the cursor stays there (_wscroll = 0, further output overwrites the edge cell),
   1 when it went to column 1 of the next row, 2 when the window scrolled and it is on column 1 of the last row. */
static coniox_inline int coniox_wrap(void)
{
	if (!_wscroll)
	{
		return 0;
	}
	ti.curx = 1;
	if (ti.cury < coniox_winheight)
	{
		ti.cury++;
		return 1;
	}
	coniox_scrollup();
	return 2;
}


/* ----------------------------------------------------------------------------------------------------------------- */
static coniox_inline void coniox_flush_cursor(void)
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
	int winwidth, wrapped;

	coniox_ready();
	winwidth = coniox_winwidth;

	/* Printable first: only CR, LF and BS are controls, so every char above 13 skips the three tests. */
	if (__c > '\r' || (__c != '\r' && __c != '\n' && __c != '\b'))
	{
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
		coniox_cursor_dirty = 1;
		if (ti.curx < winwidth)
		{
			coniox_celladd(coniox_cell, 1);
			ti.curx++;
		}
		else
		{
			/* The cell cache stays on the written edge cell: one screen row minus the winwidth - 1 columns walked back, or just the walk back when scrolled. */
			wrapped = coniox_wrap();
			if (wrapped == 1)
			{
				coniox_celladd(coniox_cell, ti.screenwidth - winwidth + 1);
			}
			else if (wrapped == 2)
			{
				coniox_celladd(coniox_cell, 1 - winwidth);
			}
		}
	}
	else
	{
		switch (__c)
		{
			case '\r':
				coniox_celladd(coniox_cell, -coniox_cr());
				break;
			case '\n':
				if (coniox_lf())
				{
					coniox_celladd(coniox_cell, ti.screenwidth);
				}
				break;
			case '\b':
				coniox_celladd(coniox_cell, -coniox_bs());
				break;
		}
		coniox_cursor_dirty = 1;
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

	coniox_ready();

	if (__str == NULL)
	{
		return(NULL);
	}
	str = __str + 2;
	maxlen = (int) (unsigned char) __str[0] - 1;
	x = ti.curx;
	y = ti.cury;

	while (ch != '\r' && ch != EOF)
	{
		ch = getch();
		if (ch == 0)
		{
			getch();	/* extended key: consume its scan code */
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
	coniox_flush_cursor();	/* the loop echoed via putch(): sync the deferred cursor, like getche() already does */
	return str;
}


/* ----------------------------------------------------------------------------------------------------------------- */
int cvprintf(const char *__format, va_list __arglist)
{
	char buffer[PRINTFBUF_SIZE];

	if (!__format || !__format[0])
	{
		return(0);
	}

	/* No conversion specifier: the format already is the output, so skip the formatting pass (and the PRINTFBUF_SIZE truncation). */
	if (strchr(__format, '%') == NULL)
	{
		return(cputs(__format));
	}

	buffer[0] = 0;
	#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32) || defined(__DJGPP__)
		vsnprintf(buffer, PRINTFBUF_SIZE, __format, __arglist);
	#elif defined(__WATCOMC__)
		_vsnprintf(buffer, PRINTFBUF_SIZE, __format, __arglist);
	#else
		vsprintf(buffer, __format, __arglist);
	#endif
	buffer[PRINTFBUF_SIZE - 1] = 0;	/* the legacy _vsnprintf does not terminate a truncated string */
	return(cputs(buffer));	/* cputs() returns the chars it consumed: strlen(buffer), without a second pass */
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
	va_list ap;
	unsigned char buffer[PRINTFBUF_SIZE + 3];

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
	int isext;

	isext = getch_last_extended_key;	/* a pending scan code is not a character: it is not echoed */
	ch = getch();
	if (!isext && ch != 0 && ch != EOF)
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

	coniox_ready();
	cputs(__prompt);
	x = ti.curx;
	y = ti.cury;

	while (ch != '\r' && ch != EOF)
	{
		ch = getch();
		if (ch == 0)
		{
			getch();	/* extended key: consume its scan code */
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
	coniox_flush_cursor();	/* the loop echoed via putch(): sync the deferred cursor, like getche() already does */
	return(str);
}


/* ----------------------------------------------------------------------------------------------------------------- */
int wherex(void)
{
	coniox_ready();
	return(ti.curx);
}


/* ----------------------------------------------------------------------------------------------------------------- */
int wherey(void)
{
	coniox_ready();
	return(ti.cury);
}


/* ----------------------------------------------------------------------------------------------------------------- */
/* WINDOWS ONLY (Portable) */
/* ----------------------------------------------------------------------------------------------------------------- */
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)


/* ----------------------------------------------------------------------------------------------------------------- */
static int coniox_vswscanf(const wchar_t* buffer, const wchar_t* format, va_list argPtr)
{
	void *a[20] = { NULL };
	size_t count = 0;
	const wchar_t *p;
	wchar_t c;

	if (!buffer || !format)
	{
		return(0);
	}

	p = format;
	while ((c = *p++) != 0)
	{
		if (c == L'%')
		{
			if (p[0] == L'%' || p[0] == L'*')
			{
				p++;
				continue;
			}
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

	coniox_ready();
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
			getwch();	/* extended key: consume its scan code */
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
	coniox_flush_cursor();	/* the loop echoed via putwch(): sync the deferred cursor, like getwche() already does */
	return str;
}


/* ----------------------------------------------------------------------------------------------------------------- */
wchar_t putwch(wchar_t __c)
{
	int winwidth;


	coniox_ready();
	winwidth = coniox_winwidth;

	switch (__c)
	{
		case L'\r':
			coniox_cr();
			break;
		case L'\n':
			coniox_lf();
			break;
		case L'\b':
			coniox_bs();
			break;
		default:
			coniox_putwchxyattr(ti.winleft + ti.curx - 1, ti.wintop + ti.cury - 1, __c, ti.attribute);
			if (ti.curx < winwidth)
			{
				ti.curx++;
			}
			else
			{
				coniox_wrap();
			}
	}
	coniox_cursor_dirty = 1;
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
	wchar_t buffer[PRINTFBUF_SIZE];
	va_list ap;

	if (!__format || !__format[0])
	{
		return(0);
	}

	buffer[0] = 0;
	va_start(ap, __format);
	vswprintf(buffer, PRINTFBUF_SIZE, __format, ap);
	va_end(ap);
	buffer[PRINTFBUF_SIZE - 1] = 0;	/* a truncated string is not always terminated */
	cputws(buffer);
	return((int) wcslen(buffer));
}
#endif


/* ----------------------------------------------------------------------------------------------------------------- */
/* WINDOWS ONLY (Non portable) */
/* ----------------------------------------------------------------------------------------------------------------- */
#if ((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32)
#include <windows.h>
#include <wincon.h>


/* ----------------------------------------------------------------------------------------------------------------- */
static HANDLE coniox_console_output = NULL;
static HANDLE coniox_console_input = NULL;
static int coniox_orgx = 0;	/* buffer column of screen column 1 */
static int coniox_orgy = 0;	/* buffer row of screen row 1 */
static CHAR_INFO coniox_charinfo[CONIOX_MAX_CHARINFO];


/* ----------------------------------------------------------------------------------------------------------------- */
void coniox_init(const void *title)
{
	int i;
	DWORD m;
	UINT acp;
	CONSOLE_SCREEN_BUFFER_INFO info;

	/* Check if already initialized: an explicit title is still applied */
	if (coniox_console_output)
	{
		if (title)
		{
			SetConsoleTitleA((LPCSTR) title);
		}
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
	ti.normattr = (unsigned short) (info.wAttributes & 0xFF);
	/* The screen is the visible window, not the (often 9001 lines high) buffer. */
	coniox_orgx = info.srWindow.Left;
	coniox_orgy = info.srWindow.Top;
	ti.screenwidth = (unsigned short) (info.srWindow.Right - info.srWindow.Left + 1);
	ti.screenheight = (unsigned short) (info.srWindow.Bottom - info.srWindow.Top + 1);
	ti.winleft = 1;
	ti.wintop = 1;
	ti.winright = ti.screenwidth;
	ti.winbottom = ti.screenheight;
	coniox_winsize();
	i = info.dwCursorPosition.X - coniox_orgx + 1;
	ti.curx = (unsigned short) ((i < 1 || i > ti.screenwidth) ? 1 : i);
	i = info.dwCursorPosition.Y - coniox_orgy + 1;
	ti.cury = (unsigned short) ((i < 1 || i > ti.screenheight) ? 1 : i);
	ti.attribute = (unsigned short) (info.wAttributes & 0xFF);
	ti.currmode = (unsigned short) ((ti.screenwidth <= 40) ? C40 : C80);
	acp = GetOEMCP();
	if (GetConsoleOutputCP() != acp)
	{
		SetConsoleOutputCP(acp);
		SetConsoleCP(acp);
	}
	if (GetConsoleMode(coniox_console_input, &m))
	{
		SetConsoleMode(coniox_console_input, m | ENABLE_WINDOW_INPUT);
	}

	atexit(coniox_flush_cursor);	/* the cursor is synced lazily: sync it once more when the program ends */

	if (title)
	{
		SetConsoleTitleA((LPCSTR) title);
	}
}

/* ----------------------------------------------------------------------------------------------------------------- */
static coniox_inline void coniox_readnormattr(void)
{
	CHAR_INFO ci;
	COORD c = { 0, 0 }, s = { 1, 1 };
	SMALL_RECT r;

	coniox_ready();
	/* Read the real startup attribute for ti.normattr from the cell under the cursor, like the DOS version: GetConsoleScreenBufferInfo returns the attribute textattr() left, not the startup one. If the read fails (redirected output) the coniox_init() value is kept. */
	r.Left = (short) (coniox_orgx + ti.winleft + ti.curx - 2);
	r.Top = (short) (coniox_orgy + ti.wintop + ti.cury - 2);
	r.Right = r.Left;
	r.Bottom = r.Top;
	if (ReadConsoleOutput(coniox_console_output, &ci, s, c, &r))
	{
		ti.normattr = (unsigned short) (ci.Attributes & 0xFF);
	}
}


/* ----------------------------------------------------------------------------------------------------------------- */
int cputs(const char *__str)
{
	const char *orig = __str;
	const char *run;
	unsigned int ch;
	unsigned int runlen, remaining;
	unsigned long attrcell;
	int winwidth;
	COORD c = { 0, 0 };
	SMALL_RECT r;
	COORD s;
	CHAR_INFO *ci = coniox_charinfo;

	if (__str == NULL)
	{
		return(0);
	}

	coniox_ready();

	winwidth = coniox_winwidth;

	while (*__str)
	{
		/* Carriage return: back to start of line. WriteConsoleOutput neither reads nor moves the console cursor; the deferred sync paints it. */
		if (*__str == '\r')
		{
			coniox_cr();
			++__str;
			continue;
		}

		/* Line feed: advance or scroll the window, keeping the column. */
		if (*__str == '\n')
		{
			coniox_lf();
			++__str;
			continue;
		}

		/* Backspace: move back without erasing. */
		if (*__str == '\b')
		{
			coniox_bs();
			++__str;
			continue;
		}

		/* Group printable characters up to the next control, the window edge or the batch size: one WriteConsoleOutputA per run. */
		run = __str;
		if (ti.curx > winwidth)
		{
			ti.curx = winwidth;
		}
		remaining = winwidth - ti.curx + 1;
		if (remaining > CONIOX_MAX_CHARINFO)
		{
			remaining = CONIOX_MAX_CHARINFO;
		}
		/* One bounded pass scans the run and builds its cells.
			One DWORD store per cell (CHAR_INFO layout): char | padding | attribute.
			Common case (ch > 13) costs a single compare. */
		attrcell = (unsigned long) ti.attribute << 16;
		for (runlen = 0; runlen < remaining; runlen++)
		{
			ch = (unsigned char) run[runlen];
			if (ch > '\r')
			{
				((unsigned long *) ci)[runlen] = attrcell | ch;
				continue;
			}
			if (ch == 0 || ch == '\r' || ch == '\n' || ch == '\b')
			{
				break;
			}
			((unsigned long *) ci)[runlen] = attrcell | ch;
		}

		if (runlen)
		{
			r.Left   = (short) (coniox_orgx + ti.winleft + ti.curx - 2);
			r.Top    = (short) (coniox_orgy + ti.wintop + ti.cury - 2);
			r.Right  = (short) (r.Left + runlen - 1);
			r.Bottom = r.Top;
			s.X = (short) runlen;
			s.Y = 1;
			WriteConsoleOutputA(coniox_console_output, ci, s, c, &r);
			ti.curx += runlen;
		}

		/* A run never crosses the window edge: the wrap is decided once. */
		if (ti.curx > winwidth)
		{
			ti.curx = winwidth;
			coniox_wrap();
		}
		__str += runlen;
	}
	/* Deferred physical cursor sync, exactly like DOS cputs() and putch(): the real cursor is programmed
		at the next wait point (kbhit/getch/cgets/getpass/delay) via coniox_flush_cursor(), or at exit. */
	coniox_cursor_dirty = 1;
	return (int)(__str - orig);
}


/* ----------------------------------------------------------------------------------------------------------------- */
int cputws(const wchar_t *__str)
{
	const wchar_t *orig = __str;
	const wchar_t *run;
	unsigned int ch;
	unsigned int runlen, remaining;
	unsigned long attrcell;
	int winwidth;
	COORD c = { 0, 0 };
	SMALL_RECT r;
	COORD s;
	CHAR_INFO *ci = coniox_charinfo;

	if (__str == NULL)
	{
		return(0);
	}

	coniox_ready();

	winwidth = coniox_winwidth;

	while (*__str)
	{
		/* Carriage return: back to start of line (deferred sync). */
		if (*__str == L'\r')
		{
			coniox_cr();
			++__str;
			continue;
		}

		/* Line feed: advance or scroll the window, keeping the column. */
		if (*__str == L'\n')
		{
			coniox_lf();
			++__str;
			continue;
		}

		/* Backspace: move back without erasing. */
		if (*__str == L'\b')
		{
			coniox_bs();
			++__str;
			continue;
		}

		/* Unicode twin of cputs(): one WriteConsoleOutputW per run. */
		run = __str;
		runlen = 0;
		if (ti.curx > winwidth)
		{
			ti.curx = winwidth;
		}
		remaining = winwidth - ti.curx + 1;
		if (remaining > CONIOX_MAX_CHARINFO)
		{
			remaining = CONIOX_MAX_CHARINFO;
		}

		/* One pass scans the run and builds its cells: one DWORD store per cell (UnicodeChar low, attribute high).
		   Common case (ch > 13) costs a single compare. */
		attrcell = (unsigned long) ti.attribute << 16;
		for (; runlen < remaining; runlen++)
		{
			ch = (unsigned short) run[runlen];
			if (ch > L'\r')
			{
				((unsigned long *) ci)[runlen] = attrcell | ch;
				continue;
			}
			if (ch == 0 || ch == L'\r' || ch == L'\n' || ch == L'\b')
			{
				break;
			}
			((unsigned long *) ci)[runlen] = attrcell | ch;
		}

		if (runlen)
		{
			r.Left   = (short) (coniox_orgx + ti.winleft + ti.curx - 2);
			r.Top    = (short) (coniox_orgy + ti.wintop + ti.cury - 2);
			r.Right  = (short) (r.Left + runlen - 1);
			r.Bottom = r.Top;
			s.X = (short) runlen;
			s.Y = 1;
			WriteConsoleOutputW(coniox_console_output, ci, s, c, &r);
			ti.curx += runlen;
		}

		/* A run never crosses the window edge: the wrap is decided once. */
		if (ti.curx > winwidth)
		{
			ti.curx = winwidth;
			coniox_wrap();
		}
		__str += runlen;
	}
	/* Deferred physical cursor sync, exactly like DOS cputs(): the real cursor is programmed
		at the next wait point via coniox_flush_cursor(), or at exit. */
	coniox_cursor_dirty = 1;
	return (int)(__str - orig);
}


/* ----------------------------------------------------------------------------------------------------------------- */
static coniox_inline void coniox_putchxyattr(int x, int y, int ch, int attr)
{
	CHAR_INFO ci;
	SMALL_RECT r;
	COORD c = {0, 0}, s = {1, 1};


	r.Left = (short) (coniox_orgx + x - 1);
	r.Top = (short) (coniox_orgy + y - 1);
	r.Right = r.Left;
	r.Bottom = r.Top;
	ci.Char.AsciiChar = (char) ch;
	ci.Attributes = (WORD) attr;
	WriteConsoleOutputA(coniox_console_output, &ci, s, c, &r);
}


/* ----------------------------------------------------------------------------------------------------------------- */
static coniox_inline void coniox_putwchxyattr(int x, int y, wchar_t ch, int attr)
{
	CHAR_INFO ci;
	SMALL_RECT r;
	COORD c = { 0, 0 }, s = { 1, 1 };

	r.Left = (short) (coniox_orgx + x - 1);
	r.Top = (short) (coniox_orgy + y - 1);
	r.Right = r.Left;
	r.Bottom = r.Top;
	ci.Char.UnicodeChar = ch;
	ci.Attributes = (WORD) attr;
	WriteConsoleOutputW(coniox_console_output, &ci, s, c, &r);
}


/* ----------------------------------------------------------------------------------------------------------------- */
static void coniox_putchxyattrwh(int x, int y, int ch, int attr, int w, int h)
{
	int i;
	int total;
	unsigned long cell;
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

	/* One DWORD store per cell, like cputs(): char | padding | attribute. */
	cell = ((unsigned long) (WORD) attr << 16) | (unsigned char) ch;
	for (i = 0; i < total; i++)
	{
		((unsigned long *) ci)[i] = cell;
	}

	r.Left = (short) (coniox_orgx + x - 1);
	r.Top = (short) (coniox_orgy + y - 1);
	r.Right = (short) (r.Left + w - 1);
	r.Bottom = (short) (r.Top + h - 1);

	s.X = (short) w;
	s.Y = (short) h;
	c.X = 0;
	c.Y = 0;

	WriteConsoleOutputA(coniox_console_output, ci, s, c, &r);
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
	int isext = getch_last_extended_key;
	wchar_t ch = getwch();

	if (!isext && ch != 0 && ch != (wchar_t) EOF)
	{
		putwch(ch);
		coniox_flush_cursor();
	}
	return(ch);
}


/* ----------------------------------------------------------------------------------------------------------------- */
static coniox_inline wchar_t coniox_ansi2unicode(char ch)
{
	wchar_t unicodech = 0;

	MultiByteToWideChar(GetOEMCP(), 0, &ch, 1, &unicodech, 1);
	return(unicodech);
}


/* ----------------------------------------------------------------------------------------------------------------- */
int getch(void)
{
	INPUT_RECORD ir;
	DWORD read, mode;
	int ch = EOF;

	coniox_ready();	/* getch() may be the first conio call: ensure handles */
	if (getch_last_extended_key)
	{
		ch = getch_last_extended_key;
		getch_last_extended_key = 0;
		return(ch);
	}
	if (!GetConsoleMode(coniox_console_input, &mode))
	{
		return(EOF);	/* redirected input */
	}
	SetConsoleMode(coniox_console_input, mode & ~ENABLE_PROCESSED_INPUT);	/* Ctrl+C is a key, not a signal */
	while (!kbhit())
	{
		WaitForSingleObject(coniox_console_input, INFINITE);
	}
	/* kbhit() left the key event at the head of the queue. */
	if (ReadConsoleInputA(coniox_console_input, &ir, 1, &read) && read != 0)
	{
		ch = (unsigned char) ir.Event.KeyEvent.uChar.AsciiChar;
		if (ch == 0)
		{
			getch_last_extended_key = ir.Event.KeyEvent.wVirtualScanCode;
		}
	}
	SetConsoleMode(coniox_console_input, mode);	/* restore the real mode, even 0 */
	return(ch);
}


/* ----------------------------------------------------------------------------------------------------------------- */
void gotoxy(int __x, int __y)
{
	COORD c;

	coniox_ready();
	if (!coniox_inwindow(ti.winleft + __x - 1, ti.wintop + __y - 1))
	{
		return;
	}
	ti.curx = (unsigned short) __x;
	ti.cury = (unsigned short) __y;
	coniox_cursor_dirty = 0;	/* gotoxy leaves the physical cursor up to date (or hidden) */
	if (coniox_setcursortype != _NOCURSOR)
	{
		c.X = (short) (coniox_orgx + ti.winleft + ti.curx - 2);
		c.Y = (short) (coniox_orgy + ti.wintop + ti.cury - 2);
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
	coniox_ready();
	ti.attribute = (unsigned short) (__newattr & 0xFF);
	SetConsoleTextAttribute(coniox_console_output, ti.attribute);
}


/* ----------------------------------------------------------------------------------------------------------------- */
void delline(void)
{
	COORD c;
	SMALL_RECT r;
	CHAR_INFO ci;


	coniox_ready();
	/* Last window line (or a one-line window): the scroll rectangle is empty and ScrollConsoleScreenBuffer would fail, so clear the line instead. */
	if (ti.cury >= ti.winbottom - ti.wintop + 1)
	{
		coniox_putchxyattrwh(ti.winleft, ti.wintop + ti.cury - 1, ' ', ti.attribute, ti.winright - ti.winleft + 1, 1);
		return;
	}
	c.X = (short) (coniox_orgx + ti.winleft - 1);
	c.Y = (short) (coniox_orgy + ti.wintop - 1 + ti.cury - 1);
	r.Left = c.X;
	r.Top = (short) (coniox_orgy + ti.wintop - 1 + ti.cury);
	r.Right = (short) (coniox_orgx + ti.winright - 1);
	r.Bottom = (short) (coniox_orgy + ti.winbottom - 1);
	ci.Attributes = ti.attribute;
	/* ScrollConsoleScreenBuffer has no ANSI variant: it always reads Char.UnicodeChar, so write the whole WCHAR, not just its low byte. */
	ci.Char.UnicodeChar = L' ';
	ScrollConsoleScreenBuffer(coniox_console_output, &r, NULL, c, &ci);
	/* ScrollConsoleScreenBuffer does not move the console cursor; the sync happens via coniox_flush_cursor(). */
}


/* ----------------------------------------------------------------------------------------------------------------- */
void insline(void)
{
	COORD c;
	SMALL_RECT r;
	CHAR_INFO ci;


	coniox_ready();
	/* Last window line (or a one-line window): the scroll rectangle is empty and ScrollConsoleScreenBuffer would fail, so clear the line instead. */
	if (ti.cury >= ti.winbottom - ti.wintop + 1)
	{
		coniox_putchxyattrwh(ti.winleft, ti.wintop + ti.cury - 1, ' ', ti.attribute, ti.winright - ti.winleft + 1, 1);
		return;
	}
	c.X = (short) (coniox_orgx + ti.winleft - 1);
	c.Y = (short) (coniox_orgy + ti.wintop - 1 + ti.cury);
	r.Left = c.X;
	r.Top = (short) (coniox_orgy + ti.wintop - 1 + ti.cury - 1);
	r.Right = (short) (coniox_orgx + ti.winright - 1);
	r.Bottom = (short) (coniox_orgy + ti.winbottom - 2);
	ci.Attributes = ti.attribute;
	/* ScrollConsoleScreenBuffer has no ANSI variant: it always reads Char.UnicodeChar, so write the whole WCHAR, not just its low byte. */
	ci.Char.UnicodeChar = L' ';
	ScrollConsoleScreenBuffer(coniox_console_output, &r, NULL, c, &ci);
	/* ScrollConsoleScreenBuffer does not move the console cursor; the sync happens via coniox_flush_cursor(). */
}


/* ----------------------------------------------------------------------------------------------------------------- */
int gettext(int __left, int __top, int __right, int __bottom, void *__destin)
{
	int i;
	int total;
	int ok;
	SMALL_RECT r;
	unsigned short *buf;
	COORD s, c = { 0,0 };
	CHAR_INFO *ci;


	coniox_ready();
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


	r.Left = (short) (coniox_orgx + __left - 1);
	r.Top = (short) (coniox_orgy + __top - 1);
	r.Right = (short) (coniox_orgx + __right - 1);
	r.Bottom = (short) (coniox_orgy + __bottom - 1);
	s.X = (short) (__right - __left + 1);
	s.Y = (short) (__bottom - __top + 1);
	buf = (unsigned short *) __destin;
	ok = ReadConsoleOutputA(coniox_console_output, ci, s, c, &r);
	for (i = 0; ok && i < total; i++)
	{
		/* One DWORD load per cell: char in bits 0-7, attribute in bits 16-23. */
		buf[i] = (unsigned short) ((((unsigned long *) ci)[i] & 0xFF) | ((((unsigned long *) ci)[i] >> 8) & 0xFF00));
	}
	if (ci != coniox_charinfo)
	{
		free(ci);
	}
	return(ok ? 1 : 0);
}


/* ----------------------------------------------------------------------------------------------------------------- */
int movetext(int __left, int __top, int __right, int __bottom, int __destleft, int __desttop)
{
	COORD s, c = { 0, 0 };
	SMALL_RECT r;
	CHAR_INFO *scratch;
	int w, h, total, ok;

	coniox_ready();

	/* Borland-style validation: source and destination inside the screen. */
	if (__left < 1 || __top < 1 || __right < __left || __bottom < __top || __right > ti.screenwidth || __bottom > ti.screenheight)
	{
		return(0);
	}
	if (__destleft < 1 || __desttop < 1 || __destleft + (__right - __left) > ti.screenwidth || __desttop + (__bottom - __top) > ti.screenheight)
	{
		return(0);
	}
	if (__left == __destleft && __top == __desttop)
	{
		return(1);
	}

	w = __right - __left + 1;
	h = __bottom - __top + 1;
	total = w * h;
	scratch = (total <= CONIOX_MAX_CHARINFO) ? coniox_charinfo : (CHAR_INFO *) malloc((size_t) total * sizeof(CHAR_INFO));
	if (scratch == NULL)
	{
		return(0);
	}

	r.Left = (short) (coniox_orgx + __left - 1);
	r.Top = (short) (coniox_orgy + __top - 1);
	r.Right = (short) (coniox_orgx + __right - 1);
	r.Bottom = (short) (coniox_orgy + __bottom - 1);
	s.X = (short) w;
	s.Y = (short) h;
	ok = ReadConsoleOutputW(coniox_console_output, scratch, s, c, &r);
	if (ok)
	{
		r.Left = (short) (coniox_orgx + __destleft - 1);
		r.Top = (short) (coniox_orgy + __desttop - 1);
		r.Right = (short) (coniox_orgx + __destleft + w - 2);
		r.Bottom = (short) (coniox_orgy + __desttop + h - 2);
		ok = WriteConsoleOutputW(coniox_console_output, scratch, s, c, &r);
	}
	if (scratch != coniox_charinfo)
	{
		free(scratch);
	}
	return(ok ? 1 : 0);
}


/* ----------------------------------------------------------------------------------------------------------------- */
int puttext(int __left, int __top, int __right, int __bottom, void *__source)
{
	int i;
	int total;
	int ok;
	SMALL_RECT r;
	unsigned short *ci;
	COORD s, c = { 0,0 };
	CHAR_INFO *buffer;


	coniox_ready();
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

	r.Left = (short) (coniox_orgx + __left - 1);
	r.Top = (short) (coniox_orgy + __top - 1);
	r.Right = (short) (coniox_orgx + __right - 1);
	r.Bottom = (short) (coniox_orgy + __bottom - 1);
	s.X = (short) (__right - __left + 1);
	s.Y = (short) (__bottom - __top + 1);

	ci = (unsigned short *) __source;
	for (i = 0; i < total; i++)
	{
		/* One DWORD store per cell, like cputs(): char | padding | attribute. */
		((unsigned long *) buffer)[i] = (unsigned long) (ci[i] & 0xFF) | ((unsigned long) (ci[i] >> 8) << 16);
	}
	ok = WriteConsoleOutputA(coniox_console_output, buffer, s, c, &r);
	if (buffer != coniox_charinfo)
	{
		free(buffer);
	}
	return(ok ? 1 : 0);
}


/* ----------------------------------------------------------------------------------------------------------------- */
typedef char coniox_wchar_info_size_check[(sizeof(wchar_info) == sizeof(CHAR_INFO)) ? 1 : -1];

int putwtext(int __left, int __top, int __right, int __bottom, const wchar_info *__source)
{
	int ok;
	SMALL_RECT r;
	COORD s, c = { 0,0 };


	coniox_ready();
	/* Borland-style validation: rectangle inside the absolute screen. */
	if (__source == NULL || __left < 1 || __top < 1 || __right > ti.screenwidth || __bottom > ti.screenheight || __right < __left || __bottom < __top)
	{
		return(0);
	}
	r.Left = (short) (coniox_orgx + __left - 1);
	r.Top = (short) (coniox_orgy + __top - 1);
	r.Right = (short) (coniox_orgx + __right - 1);
	r.Bottom = (short) (coniox_orgy + __bottom - 1);
	s.X = (short) (__right - __left + 1);
	s.Y = (short) (__bottom - __top + 1);

	/* wchar_info and CHAR_INFO share the layout (WCHAR + WORD, 4 bytes): the console reads/writes the caller buffer directly, no conversion pass nor scratch copy. */
	ok = WriteConsoleOutputW(coniox_console_output, (const CHAR_INFO *) __source, s, c, &r);
	return(ok ? 1 : 0);
}
/* ----------------------------------------------------------------------------------------------------------------- */
int getwtext(int __left, int __top, int __right, int __bottom, wchar_info *__destin)
{
	int ok;
	SMALL_RECT r;
	COORD s, c = { 0,0 };


	coniox_ready();
	/* Borland-style validation: rectangle inside the absolute screen. */
	if (__destin == NULL || __left < 1 || __top < 1 || __right > ti.screenwidth || __bottom > ti.screenheight || __right < __left || __bottom < __top)
	{
		return(0);
	}
	r.Left = (short) (coniox_orgx + __left - 1);
	r.Top = (short) (coniox_orgy + __top - 1);
	r.Right = (short) (coniox_orgx + __right - 1);
	r.Bottom = (short) (coniox_orgy + __bottom - 1);
	s.X = (short) (__right - __left + 1);
	s.Y = (short) (__bottom - __top + 1);

	/* wchar_info and CHAR_INFO share the layout (WCHAR + WORD, 4 bytes): the console reads/writes the caller buffer directly, no conversion pass nor scratch copy. */
	ok = ReadConsoleOutputW(coniox_console_output, (CHAR_INFO *) __destin, s, c, &r);
	return(ok ? 1 : 0);
}
/* ----------------------------------------------------------------------------------------------------------------- */
void textmode(int __newmode)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	coniox_ready();

	if (__newmode == LASTMODE)
	{
		__newmode = ti.currmode;
	}
	/* DigitalMars crashes if SetConsoleScreenBufferInfoEx is used */
	#if (!defined(__ZTC__))
		if (__newmode != ti.currmode)
		{
			/* Mode table: first match wins; any unlisted mode falls back to 80x25. */
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
			CONSOLE_SCREEN_BUFFER_INFOEX info;
			unsigned int mi;

			info.cbSize = sizeof(info);
			GetConsoleScreenBufferInfoEx(coniox_console_output, &info);
			info.dwSize.X = 80;
			info.dwSize.Y = 25;
			for (mi = 0; mi < sizeof(coniox_modes) / sizeof(coniox_modes[0]); mi++)
			{
				if (coniox_modes[mi].mode == __newmode)
				{
					info.dwSize.X = coniox_modes[mi].w;
					info.dwSize.Y = coniox_modes[mi].h;
					break;
				}
			}
			/* SetConsoleScreenBufferInfoEx applies the whole structure: the window and the cursor must fit the new buffer size, or the call fails. */
			info.srWindow.Left = 0;
			info.srWindow.Top = 0;
			info.srWindow.Right = info.dwSize.X - 1;
			info.srWindow.Bottom = info.dwSize.Y - 1;
			info.dwCursorPosition.X = 0;
			info.dwCursorPosition.Y = 0;
			info.dwMaximumWindowSize = info.dwSize;
			if (SetConsoleScreenBufferInfoEx(coniox_console_output, &info))
			{
				ti.currmode = (unsigned short) __newmode;
			}
		}
	#endif
	/* Like DOS: the window is the whole visible area again and the attribute goes back to the startup one. */
	if (GetConsoleScreenBufferInfo(coniox_console_output, &csbi))
	{
		coniox_orgx = csbi.srWindow.Left;
		coniox_orgy = csbi.srWindow.Top;
		ti.screenwidth = (unsigned short) (csbi.srWindow.Right - csbi.srWindow.Left + 1);
		ti.screenheight = (unsigned short) (csbi.srWindow.Bottom - csbi.srWindow.Top + 1);
		ti.winleft = 1;
		ti.wintop = 1;
		ti.winright = ti.screenwidth;
		ti.winbottom = ti.screenheight;
		coniox_winsize();
	}
	textattr(ti.normattr);
	clrscr();
	_setcursortype(_NORMALCURSOR);
}


/* ----------------------------------------------------------------------------------------------------------------- */
void _setcursortype(int __cur_t)
{
	CONSOLE_CURSOR_INFO cci;

	coniox_ready();
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
	DWORD nevents = 0, i, done;
	INPUT_RECORD ir[16];
	WORD vk;

	coniox_ready();
	if (getch_last_extended_key)
	{
		return(getch_last_extended_key);
	}
	coniox_flush_cursor();

	/* Peek a batch of events: the leading ones that are not a key (key releases, resizes, mouse) are dropped with a single ReadConsoleInput instead of a Peek + Read pair each. */
	while (PeekConsoleInputA(coniox_console_input, ir, (DWORD) (sizeof(ir) / sizeof(ir[0])), &nevents) && nevents != 0)
	{
		for (i = 0; i < nevents; i++)
		{
			if (ir[i].EventType == KEY_EVENT)
			{
				if (ir[i].Event.KeyEvent.bKeyDown)
				{
					/* A character, or one of the keys DOS reports as 0 + scan code: F1-F12, cursor block, Insert, Delete and keypad 5. */
					vk = ir[i].Event.KeyEvent.wVirtualKeyCode;
					if (ir[i].Event.KeyEvent.uChar.AsciiChar != 0 || (vk >= VK_F1 && vk <= VK_F12) || (vk >= VK_PRIOR && vk <= VK_DOWN) || vk == VK_INSERT || vk == VK_DELETE || vk == VK_CLEAR)
					{
						break;
					}
				}
			}
			else if (ir[i].EventType == WINDOW_BUFFER_SIZE_EVENT)
			{
				CONSOLE_SCREEN_BUFFER_INFO csbi;

				if (GetConsoleScreenBufferInfo(coniox_console_output, &csbi))
				{
					coniox_orgx = csbi.srWindow.Left;
					coniox_orgy = csbi.srWindow.Top;
					ti.screenwidth = (unsigned short) (csbi.srWindow.Right - csbi.srWindow.Left + 1);
					ti.screenheight = (unsigned short) (csbi.srWindow.Bottom - csbi.srWindow.Top + 1);
				}
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
				coniox_winsize();
				if (!coniox_inwindow(ti.winleft + ti.curx - 1, ti.wintop + ti.cury - 1))
				{
					gotoxy(1, 1);
				}
			}
		}
		/* Drop the i events that are not a key; the key itself (i < nevents) stays at the head of the queue for getch(). */
		if (i != 0 && (!ReadConsoleInputA(coniox_console_input, ir, i, &done) || done == 0))
		{
			return(0);
		}
		if (i < nevents)
		{
			return(1);
		}
		if (nevents < (DWORD) (sizeof(ir) / sizeof(ir[0])))
		{
			return(0);	/* the batch covered the whole queue and it is now empty: no second Peek needed */
		}
	}
	return(0);
}


/* ----------------------------------------------------------------------------------------------------------------- */
int ungetch(int __ch)
{
	INPUT_RECORD ir[2];
	DWORD written;


	coniox_ready();
	ir[0].EventType = KEY_EVENT;
	ir[0].Event.KeyEvent.bKeyDown = TRUE;
	ir[0].Event.KeyEvent.wRepeatCount = 1;
	ir[0].Event.KeyEvent.wVirtualKeyCode = 0;
	ir[0].Event.KeyEvent.wVirtualScanCode = 0;
	ir[0].Event.KeyEvent.uChar.AsciiChar = (CHAR) __ch;
	ir[0].Event.KeyEvent.dwControlKeyState = 0;
	ir[1] = ir[0];
	ir[1].Event.KeyEvent.bKeyDown = FALSE;
	if (WriteConsoleInputA(coniox_console_input, ir, 2, &written) && written == 2)
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

	coniox_ready();
	ir[0].EventType = KEY_EVENT;
	ir[0].Event.KeyEvent.bKeyDown = TRUE;
	ir[0].Event.KeyEvent.wRepeatCount = 1;
	ir[0].Event.KeyEvent.wVirtualKeyCode = 0;
	ir[0].Event.KeyEvent.wVirtualScanCode = 0;
	ir[0].Event.KeyEvent.uChar.UnicodeChar = __ch;
	ir[0].Event.KeyEvent.dwControlKeyState = 0;
	ir[1] = ir[0];
	ir[1].Event.KeyEvent.bKeyDown = FALSE;
	if (WriteConsoleInputW(coniox_console_input, ir, 2, &written) && written == 2)
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

static void coniox_idle(void);
static int coniox_get_is_emulator(void);
static int coniox_detect_dosbox_vesa(void);
#if (defined(__DJGPP__)) || (defined(__FLAT__))
static int coniox_dpmi_alloc_dos(unsigned int bytes, unsigned int *seg, unsigned int *sel);
static void coniox_dpmi_free_dos(unsigned int sel);
#endif
static void coniox_blink(unsigned int blink);
static void coniox_enhanced_palette(unsigned int pal);
static void coniox_exit(void);
static int coniox_is_emulator = 0;

/* ----------------------------------------------------------------------------------------------------------------- */
void coniox_init(const void* title)
{
	unsigned int cursor;
	union REGS r;

	(void) title;	/* W303: suppress unused parameter warning */

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
			ti.currmode = peekb(0x40, 0x49) & 0x7F;	/* mask the BIOS no-clear flag, like the INT 10h path */
		}
		else
		{
			r.h.ah = 0xF;
			coniox_int86(0x10, &r, &r);
			ti.currmode = r.h.al & 0x7F;
		}
	}

	#if (defined(__FLAT__))
		coniox_vram = (unsigned short *) ((ti.currmode == MONO) ? 0x000B0000UL : 0x000B8000UL);
	#elif (defined(__DJGPP__))
		coniox_vram = (unsigned short *) (((ti.currmode == MONO) ? 0x000B0000UL : 0x000B8000UL) + __djgpp_conventional_base);
	#else
		coniox_vram = MK_FP((ti.currmode == MONO) ? 0xB000 : 0xB800, 0);
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
			ti.screenheight = peekb(0x40, 0x84) + 1;	/* INT 10h AH=0Fh reports no row count: use the BDA one, validated below */
		}
	}
	/* BDA 0040:0084 is only meaningful on EGA/VGA-class systems.
		Reject implausible heights (including the pre-EGA value 1). */
	if (ti.screenheight < 2 || ti.screenheight > 128)
	{
		ti.screenheight = 25;
	}
	ti.winleft = 1;
	ti.wintop = 1;
	ti.winright = ti.screenwidth;
	ti.winbottom = ti.screenheight;
	coniox_winsize();
	/* Initial position of the putch()/cputs() cell cache: the just-read (curx, cury) cell; gotoxy() keeps it up to date. */
	coniox_cell = coniox_offset(ti.winleft + ti.curx - 2, ti.wintop + ti.cury - 2);
	/* Real startup attribute at the cursor cell, like Borland and the Windows version, instead of a fixed 7. */
	coniox_readnormattr();
	ti.attribute = ti.normattr;


	/* Get if under emulator */
	coniox_is_emulator = coniox_get_is_emulator();

	/* Activate blink */
	coniox_blink(1);
	coniox_enhanced_palette(1);

	{
		static int coniox_exit_hooked = 0;

		if (!coniox_exit_hooked)
		{
			coniox_exit_hooked = 1;
			atexit(coniox_exit);	/* sync the lazy cursor and give the DAC palette back when the program ends */
		}
	}
}


/* ----------------------------------------------------------------------------------------------------------------- */
static coniox_inline void coniox_readnormattr(void)
{
	union REGS r;

	coniox_ready();
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
	int winwidth, wrapped;
	unsigned short attrword;
	coniox_cellptr coniox_currentoffset;

	if (__str == NULL)
	{
		return 0;
	}

	coniox_ready();

	winwidth = coniox_winwidth;

	/* The putch()/gotoxy() cell cache already points at the (curx, cury) cell: it is tracked here and stored back on exit. */
	attrword = (unsigned short) (ti.attribute << 8);
	coniox_currentoffset = coniox_cell;

	while (*__str)
	{
		/* Carriage return: back to start of line. */
		if (*__str == '\r')
		{
			coniox_currentoffset -= coniox_cr();
			++__str;
			continue;
		}

		/* Line feed: advance or scroll the window. A scroll moves neither the cursor nor the cell cache: coniox_currentoffset still matches (curx, cury). */
		if (*__str == '\n')
		{
			if (coniox_lf())
			{
				coniox_currentoffset += ti.screenwidth;
			}
			++__str;
			continue;
		}

		/* Backspace: move back without erasing. */
		if (*__str == '\b')
		{
			coniox_currentoffset -= coniox_bs();
			++__str;
			continue;
		}

		run = __str;
		if (ti.curx > winwidth)
		{
			/* Re-sync the offset with the clamp so the run paints inside the window. */
			ti.curx = winwidth;
			coniox_currentoffset = coniox_offset(ti.winleft + ti.curx - 2, ti.wintop + ti.cury - 2);
		}
		remaining = winwidth - ti.curx + 1;

		if (directvideo)
		{
			/* Fast path: coniox_putrun() scans the run and writes it straight to VRAM in one pass; it stops at the first control or after remaining chars and returns how many cells it wrote. */
			runlen = coniox_putrun(coniox_currentoffset, run, attrword, remaining);
		}
		else
		{
			unsigned int i;

			/* Bounded scan, like cputws(): strcspn would walk the whole remaining string while a run never passes the window edge. */
			runlen = 0;
			while (runlen < remaining && ((unsigned char) run[runlen] > '\r' || (run[runlen] != 0 && run[runlen] != '\r' && run[runlen] != '\n' && run[runlen] != '\b')))
			{
				runlen++;
			}
			for (i = 0; i < runlen; i++)
			{
				coniox_putchxyattr(ti.winleft + ti.curx - 1 + (int) i, ti.wintop + ti.cury - 1, run[i], ti.attribute);
			}
		}
		coniox_currentoffset += runlen;
		ti.curx += runlen;

		/* A run never crosses the window edge: the wrap is decided once. The run has stepped one cell past the edge, so go back to the last column first and apply the same cell cache moves as putch(). */
		if (ti.curx > winwidth)
		{
			ti.curx = winwidth;
			--coniox_currentoffset;
			wrapped = coniox_wrap();
			if (wrapped == 1)
			{
				coniox_currentoffset += ti.screenwidth - winwidth + 1;
			}
			else if (wrapped == 2)
			{
				coniox_currentoffset += 1 - winwidth;
			}
		}
		__str += runlen;
	}
	/* Deferred physical cursor sync, same pattern as putch(): the real cursor is programmed at the next wait point via coniox_flush_cursor(). */
	coniox_cursor_dirty = 1;
	/* Leave the cell cache pointing at (curx, cury) for the next putch()/cputs(). */
	coniox_cell = coniox_currentoffset;
	return (int)(__str - orig);
}


/* ----------------------------------------------------------------------------------------------------------------- */
static coniox_inline void coniox_putchxyattr(int x, int y, int ch, int attr)
{
	if (directvideo)
	{
		*coniox_offset(x - 1, y - 1) = (unsigned short) ((attr << 8) | (ch & 0xFF));
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
static void coniox_putchxyattrwh(int x, int y, int ch, int attr, int w, int h)
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

		v = (unsigned short)((attr << 8) | (ch & 0xFF));
		p = coniox_offset(x - 1, y - 1);
		if (w == (int) ti.screenwidth)
		{
			/* Full-screen-width block: VRAM rows are contiguous, one fill covers all rows. */
			coniox_fmemsetw(p, v, (size_t) w * (size_t) h);
			return;
		}
		for (y1 = 0; y1 < h; y1++)
		{
			coniox_fmemsetw(p, v, w);
			p += ti.screenwidth;
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
static void coniox_idle(void)
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
/* DPMI 0x0100 (allocate DOS memory block): AX=0x0100, BX=paragraphs, returns AX=real-mode segment,
   DX=protected-mode selector, CF set on error. The same call works under DJGPP (via __dpmi_int) and
   Watcom DOS32 (via int386x(0x31)). Real-mode builds skip the function entirely. */
#if (defined(__DJGPP__)) || (defined(__FLAT__))
static int coniox_dpmi_alloc_dos(unsigned int bytes, unsigned int *seg, unsigned int *sel)
{
#if (defined(__DJGPP__))
	int local_sel = 0;
	unsigned long rm_seg = __dpmi_allocate_dos_memory((bytes + 15) >> 4, &local_sel);
	if (rm_seg == (unsigned long) -1)
	{
		return 1;
	}
	*seg = (unsigned int) rm_seg;
	*sel = (unsigned int) local_sel;
	return 0;
#else
	union REGS inregs, outregs;
	memset(&inregs, 0, sizeof(inregs));
	coniox_w(inregs).ax = 0x0100;
	coniox_w(inregs).bx = (unsigned short) ((bytes + 15) >> 4);
	int386x(0x31, &inregs, &outregs, NULL);
	if (coniox_w(outregs).cflag)
	{
		return 1;
	}
	*seg = coniox_w(outregs).ax;
	*sel = coniox_w(outregs).dx;
	return 0;
#endif
}


/* ----------------------------------------------------------------------------------------------------------------- */
/* DPMI 0x0101 (free DOS memory block): AX=0x0101, DX=selector. */
static void coniox_dpmi_free_dos(unsigned int sel)
{
#if (defined(__DJGPP__))
	__dpmi_free_dos_memory((int) sel);
#else
	union REGS inregs, outregs;
	memset(&inregs, 0, sizeof(inregs));
	coniox_w(inregs).ax = 0x0101;
	coniox_w(inregs).dx = (unsigned short) sel;
	int386x(0x31, &inregs, &outregs, NULL);
#endif
}
#endif /* (defined(__DJGPP__)) || (defined(__FLAT__)) */


/* ----------------------------------------------------------------------------------------------------------------- */
/* DOSBox / vDos detection via VESA VBE 1.x/2.0+.
   DOSBox (and derivatives: DOSBox-X, DOSBox Staging) report an OEM string that contains "DOSBox".
   vDos reports itself as "vDos" (sometimes upper-cased "VDOS"). The OEM string lives either inside
   the VbeInfoBlock (VBE 2.0+ extended block, requested by writing "VBE2" at offset 0 before the call)
   or in a far buffer pointed to by OemStringPtr (VBE 1.x). We scan the whole 512-byte block, which is
   enough for VBE 2.0+ on every emulator tested.

   The same INT 10h call works under real mode, DJGPP (DPMI) and Watcom DOS32 because coniox_int86x,
   coniox_w, coniox_dpmi_alloc_dos and coniox_dpmi_free_dos already abstract the differences. In real
   mode the local far buffer is itself in conventional memory, so we can skip the DOS-memory alloc. */
static int coniox_detect_dosbox_vesa(void)
{
	static const char coniox_dosbox[6] = { 'D','O','S','B','o','x' };
	static const char coniox_vdos_lo[4] = { 'v','D','o','s' };
	static const char coniox_vdos_hi[4] = { 'V','D','O','S' };
	unsigned char buffer[512];
	unsigned char coniox_far *block;
	unsigned int seg = 0;
#if (defined(__DJGPP__)) || (defined(__FLAT__))
	unsigned int sel = 0;
#endif
	union REGS inregs, outregs;
	struct SREGS sregs;
	int i, j;
	int ret = 0;
	static const unsigned char marker[4] = { 'V','B','E','2' };

#if (defined(__DJGPP__)) || (defined(__FLAT__))
	/* Protected mode: allocate a conventional-memory block the BIOS can touch. coniox_dpmi_alloc_dos
	   issues DPMI 0x0100 directly, so it does not depend on the Watcom _dos_allocmem signature
	   (2 args in OW 1.9, 3 args in OW 2.0). */
	if (coniox_dpmi_alloc_dos(512, &seg, &sel) != 0)
	{
		return 0;
	}
	block = (unsigned char coniox_far *) MK_FP(seg, 0);
#else
	/* Real mode: the stack buffer is already in conventional memory, addressed directly by ES:DI. */
	block = (unsigned char coniox_far *) buffer;
#endif
	/* block is __far in real mode: use the far-aware helpers instead of memset/memcpy,
	   which take near void * and would truncate the segment half of the pointer (W112). */
	coniox_fmemsetw(block, 0, 256);			/* 256 shorts = 512 bytes of zeros */
	coniox_fmemcpy(block, marker, sizeof(marker));

	memset(&inregs, 0, sizeof(inregs));
	memset(&sregs, 0, sizeof(sregs));
	coniox_w(inregs).ax = 0x4F00;
#if (defined(__DJGPP__)) || (defined(__FLAT__))
	/* seg already holds the real-mode segment returned by the DPMI alloc; offset inside the paragraph is 0. */
	coniox_w(inregs).di = 0;
#else
	/* Real mode: point ES:DI at the local far buffer (segment + offset of the stack array). */
	seg = FP_SEG(block);
	coniox_w(inregs).di = FP_OFF(block);
#endif
	sregs.es = (unsigned short) seg;
	coniox_int86x(0x10, &inregs, &outregs, &sregs);

	if (coniox_w(outregs).ax == 0x004F)
	{
#if (defined(__DJGPP__)) || (defined(__FLAT__))
		_fmemcpy(buffer, block, 512);
#endif
		/* Sanity check: a real VBE BIOS leaves the "VESA" signature at offset 0. */
		if (buffer[0] == 'V' && buffer[1] == 'E' && buffer[2] == 'S' && buffer[3] == 'A')
		{
			for (i = 0; i <= (int)(sizeof(buffer) - sizeof(coniox_dosbox)); i++)
			{
				if (buffer[i]   == (unsigned char) coniox_dosbox[0] &&
				    buffer[i+1] == (unsigned char) coniox_dosbox[1] &&
				    buffer[i+2] == (unsigned char) coniox_dosbox[2] &&
				    buffer[i+3] == (unsigned char) coniox_dosbox[3] &&
				    buffer[i+4] == (unsigned char) coniox_dosbox[4] &&
				    buffer[i+5] == (unsigned char) coniox_dosbox[5])
				{
					ret = 1;
					break;
				}
			}
			if (!ret)
			{
				for (j = 0; j <= (int)(sizeof(buffer) - sizeof(coniox_vdos_lo)); j++)
				{
					if ((buffer[j]   == (unsigned char) coniox_vdos_lo[0] &&
					     buffer[j+1] == (unsigned char) coniox_vdos_lo[1] &&
					     buffer[j+2] == (unsigned char) coniox_vdos_lo[2] &&
					     buffer[j+3] == (unsigned char) coniox_vdos_lo[3]) ||
					    (buffer[j]   == (unsigned char) coniox_vdos_hi[0] &&
					     buffer[j+1] == (unsigned char) coniox_vdos_hi[1] &&
					     buffer[j+2] == (unsigned char) coniox_vdos_hi[2] &&
					     buffer[j+3] == (unsigned char) coniox_vdos_hi[3]))
					{
						ret = 1;
						break;
					}
				}
			}
		}
	}

#if (defined(__DJGPP__)) || (defined(__FLAT__))
	coniox_dpmi_free_dos(sel);
#endif
	return ret;
}


/* ----------------------------------------------------------------------------------------------------------------- */
static int coniox_get_is_emulator(void)
{
	unsigned char result;

	/* Fast path: the 0x4F4 port is DOSBox's documented detection port (write 0x00, read back 0xFF).
	   Cheap (two port I/Os) and covers vanilla DOSBox, but DOSBox-X, DOSBox Staging and vDos
	   do not implement it, so a non-hit falls through to the slower VESA VBE OEM scan. */
	outportb(0x04F4, 0x00);
	result = inportb(0x04F4);
	if (result == 0xFF)
	{
		return 1;
	}

	/* Slow path: VESA VBE OEM string scan. Catches DOSBox variants that do not hook 0x4F4
	   and still reports DOSBox itself when 0x4F4 happens to be wired to a real device on hardware. */
	return coniox_detect_dosbox_vesa();
}


/* ----------------------------------------------------------------------------------------------------------------- */
static void coniox_enhanced_palette(unsigned int pal)
{
	unsigned int i;
	const unsigned char (*coniox_rgb)[3];
	/* In text modes the attribute controller maps colour 0-15 to these DAC registers, not to DAC 0-15. */
	static const unsigned char coniox_dac[16] = {0, 1, 2, 3, 4, 5, 20, 7, 56, 57, 58, 59, 60, 61, 62, 63};
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
	/* Standard VGA colours, as the BIOS programs them in the DAC registers above. */
	static const unsigned char coniox_default[16][3] =
	{
		{0x00, 0x00, 0x00},
		{0x00, 0x00, 0x2A},
		{0x00, 0x2A, 0x00},
		{0x00, 0x2A, 0x2A},
		{0x2A, 0x00, 0x00},
		{0x2A, 0x00, 0x2A},
		{0x2A, 0x15, 0x00},
		{0x2A, 0x2A, 0x2A},
		{0x15, 0x15, 0x15},
		{0x15, 0x15, 0x3F},
		{0x15, 0x3F, 0x15},
		{0x15, 0x3F, 0x3F},
		{0x3F, 0x15, 0x15},
		{0x3F, 0x15, 0x3F},
		{0x3F, 0x3F, 0x15},
		{0x3F, 0x3F, 0x3F}
	};

	if (ti.currmode == MONO)
	{
		return;
	}

	/* pal = 0: default VGA palette; pal != 0: CONIOX enhanced palette. */
	coniox_rgb = pal ? coniox_palette : coniox_default;

	if (directvideo)
	{
		/* Text-mode attribute → DAC map is fixed and non-linear: 0-5, 20, 7, 56-63.
		   Four index writes + four bulk data bursts (no runtime run detection). */
		outportb(0x3C8, 0);
		coniox_fmemout(0x3C9, &coniox_rgb[0][0], 18);	/* attrs 0-5  → DAC 0-5   */
		outportb(0x3C8, 20);
		coniox_fmemout(0x3C9, &coniox_rgb[6][0], 3);	/* attr  6   → DAC 20    */
		outportb(0x3C8, 7);
		coniox_fmemout(0x3C9, &coniox_rgb[7][0], 3);	/* attr  7   → DAC 7     */
		outportb(0x3C8, 56);
		coniox_fmemout(0x3C9, &coniox_rgb[8][0], 24);	/* attrs 8-15 → DAC 56-63 */
	}
	else
	{
		union REGS r;
		for (i = 0; i < 16; i++)
		{
			#if defined(__WATCOMC__)
				r.w.ax = 0x1010;	/* Set DAC register */
				r.w.bx = coniox_dac[i];	/* BX = DAC register */
			#else
				r.x.ax = 0x1010;
				r.x.bx = coniox_dac[i];
			#endif
			r.h.dh = coniox_rgb[i][0];	/* R */
			r.h.ch = coniox_rgb[i][1];	/* G */
			r.h.cl = coniox_rgb[i][2];	/* B */
			coniox_int86(0x10, &r, &r);
		}
	}
}

/* ----------------------------------------------------------------------------------------------------------------- */
static void coniox_exit(void)
{
	/* atexit() hook: plain void (void) cdecl function, valid for Watcom, Borland/Turbo C, DJGPP, Digital Mars and Microsoft C. */
	coniox_flush_cursor();
	coniox_enhanced_palette(0);	/* the enhanced palette is programmed in the DAC: leave the standard VGA one for the shell */
}


/* ----------------------------------------------------------------------------------------------------------------- */
static void coniox_blink(unsigned int blink)
{
	if ((directvideo) && (ti.currmode != MONO))
	{
		unsigned char v;

		_disable();
		inportb(coniox_basecrt + 6);	/* 3DAh color input status: reset the ATTR flip-flop */
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
void delay(unsigned int ms)
{
	union REGS r;
	unsigned long lTicks, lWait, lNow;
	unsigned long lIdleTick = 0xFFFFFFFFUL;	/* the tick counter never reaches this value: forces a coniox_idle() on the first lap */

	coniox_flush_cursor();

	lWait = (((unsigned long) ms * 91UL) + 4999UL) / 5000UL;	/* ms -> ticks at 18.2 Hz, rounded up */
	lTicks = 0xFFFFFFFFUL;
	while (lWait)
	{
		if (directvideo)
		{
			lNow = peekl(0x40, 0x6C);	/* BDA 0040:006C tick counter */
		}
		else
		{
			r.h.ah = 0;	/* INT 1Ah AH=00h: read ticks */
			coniox_int86(0x1A, &r, &r);
			#if defined(__WATCOMC__)
				lNow = ((unsigned long) r.w.cx << 16) | r.w.dx;
			#else
				lNow = ((unsigned long) r.x.cx << 16) | r.x.dx;
			#endif
		}
		if (lTicks == 0xFFFFFFFFUL)
		{
			lTicks = lNow;	/* first lap: start of the wait */
		}
		if (lNow < lTicks)
		{
			lNow += 0x1800B0UL;	/* the BDA counter wraps at midnight after 0x1800B0 ticks, not at 2^32 */
		}
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


/* ----------------------------------------------------------------------------------------------------------------- */
void textmode(int __newmode)
{
	union REGS r;

	coniox_ready();
	if (__newmode == LASTMODE)
	{
		__newmode = ti.currmode;
	}
	/* INT 10h only knows the BIOS text modes (BW40, C40, BW80, C80, MONO) plus the emulated C4350: anything else would select a graphics mode. */
	if (__newmode != ti.currmode && (__newmode == C4350 || __newmode == MONO || (unsigned int) __newmode <= C80))
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
			pokeb(0x40, 0x87, peekb(0x40, 0x87) | 1);	/* Disable cursor size emulation */
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
		ti.currmode = (unsigned short) __newmode;
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


	coniox_ready();
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
		outportw(coniox_basecrt, (cursor_start << 8) | 0x0A);	/* index 3D4h + data 3D5h in one 16-bit OUT, like gotoxy() */
		outportw(coniox_basecrt, (cursor_end << 8) | 0x0B);

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
					/* 6..7 works for the 16-scan cells of 25-line modes and for the 8-scan cells of the 43/50-line ones; end < start would hide the cursor. */
					cursor_start = 6;
					cursor_end = 7;
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
	coniox_ready();
	ti.attribute = (unsigned short) (__newattr & 0xFF);
}


/* ----------------------------------------------------------------------------------------------------------------- */
void gotoxy(int __x, int __y)
{
	int cursor;
	unsigned int scrx;
	unsigned int scry;

	coniox_ready();
	if (!coniox_inwindow(ti.winleft + __x - 1, ti.wintop + __y - 1))
	{
		return;
	}
	ti.curx = __x;
	ti.cury = __y;
	scrx = (unsigned int) (ti.winleft + __x - 2);
	scry = (unsigned int) (ti.wintop  + __y - 2);
	coniox_cursor_dirty = 0;	/* gotoxy leaves the physical cursor up to date (or hidden) */

	/* The cell cache and the 6845 cursor address share the same linear offset: compute it just once. */
	cursor = scry * ti.screenwidth + scrx;
	coniox_cell = coniox_vram + cursor;

	/* Only move cursor if visible */
	if (coniox_setcursortype != _NOCURSOR)
	{
		if (directvideo)
		{
			/* The BIOS data area word and the CRTC address are always written together, so when the word already holds this position the CRTC does too: no port writes at all, they are the slowest thing a gotoxy() does. */
			if (peekw(0x40, 0x50) != (unsigned short) ((scry << 8) | (scrx & 0xFF)))
			{
				pokew(0x40, 0x50, (scry << 8) | (scrx & 0xFF)); /* Put cursor position in BIOS */
				/* CRTC index+data in one 16-bit OUT (index at 3D4h, data at 3D5h): two port writes instead of four. */
				outportw(coniox_basecrt, ((cursor & 0xFF) << 8) | 0x0F);
				outportw(coniox_basecrt, (cursor & 0xFF00) | 0x0E);
			}
		}
		else
		{
			union REGS r;
			r.h.ah = 2;
			r.h.bh = 0;
			r.h.dh = (unsigned char) scry;
			r.h.dl = (unsigned char) scrx;
			coniox_int86(0x10, &r, &r);
		}
	}
}


/* ----------------------------------------------------------------------------------------------------------------- */
void delline(void)
{
	coniox_ready();
	/* Seems to be faster using BIOS under emulators */
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
		coniox_fmemsetw(coniox_offset(ti.winleft - 1, ti.winbottom - 1), (short)(unsigned short)((ti.attribute << 8) | ' '), (size_t) width);
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
		r.h.cl = ti.winleft - 1;	/*  Left column number */
		r.h.ch = ti.cury + ti.wintop - 2;	/* Upper row number, */
		r.h.dl = ti.winright - 1;	/*  Right column number */
		r.h.dh = ti.winbottom - 1;	/* Lower row number */
		coniox_int86(0x10, &r, &r);	/* Scroll up */
	}
}


/* ----------------------------------------------------------------------------------------------------------------- */
void insline(void)
{
	coniox_ready();
	/* Seems to be faster using BIOS under emulators */
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
		coniox_fmemsetw(coniox_offset(ti.winleft - 1, ti.cury + ti.wintop - 2), (short)(unsigned short)((ti.attribute << 8) | ' '), (size_t) width);
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
		r.h.cl = ti.winleft - 1;	/*  Left column number */
		r.h.ch = ti.cury + ti.wintop - 2;	/* Upper row number, */
		r.h.dl = ti.winright - 1;	/*  Right column number */
		r.h.dh = ti.winbottom - 1;	/* Lower row number */
		coniox_int86(0x10, &r, &r);	/* Scroll down */
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

	coniox_ready();

	/* Validate destination and rectangle, absolute 1-based coordinates. */
	if (__destin == NULL || __left < 1 || __top < 1 || __right < __left || __bottom < __top || __right > ti.screenwidth || __bottom > ti.screenheight)
	{
		return 0;
	}

	width = (unsigned int)(__right - __left + 1);
	rows = __bottom - __top + 1;

	row_bytes = (size_t)width * sizeof(unsigned short);
	total_bytes = row_bytes * (size_t)rows;

	p = coniox_offset(__left - 1, __top - 1);

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
int puttext(int __left, int __top, int __right, int __bottom, void coniox_far *__source)
{
	unsigned short coniox_far *p;
	unsigned short coniox_far *src;
	unsigned int width;
	unsigned int sw;
	int rows;
	int y;

	size_t row_bytes;
	size_t total_bytes;

	coniox_ready();

	/* Validate source and rectangle, absolute 1-based coordinates. */
	if (__source == NULL || __left < 1 || __top < 1 || __right < __left || __bottom < __top || __right > ti.screenwidth || __bottom > ti.screenheight)
	{
		return 0;
	}

	width = (unsigned int)(__right - __left + 1);
	rows = __bottom - __top + 1;

	row_bytes = (size_t)width * sizeof(unsigned short);
	total_bytes = row_bytes * (size_t)rows;

	p = coniox_offset(__left - 1, __top - 1);

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
	unsigned short coniox_far *src;
	unsigned short coniox_far *dst;
	int w, h, sw, dx, dy, row;

	coniox_ready();

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

	w = __right - __left + 1;
	h = __bottom - __top + 1;
	sw = ti.screenwidth;
	dx = __destleft - __left;
	dy = __desttop - __top;
	src = coniox_offset(__left - 1, __top - 1);
	dst = coniox_offset(__destleft - 1, __desttop - 1);

	if (dy == 0 && dx > 0 && dx < w)
	{
		/* Partial overlap within one row (0 < dx < w): each row has to be copied backwards. */
		for (row = 0; row < h; row++)
		{
			coniox_fmemcpyr(dst, src, (size_t) w * sizeof(unsigned short));
			src += sw;
			dst += sw;
		}
	}
	else if (dy > 0 && dy < h)
	{
		/* Destination below and overlapping. */
		if (w == sw)
		{
			/* Full width: VRAM rows are contiguous, so the whole block is one backwards copy (dst > src). */
			coniox_fmemcpyr(dst, src, (size_t) w * (size_t) h * sizeof(unsigned short));
		}
		else
		{
			/* Copy bottom-up, like insline(), to read each source row before it is overwritten. A row never overlaps its own copy, so whatever dx is, each row can be moved forward. */
			src += (h - 1) * sw;
			dst += (h - 1) * sw;
			for (row = 0; row < h; row++)
			{
				coniox_fmemcpy(dst, src, (size_t) w * sizeof(unsigned short));
				src -= sw;
				dst -= sw;
			}
		}
	}
	else if (w == sw)
	{
		/* Full width: contiguous rows, single block, copying forward and top-down. */
		coniox_fmemcpy(dst, src, (size_t) w * (size_t) h * sizeof(unsigned short));
	}
	else
	{
		/* All remaining cases: copying forward and top-down, reads always stay ahead of writes. */
		for (row = 0; row < h; row++)
		{
			coniox_fmemcpy(dst, src, (size_t) w * sizeof(unsigned short));
			src += sw;
			dst += sw;
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
		r.h.cl = ti.winleft - 1;	/*  Left column number */
		r.h.ch = ti.wintop - 1;	/* Upper row number */
		r.h.dl = ti.winright - 1;	/*  Right column number */
		r.h.dh = ti.winbottom - 1;	/* Lower row number */
		coniox_int86(0x10, &r, &r);
	}
}


/* ----------------------------------------------------------------------------------------------------------------- */
wchar_t getwch(void)
{
	return((wchar_t) getch());
}

/* ----------------------------------------------------------------------------------------------------------------- */
int kbhit(void)
{
	unsigned long q;

	coniox_ready();
	if (getch_last_extended_key)
	{
		return(getch_last_extended_key);
	}

	coniox_flush_cursor();

	/* One far dword read: head in the low word (0x1A), tail in the high word (0x1C); they differ iff a key is pending. INT 16h AH=01h answers from this same buffer, so the BIOS call is redundant here, and not every compiler's union REGS exposes the ZF. */
	q = peekl(0x40, 0x1A);
	return ((unsigned short) q != (unsigned short) (q >> 16));
}


/* ----------------------------------------------------------------------------------------------------------------- */
int getch(void)
{
	coniox_ready();
	if (getch_last_extended_key)
	{
		int ch = getch_last_extended_key;
		getch_last_extended_key = 0;
		return ch;
	}

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
		if (start == 0 || end <= start)
		{
			start = 0x1E;
			end = 0x3E;
		}
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

		if (((scancode & 0xFF) == 0) || (((scancode & 0xFF) == 0xE0) && (scancode > 0xFF)))
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

		coniox_flush_cursor();
		r.h.ah = 0;
		coniox_int86(0x16, &r, &r);
		if ((r.h.al == 0) || ((r.h.al == 0xE0) && (r.h.ah != 0)))
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
	coniox_ready();
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
