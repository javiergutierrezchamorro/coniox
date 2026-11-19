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
static int coniox_cursor_shape_set = 0;	/* 0 until the first _setcursortype() actually programmed the shape; lets the unchanged-style short-circuit fire only after the cache reflects reality (init does not call _setcursortype, so the static _NORMALCURSOR default is not yet programmed) */
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

		/* Backward copy in 4-byte (2-cell) steps, mirroring the Watcom FLAT std;rep movsd
		   path; a 2-byte tail handles the odd-cell case. count is always a whole number
		   of cells (even bytes), so no 1-byte tail. Reading the high 2 cells before
		   writing them is overlap-safe for the right-shift moves movetext() needs. */
		if (count & 2)
		{
			*--d = *--s;
			count -= 2;
		}
		while (count >= 4)
		{
			d -= 2;
			s -= 2;
			*(unsigned long coniox_far *) d = *(const unsigned long coniox_far *) s;
			count -= 4;
		}
	}
	static coniox_inline void coniox_fmemsetw(void coniox_far *m, short val, size_t count)
	{
		short coniox_far *buf = (short coniox_far *) m;
		unsigned long wide;

		/* Replicate the 16-bit cell word into both halves of a dword so one store
		   fills two cells (DJGPP/DOS32: a 32-bit store; Borland DOS16: two 16-bit
		   stores with half the loop overhead). Mirrors the Watcom FLAT REP STOSD
		   path: a leading short reaches dword alignment when the cell offset is
		   odd, then bulk dword stores, then a trailing short for an odd count. */
		wide = (unsigned long) (unsigned short) val | ((unsigned long) (unsigned short) val << 16);
		if ((count != 0) && (((unsigned long) buf & 2) != 0))
		{
			*buf = val;
			buf++;
			count--;
		}
		while (count >= 2)
		{
			*(unsigned long coniox_far *) buf = wide;
			buf += 2;
			count -= 2;
		}
		if (count != 0)
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
	unsigned int count = piCount;
	unsigned long celldw;

	/* Replicate the attr byte into both halves of a dword: one store fills two cells.
	   This is a faithful C port of the Watcom FLAT coniox_putrun pragma aux (asm above):
	   same SWAR (s - 0x0E0E0E0E) & ~s & 0x80808080 test for any byte <= 0x0D, same dword
	   splat, same page-crossing guard, same 1-cell align prologue. On DJGPP (32-bit) each
	   store is one 32-bit write = 2 cells; on Borland DOS16 it is two 16-bit writes with a
	   4x-reduced scan. The Watcom FLAT build keeps its own pragma (REP STOSD/SWAR). */
	celldw = (unsigned long) cellword | ((unsigned long) cellword << 16);

	/* Align prologue: if dst is at an odd cell (2-byte-but-not-4-byte aligned), emit one
	   leading cell per-byte to reach a dword boundary. Mirrors 'test edi,2; jnz _prsingle'. */
	if (count && (((unsigned long) dst & 2u) != 0u))
	{
		unsigned char ch = (unsigned char) *src;
		if (ch == 0 || ch == '\r' || ch == '\n' || ch == '\b')
		{
			return n;
		}
		*dst++ = (unsigned short) (cellword | ch);
		src++;
		n++;
		count--;
	}

	/* Quad loop: 4 source bytes -> 4 cells (2 dwords) per pass. */
	while (count >= 4)
	{
		unsigned long s, hit;
#if (defined(__DJGPP__))
		/* Page-crossing guard (DJGPP DPMI, linear 4K pages): the source C string may end
		   mid-quad on an unmapped page. Borland DOS16 segment:offset wraps within the
		   segment, so the guard is DJGPP-only. Mirrors 'and eax,0FFFh; cmp 0FFCh; ja'. */
		if (((unsigned long) src & 0xFFFu) > (0x1000u - 4u))
		{
			goto perbyte;
		}
#endif
		s = *(const unsigned long coniox_far *) src;
		/* SWAR: any byte <= 0x0D ?  If so, drop to per-byte (the precise test there
		   distinguishes stop-chars {0,8,10,13} from writable {1..7,9,11,12}). */
		hit = (s - 0x0E0E0E0Eu) & (~s) & 0x80808080u;
		if (hit)
		{
			goto perbyte;
		}
		/* All 4 bytes > 13: splat into 2 dwords (4 cells). Each dword = (cellword|b_lo)
		   in the low half, (cellword|b_hi) in the high half. */
		{
			unsigned long lo, hi;
			lo = celldw | (s & 0xFFu) | ((s & 0xFF00u) << 8);
			hi = celldw | ((s >> 16) & 0xFFu) | (((s >> 16) & 0xFF00u) << 8);
			*(unsigned long coniox_far *) dst = lo;
			*(((unsigned long coniox_far *) dst) + 1) = hi;
		}
		dst += 4;
		src += 4;
		n += 4;
		count -= 4;
		continue;

	perbyte:
		/* Per-byte fallback for one quad (page-cross or a byte <= 0x0D candidate).
		   At most 4 iterations; preserves the exact stop-set {0,8,10,13}. */
		{
			unsigned int q;
			for (q = 0; q < 4 && count; q++)
			{
				unsigned char ch = (unsigned char) *src;
				if (ch == 0 || ch == '\r' || ch == '\n' || ch == '\b')
				{
					return n;
				}
				*dst++ = (unsigned short) (cellword | ch);
				src++;
				n++;
				count--;
			}
		}
	}

	/* Tail: 0..3 cells, per-byte. */
	while (count)
	{
		unsigned char ch = (unsigned char) *src;
		if (ch == 0 || ch == '\r' || ch == '\n' || ch == '\b')
		{
			break;
		}
		*dst++ = (unsigned short) (cellword | ch);
		src++;
		n++;
		count--;
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
	/* Use the cached window size (kept in sync by coniox_winsize() at every window/textmode/init/kbhit-resize)
	   instead of recomputing winright-winleft / winbottom-wintop on every call. */
	return ((unsigned int)(x - ti.winleft) < (unsigned int) coniox_winwidth) && ((unsigned int)(y - ti.wintop) < (unsigned int) coniox_winheight);
}


/* ----------------------------------------------------------------------------------------------------------------- */
/* PORTABLE FUNCTIONS */
/* ----------------------------------------------------------------------------------------------------------------- */
void clreol(void)
{
	int aw;


	coniox_ready();
	aw = coniox_winwidth - ti.curx + 1;	/* cells remaining to the end of the line */
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
	ti.attribute &= (unsigned short) ~0x08;
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
		if (ti.curx < winwidth)
		{
			coniox_celladd(coniox_cell, 1);
			ti.curx++;
			coniox_cursor_dirty = 1;	/* cursor advanced */
		}
		else
		{
			/* The cell cache stays on the written edge cell: one screen row minus the winwidth - 1 columns walked back, or just the walk back when scrolled. */
			wrapped = coniox_wrap();
			if (wrapped == 1)
			{
				coniox_celladd(coniox_cell, ti.screenwidth - winwidth + 1);
				coniox_cursor_dirty = 1;	/* scrolled: cursor at the new line start */
			}
			else if (wrapped == 2)
			{
				coniox_celladd(coniox_cell, 1 - winwidth);
				coniox_cursor_dirty = 1;	/* wrapped to the next line */
			}
			/* wrapped == 0 (_wscroll=0): the cursor stays on the edge cell, so the hardware
			   cursor (if synced) is already there and the dirty flag must not flip — otherwise
			   the next kbhit()/getch() flush would call gotoxy() for a position that did not move. */
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

	/* No conversion specifier: the format already is the output, so skip the formatting pass (and the PRINTFBUF_SIZE truncation). Mirrors cvprintf(). */
	if (wcschr(__format, L'%') == NULL)
	{
		return(cputws(__format));
	}

	buffer[0] = 0;
	va_start(ap, __format);
	vswprintf(buffer, PRINTFBUF_SIZE, __format, ap);
	va_end(ap);
	buffer[PRINTFBUF_SIZE - 1] = 0;	/* a truncated string is not always terminated */
	/* cputws() returns the chars it consumed: wcslen(buffer) without a second pass. */
	return(cputws(buffer));
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
static wchar_t coniox_oem2uni[256];	/* OEM byte -> Unicode, built once at init; replaces a per-key GetOEMCP()+MultiByteToWideChar pair */
static unsigned short coniox_last_console_attr = 0xFFFF;	/* last attribute passed to SetConsoleTextAttribute; 0xFFFF forces the first textattr() to sync */
static DWORD coniox_input_mode = 0;	/* console input mode cached at init; getch() toggles ENABLE_PROCESSED_INPUT off this value instead of a per-key GetConsoleMode() syscall */
static int coniox_input_ok = 0;		/* GetConsoleMode() succeeded at init (false => redirected stdin, getch() returns EOF) */


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
	coniox_last_console_attr = ti.attribute;	/* the console already has this attr: a matching first textattr() can skip the syscall */
	ti.currmode = (unsigned short) ((ti.screenwidth <= 40) ? C40 : C80);
	acp = GetOEMCP();
	if (GetConsoleOutputCP() != acp)
	{
		SetConsoleOutputCP(acp);
		SetConsoleCP(acp);
	}
	/* Build the 256-entry OEM byte -> Unicode table once: every getwch()/cgetws()/cwscanf()
	   would otherwise pay a GetOEMCP() + MultiByteToWideChar() kernel pair per key. The
	   OEM CP is single-byte, so one call maps all 256 bytes. */
	{
		unsigned char byte256[256];
		int k;
		for (k = 0; k < 256; k++)
		{
			byte256[k] = (unsigned char) k;
		}
		MultiByteToWideChar(acp, 0, (const char *) byte256, 256, coniox_oem2uni, 256);
	}
	if (GetConsoleMode(coniox_console_input, &m))
	{
		/* Cache the resting input mode once: getch() toggles ENABLE_PROCESSED_INPUT off
		   this cached value per key instead of re-reading it with a GetConsoleMode() call.
		   coniox_input_ok stays 0 for redirected stdin so getch() can return EOF. */
		coniox_input_mode = m | ENABLE_WINDOW_INPUT;
		coniox_input_ok = 1;
		SetConsoleMode(coniox_console_input, coniox_input_mode);
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
	int runlen, remaining;
	unsigned long attrcell;
	int winwidth;
	int baseX, baseY;
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
	/* Hoisted out of the loop: ti.attribute, the org+window origin and the run's attrcell
	   are constant for the whole call; only ti.curx/ti.cury advance. */
	attrcell = (unsigned long) ti.attribute << 16;
	baseX = coniox_orgx + ti.winleft - 2;
	baseY = coniox_orgy + ti.wintop - 2;

	while (*__str)
	{
		/* Read the byte once: three separate comparisons used to reload *__str three times. */
		ch = (unsigned char) *__str;
		/* Carriage return: back to start of line. WriteConsoleOutput neither reads nor moves the console cursor; the deferred sync paints it. */
		if (ch == '\r')
		{
			coniox_cr();
			++__str;
			continue;
		}

		/* Line feed: advance or scroll the window, keeping the column. */
		if (ch == '\n')
		{
			coniox_lf();
			++__str;
			continue;
		}

		/* Backspace: move back without erasing. */
		if (ch == '\b')
		{
			coniox_bs();
			++__str;
			continue;
		}

		/* Group printable characters up to the next control, the window edge or the batch size: one WriteConsoleOutputA per run. */
		run = __str;
		if (ti.curx > winwidth)
		{
			ti.curx = (unsigned short) winwidth;
		}
		remaining = winwidth - ti.curx + 1;
		if (remaining > CONIOX_MAX_CHARINFO)
		{
			remaining = CONIOX_MAX_CHARINFO;
		}
		runlen = 0;	/* the SWAR run-build below reads runlen before writing it; reset per run (cputws does the same) */
		/* SWAR run-build: scan 4 source bytes at a time and splat 4 CHAR_INFOs (16 bytes)
		   per quad via two 8-byte stores. Falls back to per-byte for the tail, for any quad
		   containing a byte <= 0x0D (the per-byte path distinguishes stop-chars {0,8,10,13}
		   from writable {1..7,9,11,12}), and when a 4-byte load would cross a 4K page (the
		   source C string may end mid-quad on an unmapped page). Mirrors the Watcom FLAT
		   coniox_putrun SWAR quad path. ci is 4-byte aligned; x86 tolerates the unaligned
		   8-byte store (precedent: coniox_putchxyattrwh fill). runlen is always even in the
		   quad path, so runlen/2 is the qword slot index. */
		{
			unsigned long long ab;

			ab = (unsigned long long) attrcell | ((unsigned long long) attrcell << 32);
			while (remaining - runlen >= 4)
			{
				unsigned long s, hit;
				/* Page-crossing guard: a 4-byte load must not straddle a 4K page. */
				if (((size_t) (run + runlen) & 0xFFFu) > (0x1000u - 4u))
				{
					goto perbyte_l1;
				}
				s = *(const unsigned long *) (run + runlen);
				/* SWAR: any byte <= 0x0D ?  (0x0E0E0E0E / 0x80808080 trick.) */
				hit = (s - 0x0E0E0E0Eu) & (~s) & 0x80808080u;
				if (hit)
				{
					goto perbyte_l1;
				}
				/* All 4 bytes > 13: splat into 2 qwords (4 CHAR_INFOs). lo = [cell(b0),cell(b1)], hi = [cell(b2),cell(b3)]. */
				{
					unsigned long long lo, hi;
					lo = ab | (unsigned long long) (s & 0xFFu) | ((unsigned long long) ((s >> 8) & 0xFFu) << 32);
					hi = ab | (unsigned long long) ((s >> 16) & 0xFFu) | ((unsigned long long) ((s >> 24) & 0xFFu) << 32);
					((unsigned long long *) ci)[runlen / 2] = lo;
					((unsigned long long *) ci)[runlen / 2 + 1] = hi;
				}
				runlen += 4;
				continue;

			perbyte_l1:
				/* Per-byte fallback for one quad (page-cross or a byte <= 0x0D candidate). */
				{
					unsigned int q;
					for (q = 0; q < 4 && runlen < remaining; q++)
					{
						ch = (unsigned char) run[runlen];
						if (ch > '\r')
						{
							((unsigned long *) ci)[runlen] = attrcell | ch;
							runlen++;
							continue;
						}
						if (ch == 0 || ch == '\r' || ch == '\n' || ch == '\b')
						{
							goto cputs_run_done;
						}
						((unsigned long *) ci)[runlen] = attrcell | ch;
						runlen++;
					}
				}
			}
			/* Tail: 0..3 cells, per-byte. */
			while (runlen < remaining)
			{
				ch = (unsigned char) run[runlen];
				if (ch > '\r')
				{
					((unsigned long *) ci)[runlen] = attrcell | ch;
					runlen++;
					continue;
				}
				if (ch == 0 || ch == '\r' || ch == '\n' || ch == '\b')
				{
					break;
				}
				((unsigned long *) ci)[runlen] = attrcell | ch;
				runlen++;
			}
		cputs_run_done: ;
		}

		if (runlen)
		{
			r.Left   = (short) (baseX + ti.curx);
			r.Top    = (short) (baseY + ti.cury);
			r.Right  = (short) (r.Left + runlen - 1);
			r.Bottom = r.Top;
			s.X = (short) runlen;
			s.Y = 1;
			WriteConsoleOutputA(coniox_console_output, ci, s, c, &r);
			ti.curx = (unsigned short)(ti.curx + runlen);
		}

		/* A run never crosses the window edge: the wrap is decided once. */
		if (ti.curx > winwidth)
		{
			ti.curx = (unsigned short) winwidth;
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
	int runlen, remaining;
	unsigned long attrcell;
	int winwidth;
	int baseX, baseY;
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
	/* Hoisted out of the loop: attrcell and the org+window origin are constant for the whole call. */
	attrcell = (unsigned long) ti.attribute << 16;
	baseX = coniox_orgx + ti.winleft - 2;
	baseY = coniox_orgy + ti.wintop - 2;

	while (*__str)
	{
		/* Read the wide char once: three separate comparisons used to reload *__str three times. */
		ch = (unsigned short) *__str;
		/* Carriage return: back to start of line (deferred sync). */
		if (ch == L'\r')
		{
			coniox_cr();
			++__str;
			continue;
		}

		/* Line feed: advance or scroll the window, keeping the column. */
		if (ch == L'\n')
		{
			coniox_lf();
			++__str;
			continue;
		}

		/* Backspace: move back without erasing. */
		if (ch == L'\b')
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
			ti.curx = (unsigned short) winwidth;
		}
		remaining = winwidth - ti.curx + 1;
		if (remaining > CONIOX_MAX_CHARINFO)
		{
			remaining = CONIOX_MAX_CHARINFO;
		}

		/* SWAR run-build (Unicode twin of cputs): scan 4 wchars (8 bytes) at a time and splat
		   4 CHAR_INFOs (16 bytes) per quad via two 8-byte stores. Falls back to per-wchar for
		   the tail, for any quad containing a wchar whose high byte is 0 AND low byte <= 0x0D
		   (the per-wchar path distinguishes stop-chars {0,8,10,13} from writable {1..7,9,11,12}
		   and from fast wchars > 13), and when an 8-byte load would cross a 4K page. The
		   two-test (high==0 AND low<=13) is required because ASCII wchars carry a 0 high byte:
		   a single all-bytes-<=13 test would trip on every ASCII quad and never engage the
		   fast path. ci is 4-byte aligned; x86 tolerates the unaligned 8-byte store. runlen is
		   always even in the quad path, so runlen/2 is the qword slot index. */
		{
			unsigned long long ab;

			ab = (unsigned long long) attrcell | ((unsigned long long) attrcell << 32);
			while (remaining - runlen >= 4)
			{
				unsigned long long q;
				unsigned long h, l, h_zero, l_low13, cand;
				const wchar_t *wp = run + runlen;

				/* Page-crossing guard: an 8-byte load must not straddle a 4K page. */
				if (((size_t) wp & 0xFFFu) > (0x1000u - 8u))
				{
					goto perbyte_l2;
				}
				q = *(const unsigned long long *) wp;
				/* Pack the 4 high bytes (qword offsets 1,3,5,7) and 4 low bytes (0,2,4,6). */
				l = (unsigned long) (q        & 0xFFu)
				  | (unsigned long) ((q >> 16) & 0xFFu) << 8
				  | (unsigned long) ((q >> 32) & 0xFFu) << 16
				  | (unsigned long) ((q >> 48) & 0xFFu) << 24;
				h = (unsigned long) ((q >>  8) & 0xFFu)
				  | (unsigned long) ((q >> 24) & 0xFFu) << 8
				  | (unsigned long) ((q >> 40) & 0xFFu) << 16
				  | (unsigned long) ((q >> 56) & 0xFFu) << 24;
				/* Per-lane candidate test: high byte == 0 AND low byte <= 0x0D. */
				h_zero  = (h - 0x01010101u) & (~h) & 0x80808080u;
				l_low13 = (l - 0x0E0E0E0Eu) & (~l) & 0x80808080u;
				cand = h_zero & l_low13;
				if (cand)
				{
					goto perbyte_l2;
				}
				/* All 4 wchars fast (> 13): splat into 2 qwords (4 CHAR_INFOs). lo = [cell(w0),cell(w1)], hi = [cell(w2),cell(w3)]. */
				{
					unsigned long long lo, hi;
					lo = ab | (unsigned long long) (q & 0xFFFFu) | ((unsigned long long) ((q >> 16) & 0xFFFFu) << 32);
					hi = ab | (unsigned long long) ((q >> 32) & 0xFFFFu) | ((unsigned long long) ((q >> 48) & 0xFFFFu) << 32);
					((unsigned long long *) ci)[runlen / 2]     = lo;
					((unsigned long long *) ci)[runlen / 2 + 1] = hi;
				}
				runlen += 4;
				continue;

			perbyte_l2:
				/* Per-wchar fallback for one quad (page-cross or a candidate wchar). */
				{
					unsigned int q4;
					for (q4 = 0; q4 < 4 && runlen < remaining; q4++)
					{
						ch = (unsigned short) run[runlen];
						if (ch > L'\r')
						{
							((unsigned long *) ci)[runlen] = attrcell | ch;
							runlen++;
							continue;
						}
						if (ch == 0 || ch == L'\r' || ch == L'\n' || ch == L'\b')
						{
							goto cputws_run_done;
						}
						((unsigned long *) ci)[runlen] = attrcell | ch;
						runlen++;
					}
				}
			}
			/* Tail: 0..3 cells, per-wchar. */
			while (runlen < remaining)
			{
				ch = (unsigned short) run[runlen];
				if (ch > L'\r')
				{
					((unsigned long *) ci)[runlen] = attrcell | ch;
					runlen++;
					continue;
				}
				if (ch == 0 || ch == L'\r' || ch == L'\n' || ch == L'\b')
				{
					break;
				}
				((unsigned long *) ci)[runlen] = attrcell | ch;
				runlen++;
			}
		cputws_run_done: ;
		}

		if (runlen)
		{
			r.Left   = (short) (baseX + ti.curx);
			r.Top    = (short) (baseY + ti.cury);
			r.Right  = (short) (r.Left + runlen - 1);
			r.Bottom = r.Top;
			s.X = (short) runlen;
			s.Y = 1;
			WriteConsoleOutputW(coniox_console_output, ci, s, c, &r);
			ti.curx = (unsigned short)(ti.curx + runlen);
		}

		/* A run never crosses the window edge: the wrap is decided once. */
		if (ti.curx > winwidth)
		{
			ti.curx = (unsigned short) winwidth;
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

	/* Fill two CHAR_INFO cells per store (8 bytes): a replicated DWORD in an
	   unsigned long long. On Win64 this is one 64-bit store; on Win32 it is two
	   32-bit stores with half the loop overhead. CHAR_INFO is 4 bytes so the
	   array is at least 4-byte aligned; x86 tolerates the unaligned 8-byte store. */
	cell = ((unsigned long) (WORD) attr << 16) | (unsigned char) ch;
	{
		unsigned long long cellpair;
		int pairs;

		cellpair = (unsigned long long) cell | ((unsigned long long) cell << 32);
		pairs = total >> 1;
		for (i = 0; i < pairs; i++)
		{
			((unsigned long long *) ci)[i] = cellpair;
		}
		if (total & 1)
		{
			((unsigned long *) ci)[total - 1] = cell;
		}
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
	/* One table load instead of GetOEMCP() + MultiByteToWideChar() per key. The table is
	   built in coniox_init() for the active OEM code page. */
	return(coniox_oem2uni[(unsigned char) ch]);
}


/* ----------------------------------------------------------------------------------------------------------------- */
int getch(void)
{
	INPUT_RECORD ir;
	DWORD read;
	int ch = EOF;

	coniox_ready();	/* getch() may be the first conio call: ensure handles */
	if (getch_last_extended_key)
	{
		ch = getch_last_extended_key;
		getch_last_extended_key = 0;
		return(ch);
	}
	if (!coniox_input_ok)
	{
		return(EOF);	/* redirected input (GetConsoleMode failed at init) */
	}
	SetConsoleMode(coniox_console_input, coniox_input_mode & ~(DWORD)ENABLE_PROCESSED_INPUT);	/* Ctrl+C is a key, not a signal */
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
	SetConsoleMode(coniox_console_input, coniox_input_mode);	/* restore the cached resting mode */
	return(ch);
}


/* ----------------------------------------------------------------------------------------------------------------- */
void gotoxy(int __x, int __y)
{
	COORD c;

	coniox_ready();
	/* Short-circuit: if the logical position is already (__x,__y) and the hardware cursor is
	   synced (not dirty), SetConsoleCursorPosition would be a no-op. Mirrors the DOS gotoxy()
	   BDA-peek short-circuit (peekw(0x40,0x50) == target). Saves the syscall on the
	   flush_cursor-driven gotoxy(ti.curx,ti.cury) calls fired by kbhit()/getch()/delay(). */
	if ((__x == ti.curx) && (__y == ti.cury) && (!coniox_cursor_dirty))
	{
		return;
	}
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
	coniox_putchxyattrwh(ti.winleft, ti.wintop, ' ', ti.attribute, coniox_winwidth, coniox_winheight);
}


/* ----------------------------------------------------------------------------------------------------------------- */
void textattr(int __newattr)
{
	unsigned short na;

	coniox_ready();
	na = (unsigned short) (__newattr & 0xFF);
	ti.attribute = na;
	/* No coniox output path reads the console's current attribute (every cell carries its
	   own via CHAR_INFO), so the kernel call here only matters for stdio printf sync.
	   Skip it when the attribute is unchanged — color setup is called very frequently. */
	if (na != coniox_last_console_attr)
	{
		coniox_last_console_attr = na;
		SetConsoleTextAttribute(coniox_console_output, na);
	}
}


/* ----------------------------------------------------------------------------------------------------------------- */
/* Scroll the current window row up (delline: delete the current line) or down (insline: open a blank line).
   The two public functions differ only in which rows are the scroll source and destination, so they share
   the validation, the last-line clear fallback, the CHAR_INFO fill and the ScrollConsoleScreenBuffer call. */
static void coniox_scroll_line(int ins)
{
	COORD c;
	SMALL_RECT r;
	CHAR_INFO ci;
	int baseX, baseY;

	coniox_ready();
	/* Last window line (or a one-line window): the scroll rectangle is empty and ScrollConsoleScreenBuffer would fail, so clear the line instead. */
	if (ti.cury >= coniox_winheight)
	{
		coniox_putchxyattrwh(ti.winleft, ti.wintop + ti.cury - 1, ' ', ti.attribute, coniox_winwidth, 1);
		return;
	}
	/* Hoisted once: the org+window origin is constant for the call, only ti.cury selects the row. */
	baseX = coniox_orgx + ti.winleft - 1;
	baseY = coniox_orgy + ti.wintop - 1;
	c.X = (short) baseX;
	r.Left = c.X;
	r.Right = (short) (baseX + coniox_winwidth - 1);
	if (ins)
	{
		/* Insert: scroll cury..winbottom-2 down by one; the current row becomes the blank. */
		c.Y = (short) (baseY + ti.cury);
		r.Top = (short) (baseY + ti.cury - 1);
		r.Bottom = (short) (baseY + coniox_winheight - 2);
	}
	else
	{
		/* Delete: scroll cury+1..winbottom-1 up into the current row. */
		c.Y = (short) (baseY + ti.cury - 1);
		r.Top = (short) (baseY + ti.cury);
		r.Bottom = (short) (baseY + coniox_winheight - 1);
	}
	ci.Attributes = ti.attribute;
	/* ScrollConsoleScreenBuffer has no ANSI variant: it always reads Char.UnicodeChar, so write the whole WCHAR, not just its low byte. */
	ci.Char.UnicodeChar = L' ';
	ScrollConsoleScreenBuffer(coniox_console_output, &r, NULL, c, &ci);
	/* ScrollConsoleScreenBuffer does not move the console cursor; the sync happens via coniox_flush_cursor(). */
}


/* ----------------------------------------------------------------------------------------------------------------- */
void delline(void)
{
	coniox_scroll_line(0);
}


/* ----------------------------------------------------------------------------------------------------------------- */
void insline(void)
{
	coniox_scroll_line(1);
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
	/* Four cells per pass: two 8-byte loads from ci (4 CHAR_INFO) and one 8-byte store to
	   buf (4 packed cells). Halves the loop iterations and the store count vs the 2-cell
	   pass; on Win64 the 8-byte store is native. Each CHAR_INFO has char in bits 0-7,
	   attribute in bits 16-23; the cell word is char | (attr << 8). x86 tolerates the
	   unaligned 8-byte load / 8-byte store. Mirrors the cputs()/cputws() SWAR splat. */
	{
		int quads;

		quads = total >> 2;
		for (i = 0; ok && i < quads; i++)
		{
			unsigned long long q0 = ((unsigned long long *) ci)[i * 2];
			unsigned long long q1 = ((unsigned long long *) ci)[i * 2 + 1];
			unsigned long v0 = (unsigned long) q0;
			unsigned long v1 = (unsigned long) (q0 >> 32);
			unsigned long v2 = (unsigned long) q1;
			unsigned long v3 = (unsigned long) (q1 >> 32);
			unsigned long c0 = (v0 & 0xFF) | ((v0 >> 8) & 0xFF00);
			unsigned long c1 = (v1 & 0xFF) | ((v1 >> 8) & 0xFF00);
			unsigned long c2 = (v2 & 0xFF) | ((v2 >> 8) & 0xFF00);
			unsigned long c3 = (v3 & 0xFF) | ((v3 >> 8) & 0xFF00);
			((unsigned long long *) buf)[i] = (unsigned long long) c0
			                                | ((unsigned long long) c1 << 16)
			                                | ((unsigned long long) c2 << 32)
			                                | ((unsigned long long) c3 << 48);
		}
		/* 2-cell tail. */
		if (ok && (total & 2))
		{
			unsigned long long q = ((unsigned long long *) ci)[quads * 2];
			unsigned long v0 = (unsigned long) q;
			unsigned long v1 = (unsigned long) (q >> 32);
			unsigned long c0 = (v0 & 0xFF) | ((v0 >> 8) & 0xFF00);
			unsigned long c1 = (v1 & 0xFF) | ((v1 >> 8) & 0xFF00);
			((unsigned long *) buf)[quads * 2] = c0 | (c1 << 16);
		}
		/* 1-cell tail. */
		if (ok && (total & 1))
		{
			unsigned long v = ((unsigned long *) ci)[total - 1];
			buf[total - 1] = (unsigned short) ((v & 0xFF) | ((v >> 8) & 0xFF00));
		}
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
	/* Four cells per pass: one 8-byte load from the source (4 ushorts) and two 8-byte stores
	   to buffer (4 CHAR_INFO). Halves the loop iterations and the load count vs the 2-cell
	   pass; on Win64 the 8-byte load is native. Each cell word is char | (attr << 8); the
	   CHAR_INFO form is char | padding | (attr << 16). x86 tolerates the unaligned 8-byte
	   load / 8-byte store. Mirrors the cputs()/cputws() SWAR splat. */
	{
		int quads;

		quads = total >> 2;
		for (i = 0; i < quads; i++)
		{
			unsigned long long s = ((unsigned long long *) ci)[i];
			unsigned long s0 = (unsigned long) s;
			unsigned long s1 = (unsigned long) (s >> 32);
			unsigned long long out0, out1;
			out0 = (unsigned long long) ((s0 & 0xFF) | (((s0 >> 8) & 0xFF) << 16))
			     | ((unsigned long long) (((s0 >> 16) & 0xFF) | (((s0 >> 24) & 0xFF) << 16)) << 32);
			out1 = (unsigned long long) ((s1 & 0xFF) | (((s1 >> 8) & 0xFF) << 16))
			     | ((unsigned long long) (((s1 >> 16) & 0xFF) | (((s1 >> 24) & 0xFF) << 16)) << 32);
			((unsigned long long *) buffer)[i * 2]     = out0;
			((unsigned long long *) buffer)[i * 2 + 1] = out1;
		}
		/* 2-cell tail. */
		if (total & 2)
		{
			unsigned long spair = ((unsigned long *) ci)[quads * 2];
			unsigned long long out;
			out = (unsigned long long) ((spair & 0xFF) | (((spair >> 8) & 0xFF) << 16))
			    | ((unsigned long long) (((spair >> 16) & 0xFF) | (((spair >> 24) & 0xFF) << 16)) << 32);
			((unsigned long long *) buffer)[quads * 2] = out;
		}
		/* 1-cell tail. */
		if (total & 1)
		{
			unsigned short cell = ci[total - 1];
			((unsigned long *) buffer)[total - 1] = (unsigned long) (cell & 0xFF) | ((unsigned long) (cell >> 8) << 16);
		}
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

/* Shared validate + rect/size setup for putwtext/getwtext: wchar_info and CHAR_INFO share
   the WCHAR+WORD (4-byte) layout, so the console reads/writes the caller buffer directly —
   no conversion pass, no scratch copy. The two public functions differ only in direction. */
static int coniox_wtext_io(int left, int top, int right, int bottom, void *buf, int is_write)
{
	SMALL_RECT r;
	COORD s, c = { 0, 0 };

	coniox_ready();
	/* Borland-style validation: rectangle inside the absolute screen. */
	if (buf == NULL || left < 1 || top < 1 || right > ti.screenwidth || bottom > ti.screenheight || right < left || bottom < top)
	{
		return(0);
	}
	r.Left = (short) (coniox_orgx + left - 1);
	r.Top = (short) (coniox_orgy + top - 1);
	r.Right = (short) (coniox_orgx + right - 1);
	r.Bottom = (short) (coniox_orgy + bottom - 1);
	s.X = (short) (right - left + 1);
	s.Y = (short) (bottom - top + 1);
	if (is_write)
	{
		return(WriteConsoleOutputW(coniox_console_output, (const CHAR_INFO *) buf, s, c, &r) ? 1 : 0);
	}
	return(ReadConsoleOutputW(coniox_console_output, (CHAR_INFO *) buf, s, c, &r) ? 1 : 0);
}
/* ----------------------------------------------------------------------------------------------------------------- */
int putwtext(int __left, int __top, int __right, int __bottom, const wchar_info *__source)
{
	return(coniox_wtext_io(__left, __top, __right, __bottom, (void *) __source, 1));
}
/* ----------------------------------------------------------------------------------------------------------------- */
int getwtext(int __left, int __top, int __right, int __bottom, wchar_info *__destin)
{
	return(coniox_wtext_io(__left, __top, __right, __bottom, (void *) __destin, 0));
}
/* ----------------------------------------------------------------------------------------------------------------- */
void textmode(int __newmode)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	coniox_ready();
	/* A mode switch reprograms the console cursor shape to the new buffer's default, so the
	   coniox_cursor_shape_set cache no longer reflects reality: force the _setcursortype() below
	   (and any later same-style call) to actually reprogram the shape. */
	coniox_cursor_shape_set = 0;

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
	/* Short-circuit when the style is unchanged AND the shape has been programmed at least
	   once (coniox_setcursortype defaults to _NORMALCURSOR before any call, so the first call
	   must run to actually set the console's cursor shape). Saves the SetConsoleCursorInfo
	   syscall on redundant calls (textmode() re-init, defensive "ensure cursor visible"). */
	if (coniox_cursor_shape_set && (__cur_t == coniox_setcursortype))
	{
		return;
	}
	coniox_setcursortype = __cur_t;
	coniox_cursor_shape_set = 1;
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
/* Shared key-event push for ungetch/ungetwch: builds a down+up INPUT_RECORD pair and
   writes it to the console input queue. The two public functions differ only in the
   union char field (AsciiChar vs UnicodeChar) and the A/W write call. */
static int coniox_ungetk(int ch, int is_wide)
{
	INPUT_RECORD ir[2];
	DWORD written;
	BOOL ok;

	coniox_ready();
	ir[0].EventType = KEY_EVENT;
	ir[0].Event.KeyEvent.bKeyDown = TRUE;
	ir[0].Event.KeyEvent.wRepeatCount = 1;
	ir[0].Event.KeyEvent.wVirtualKeyCode = 0;
	ir[0].Event.KeyEvent.wVirtualScanCode = 0;
	if (is_wide)
	{
		ir[0].Event.KeyEvent.uChar.UnicodeChar = (wchar_t) ch;
	}
	else
	{
		ir[0].Event.KeyEvent.uChar.AsciiChar = (CHAR) ch;
	}
	ir[0].Event.KeyEvent.dwControlKeyState = 0;
	ir[1] = ir[0];
	ir[1].Event.KeyEvent.bKeyDown = FALSE;
	ok = is_wide ? WriteConsoleInputW(coniox_console_input, ir, 2, &written)
	             : WriteConsoleInputA(coniox_console_input, ir, 2, &written);
	return((ok && written == 2) ? ch : EOF);
}
/* ----------------------------------------------------------------------------------------------------------------- */
int ungetch(int __ch)
{
	return(coniox_ungetk(__ch, 0));
}


/* ----------------------------------------------------------------------------------------------------------------- */
wchar_t ungetwch(wchar_t __ch)
{
	return((wchar_t) coniox_ungetk(__ch, 1));
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
static unsigned short coniox_kb_start = 0;	/* BIOS keyboard buffer start (BDA 0040:0080), cached at init; getch()/ungetch() read this instead of two far peeks per key */
static unsigned short coniox_kb_end = 0;	/* BIOS keyboard buffer end   (BDA 0040:0082), cached at init */

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
	/* Cache the BIOS keyboard buffer bounds once: getch()/ungetch() used to peek
	   0040:0080/0082 every call. Fixed at POST; the start==0 fallback covers the
	   same emulators the per-call guard did. */
	coniox_kb_start = peekw(0x40, 0x80);
	coniox_kb_end = peekw(0x40, 0x82);
	if (coniox_kb_start == 0 || coniox_kb_end <= coniox_kb_start)
	{
		coniox_kb_start = 0x1E;
		coniox_kb_end = 0x3E;
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
		/* Read the byte once: three separate comparisons used to reload *__str three times. */
		unsigned char c = (unsigned char) *__str;

		/* Carriage return: back to start of line. */
		if (c == '\r')
		{
			coniox_currentoffset -= coniox_cr();
			++__str;
			continue;
		}

		/* Line feed: advance or scroll the window. A scroll moves neither the cursor nor the cell cache: coniox_currentoffset still matches (curx, cury). */
		if (c == '\n')
		{
			if (coniox_lf())
			{
				coniox_currentoffset += ti.screenwidth;
			}
			++__str;
			continue;
		}

		/* Backspace: move back without erasing. */
		if (c == '\b')
		{
			coniox_currentoffset -= coniox_bs();
			++__str;
			continue;
		}

		run = __str;
		if (ti.curx > winwidth)
		{
			/* Re-sync the offset with the clamp so the run paints inside the window. */
			ti.curx = (unsigned short) winwidth;
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

			/* Bounded scan, like cputws(): strcspn would walk the whole remaining string while a run never passes the window edge. Read each byte once (it was reloaded up to 4x per char). */
			runlen = 0;
			while (runlen < remaining)
			{
				unsigned char rc = (unsigned char) run[runlen];
				if (rc > '\r' || (rc != 0 && rc != '\r' && rc != '\n' && rc != '\b'))
				{
					runlen++;
				}
				else
				{
					break;
				}
			}
			for (i = 0; i < runlen; i++)
			{
				coniox_putchxyattr(ti.winleft + ti.curx - 1 + (int) i, ti.wintop + ti.cury - 1, run[i], ti.attribute);
			}
		}
		coniox_currentoffset += runlen;
		ti.curx = (unsigned short)(ti.curx + runlen);

		/* A run never crosses the window edge: the wrap is decided once. The run has stepped one cell past the edge, so go back to the last column first and apply the same cell cache moves as putch(). */
		if (ti.curx > winwidth)
		{
			/* The run stepped one cell past the edge, so coniox_currentoffset is at (winwidth+1, row):
			   fold that into the wrap deltas (one fewer op than backing up to the edge then applying
			   putch()'s at-edge deltas). wrap==1: past-edge → col 1 next row = +(sw - winwidth).
			   wrap==2: past-edge → col 1 bottom row (scrolled) = -winwidth. */
			ti.curx = (unsigned short) winwidth;
			wrapped = coniox_wrap();
			if (wrapped == 1)
			{
				coniox_currentoffset += ti.screenwidth - winwidth;
			}
			else if (wrapped == 2)
			{
				coniox_currentoffset -= winwidth;
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
			coniox_w(r).cx = 1;
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
		int sw;

		v = (unsigned short)((attr << 8) | (ch & 0xFF));
		sw = ti.screenwidth;
		p = coniox_offset(x - 1, y - 1);
		if (w == sw)
		{
			/* Full-screen-width block: VRAM rows are contiguous, one fill covers all rows. */
			coniox_fmemsetw(p, v, (size_t) w * (size_t) h);
			return;
		}
		for (y1 = 0; y1 < h; y1++)
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
				coniox_w(r).cx = (unsigned short)w;
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
		coniox_w(r).ax = 0x1680;
	coniox_int86(0x2F, &r, &r);


	/*
		Under DESQview, if the process issuing this call has hooked INT 08h,
		the current time-slice is set to expire at the next clock tick rather
		than immediately
	*/
		coniox_w(r).ax = 0x1000;
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
			coniox_w(r).ax = 0x1010;	/* Set DAC register */
			coniox_w(r).bx = coniox_dac[i];	/* BX = DAC register */
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
			coniox_w(r).ax = 0x1003;
			coniox_w(r).bx = blink ? 1 : 0;
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
				lNow = ((unsigned long) coniox_w(r).cx << 16) | coniox_w(r).dx;
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
	/* A BIOS mode switch (INT 10h AH=00h) reprograms the CRTC cursor shape to the new mode's
	   default, so the coniox_cursor_shape_set cache no longer reflects reality: force the
	   _setcursortype() below (and any later same-style call) to actually reprogram the shape. */
	coniox_cursor_shape_set = 0;
	if (__newmode == LASTMODE)
	{
		__newmode = ti.currmode;
	}
	/* INT 10h only knows the BIOS text modes (BW40, C40, BW80, C80, MONO) plus the emulated C4350: anything else would select a graphics mode. */
	if (__newmode != ti.currmode && (__newmode == C4350 || __newmode == MONO || (unsigned int) __newmode <= C80))
	{
		if (__newmode == C4350)
		{
				coniox_w(r).ax = C80;
			coniox_int86(0x10, &r, &r);
			/* For 4350 additionally we need to load 8x8 EGA/VGA font */
				coniox_w(r).ax = 0x1112;
				coniox_w(r).bx = 0;
			coniox_int86(0x10, &r, &r);
			/* For 4350 select alternate print screen routine */
				coniox_w(r).ax = 0x1200;
			r.h.bl = 0x20;
			coniox_int86(0x10, &r, &r);
			pokeb(0x40, 0x87, peekb(0x40, 0x87) | 1);	/* Disable cursor size emulation */
			pokew(0x40, 0x4C, 8000);	/* Correct video buffer size */
		}
		else
		{
				coniox_w(r).ax = __newmode;
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
	/* Short-circuit when the style is unchanged AND the shape has been programmed at least
	   once (coniox_setcursortype defaults to _NORMALCURSOR before any call). Saves the
	   peekb(0x40,0x85) + 2 CRTC OUT writes (directvideo) or the INT 10h AH=01h call (BIOS) on
	   redundant same-style calls — the CRTC OUTs are the slowest thing _setcursortype() does
	   (~1 us each on real VGA, uncached on emulators). The cursor-position resync below still
	   runs, matching the original always-resync behavior (no observable change). */
	if (coniox_cursor_shape_set && (__cur_t == coniox_setcursortype))
	{
		/* Style unchanged: skip the shape reprogramming, but still resync the cursor position —
		   the original _setcursortype() always did this (it recovers a stale cursor address after
		   a _NOCURSOR period and re-syncs a lagging cursor when dirty=1; cheap BDA peek + the
		   existing BDA-peek short-circuit when the cursor is already synced). */
		if (__cur_t != _NOCURSOR)
		{
			gotoxy(ti.curx, ti.cury);
		}
		return;
	}
	coniox_setcursortype = __cur_t;
	coniox_cursor_shape_set = 1;
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


/* Shared INT 10h AH=06h/AH=07h scroll setup for the DOS clrscr/delline/insline BIOS
   branches: only the AX value (0601h up / 0701h down / 0600h clear) and the top row differ. */
static void coniox_bios_scroll(int ax, int top)
{
	union REGS r;
	coniox_w(r).ax = (unsigned short) ax;
	r.h.bh = ti.attribute;
	r.h.cl = ti.winleft - 1;
	r.h.ch = (unsigned char) top;
	r.h.dl = ti.winright - 1;
	r.h.dh = ti.winbottom - 1;
	coniox_int86(0x10, &r, &r);
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
		width = coniox_winwidth;
		rowbytes = (size_t) width * sizeof(unsigned short);

		srcrow = coniox_offset(ti.winleft - 1, yfirst - 1);
		dstrow = srcrow - sw;
		if ((width == sw) && (yfirst <= ylast))
		{
			/* Full window width: VRAM rows contiguous and copying upwards (dst < src), so the whole block is one forward memcpy. */
			size_t blockbytes = rowbytes * (size_t)(ylast - yfirst + 1);
			unsigned short coniox_far *blank;
			coniox_fmemcpy(dstrow, srcrow, blockbytes);
			/* The blank-fill row is exactly blockbytes past dstrow (the row just past the moved
			   block): reuse the byte count already computed for the copy instead of recomputing
			   coniox_offset(winleft-1, winbottom-1), which would re-multiply screenwidth*(winbottom-1). */
			blank = dstrow + (blockbytes / sizeof(unsigned short));
			coniox_fmemsetw(blank, (short)(unsigned short)((ti.attribute << 8) | ' '), (size_t) width);
		}
		else
		{
			for (y = yfirst; y <= ylast; y++)
			{
				coniox_fmemcpy(dstrow, srcrow, rowbytes);
				srcrow += sw;
				dstrow += sw;
			}
			/* dstrow now points at the last window row (the blank-fill target), saving a coniox_offset() multiply. */
			coniox_fmemsetw(dstrow, (short)(unsigned short)((ti.attribute << 8) | ' '), (size_t) width);
		}
	}
	else
	{
		coniox_bios_scroll(0x0601, ti.cury + ti.wintop - 2);	/* Scroll up */
	}
}


/* ----------------------------------------------------------------------------------------------------------------- */
void insline(void)
{
	coniox_ready();
	/* Seems to be faster using BIOS under emulators */
	if ((directvideo) && (!coniox_is_emulator))
	{
		size_t rowbytes;
		int sw, ytop, width;

		sw = ti.screenwidth;
		ytop = ti.cury + ti.wintop - 1;
		width = coniox_winwidth;
		rowbytes = (size_t) width * sizeof(unsigned short);

		if ((width == sw) && (ytop < ti.winbottom))
		{
			/* Full window width: VRAM rows contiguous. The block moves down (dst = src + sw), so
			   copy backward to read each row before it is overwritten. The freed top row (the
			   cursor's row) is exactly srcblk, saving a coniox_offset() multiply — same pattern as
			   delline()'s full-width branch. ytop < winbottom guarantees >= 1 row to copy. */
			size_t blockbytes = rowbytes * (size_t)(ti.winbottom - ytop);
			unsigned short coniox_far *srcblk = coniox_offset(ti.winleft - 1, ytop - 1);
			coniox_fmemcpyr(srcblk + sw, srcblk, blockbytes);
			coniox_fmemsetw(srcblk, (short)(unsigned short)((ti.attribute << 8) | ' '), (size_t) width);
		}
		else
		{
			int y;
			unsigned short coniox_far *srcrow;
			unsigned short coniox_far *dstrow;

			srcrow = coniox_offset(ti.winleft - 1, ti.winbottom - 2);
			dstrow = srcrow + sw;
			for (y = ti.winbottom - 1; y >= ytop; y--)
			{
				coniox_fmemcpy(dstrow, srcrow, rowbytes);
				srcrow -= sw;
				dstrow -= sw;
			}
			/* dstrow now points at the freed top row (the blank-fill target), saving a coniox_offset() multiply. */
			coniox_fmemsetw(dstrow, (short)(unsigned short)((ti.attribute << 8) | ' '), (size_t) width);
		}
	}
	else
	{
		coniox_bios_scroll(0x0701, ti.cury + ti.wintop - 2);	/* Scroll down */
	}
}


/* ----------------------------------------------------------------------------------------------------------------- */
/* Shared geometry for gettext/puttext: validate the rectangle inside the screen and compute the VRAM cell,
   the real VRAM stride and the row/total byte counts. The two public functions differ only in copy direction. */
static int coniox_rect_geom(int left, int top, int right, int bottom, unsigned short coniox_far **pp,
                            unsigned int *pwidth, unsigned int *psw, int *prows, size_t *prow_bytes, size_t *ptotal_bytes)
{
	coniox_ready();
	if (left < 1 || top < 1 || right < left || bottom < top || right > ti.screenwidth || bottom > ti.screenheight)
	{
		return 0;
	}
	*pwidth = (unsigned int)(right - left + 1);
	*prows = bottom - top + 1;
	*prow_bytes = (size_t)(*pwidth) * sizeof(unsigned short);
	*ptotal_bytes = (*prow_bytes) * (size_t)(*prows);
	*pp = coniox_offset(left - 1, top - 1);
	*psw = ti.screenwidth;
	return 1;
}


/* ----------------------------------------------------------------------------------------------------------------- */
int gettext(int __left, int __top, int __right, int __bottom, void coniox_far *__destin)
{
	unsigned short coniox_far *p;
	unsigned short coniox_far *dst;
	unsigned int width, sw;
	int rows, y;
	size_t row_bytes, total_bytes;

	if (__destin == NULL || !coniox_rect_geom(__left, __top, __right, __bottom, &p, &width, &sw, &rows, &row_bytes, &total_bytes))
	{
		return 0;
	}
	dst = (unsigned short coniox_far *)__destin;
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
	unsigned int width, sw;
	int rows, y;
	size_t row_bytes, total_bytes;

	if (__source == NULL || !coniox_rect_geom(__left, __top, __right, __bottom, &p, &width, &sw, &rows, &row_bytes, &total_bytes))
	{
		return 0;
	}
	src = (unsigned short coniox_far *)__source;
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
	size_t rowbytes, blockbytes;

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
	/* Hoisted out of the per-row loops: the compiler could not prove w loop-invariant through the coniox_fmemcpy/fmemcpyr call. */
	rowbytes = (size_t) w * sizeof(unsigned short);
	blockbytes = rowbytes * (size_t) h;

	if (dy == 0 && dx > 0 && dx < w)
	{
		/* Partial overlap within one row (0 < dx < w): each row has to be copied backwards. */
		for (row = 0; row < h; row++)
		{
			coniox_fmemcpyr(dst, src, rowbytes);
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
			coniox_fmemcpyr(dst, src, blockbytes);
		}
		else
		{
			/* Copy bottom-up, like insline(), to read each source row before it is overwritten. A row never overlaps its own copy, so whatever dx is, each row can be moved forward. */
			src += (h - 1) * sw;
			dst += (h - 1) * sw;
			for (row = 0; row < h; row++)
			{
				coniox_fmemcpy(dst, src, rowbytes);
				src -= sw;
				dst -= sw;
			}
		}
	}
	else if (w == sw)
	{
		/* Full width: contiguous rows, single block, copying forward and top-down. */
		coniox_fmemcpy(dst, src, blockbytes);
	}
	else
	{
		/* All remaining cases: copying forward and top-down, reads always stay ahead of writes. */
		for (row = 0; row < h; row++)
		{
			coniox_fmemcpy(dst, src, rowbytes);
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
		coniox_putchxyattrwh(ti.winleft, ti.wintop, ' ', ti.attribute, coniox_winwidth, coniox_winheight);
	}
	else
	{
		coniox_bios_scroll(0x0600, ti.wintop - 1);
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
		start = coniox_kb_start;
		end   = coniox_kb_end;
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

		start = coniox_kb_start;
		end   = coniox_kb_end;

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
			coniox_w(r).cx = (unsigned short) (__ch & 0xFF);
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
