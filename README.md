# coniox
coniox is a conio library replacement that implementents conio.h functions in Borland/Turbo C/C++ compatible with DOS16/DOS32 and Windows Win32/Win32 text consoles. It is suitable for Microsoft/C Compiler/QuickC/Visual C/C++, Borland, Zortech/Symantec/DigitalMars C/C++ and Watcom/Openwatcom C/C++ compilers among others.

If you are already using Turbo/Borland conio.h in you DOS development, coniox will have same functionality, but with a performance being 1.5x to 3x faster.


DOS			putch	cputs	cprintf
CONIO		4.3s	2.7s	2.7s
DIRIO 2.4	1.0s	0.6s	0.8s
CONIOX 5	2.5s	0.8s	1.3s


Windows
CONIOX	5	2.4s	1.7s	2.1s

![Screenshot 1: Conio Demo Under OpenWatcom DOS real mode (screenshot01.png)

![Screenshot 2: Conio Demo Under OpenWatcom DOS Win32 (screenshot02.png)
