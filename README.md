# coniox
coniox is a conio library replacement that implementents conio.h functions in Borland/Turbo C/C++ compatible with DOS16/DOS32 and Windows Win32/Win32 text consoles. It is suitable for Microsoft/C Compiler/QuickC/Visual C/C++, Borland, Zortech/Symantec/DigitalMars C/C++ and Watcom/Openwatcom C/C++ compilers among others.

If you are already using Turbo/Borland conio.h in you DOS development, coniox will have same functionality, but with a performance being 1.5x to 3x faster. DOSBOX-X Performance at 3000 cycles (~286 @ 20Mhz)


DOS					putch	cputs	cprintf
CONIO					4.3s	2.7s	2.7s
DIRIO 2.4				1.0s	0.6s	0.8s
CONIOX 5 (8086)			2.5s	0.4s	0.8s
CONIOX 5 (386)			2.1s	0.4s	0.9s


Windows
CONIOX 5 (WATCOM x86)	8.8s	5.4s	13.9s
CONIOX 5 (VC x64)		2.5s	1.9s	2.3s

![screenshot01](https://user-images.githubusercontent.com/4672971/218527583-8dcacafe-f736-4279-9cc9-7967207ce553.png)

![screenshot02](https://user-images.githubusercontent.com/4672971/218527622-851ba3e2-f364-4645-95f7-0e4cbd01ba14.png)
