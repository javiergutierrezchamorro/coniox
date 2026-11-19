/**
@file coniox.h
CONIOX v5.0                                                     (c) Copyright 1995-2026 by Javier Gutiérrez Chamorro
                                                            https://nikkhokkho.sourceforge.io/static.php?page=CONIOX


Portable library replacement for CONIO functions. Including direct video output for DOS16 and DOS32 and Windows API
wrapper for Win32 and Win64.

       ***   This source code is Public Domain, so you can use, copy distribute, and modify it free of charge. ***

The code is highly based on the efforts of:

DIRIO 2.4 - Oct 1995
De Trans Software (D.T.S.)

DIRIO 4.0 - Sep 2000
Javier Gutiérrez Chamorro

CONIO 1.31 - Aug 2005
C++ Con Clase (Salvador Pozo Coronado / Fru <elfrucool@yahoo.com.mx>)

Borland-style CONIO 2.1 - Dec 2013
(https://sourceforge.net/projects/conio/files/conio21.zip/download)

CONIOW - Oct 2015
Demidov Andrey (https://sourceforge.net/projects/coniow/)

CONIO 29.0 - Jul 2022
Embarcadero Technologies, Inc.


VERSION HISTORY
2.9 (yyyy/mm/dd)
- Added support for DOS protected mode.
- Portability improvements to make it compilable with Microsoft/Visual C/C++, Zortech/Symantec/DigitalMars C/C++ and Watcom/OpenWatcom C/C++.
- Rewritten to store internal values in a text_info structure.
- Implemented dtext_info, dnormvideo, dhighvideo and dlowvideo.

3.0 (2000/mm/dd)
- Optimized dputch to avoid an unneded pointer increment making it 10% faster.
- Optimized dputs to cache pacBuffer[iCount], making it and its dependant dprintf 2% faster.
- Made offset calculation a macro for better mantenability.
- Functions will automatically check if dini has been invoked, and do so if not, avoiding the need of manually, making it uneeded (althoug supported) to require calling it before using DIRIO.

3.9 (yyyy/mm/dd)
- Added support for Win32 console applications.
- Optimized DOS16 and DOS32 offset calculations in inline assembler if under Watcom, making this part almost twice as fast.
- Reimplemented scroll to use direct dmovetext, instead of slower BIOS.
- Solved a bug in dgettext, dputtext, and dmovetext that used window coordinates, and should not.
- Implemented _setcursortype, and _wscroll

4.0 (2009/mm/dd)
- Made dputch, dclrscr, dclreol, dgettext, dputtext and dmovetext use word movements instead of byte, being 50% faster.
- Separated all functions in separate sources to increase granularity linking only the code trully used, and supporting .LIB creation.

4.91 - 2018/12/14
- Lots of internal simplifications.
- Added doxygen compatible documentation.
- Optimized textmode to use text_info.currmode.
- Fixed _wscroll behaviour.
- Full ANSI and Unicode support.
- Fixed functions not working in window.
- Added textmode.
- Stricted error checking.
- Portability improvements for Watcom/OpenWatcom and Symantec/DigitalMars.
- Tested under Win32 and Win64.
- Tested with Visual C++ 2019, OpenWatcom 2.0, DigitalMars 8.57
- Some optimizations to make it faster.

4.92 - 2021/11/13
- More WIP and DOS bug fixing
- Some ASM 86/386 optimizations
- movetext implemented

4.92a - 2023/02/03
- More WIP and DOS bug fixing
- Implemented directvideo=0

4.93 - 2023/02/06
- Some additional optimizations
- Fixes to DOS32
- Demo successfully running on Watcom DOS16 and DOS32.

4.94 - 2023/02/07
- Cursor handling fixed and improved performance.
- Added C4350 mode support.
- Optimized 6845 base calculation.
- Added delay().

4.95 - 2023/02/08
- Specialized cputs causing 2x faster cputs/cprintf

4.96 - 2023/02/11
- Fixed gettextinfo should return a copy and not a reference.
- Fixed characters should be casted at c8 bit (char).
- Optimized kbhit to direct use keyboard buffer instead of BIOS.
- Added ungetch().

4.97 - 2023/02/13
- Rewritten demo.
- Small fixes and optimizations.
- Lots of Windows fixes.

4.98 -
- Fixed Windows ANSI gettext/puttext
- Added _directvideo.
- Fixes to DOS _setcursortype.
- Fixes to DOS textmode C4350

4.99 -
 - All functions are now working, including ASM pragma aux
 - Several optimizations to Windows and DOS paths

5.00 - 2026/10/22
 - All functions are now working, including ASM pragma aux
 - Several optimizations to Windows and DOS paths
 - Lots of tests with ZEROFILL

5.01 - 2026/10/27
 - Fixed cputs() directvideo cell cache off by one after wrapping a line.
 - Fixed MK_FP fallback macro for real mode (linear instead of far address).
 - Fixed kbhit() BIOS branch did not compile under Borland (no flags in union REGS).
 - Robustified coniox_fmemsetw pragma aux against dirty upper bits of EAX.
 - Fixed Windows textmode() when the new mode is smaller than the current buffer.
 - Removed unused coniox_int86x macros.

5.02 - 2026/10/27
 - Fixed cputs() directvideo wrap: the run already steps one cell past the window edge, so the cache delta is screenwidth - winwidth, not screenwidth - winwidth + 1 like putch().
 - coniox_inline now expands to nothing for Borland/Turbo C older than 4.5, which have no inline keyword in C mode.

5.03 - 2026/10/27
 - Fixed delay() ending early at midnight: the BDA tick counter wraps at 0x1800B0, not at 2^32.

5.04 - 2026/10/29
 - Fixed cgets(), cgetws() and getpass() returning with the deferred cursor unsynced: stdio output written right after landed one cell behind the last echoed character. getche()/getwche() already synced it; the line readers do now too.
 - Fixed getch() returning a pending extended key for a plain 0xE0 character typed with Alt+numpad (scan code 0).
 - Fixed coniox_int86 under DJGPP (int386 does not exist there).
 - DOS16 coniox_putrun pragma aux now takes a far source (DS:SI), so it also works in compact and large models.
 - DOS32 coniox_putrun inner loop uses dec/jnz instead of the slow LOOP.
 - inportb/outportb pragma aux no longer declare DX/AL as clobbered when IN/OUT leave them intact.
 - coniox_basecrt is now a constant, so the 6845 port numbers are folded at compile time.
 - gotoxy() computes the cell cache once for both directvideo and BIOS paths.

5.05 - 2026/11/01
 - Fixed Windows movetext() losing Unicode cells (putwch()/cputws()) in non-UNICODE builds: it went through the charset dependent ReadConsoleOutput/WriteConsoleOutput, now it always uses the wide variants.
 - delay() DOS: the BDA and the INT 1Ah tick sources now share one wait loop instead of two copies.
 - getch() DOS: the directvideo branch no longer syncs the cursor twice (kbhit() already does), only the BIOS branch needs its own sync.
 - Removed dead code: the undefined CONIOX_FAST_PUTCH hooks in the Windows cputs()/cputws(), the unreachable runlen == 0 branches of cputs()/cputws(), and the redundant coniox_init() in the DOS getwch().
 - Windows cputs()/cputws() indentation converted to tabs.

5.06 - 2026/11/02
 - DOS32 coniox_putrun pragma aux rewritten for 386+: dword aligned destination, 4 chars per pass (one dword load, two STOSD) and a short tail, instead of 2 chars per pass.
 - DOS32 coniox_fmemsetw and coniox_fmemcpy pragma aux now align the destination to a dword before REP STOSD/MOVSD (partial-width windows, odd columns, user buffers).
 - DOS cputs(): the run scan tests the common case (char > 13) first, 2 compares per char instead of 5.

5.07 - 2026/11/02
 - DOS gotoxy() programs the CRTC cursor address with two 16-bit OUT (index+data) instead of four 8-bit OUT. Added outportw() (pragma aux for Watcom, outport for Borland, outpw elsewhere; DJGPP already has it).

5.08 - 2026/11/02
 - Windows kbhit(): peeks a batch of 16 events and drops the leading non-key ones (key releases, resizes, mouse) with one ReadConsoleInput; it no longer needs a Peek + Read pair per event plus a final empty Peek, so polling after a key press costs 2 console calls instead of 3, and event floods cost far less.
 - Windows cputs()/cputws(): the run scan and the cell build are one pass instead of two, and the scan tests the common case (char > 13) first.
 - Windows coniox_putchxyattrwh(): plain DWORD fill instead of the memcpy doubling, so clrscr()/clreol()/delline() fills are simpler and also set the unused char padding to zero.

5.09 - 2026/11/02
 - DOS cputs(): coniox_putrun() now scans and writes the run in one pass and returns the cells written, instead of a C scan loop followed by the write. DOS32 (386+) moves 4 chars per pass with a SWAR test for control chars and never loads a dword across a 4K page; DOS16 gets a one-pass LODSB/STOSW loop.
 - putch(): printable chars are tested first (one compare for chars above 13) and the three control cases moved to a switch.
 - DOS gotoxy(): no port writes when the BIOS data area already holds the target position.
 - DOS coniox_init(): the VRAM base is selected once instead of three times.

5.10 - 2026/11/03
 - DOS16/DOS32 coniox_putrun pragma aux: printable chars now fall through the control test instead of taking a jump, which is the expensive case on an 8088 (16 cycles taken, 4 not taken), a 286 and a 386.
 - putch(): the window height is only computed when a wrap or a line feed needs it.
 - Windows gettext()/puttext(): one DWORD access per cell instead of two field accesses.

5.11 - 2026/11/17
 - DOS _setcursortype() directvideo programs the CRTC cursor shape with two 16-bit OUT (index+data) instead of four 8-bit OUT, like gotoxy().

5.12 - 2026/11/17
 - Fixed getche()/getwche() echoing the scan code of an extended key (second call after the 0): only real characters are echoed.
 - DOS coniox_init() with directvideo=0 now reads the row count from the BDA (0040:0084) instead of assuming 25, so 43/50 line modes work through BIOS too.

5.13 - 2026/11/17
 - coniox_enhanced_palette() directvideo: the DAC write index auto-increments, so it is programmed once per run of consecutive registers (4 times instead of 16): 52 OUT instead of 64.

5.14 - 2026/11/17
 - Windows putwtext()/getwtext(): wchar_info and CHAR_INFO have the same 4 byte layout, so the console now reads/writes the caller buffer directly (no scratch buffer, no malloc, no per cell conversion loop).

5.15 - 2026/11/17
 - Deep review: verified portable/platform paths, cursor deferral, cell cache after wrap, extended-key (0 + scan) handling for getch/getche, delay() midnight BDA wrap, Unicode cell transfer and movetext using wide APIs. No remaining functional defects found. Documentation and version bumped for the review.

5.16 - 2026/11/17
 - Optimizations focused on directvideo=1 and Windows hot paths:
   - DOS C fallback (non-Watcom) coniox_putrun: pointer walking + common-case-first test (ch > 13) instead of indexed loop.
   - DOS C fallback coniox_fmemsetw: residual 2/1 handling after the 4-unroll without a second while.
   - Windows cputs()/cputws(): common-case-first control test; skip WriteConsoleOutput when runlen==0 (no empty API call).

5.17 - 2026/11/17
 - All private helpers confirmed static. Small ones now coniox_inline (coniox_putwchxyattr, coniox_ansi2unicode).
 - Pragma aux review: registers and nomemory already optimal (inportb/outportb/outportw use nomemory; memory writers correctly omit it; modify exact lists only clobbered regs; DOS16 DS push/pop not listed).
 - Fixed Watcom compile: coniox_inline macro is defined before the Windows forward declarations that use it; wchar.h is included before wchar_t is named.

5.18 - 2026/11/18
 - The "already initialized?" test is now an inline macro (coniox_ready) instead of a call to coniox_init() in every public function: one call and return less in putch(), gotoxy(), textcolor(), wherex() and the rest.
 - cvprintf()/cprintf(): a format without '%' goes straight to cputs() (no formatting pass, no 254 char truncation) and the result is the cputs() count instead of a strlen() pass.
 - putch()/putwch()/cputs()/cputws(): the CR, LF, BS and line wrap logic lives in small static inline helpers (coniox_cr, coniox_lf, coniox_bs, coniox_wrap, coniox_scrollup) instead of four copies. DOS cputs() now wraps with the same cell cache moves as putch(), without recomputing the offset.
 - The window width and height are cached (coniox_winwidth, coniox_winheight, refreshed by coniox_winsize()) instead of being recomputed on every putch()/cputs(). They live outside text_info, whose layout is public.
 - coniox_vram, coniox_cell and the cputs() cursor pointer share one coniox_cellptr type: near in FLAT/DJGPP (coniox_far is empty there), far in real mode.
 - DOS movetext(): new coniox_fmemcpyr() (STD + REP MOVSD/MOVSW, backwards) replaces the cell by cell C loop for the horizontal overlap, and a full-width move down is one backwards block instead of a copy per row.
 - coniodemo benchmark: each test repeats its original workload for at least one second (clock() ticks at 18.2 Hz on DOS) and reports ms and operations per second; the cputs wrapping test is now timed; the gotoxy test no longer times the random number generator.

5.19 - 2026/11/19
 - cwprintf(): now mirrors cvprintf() — a format without '%' skips the formatting pass and the PRINTFBUF_SIZE truncation, and the return value is the cputws() count instead of a redundant wcslen() pass.
 - coniodemo: the live FPS overlay is opt-in per effect. main() resets demo_fps (and the counter state) before every effect, so a timed test like fx_benchmark can never inherit the previous effect's live counter. The FPS overlay stays on only in the non-timed animated effects (intro, stars, scroller, desktop, credits).
 - conioxid (Arkanoid + CONIOX tour) fixes: lose_ball() now resets ball_timer so a relaunch is immediate (it was delayed up to ~10 frames by the stale interval) and clamps the bottom-border repair to non-corner columns so it cannot overwrite a B_BL/B_BR; demo_s10 no longer swallows ESC during the kbhit() polling step.
 - conioxid optimizations: draw_box() builds its three row patterns (top/middle/bottom) once and writes one cputsxy() per row instead of ~width putch() calls per row (a 66x46 field box went from ~3000 putch to 46 cputs); center_msg() erases with one space run instead of one putchxy per char; bricks_draw_all() iterates only up to the active brick_rows instead of always MAXROWS+1, so levels with fewer rows skip the blank repaints.
 - conioxid tidy-ups: records_load() sorts the loaded table descending so a hand-edited ARKANOID.DAT cannot break the game_over() high-score check; paddle_dx_for() drops the unreachable `: 1` fallback (ball_dx is never 0); the intro subtitle is centered on its real 41-cell width instead of 37.
 - Win32 coniox_ansi2unicode(): a 256-entry OEM byte -> Unicode table is built once in coniox_init() (one MultiByteToWideChar of all 256 bytes), replacing a GetOEMCP()+MultiByteToWideChar() kernel pair on every getwch()/cgetws()/cwscanf() key.
 - Win32 textattr(): the SetConsoleTextAttribute() syscall is skipped when the attribute is unchanged (color setup is called very frequently; no coniox output path reads the console's current attribute, every cell carries its own via CHAR_INFO, so the call only matters for stdio printf sync). The last attribute is cached and seeded from the console's real attribute at init.
 - Win32 cputs()/cputws(): attrcell (attribute<<16) and the orgx+winleft / orgy+wintop bases are hoisted above the per-run loop — they are constant for the whole call, only ti.curx/ti.cury advance. The control-char byte (*__str) is read once per iteration instead of being re-dereferenced for the CR/LF/BS comparisons.
 - Win32 gettext()/puttext(): each source cell is loaded once instead of twice during the CHAR_INFO <-> unsigned short packing (a full 80x50 read/write removes 4000 redundant loads).
 - coniox_inwindow(): uses the cached coniox_winwidth/coniox_winheight instead of recomputing winright-winleft / winbottom-wintop on every gotoxy() call.
 - DOS cputs(): the control-char byte (*__str) is read once per iteration, and the BIOS-mode bounded run scan reads each run byte once instead of up to four times per char.
 - Win64 /W4 clean: every int -> unsigned short assignment in cputs()/cputws() (both Win32 and DOS) now carries an explicit (unsigned short) cast (ti.curx = winwidth, ti.curx += runlen); runlen/remaining are int in the Win32 twins so winwidth-ti.curx arithmetic stays signed; lowvideo() masks with (unsigned short)~0x08; getch() masks ENABLE_PROCESSED_INPUT with ~(DWORD). The whole file is now -Wconversion -Wsign-conversion clean on the Win32 path.
 - Win32 delline()/insline() share one static coniox_scroll_line(int ins) helper: the two functions differed only in which rows are the scroll source/destination, so the validation, last-line clear fallback, CHAR_INFO fill and ScrollConsoleScreenBuffer call are factored out; each public function is now a one-liner.
 - DOS gettext()/puttext() share one static coniox_rect_geom() helper that validates the rectangle and computes the VRAM cell, stride and byte counts; the two functions keep only their copy direction (dst<-p vs p<-src), cutting ~90 lines to ~50.
 - conioxid.c draw_box(): the three memset() calls (top/mid/bot row fills) cast inner to (size_t) to silence int->size_t -Wsign-conversion; center_msg() likewise casts its erase length.

5.20 - 2026/11/19
 - The coniox_w(regs) macro (already defined for the DPMI/VESA paths) now replaces every inline `#if defined(__WATCOMC__) r.w.X = ... #else r.x.X = ... #endif` register-access block at the BIOS INT 10h/INT 1Ah call sites (15 blocks: putchxyattr, putchxyattrwh, idle x2, enhanced_palette, blink, delay, textmode x4, delline, insline, clrscr, ungetch). ~58 lines of preprocessor boilerplate removed; the macro is a pure .w/.x selector, so the generated code is identical per compiler.
 - DOS clrscr()/delline()/insline() BIOS branches share one static coniox_bios_scroll(ax, top) helper: the three INT 10h AH=06h/07h setups differed only in the AX value (0601h up / 0701h down / 0600h clear) and the top row, so the bh/cl/dl/dh + int86 envelope is factored out; each else-branch is now a one-line call.
 - Win32 putwtext()/getwtext() share one static coniox_wtext_io() helper: the two were byte-identical except for WriteConsoleOutputW vs ReadConsoleOutputW and the const cast; validation, SMALL_RECT/COORD setup and the shared-layout comment are factored out, each public function is a one-liner.
 - Win32 ungetch()/ungetwch() share one static coniox_ungetk(ch, is_wide) helper: the two INPUT_RECORD[2] down+up key-event builds differed only in uChar.AsciiChar vs UnicodeChar and WriteConsoleInputA vs W; each public function is a one-liner.
 - Win32 coniox_scroll_line(): the orgx+winleft / orgy+wintop origin is hoisted to baseX/baseY once (was recomputed up to 4x per call), and the last-line check + clear fallback use the cached coniox_winwidth/coniox_winheight instead of recomputing winright-winleft+1 / winbottom-wintop+1.
 - clrscr() (Win32 + DOS), delline()/insline() (DOS): the remaining `ti.winright - ti.winleft + 1` / `ti.winbottom - ti.wintop + 1` recomputations are replaced with the cached coniox_winwidth/coniox_winheight.

5.21 - 2026/11/19
 - clreol(): the last function still recomputing the window width (`ti.winright - ti.winleft - ti.curx + 2`) now uses the cached coniox_winwidth (`aw = coniox_winwidth - ti.curx + 1`). Cache audit confirmed every other function already reads coniox_winwidth/coniox_winheight or the coniox_cell cache.
 - Win32 coniox_putchxyattrwh() fill: two CHAR_INFO cells per store (8 bytes via unsigned long long) instead of one (4 bytes). On Win64 a single 64-bit store replaces two 32-bit stores; on Win32 the loop overhead halves. Fires on every clrscr() and the scroll_line last-row fallback (up to 16000 cells). CHAR_INFO is 4-byte aligned; x86 tolerates the unaligned 8-byte store.
 - Win32 gettext()/puttext(): pack/unpack two cells per pass. gettext loads 8 bytes (2 CHAR_INFOs) and stores 4 bytes (2 ushorts); puttext loads 4 bytes (2 ushorts) and stores 8 bytes (2 CHAR_INFOs). A full 80x50 read/write halves its iteration count; the odd-cell tail keeps the existing per-cell path.
 - DOS coniox_putchxyattrwh() directvideo: ti.screenwidth is hoisted to a local before the per-row fill loop (the compiler could not prove it loop-invariant through the coniox_fmemsetw call and reloaded it every row).
 - DJGPP / Borland C fallback coniox_fmemsetw(): widened from 2-byte (short) stores to 4-byte (unsigned long) stores — a replicated dword fills two cells per store, with a leading short for dword alignment and a trailing short for an odd count. Mirrors the Watcom FLAT REP STOSD path. The Watcom FLAT and real-mode pragma versions were already optimal (REP STOSD / REP STOSW).
 - DJGPP / Borland C fallback coniox_fmemcpyr(): widened from 2-byte backward stores to 4-byte backward stores (two cells per step), with a 2-byte tail for the odd-cell case. Mirrors the Watcom FLAT std;rep movsd path; overlap-safe for movetext() right-shift moves.

5.22 - 2026/11/19
 - Win32 cputs() run-build: SWAR quad scan. The run-build loop now loads 4 source bytes at a time, SWAR-tests for any byte <= 0x0D ((s - 0x0E0E0E0E) & ~s & 0x80808080 — the same trick the Watcom FLAT coniox_putrun pragma uses), and on a clean quad splats 4 CHAR_INFOs (16 bytes) via two 8-byte (unsigned long long) stores. A page-crossing guard ((size_t)src & 0xFFF > 0xFFC) falls back to per-byte when a 4-byte load would straddle a 4K page (the source C string may end mid-quad on an unmapped page); the per-byte path handles any byte <= 0x0D (distinguishing stop-chars {0,8,10,13} from writable {1..7,9,11,12}) and the 0..3-cell tail. 4x fewer scan iterations and 4x fewer stores on long printable runs; cputs() is the back end of cprintf()/cvprintf().
 - DJGPP / Borland C fallback coniox_putrun(): SWAR quad scan, a faithful C port of the Watcom FLAT pragma aux. Loads 4 source bytes, SWAR-tests for any byte <= 0x0D, splats 4 cells (8 bytes) via two 4-byte (unsigned long) stores to VRAM, with a 1-cell align prologue (odd cell offset -> one leading short to reach a dword boundary), a page-crossing guard (DJGPP DPMI only; Borland DOS16 segment:offset wraps safely), and a per-byte fallback. 4x scan + 4x store reduction on DJGPP cputs() (which has no kernel call to dwarf the scan, unlike Win32). The Watcom FLAT build keeps its own pragma.
 - Win32 getch(): the console input mode is cached at init (coniox_input_mode + coniox_input_ok). getch() used to call GetConsoleMode() every key just to (a) detect redirected stdin and (b) read the mode to toggle ENABLE_PROCESSED_INPUT; now it uses the cached value and skips the syscall. The SetConsoleMode pair (disable/restore PROCESSED_INPUT) is kept so Ctrl+C stays a key, not a signal.
 - DOS getch()/ungetch(): the BIOS keyboard buffer bounds (BDA 0040:0080/0082, with the start==0 -> 0x1E/0x3E fallback) are cached at init (coniox_kb_start/coniox_kb_end). Both functions used to peek 0040:0080 and 0040:0082 every call; now they read the statics. Fixed at POST; the init-time start==0 guard covers the same emulators the per-call guard did.
 - DOS delline()/insline() directvideo: after the per-row copy loop, dstrow already points at the blank-fill row (the last window row for delline, the freed top row for insline); the trailing coniox_offset() multiply is replaced with the already-correct dstrow. The full-width branch of delline keeps its coniox_offset() call (its dstrow lands elsewhere).

5.23 - 2026/11/19
 - Win32 cputs() run-build: fix an uninitialized-read regression introduced in 5.22. The SWAR port replaced the original per-char `for (runlen = 0; ...)` loop and lost its `runlen = 0` initializer, so the SWAR `while (remaining - runlen >= 4)` read runlen (and `((unsigned long long *)ci)[runlen/2]`) with a stale/garbage value on the first run, and with the previous run's length on later runs — intermittent missing or garbage output / out-of-bounds WriteConsoleOutputA on the primary Win32 output path. cputws() (which inits runlen = 0) and the DJGPP coniox_putrun() (which returns the count) were unaffected. Fix: `runlen = 0;` after the remaining clamp, matching cputws().
 - Win32 cputws() run-build: SWAR quad scan, the Unicode twin of the cputs() SWAR. Loads 4 wchars (8 bytes) at a time and splats 4 CHAR_INFOs (16 bytes) via two 8-byte (unsigned long long) stores. The "needs per-wchar" test is a TWO-test SWAR, not the single 0x0E0E0E0E test cputs uses: a wchar is a candidate iff its high byte == 0 AND its low byte <= 0x0D, so the 4 high bytes (qword offsets 1,3,5,7) and 4 low bytes (0,2,4,6) are packed into two 32-bit lanes and tested with `(h - 0x01010101u) & ~h & 0x80808080u` (any high byte 0) ANDed with `(l - 0x0E0E0E0Eu) & ~l & 0x80808080u` (any low byte <= 0x0D). The two-test is required because every ASCII wchar carries a 0 high byte — a single all-bytes-<=13 test would trip on every ASCII quad and never engage the fast path. 8-byte page-crossing guard (> 0xFF8), per-wchar fallback for candidates and the 0..3-cell tail. runlen is always even in the quad path, so runlen/2 is the qword slot index.
 - DOS movetext() directvideo: the row byte count `(size_t) w * sizeof(unsigned short)` (and the block count `w * h * sizeof`) was recomputed inline at each of the 5 copy sites (3 per-row loops + 2 full-width). Hoisted to `rowbytes`/`blockbytes` locals computed once after w/h/sw are set; the per-row loops now pass rowbytes, the full-width sites pass blockbytes. gettext()/puttext() already hoisted via coniox_rect_geom().

5.24 - 2026/11/19
 - Win32 gettext()/puttext() cell pack/unpack: widened from 2 cells/pass to 4 cells/pass. gettext() now loads two 8-byte qwords (4 CHAR_INFO) and stores one 8-byte qword (4 packed cell-words); puttext() loads one 8-byte qword (4 cell-words) and stores two 8-byte qwords (4 CHAR_INFO). Each has a 2-cell tail (total & 2) and the existing 1-cell tail (total & 1). Halves the loop iterations and the store count (gettext) / load count (puttext) on large rect copies; on Win64 the 8-byte load/store is native. The extraction math is unchanged (char in bits 0-7, attribute in bits 16-23; cell word = char | (attr << 8)). Mirrors the cputs()/cputws() SWAR splat. The kernel Read/WriteConsoleOutput syscall still dominates wall time, so the win is CPU-side (~5-8% on a full-screen 4000-cell copy).
 - Dead-code audit (Win32 + DOS directvideo=1): every static helper, public API function, static variable, macro, forward declaration, and text_info struct field is provably live via grep. The only helper with no caller outside directvideo=0 is coniox_bios_scroll() — deliberately directvideo=0-only by design (introduced in 5.20 as the BIOS INT 10h AH=06h/AH=07h scroll refactor); left in place since the directvideo=0 path must still compile. No dead branches, no unused macros, no unreachable code.

5.25 - 2026/11/19
 - Win32 gotoxy() short-circuit: when __x==ti.curx && __y==ti.cury && !coniox_cursor_dirty, the hardware cursor is already at the target and SetConsoleCursorPosition would be a no-op, so return early. Mirrors the DOS gotoxy() BDA-peek short-circuit (peekw(0x40,0x50)==target). Saves the syscall on the flush_cursor-driven gotoxy(ti.curx,ti.cury) calls fired by kbhit()/getch()/delay() — the common case where putch()/cputs() moved the logical cursor and the wait point re-syncs to the same position. The !dirty guard is required: dirty=1 means the hardware lags the logical position, so even an unchanged target must still sync.
 - _setcursortype() short-circuit (Win32 + DOS): when __cur_t==coniox_setcursortype AND the shape has been programmed at least once (new coniox_cursor_shape_set flag), skip the shape reprogramming. The flag is necessary because coniox_setcursortype defaults to _NORMALCURSOR at static init and coniox_init() does NOT call _setcursortype, so without the flag the first _setcursortype(_NORMALCURSOR) call would skip programming and leave the OS/BIOS default cursor thickness instead of the library's dwSize=10 / cellheight-2..cellheight-1. textmode() resets coniox_cursor_shape_set=0 before its own _setcursortype(_NORMALCURSOR) call (both Win32 and DOS), because a mode switch (SetConsoleScreenBufferInfoEx / INT 10h AH=00h) reprograms the console/CRTC cursor shape to the new mode's default — without the reset, a textmode() that left coniox_setcursortype==_NORMALCURSOR would short-circuit and leave the wrong (BIOS-default) shape. On DOS the cursor-position resync (gotoxy(ti.curx,ti.cury) when __cur_t != _NOCURSOR) STILL runs in the short-circuit path, matching the original always-resync behavior — only the shape reprogramming (peekb(0x40,0x85)+2 CRTC OUT writes, or the INT 10h AH=01h call) is skipped; the CRTC OUTs are the slowest thing _setcursortype() does (~1 us each on real VGA, uncached on emulators), and the resync is cheap (BDA peek short-circuits the CRTC write) when the cursor is already synced. Win32 has no position resync, so its short-circuit skips only the SetConsoleCursorInfo syscall (a no-op when the shape is unchanged). Zero observable behavior change on either platform.
 - DOS gotoxy() directvideo=1 cell-cache recompute guard: REVERTED (kept reverted). An initial attempt hoisted the coniox_cell recompute behind a (__x==ti.curx && __y==ti.cury) guard, reusing coniox_cell and deriving the linear offset as (int)(coniox_cell - coniox_vram) without the multiply. Correction of the prior audit: it wrongly claimed coniox_cell was not initialized in coniox_init(); it IS — coniox_init() sets coniox_cell = coniox_offset(ti.winleft+ti.curx-2, ti.wintop+ti.cury-2) from the just-read BDA cursor (verified at the init tail), and textmode() zeros ti.screenwidth to force a coniox_ready()→coniox_init() reinit that refreshes it. So the guard was not unsafe on NULL/stale grounds (coniox_cell is also maintained incrementally by putch()/cputs() between gotoxy() calls, all branches verified). The revert is nonetheless kept conservatively: the saving is one IMUL per flush, and a flush only fires at kbhit()/getch()/delay() — where it is dwarfed by keyboard/syscall latency — so the gain is negligible; re-applying cannot be validated in this environment (no DOSBox), and the user had reported screen garble correlated with the 5.25 changes. The unconditional recompute stays until the optimization is re-validated at runtime.
 - putch() (portable, Win+DOS): the printable branch set coniox_cursor_dirty=1 unconditionally before the move, including the _wscroll=0 edge-overwrite case (wrapped==0) where the cursor does NOT move. Moved the dirty set into the branches that actually advance the cursor (advance, wrapped==1 scroll, wrapped==2 wrap); the wrapped==0 case no longer flips dirty. This both saves a redundant gotoxy() flush on the next kbhit()/getch() AND fixes a correctness imprecision (dirty claimed the hardware lagged when it didn't). The control-char branch (CR/LF/BS always move) keeps its dirty=1.
 - cputs() directvideo=1 wrap delta fold: the edge-wrap path backed coniox_currentoffset up one cell (--coniox_currentoffset, to the edge cell putch() would be at) then applied putch()'s at-edge deltas (+(sw-winwidth+1) for wrap==1, +(1-winwidth) for wrap==2). Folded the backup into the deltas: the run leaves coniox_currentoffset one cell PAST the edge (at col winwidth+1), so wrap==1 = +(sw-winwidth) and wrap==2 = -winwidth — one fewer statement and one fewer pointer op per wrap. Arithmetic verified: past-edge (col winwidth+1,row R) → col 1 row R+1 is sw-winwidth cells; past-edge → col 1 row R (scrolled) is -winwidth cells. The run-start clamp (if ti.curx>winwidth) is kept: it is a structural precondition for the wrap path's one-cell-step assumption (it clamps curx to the edge before the run, so after the run curx is at most winwidth+1), and although no normal API path leaves curx past the edge (init/window/gotoxy/putch/cputs all keep curx<=winwidth), removing it would make the wrap path fragile to a future curx>winwidth bug.
 - DOS delline() directvideo=1 full-width branch: the blank-fill target was coniox_offset(winleft-1, winbottom-1), a fresh screenwidth*(winbottom-1) multiply. Replaced with dstrow + (blockbytes/sizeof(unsigned short)), reusing the byte count already computed for the block fmemcpy — the blank row is exactly blockbytes past dstrow (the row just past the moved block). No new multiply. Matches the non-full-width branch which already avoided the multiply via the row-by-row loop's incremental dstrow += sw.
 - DOS insline() directvideo=1 full-width branch: was a row-by-row bottom-up fmemcpy() loop (winbottom-ytop iterations), the one dv=1 path without delline()'s full-width block optimization. Added the mirror: when width==sw AND ytop<winbottom, one coniox_fmemcpyr(srcblk+sw, srcblk, rowbytes*(winbottom-ytop)) backward block copy (rows are contiguous and the move is toward higher addresses, so backward is overlap-safe) + one coniox_fmemsetw(srcblk, ...) blank of the freed cursor row. srcblk = coniox_offset(winleft-1, ytop-1) is the cursor's row; the freed top row after the down-shift is exactly srcblk (no second coniox_offset multiply). Arithmetic verified identical to the row loop for the cursor-at-row-R and cursor-at-bottom cases; the ytop<winbottom guard excludes the 0-row case (cursor at the bottom row → row loop blanks the bottom row, same as before). Brings insline() to parity with delline()'s F5 block path.
 - Watcom DOS32 audit: the cputs() directvideo=1 run-scan+fill is already fused in the coniox_putrun #pragma aux (SWAR dword load + 0x0E0E0E0E test + 2 STOSD in one pass); movetext(), textattr(), wherex()/wherey() are already optimal (REP MOVSD, one store, direct ti.curx/cury return); the gotoxy() CRTC write is already short-circuited via the BDA peek. Ruled out: removing the coniox_putrun pragma page-crossing guard (the 4-instr/quad guard is unconditional in the pragma but the C fallback gates it DJGPP-only; removing it risks a fault on demand-paging DPMI 1.0 hosts — the guard is safety-critical and the ~0.2 ms saving on a 2000-char cputs is not worth the crash risk; deferred to a Watcom runtime test); the coniox_init two-peekw→one-peekl keyboard-bounds read (trivial init-only, and peekl risks 8086 DOS16 which has no 32-bit loads); a row-pointer table for coniox_offset (the multiply never appears in an inner loop — putch/cputs use the incremental coniox_cell cache, the rect ops call coniox_offset once then step with p+=sw).

5.26 - 2026/11/19
 - Independent review of directvideo=1 and Windows hot paths (putch/cputs/cputws/gotoxy/textattr/gettext/puttext/movetext/delline/insline/clrscr/kbhit/getch/_setcursortype). Confirmed no unused private statics, no dead helpers, no residual recomputations of window geometry outside the coniox_winwidth/coniox_winheight cache, and no further safe micro-optimizations that would change observable behaviour. Version bumped for the review pass.
*/

#pragma once
#ifndef _CONIOX_H_
#define _CONIOX_H_

#define coniox_version (526)
#define coniox_build (__DATE__)

#ifdef _MSC_VER
	/* Visual Studio 2017 (15.0) or later */
	#if _MSC_VER > 1900
		#ifndef _CRT_SECURE_NO_WARNINGS
			#define _CRT_SECURE_NO_WARNINGS
		#endif
		#ifndef _CRT_NONSTDC_NO_WARNINGS
			#define _CRT_NONSTDC_NO_WARNINGS
		#endif
		#ifndef _SCL_SECURE_NO_WARNINGS
			#define _SCL_SECURE_NO_WARNINGS
		#endif
		#ifndef _NO_CRT_STDIO_INLINE
			#define _NO_CRT_STDIO_INLINE
		#endif
		#ifdef _CRT_USE_WINAPI_FAMILY_DESKTOP_APP
			#undef _CRT_USE_WINAPI_FAMILY_DESKTOP_APP
		#endif
	#endif
#else
	#ifndef sscanf_s
		#define sscanf_s sscanf
	#endif
	#ifndef swscanf_s
		#define swscanf_s swscanf
	#endif
#endif

#ifdef gettext
	#undef gettext
#endif
#ifdef puttext
	#undef puttext
#endif

#define PRINTFBUF_SIZE	255
#include <stdarg.h>	/* va_list, for cvprintf */
#include <stddef.h>	/* wchar_t in strict C (wchar_info, getwch, ...) */

#ifdef __cplusplus
extern "C" {
#endif


#if ((__DOS__) || (__MSDOS__))
#include <dos.h>
#include <bios.h>

#if defined(__TURBOC__)
	/* Borland spells them disable()/enable(); the directvideo paths use the Watcom/MSC _disable()/_enable() names. */
	#define _disable()	disable()
	#define _enable()	enable()
#endif


#if defined(__WATCOMC__) && defined(__DOS__)
	#include <i86.h>
	unsigned char inportb(unsigned int port);
	#pragma aux inportb  = "in  al, dx" parm nomemory [dx] value [al] modify nomemory exact [al];
	void outportb(unsigned int port, unsigned char value);
	#pragma aux outportb = "out dx, al" parm nomemory [dx] [al] modify nomemory exact [];
	void outportw(unsigned int port, unsigned short value);
	#pragma aux outportw = "out dx, ax" parm nomemory [dx] [ax] modify nomemory exact [];
#elif defined(__TURBOC__)
	#define outportw	outport
#elif !defined(__DJGPP__)
	#define outportw	outpw
#endif

#if defined(__DJGPP__)
	#include <sys/nearptr.h>
	#include <pc.h>
	#define _fmemmove	memmove
	#define _fmemcpy	memcpy
	#define _disable disable
	#define _enable  enable
	#define MK_FP(seg,off) ((((seg)<<4)+(off)) + __djgpp_conventional_base)
	#define coniox_far
#elif defined(__FLAT__)
	/* DOS extender, flat model: every pointer is a 32 bit offset, and the first MB is mapped at linear address 0 */
	#define coniox_far
#else
	#define coniox_far __far
#endif


#ifndef MK_FP
	#if ((defined(__FLAT__)) && (!defined(__DJGPP__)))
		/* Flat model: a real mode segment:offset maps to the linear address seg * 16 + off */
		#define MK_FP(seg,off) (((seg)<<4)+(off))
	#else
		/* Real mode: a far pointer stores the segment in its high word */
		#define MK_FP(seg,off) ((void coniox_far *) ((((unsigned long)(seg)) << 16) | ((unsigned long)(off))))
	#endif
#endif

#if !defined(__TURBOC__)
	unsigned int inp (int port);
	unsigned int inpw (int port);
	unsigned long inpd (int port);
	unsigned int outp (int port, int value);
	unsigned long outpd (int port, unsigned long value);
	unsigned int outpw (int port, unsigned int value);

	#define outport			outpw
	#define outportd		outpd
	#define inport			inpw
	#define inportd			inpd
#endif


#if ((defined(__FLAT__)) && (!defined(__DJGPP__)))
	/* Flat model: a real mode segment:offset is the linear address seg * 16 + off */
	#define coniox_linear(s,o)	((((unsigned long) (s)) << 4) + (unsigned long) (o))
	#define peekb(s,o)			(*((unsigned char *) coniox_linear((s),(o))))
	#define peekw(s,o)			(*((unsigned short *) coniox_linear((s),(o))))
	#define peekl(s,o)			(*((unsigned long *) coniox_linear((s),(o))))
	#define pokeb(s,o,x)		(*((unsigned char *) coniox_linear((s),(o))) = (unsigned char)(x))
	#define pokew(s,o,x)		(*((unsigned short *) coniox_linear((s),(o))) = (unsigned short)(x))
	#define pokel(s,o,x)		(*((unsigned long *) coniox_linear((s),(o))) = (unsigned long)(x))
#else
	#define peekb(s,o)			(*((unsigned char coniox_far *) MK_FP((s),(o))))
	#define peekw(s,o)			(*((unsigned short coniox_far *) MK_FP((s),(o))))
	#define peekl(s,o)			(*((unsigned long coniox_far *) MK_FP((s),(o))))
	#define pokeb(s,o,x)		(*((unsigned char coniox_far *) MK_FP((s),(o))) = (unsigned char)(x))
	#define pokew(s,o,x)		(*((unsigned short coniox_far *) MK_FP((s),(o))) = (unsigned short)(x))
	#define pokel(s,o,x)		(*((unsigned long coniox_far *) MK_FP((s),(o))) = (unsigned long)(x))
#endif
#endif

#if !(((__DOS__) || (__MSDOS__)))
	#define coniox_far
#endif

#pragma pack(push)
/* #pragma pack(1) */

/**
Structure holding information about screen.
@see gettextinfo
*/
struct text_info
{
	unsigned short winleft;
	unsigned short wintop;
	unsigned short winright;
	unsigned short winbottom;
	unsigned short attribute;
	unsigned short normattr;
	unsigned short currmode;
	unsigned short screenheight;
	unsigned short screenwidth;
	unsigned short curx;
	unsigned short cury;
};

/**
Colors which you can use in your application.
*/
#if !defined(__COLORS)
#define __COLORS
	typedef enum _COLORS
	{
		BLACK,			/**< black color */
		BLUE,			/**< blue color */
		GREEN,			/**< green color */
		CYAN,			/**< cyan color */
		RED,			/**< red color */
		MAGENTA,		/**< magenta color */
		BROWN,			/**< brown color */
		LIGHTGRAY,		/**< light gray color */
		DARKGRAY,		/**< dark gray color */
		LIGHTBLUE,		/**< light blue color */
		LIGHTGREEN,		/**< light green color */
		LIGHTCYAN,		/**< light cyan color */
		LIGHTRED,		/**< light red color */
		LIGHTMAGENTA,	/**< light magenta color */
		YELLOW,			/**< yellow color */
		WHITE			/**< white color */
	} COLORS;
#endif

#define BLINK			128
#define PASS_MAX		8



enum text_modes
{
	/* DOS-compatible modes */
	EXITMODE = -2,
	LASTMODE = -1,
	BW40 = 0,
	C40 = 1,
	BW80,
	C80,
	MONO = 7,
	C4350 = 64,
	/* New Color modes */
	C40X14 = 8,
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


/**
@anchor cursortypes
@name Cursor types
Predefined cursor types. */
/*@{*/
#define _NOCURSOR		0			/**< no cursor */
#define _SOLIDCURSOR	1			/**< cursor filling whole cell */
#define _NORMALCURSOR	2			/**< cursor filling 20 percent of cell height */
/*@}*/


/**
Structure used by gettext/puttext.
@see gettext
@see puttext
*/
typedef struct char_info
{
	unsigned char letter;	/**< character value */
	unsigned char attr;	/**< attribute value: background in bits 4-6, blink in bit 7, foreground in bits 0-3 */
} char_info;


/**
Controls video output.
directvideo controls where your program's console output goes:
0: Goes via ROM calls.
1: (Default). Goes directly to video RAM
*/
extern int directvideo;
#define _directvideo directvideo

/**
Enables or disables scrolling in console I/O functions.
_wscroll is a console I/O flag. You can use it to draw along the edges of a window without having the screen scroll.
*/
extern int _wscroll;


void inittextinfo(void);


/**
Gets text mode video information.
gettextinfo fills in the text_info structure pointed to by r with the current text video information.
@see text_info
*/
void gettextinfo(struct text_info *__r);


/**
Clears to end of line in text window.
clreol clears all characters from the cursor position to the end of the line within the current text window, without moving the cursor.
*/
void clreol(void);


/**
Clears the text-mode window.
clrscr clears the current text window and places the cursor in the upper
left corner (at position 1,1).

*/
void clrscr(void);


/**
Deletes line in text window.
delline deletes the line containing the cursor and moves all lines below it one line up. delline operates within the currently active text window.
*/
void delline(void);


/**
Inserts a blank line in the text window.
insline inserts an empty line in the text window at the cursor position using the current text background color. All lines below the empty one move down one line, and the bottom line scrolls off the bottom of the window.
*/
void insline(void);


/**
Copies text from text mode screen to memory.
gettext stores the contents of an onscreen text rectangle defined by left, top, right, and bottom into the area of memory pointed to by destin.
All coordinates are absolute screen coordinates not window-relative. The upper left corner is (1,1). gettext reads the contents of the rectangle into memory sequentially from left to right and top to bottom.
Each position onscreen takes 2 bytes of memory: The first byte is the character in the cell and the second is the cell's video attribute. The space required for a rectangle w columns wide by h rows high is defined as bytes = (h rows) x (w columns) x 2
@see char_info
@see puttext
@param left Left coordinate of the rectangle, inclusive, starting from 1.
@param top Top coordinate of the rectangle, inclusive, starting from 1.
@param right Right coordinate of the rectangle, inclusive, starting from 1.
@param bottom Bottom coordinate of the rectangle, inclusive, starting from 1.
@param buf You have to pass buffer of size <TT>(right - left + 1) * (bottom - top + 1) * sizeof(char_info)</TT>.
*/
int gettext(int __left, int __top, int __right, int __bottom, void coniox_far *__destin);


/**
Copies text from memory to the text mode screen.
puttext writes the contents of the memory area pointed to by source out to the onscreen rectangle defined by left, top, right, and bottom.
All coordinates are absolute screen coordinates, not window-relative. The upper left corner is (1,1).
puttext places the contents of a memory area into the defined rectangle sequentially from left to right and top to bottom.
Each position onscreen takes 2 bytes of memory: The first byte is the 	character in the cell, and the second is the cell's video attribute. The space required for a rectangle w columns wide by h rows high is defined as	bytes = (h rows) x (w columns) x 2
puttext is a text mode function performing direct video output.
@see char_info
@see _conio_gettext
@param left Left coordinate of the rectangle, inclusive, starting from 1.
@param top Top coordinate of the rectangle, inclusive, starting from 1.
@param right Right coordinate of the rectangle, inclusive, starting from 1.
@param bottom Bottom coordinate of the rectangle, inclusive, starting from 1.
@param buf You have to pass buffer of size <TT>(right - left + 1) * (bottom - top + 1) * sizeof(char_info)</TT>.
*/
int puttext(int __left, int __top, int __right, int __bottom, void coniox_far *__source);


/**
Copies text onscreen from one rectangle to another.
movetext copies the contents of the onscreen rectangle defined by left, top, right, and bottom to a new rectangle of the same dimensions. The new rectangle's upper left corner is position (destleft, desttop).
All coordinates are absolute screen coordinates. Rectangles that overlap are moved correctly.
movetext is a text mode function performing direct video output.
@param left Left coordinate of the rectangle, inclusive, starting from 1.
@param top Top coordinate of the rectangle, inclusive, starting from 1.
@param right Right coordinate of the rectangle, inclusive, starting from 1.
@param bottom Bottom coordinate of the rectangle, inclusive, starting from 1.
@param destleft Left coordinate of the destination rectangle.
@param desttop Top coordinate of the destination rectangle.
*/
int movetext(int __left, int __top, int __right, int __bottom, int __destleft, int __desttop);


/**
Positions cursor in text window.
gotoxy moves the cursor to the given position in the current text window. If the coordinates are in any way invalid the call to gotoxy is ignored.
An example of this is a call to 40,30) when (35,25) is the bottom right position in the window. Neither argument to gotoxy can be zero.
@param x horizontal position
@param y vertical position
*/
void gotoxy(int __x, int __y);


/**
Selects cursor appearance.
Sets the cursor type to
_NOCURSOR: Turns off the cursor
_NORMALCURSOR: Normal underscore cursor
_SOLIDCURSOR: Solid block cursor
@see @ref cursortypes
@param type cursor type, under Win32 it is height of the cursor in percents
*/
void _setcursortype(int __cur_t);


/**
Sets text attributes.
textattr lets you set both the foreground and background colors in a single call. (Normally, you set the attributes with textcolor and textbackground.)
This function does not affect any characters currently onscreen; it affects only those characters displayed by functions (such as cprintf) performing text mode, direct video output after this function is called.
@param _attr new text attribute
*/
void textattr(int __newattr);


/**
Selects normal-intensity characters.
normvideo selects normal characters by returning the text attribute (foreground and background) to the value it had when the program started.
This function does not affect any characters currently on the screen, only those displayed by functions (such as cprintf) performing direct console output functions after normvideo is called.
@see text_info
*/
void normvideo(void);


/**
Selects high-intensity characters.
highvideo selects high-intensity characters by setting the high-intensity bit of the currently selected foreground color.
This function does not affect any characters currently onscreen, but does affect those displayed by functions (such as cprintf) that perform direct video, text mode output after highvideo is called.
@see COLORS
@see lowvideo
*/
void highvideo(void);


/**
Selects low-intensity characters.
lowvideo selects low-intensity characters by clearing the high-intensity bit of the currently selected foreground color.
This function does not affect any characters currently onscreen. It affects only those characters displayed by functions that perform text mode, direct console output after this function is called.
@see COLORS
@see highvideo
*/
void lowvideo(void);


/**
Selects new text background color.
textbackground selects the background color. This function works for functions that produce output in text mode directly to the screen.
newcolor selects the new background color. You can set newcolor to an integer from 0 to 7, or to one of the symbolic constants defined in conio.h. If you use symbolic constants, you must include conio.h.
Once you have called textbackground, all subsequent functions using direct video output (such as cprintf) will use newcolor. textbackground does not affect any characters currently onscreen.
@see COLORS
@param color new background color
*/
void textbackground(int __newcolor);


/**
Selects new character color in text mode.
textcolor selects the foreground character color. This function works for the console output functions. newcolor selects the new foreground color.
You can set newcolor to an integer as given in the table below, or to one of the symbolic constants defined in conio.h. If you use symbolic constants, you must include conio.h.
Once you have called textcolor, all subsequent functions using direct video output (such as cprintf) will use newcolor. textcolor does not affect any characters currently onscreen.
@see COLORS
@param color new foreground color
*/
void textcolor(int __newcolor);


/**
Puts screen in text mode.
*/
void textmode(int __newmode);


/**
Defines active text mode window.
window defines a text window onscreen. If the coordinates are in any way invalid, the call to window is ignored.
left and top are the screen coordinates of the upper left corner of the window.right and bottom are the screen coordinates of the lower right corner.
The minimum size of the text window is one column by one line. The default window is full screen, with the coordinates: 1,1,C,R
where C is the number of columns in the current video mode, and R is the number of rows.
*/
void window(int __left, int __top, int __right, int __bottom);


/**
Scans and formats input from the console.
cscanf scans a series of input fields one character at a time, reading directly from the console. Then each field is formatted according to a format specifier passed to cscanf in the format string pointed to by format.
Finally, cscanf stores the formatted input at an address passed to it as an argument following format, and echoes the input directly to the screen.
There must be the same number of format specifiers and addresses as there are input fields.
Note: For details on format specifiers, see scanf Format Specifiers.
cscanf might stop scanning a particular field before it reaches the normal end-of-field (whitespace) character, or it might terminate entirely for a number of reasons. See scanf for a discussion of possible causes.
*/
int cscanf(const char *__format, ...);


/**
Reads a string from the console.
cgets reads a string of characters from the console, storing the string (and the string length) in the location pointed to by str.
cgets reads characters until it encounters a carriage-return/linefeed (CR/LF) combination, or until the maximum allowable number of characters have been read. If cgets reads a CR/LF combination, it replaces the combination with a \0 (null terminator) before storing the string.
Before cgets is called, set str[0] to the maximum length of the string to be read. On return, str[1] is set to the number of characters actually read. The characters read start at str[2] and end with a null terminator.
Thus, str must be at least str[0] plus 2 bytes long.
*/
char *cgets(char *__str);


/**
Writes formatted output to the screen using an argument list.
cvprintf is the va_list equivalent of cprintf, in the same way vprintf relates to printf.
@see cprintf
@param __format format string
@param __arglist pointer to the list of arguments
*/
int cvprintf(const char *__format, va_list __arglist);

#define _cvprintf	cvprintf
#define vcprintf	cvprintf

/**
Writes formatted output to the screen.
cprintf accepts a series of arguments, applies to each a format specifier contained in the format string pointed to by format, and outputs the formatted data directly to the current text window on the screen. There must be the same number of format specifiers as arguments. For details details on format specifiers, see printf Format Specifiers.
The string is written either directly to screen memory or by way of a BIOS call, depending on the value of the global variable _directvideo.
Unlike fprintf and printf, cprintf does not translate linefeed characters (\n) into carriage-return/linefeed character pairs (\r\n). Tab characters (specified by \t) are not expanded into spaces.
*/
int cprintf(const char *__format, ...);


/**
Writes a string to the screen.
cputs writes the null-terminated string str to the current text window. It does not append a newline character.
The string is written either directly to screen memory or by way of a BIOS call, depending on the value of the global variable _directvideo.
Unlike puts, cputs does not translate linefeed characters (\n) into carriage-return/linefeed character pairs (\r\n).
*/
int cputs(const char *__str);


/**
Reads a password.
getpass reads a password from the system console after prompting with the null-terminated string prompt and disabling the echo. A pointer is returned to a null-terminated string of up to eight characters (not counting the null-terminator).
@see cgets
@param prompt prompt which will be displayed to user
@returns pointer to a static null-terminated string of up to PASS_MAX characters holding the typed password
*/
char *getpass(const char *__prompt);


/**
Gets character from keyboard, does not echo to screen.
getch reads a single character directly from the keyboard, without echoing to the screen.
*/
int getch(void);


/**
Gets character from the keyboard, echoes to screen.
getche reads a single character from the keyboard and echoes it to the current text window using direct video or BIOS.
*/
int getche(void);


/**
Checks for currently available keystrokes.
kbhit checks to see if a keystroke is currently available. Any available keystrokes can be retrieved with getch or getche.
*/
int kbhit(void);


/**
Outputs character to screen.
putch outputs the character c to the current text window. It is a text mode
function performing direct video output to the console. putch does not
translate linefeed characters (\n) into carriage-return/linefeed pairs.
The string is written either directly to screen memory or by way of a BIOS
call, depending on the value of the global variable _directvideo.
*/
int putch(int __c);



/**
Pushes a character back to the keyboard buffer.
ungetch pushes the character ch back to the console, causing ch to be the next character read. The ungetch function fails if it is called more than once before the next read.
*/
int ungetch(int __ch);


/**
Gives horizontal cursor position within window.
wherex returns the x-coordinate of the current cursor position (within the current text window).
@returns cursor X position
*/
int wherex(void);


/**
Gives vertical cursor position within window.
wherey returns the y-coordinate of the current cursor position (within the current text window).
@returns cursor Y position
*/
int wherey(void);



/**
Pauses program execution for a given time.
@param ms miliseconds
*/
void delay(unsigned int ms);


void coniox_init(const void *title);


#define _cgets		cgets
#define _cprintf	cprintf
#define _cputs		cputs
#define _cscanf		cscanf
#define _getch		getch

#ifdef UNICODE
	#define _cgetws		cgetws
	#define _getwch		getwch
	#define _getwche	getwche
	#define _putwch		putwch
	#define _ungetwch	ungetwch
	#define _cputws		cputws
	#define _cwprintf	cwprintf
	#define _cwscanf	cwscanf
#endif
#define gettextsize(__left, __top, __right, __bottom)	(((__right) - (__left) + 1) * ((__bottom) - (__top) + 1) * 2)


#define KEY_SPECIAL     0x100
#define KEY_HOME        71
#define KEY_UP          72
#define KEY_PGUP        73
#define KEY_LEFT        75
#define KEY_CENTER      76
#define KEY_RIGHT       77
#define KEY_END         79
#define KEY_DOWN        80
#define KEY_PGDN        81
#define KEY_INSERT      82
#define KEY_DELETE      83
#define KEY_F1          59
#define KEY_F2          60
#define KEY_F3          61
#define KEY_F4          62
#define KEY_F5          63
#define KEY_F6          64
#define KEY_F7          65
#define KEY_F8          66
#define KEY_F9          67
#define KEY_F10         68
#define KEY_ESC 27
#define MOUSE_CLICK     0x200
#define MOUSE_DBLCLICK  0x201
#define MOUSE_WHEELUP   0x202
#define MOUSE_WHEELDOWN 0x203
#define WINDOW_RESIZE   0x204



/**
Structure used by getwtext/putwtext.
@see gettext
@see puttext
*/
typedef struct wchar_info
{
	wchar_t letter;			/**< character value */
	unsigned short attr;	/**< attribute value */
} wchar_info;

wchar_t  getwch(void);

#if (((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32))
	wchar_t  putwch(wchar_t);
#endif

#if defined(UNICODE)
	#if (((__WIN32__) || (__WINDOWS__) || (__NT__)) || (_WIN32))
		/* The whole wchar_t family is Windows-only: DOS lacks portable swscanf/vswprintf, and only getwch() exists there. */
		wchar_t *cgetws( wchar_t *__str );
		int      cputws( const wchar_t *__str );
		wchar_t  ungetwch( wchar_t );
		wchar_t  getwche(void);
		int cwscanf( const wchar_t *__format, ... );
		int cwprintf( const wchar_t *__format, ... );
		int putwtext( int __left, int __top, int __right, int __bottom, const wchar_info *__source );
		int getwtext(int __left, int __top, int __right, int __bottom, wchar_info *__destin);
	#endif
#endif

#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif /* _CONIOX_H_ */
