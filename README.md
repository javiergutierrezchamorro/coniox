# coniox
coniox is a conio library replacement that implementents conio.h functions in Borland/Turbo C/C++ compatible with DOS16/DOS32 and Windows Win32/Win64 text consoles. It is suitable for Microsoft/C Compiler/QuickC/Visual C/C++, Turbo/Borland C/C++, Zortech/Symantec/DigitalMars C/C++, DJGPP/GCC and Watcom/Openwatcom C/C++ compilers among others.

If you are already using Turbo/Borland conio.h in you DOS development, coniox will have same functionality, but with a performance being 3x to 12x faster. DOSBOX-X Performance at 3000 cycles (~286 @ 20Mhz). Compared to Borland's CONIO, memory usage as well as COM/EXE sizes should be equivalent or even smaller.


PRIMITIVES PER SECOND
DOS									putch	cputs1	cputs2	cprintf	puttext	gettext	movetext	gotoxy	clrscr
CONIOX 5 (8086. directvideo=0)		
CONIOX 5 (8086)						75872	3639	5752	1400	1183	1099	1245		24813	1241
CONIOX 5 (386)						79113	3469	4793	1400	2777	2777	2873		28735	2876
Windows
CONIOX 5 (WATCOM x86)				13276	1684	1706	2042	1970	3400	2400		24000	2626
CONIOX 5 (VC x64)					


COMPARISON
DOS									putch	cputs	cprintf
CONIO								8.7s	13.5s	5.6s
DIRIO 2.4							2.0s	3.0s	1.6s
CONIOX 5 (8086. directvideo=0)		13s		14.5s	9.2s
CONIOX 5 (8086)						1.3s	1.0s	1.4s
CONIOX 5 (386)						1.3s	1.0s	1.s
Windows
CONIOX 5 (WATCOM x86)				2.4s	0.4s	0.5s
CONIOX 5 (VC x64)					2.7s	0.5s	0.6s




<img width="720" height="400" alt="image" src="https://github.com/user-attachments/assets/a0762e68-2e40-4160-9310-f92f6e3bd111" />

<img width="754" height="590" alt="image" src="https://github.com/user-attachments/assets/f8a0cd21-d39f-4f88-908d-f03f58ee7a4b" />
