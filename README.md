# coniox
coniox is a conio library replacement that implementents conio.h functions in Borland/Turbo C/C++ compatible with DOS16/DOS32 and Windows Win32/Win32 text consoles. It is suitable for Microsoft/C Compiler/QuickC/Visual C/C++, Borland, Zortech/Symantec/DigitalMars C/C++ and Watcom/Openwatcom C/C++ compilers among others.

If you are already using Turbo/Borland conio.h in you DOS development, coniox will have same functionality, but with a performance being 2x to 8x faster. DOSBOX-X Performance at 3000 cycles (~286 @ 20Mhz)


DOS									putch	cputs	cprintf
CONIO								4.3s	2.7s	2.8s
DIRIO 2.4							1.0s	0.6s	0.8s
CONIOX 5 (8086. directvideo=0)		6.5s	2.9s	4.4s
CONIOX 5 (8086)						1.2s	0.3s	0.8s
CONIOX 5 (386)						1.1s	0.3s	0.8s


Windows
CONIOX 5 (WATCOM x86)				2.9s	0.5s	0.6s
CONIOX 5 (VC x64)					2.7s	0.5s	0.6s


<img width="720" height="400" alt="image" src="https://github.com/user-attachments/assets/a0762e68-2e40-4160-9310-f92f6e3bd111" />

<img width="754" height="590" alt="image" src="https://github.com/user-attachments/assets/f8a0cd21-d39f-4f88-908d-f03f58ee7a4b" />
