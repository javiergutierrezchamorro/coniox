/*ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿*/
/*³  DIRIO v4.0      (c) Copyright 1995-2009 by Javier Gutiérrez Chamorro ³*/
/*³                                           http://guti.webcindario.com ³*/
/*³                                                /static.php?page=DIRIO ³*/ 
/*³                                                                       ³*/
/*³                                                                       ³*/
/*³                                                                       ³*/
/*³  Portable library replacement for CONIO functions.                    ³*/
/*³  Including direct video output for DOS16 and DOS32 and Windows API    ³*/
/*³  wrapper for Win32.                                                   ³*/
/*³                                                                       ³*/
/*³  ***   This source code is Public Domain, so you can use, copy  ***   ³*/
/*³  ***         distribute, and modify it free of charge.          ***   ³*/
/*³                                                                       ³*/
/*³ The code is highly based on the efforts of:                           ³*/
/*³                                                                       ³*/
/*³ DIRIO 2.4                                                    Oct 1995 ³*/
/*³ De Trans Software (D.T.S.)                                            ³*/
/*³                                                                       ³*/
/*³ DIRIO 3.0                                                    Sep 2000 ³*/
/*³ Javier Gutiérrez Chamorro                                             ³*/
/*³                                                                       ³*/
/*³ CONIO 1.31                                                   Aug 2005 ³*/
/*³ C++ Con Clase (Salvador Pozo Coronado / Fru <elfrucool@yahoo.com.mx>) ³*/
/*³                                                                       ³*/
/*³ CONIO 15.0                                                   Jul 2009 ³*/
/*³ Embarcadero Technologies                                              ³*/
/*³                                                                       ³*/
/*ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <dos.h>
#include <bios.h>
#if !defined (__DOS__)
	#include <windows.h>
#endif
#include "dirio.h"

struct text_info gudtTextInfo={0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int _wscroll=0;
	
	
#if defined (__DOS__)
	int directvideo=1;
	#if (defined(__FLAT__))
		unsigned char *gpVram;
		#define dfar
		#define dint86 int386
		#if defined(__WATCOMC__)	
			unsigned char *doffset(unsigned int piX, unsigned int piY);
			#pragma aux doffset= 						\
			"		.386								"\
			"		movzx eax, byte ptr gudtTextInfo+8	"\
			"		imul eax, edi  						"\
			"		add eax, esi   						"\
			"		shl eax, 1							"\
			"		add eax, gpVram						"\
			parm [ESI][EDI]								\
			modify []									\
			value [EAX];
		#else
			#define doffset(piX, piY) (gpVram+(((gudtTextInfo.screenwidth*(piY))+(piX))<<1))
		#endif
	#else
		unsigned char far *gpVram;	
		#define dfar far
		#define dint86 int86
		#if defined(__WATCOMC__)	
			unsigned char far *doffset(unsigned int piX, unsigned int piY);
			#pragma aux doffset= 						\
			"		.8086								"\
			"		xor ax, ax							"\
			"		mov al, byte ptr gudtTextInfo+8		"\
			"		mul di  							"\
			"		add ax, si   						"\
			"		shl ax, 1							"\
			"		mov dx, word ptr gpVram+2			"\
			"		add ax, word ptr gpVram				"\
			parm [SI][DI]								\
			modify []									\
			value [DX AX];
			#define doffset(piX, piY) (gpVram+(((gudtTextInfo.screenwidth*(piY))+(piX))<<1))
		#else
			#define doffset(piX, piY) (gpVram+(((gudtTextInfo.screenwidth*(piY))+(piX))<<1))
		#endif
	#endif
#else
	int directvideo=0;
	#define dfar
#endif

/* ----------------------------------------------------------------------------
*/
void dini (void)
{
	#if defined (__DOS__)
		union REGS r;

		if (gudtTextInfo.normattr==7)
		{
			return;
		}
		/* Get current video mode */
		#if (defined(__FLAT__))
			r.h.ah=0xF;
			int386(0x10, &r, &r);
			if ((r.h.al==1) || (r.h.al==3))
			{
				gpVram=0x000B8000UL;
			}
			else
			{
				gpVram=0x000B0000UL;

			}	
		#else
			r.h.ah=0xF;
			int86(0x10, &r, &r);
			if ((r.h.al==1) || (r.h.al==3))
			{
				gpVram=MK_FP(0xB800, 0);
			}
			else
			{
				gpVram=MK_FP(0xB000, 0);
			}
		#endif
		
		/* Get cursor position */
		r.h.ah=0x3;
		r.h.bh=0;
		dint86(0x10, &r, &r);
		gudtTextInfo.curx=r.h.dl;
		gudtTextInfo.cury=r.h.dh;	
	#else
		if (gudtTextInfo.normattr==7)
		{
			return;
		}
	#endif
	gudtTextInfo.currmode=C80;
	gudtTextInfo.screenwidth=80;
	gudtTextInfo.screenheight=25;
	gudtTextInfo.normattr=7;
	gudtTextInfo.attribute=7;

	dwindow(1, 1, 80, 25);
	//_setcursortype(_NOCURSOR);
}


/* ----------------------------------------------------------------------------
*/void dend (void)
{
	#if defined (__DOS__)	
		union REGS r;

		/* Set cursor position */	
		r.h.ah=2;
		r.h.bh=0;
		r.h.dh=gudtTextInfo.curx;
		r.h.dl=gudtTextInfo.cury;
		dint86(0x10, &r, &r);
		//_setcursortype(_NORMALCURSOR);
	#endif
}


/* ----------------------------------------------------------------------------
	Positions cursor in text window.
	gotoxy moves the cursor to the given position in the current text window. 
	If the coordinates are in any way invalid the call to gotoxy is ignored. 
	An example of this is a call to 40,30) when (35,25) is the bottom 
	right position in the window. Neither argument to gotoxy can be zero.
*/
void dgotoxy (int piX, int piY)
{
	#if !defined (__DOS__)
		COORD udtCoord;
	
		dini();
		udtCoord.X=gudtTextInfo.winleft + piX - 2;
		udtCoord.Y=gudtTextInfo.wintop + piY - 2;
		SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), udtCoord);	
	#endif
	gudtTextInfo.curx=piX;
	gudtTextInfo.cury=piY;
	if (gudtTextInfo.curx>(gudtTextInfo.winright-gudtTextInfo.winleft+1))
	{
		gudtTextInfo.curx=1;
		gudtTextInfo.cury++;
	}
	if (gudtTextInfo.cury>(gudtTextInfo.winbottom-gudtTextInfo.wintop+1))
	{
		gudtTextInfo.cury=gudtTextInfo.winbottom-gudtTextInfo.wintop+1;
	}
}


/* ----------------------------------------------------------------------------
	Sets text attributes.
	textattr lets you set both the foreground and background colors in a 
	single call. (Normally, you set the attributes with textcolor and 
	textbackground.)
	This function does not affect any characters currently onscreen; it affects 
	only those characters displayed by functions (such as cprintf) performing 
	text mode, direct video output after this function is called.
*/
void dtextattr (int piAttrib)
{
	#if defined (__DOS__)
		dini();
		gudtTextInfo.attribute=piAttrib;
	#else		
		CONSOLE_SCREEN_BUFFER_INFO udtConsole;

		dini();
		gudtTextInfo.attribute=piAttrib;
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &udtConsole);
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), gudtTextInfo.attribute);
   #endif		
}


/* ----------------------------------------------------------------------------
	Selects new character color in text mode.
	textcolor selects the foreground character color. This function works for 
	the console output functions. newcolor selects the new foreground color. 
	You can set newcolor to an integer as given in the table below, or to one 
	of the symbolic constants defined in conio.h. If you use symbolic constants, 
	you must include conio.h.
	Once you have called textcolor, all subsequent functions using direct video 
	output (such as cprintf) will use newcolor. textcolor does not affect any 
	characters currently onscreen.
*/
void dtextcolor (int piColor)
{
	#if defined (__DOS__)
		dini();
		gudtTextInfo.attribute=(gudtTextInfo.attribute&0xF0)|piColor;
	#else
		CONSOLE_SCREEN_BUFFER_INFO udtConsole;
	
		dini();
		gudtTextInfo.attribute=(gudtTextInfo.attribute&0xF0)|piColor;
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &udtConsole);
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), gudtTextInfo.attribute);
   #endif		
}


/* ----------------------------------------------------------------------------
	Selects new text background color.
	textbackground selects the background color. This function works for 
	functions that produce output in text mode directly to the screen. 
	newcolor selects the new background color. You can set newcolor to an 
	integer from 0 to 7, or to one of the symbolic constants defined in 
	conio.h. If you use symbolic constants, you must include conio.h.
	Once you have called textbackground, all subsequent functions using direct 
	video output (such as cprintf) will use newcolor. textbackground does not 
	affect any characters currently onscreen.
*/
void dtextbackground (int piBackground)
{
	#if defined (__DOS__)
		dini();
		gudtTextInfo.attribute=(gudtTextInfo.attribute&0x0F)|(piBackground<<4);
	#else
		CONSOLE_SCREEN_BUFFER_INFO udtConsole;
	
		dini();
		gudtTextInfo.attribute=(gudtTextInfo.attribute&0x0F)|(piBackground<<4);
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &udtConsole);
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), gudtTextInfo.attribute);
   #endif		
}


/* ----------------------------------------------------------------------------
	Defines active text mode window.
	window defines a text window onscreen. If the coordinates are in any way 
	invalid, the call to window is ignored.
	left and top are the screen coordinates of the upper left corner of the 
	window.
	right and bottom are the screen coordinates of the lower right corner.
	The minimum size of the text window is one column by one line. The default 
	window is full screen, with the coordinates:
	1,1,C,R
	where C is the number of columns in the current video mode, and R is the 
	number of rows.
*/
void dwindow (int piX1, int piY1, int piX2, int piY2)
{
	dini();
	gudtTextInfo.winleft=piX1;
	gudtTextInfo.winright=piX2;
	gudtTextInfo.wintop=piY1;
	gudtTextInfo.winbottom=piY2;
	dgotoxy(1, 1);	
}


/* ----------------------------------------------------------------------------
	Gives horizontal cursor position within window.
	wherex returns the x-coordinate of the current cursor position (within the 
	current text window).
*/
int dwherex (void)
{
	dini();
	return(gudtTextInfo.curx);
}


/* ----------------------------------------------------------------------------
	Gives vertical cursor position within window.
	wherey returns the y-coordinate of the current cursor position (within the 
	current text window).
*/
int dwherey (void)
{
	dini();
	return(gudtTextInfo.cury);
}


/* ----------------------------------------------------------------------------
*/
void dscrolldw (int piX1, int piY1, int piX2, int piY2)
{
	#if defined (__DOS__)
		union REGS r;
	
		dini();
		r.h.ah=7;
		r.h.al=1;
		r.h.bh=gudtTextInfo.attribute;
		r.h.ch=gudtTextInfo.wintop-1;
		r.h.cl=gudtTextInfo.winleft-1;
		r.h.dh=gudtTextInfo.winbottom-1;
		r.h.dl=gudtTextInfo.winright-1;
		dint86(0x10, &r, &r);	
	#endif
}


/* ----------------------------------------------------------------------------
*/
void dscrollup (int piX1, int piY1, int piX2, int piY2)
{
	#if defined (__DOS__)	
		union REGS r;

		dini();
		r.h.ah=6;
		r.h.al=1;
		r.h.bh=gudtTextInfo.attribute;
		r.h.ch=gudtTextInfo.wintop-1;
		r.h.cl=gudtTextInfo.winleft-1;
		r.h.dh=gudtTextInfo.winbottom-1;
		r.h.dl=gudtTextInfo.winright-1;
		dint86(0x10, &r, &r);
	#endif
}


/* ----------------------------------------------------------------------------
	Outputs character to screen.
	putch outputs the character c to the current text window. It is a text mode 
	function performing direct video output to the console. putch does not 
	translate linefeed characters (\n) into carriage-return/linefeed pairs.
	The string is written either directly to screen memory or by way of a BIOS 
	call, depending on the value of the global variable _directvideo.
*/
void dputch (int piChar)
{
	#if defined (__DOS__)	
		register unsigned char dfar *pPtr;

		dini();
		pPtr=doffset(gudtTextInfo.curx+gudtTextInfo.winleft-2, gudtTextInfo.cury+gudtTextInfo.wintop-2);

		*(((unsigned int dfar *) pPtr))=(gudtTextInfo.attribute<<8) | piChar;
		//pPtr++;
		//*pPtr=gudtTextInfo.attribute;
		//pPtr++;
		gudtTextInfo.curx++;
		if (gudtTextInfo.curx>(gudtTextInfo.winright-gudtTextInfo.winleft+1))
		{
			gudtTextInfo.curx=1;
			gudtTextInfo.cury++;
		}
		if (gudtTextInfo.cury>(gudtTextInfo.winbottom-gudtTextInfo.wintop+1))
		{
			gudtTextInfo.cury=gudtTextInfo.winbottom-gudtTextInfo.wintop+1;
			dscrollup(gudtTextInfo.winleft, gudtTextInfo.wintop, gudtTextInfo.winright, gudtTextInfo.winbottom);
		}
	#else
		DWORD iWritten;
	
		dini();
		//dgotoxy(gudtTextInfo.curx, gudtTextInfo.cury);
		WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), (char*)&piChar, 1, &iWritten, NULL);
		gudtTextInfo.curx++;
		if (gudtTextInfo.curx>(gudtTextInfo.winright-gudtTextInfo.winleft+1))
		{
			gudtTextInfo.curx=1;
			gudtTextInfo.cury++;
			if (gudtTextInfo.cury>(gudtTextInfo.winbottom-gudtTextInfo.wintop+1))
			{
				gudtTextInfo.cury=gudtTextInfo.winbottom-gudtTextInfo.wintop+1;
				dmovetext(gudtTextInfo.winleft, gudtTextInfo.wintop+1, gudtTextInfo.winright, gudtTextInfo.winbottom, gudtTextInfo.winleft, gudtTextInfo.wintop);
			}
		}
	#endif
	dgotoxy(gudtTextInfo.curx, gudtTextInfo.cury);
}


/* ----------------------------------------------------------------------------
	Writes a string to the screen.
	cputs writes the null-terminated string str to the current text 
	window. It does not append a newline character.
	The string is written either directly to screen memory or by way of a 
	BIOS call, depending on the value of the global variable _directvideo. 
	Unlike puts, cputs does not translate linefeed characters (\n) into 
	carriage-return/linefeed character pairs (\r\n).
*/
int dputs (char *pacString)
{
	#if defined (__DOS__)	
		register unsigned char dfar *pPtr;
		register unsigned int iCount;
		register char cChar;
		
		dini();
		iCount=0;
		pPtr=doffset(gudtTextInfo.curx+gudtTextInfo.winleft-2, gudtTextInfo.cury+gudtTextInfo.wintop-2);
		while ((cChar=pacString[iCount])!=NULL)
		{
			if (gudtTextInfo.curx>(gudtTextInfo.winright-gudtTextInfo.winleft+1))
			{
				gudtTextInfo.curx=1;
				gudtTextInfo.cury++;
				pPtr=doffset(gudtTextInfo.curx+gudtTextInfo.winleft-2, gudtTextInfo.cury+gudtTextInfo.wintop-2);
			}
			if (gudtTextInfo.cury>(gudtTextInfo.winbottom-gudtTextInfo.wintop+1))
			{
				gudtTextInfo.cury=gudtTextInfo.winbottom-gudtTextInfo.wintop+1;
				dscrollup(gudtTextInfo.winleft, gudtTextInfo.wintop, gudtTextInfo.winright, gudtTextInfo.winbottom);
				pPtr=doffset(gudtTextInfo.curx+gudtTextInfo.winleft-2, gudtTextInfo.cury+gudtTextInfo.wintop-2);
			}
			switch (cChar)
			{
				case '\n':
					gudtTextInfo.cury++;
					gudtTextInfo.curx=1;
					break;
				case '\r':
					gudtTextInfo.curx=1;
					break;
				case '\b':
					gudtTextInfo.curx--;
					pPtr--;
					break;
				default:
					*(((unsigned int dfar *) pPtr))=(gudtTextInfo.attribute<<8) | cChar;
					gudtTextInfo.curx++;
					pPtr+=2;
					break;
			}
			iCount++;
		}
		dgotoxy(gudtTextInfo.curx, gudtTextInfo.cury);
		return(iCount);
	#else
		DWORD iWritten;
		int iCount;
		int iRest;

		iCount=0;
		do
		{
			iRest=gudtTextInfo.winright-gudtTextInfo.winleft-gudtTextInfo.curx+2;
			if (strlen(&pacString[iCount])<iRest)
			{
				iRest=strlen(&pacString[iCount]);
			}
			dgotoxy(gudtTextInfo.curx, gudtTextInfo.cury);
			WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), &pacString[iCount], iRest, &iWritten, NULL);
			iCount+=iRest;
			gudtTextInfo.curx+=iRest;
			if (gudtTextInfo.curx>(gudtTextInfo.winright-gudtTextInfo.winleft+1))
			{
				gudtTextInfo.curx=1;
				gudtTextInfo.cury++;
				if (gudtTextInfo.cury>(gudtTextInfo.winbottom-gudtTextInfo.wintop+1))
				{
					gudtTextInfo.cury=gudtTextInfo.winbottom-gudtTextInfo.wintop+1;
					dmovetext(gudtTextInfo.winleft, gudtTextInfo.wintop+1, gudtTextInfo.winright, gudtTextInfo.winbottom, gudtTextInfo.winleft, gudtTextInfo.wintop);
				}
			}
		}
		while(pacString[iCount]!=0);
		return(iCount);
	#endif
}


/* ----------------------------------------------------------------------------
	Writes formatted output to the screen.
	cprintf accepts a series of arguments, applies to each a format specifier 
	contained in the format string pointed to by format, and outputs the 
	formatted data directly to the current text window on the screen. There 
	must be the same number of format specifiers as arguments. 
	For details details on format specifiers, see printf Format Specifiers.
	The string is written either directly to screen memory or by way of a 
	BIOS call, depending on the value of the global variable _directvideo.
	Unlike fprintf and printf, cprintf does not translate linefeed characters 
	(\n) into carriage-return/linefeed character pairs (\r\n). Tab characters 
	(specified by \t) are not expanded into spaces.
*/
void dprintf (char *pacString, ...)
{
	register char *acAux;	
	va_list argptr;

	va_start(argptr, pacString);
	acAux=malloc(sizeof(int) * 512);
	vsprintf(acAux, pacString, argptr);
	va_end(argptr);
	dputs(acAux);
	free(acAux);
}


/* ----------------------------------------------------------------------------
	Clears to end of line in text window.
	clreol clears all characters from the cursor position to the end of the 
	line within the current text window, without moving the cursor.
*/
void dclreol (void)
{
	#if defined (__DOS__)	
		register unsigned char dfar *pPtr;
		register unsigned int iX;

		dini();
		pPtr=doffset(gudtTextInfo.curx+gudtTextInfo.winleft-2, gudtTextInfo.cury+gudtTextInfo.wintop-2);
		for (iX=gudtTextInfo.curx; iX<(gudtTextInfo.winright-gudtTextInfo.winleft); iX++)
		{
			*(((unsigned int dfar *) pPtr))=(gudtTextInfo.attribute<<8);
			pPtr+=2;
		}
	#else
		COORD udtCoord;
		DWORD iWritten;

		dini();
		udtCoord.X=gudtTextInfo.winleft+gudtTextInfo.curx-1;
		udtCoord.Y=gudtTextInfo.wintop+gudtTextInfo.cury-1;
  
		FillConsoleOutputCharacter(GetStdHandle(STD_OUTPUT_HANDLE), ' ', gudtTextInfo.screenwidth - gudtTextInfo.curx + 1, udtCoord, &iWritten);
		FillConsoleOutputAttribute(GetStdHandle(STD_OUTPUT_HANDLE), gudtTextInfo.attribute, gudtTextInfo.screenwidth - gudtTextInfo.curx + 1, udtCoord, &iWritten);
		//dgotoxy(gudtTextInfo.curx, gudtTextInfo.cury);		
	#endif
}


/* ----------------------------------------------------------------------------
	Clears the text-mode window.
	clrscr clears the current text window and places the cursor in the upper 
	left corner (at position 1,1). 
*/
void dclrscr (void)
{
	#if defined (__DOS__)	
		register unsigned char dfar *pPtr;
		register unsigned int iX, iY;

		dini();
		for (iY=gudtTextInfo.wintop-1; iY<gudtTextInfo.winbottom; iY++)
		{
			pPtr=doffset(gudtTextInfo.winleft-1, iY);		
			for (iX=gudtTextInfo.winleft-1; iX<gudtTextInfo.winright; iX++)
			{
				*(((unsigned int dfar *) pPtr))=(gudtTextInfo.attribute<<8)|' ';
				pPtr+=2;
			}
		}
	#else
		int iY;
		DWORD iWritten;
		COORD udtCoord;

		dini();
		for (iY=0; iY<gudtTextInfo.screenheight; iY++)
		{
			udtCoord.X=gudtTextInfo.winleft;
			udtCoord.Y=gudtTextInfo.wintop+iY;
			FillConsoleOutputAttribute(GetStdHandle(STD_OUTPUT_HANDLE), gudtTextInfo.attribute, gudtTextInfo.screenwidth, udtCoord, &iWritten);
			FillConsoleOutputCharacter(GetStdHandle(STD_OUTPUT_HANDLE), ' ', gudtTextInfo.screenwidth, udtCoord, &iWritten);
		}
	#endif	
	dgotoxy(1, 1);
}


/* ----------------------------------------------------------------------------
	Checks for currently available keystrokes.
	kbhit checks to see if a keystroke is currently available. Any available 
	keystrokes can be retrieved with getch or getche.
*/
int dkbhit (void)
{
	#if defined (__DOS__)	
		union REGS r;

		r.h.ah=1;
		dint86(0x16, &r, &r);
		return(r.h.al);
	#else
		DWORD nEventos;
		INPUT_RECORD *eventos;
		DWORD leidos;
		int retval=0;
		int i;
   
		GetNumberOfConsoleInputEvents(GetStdHandle(STD_INPUT_HANDLE), &nEventos);
		eventos=(INPUT_RECORD *)malloc(nEventos*sizeof(INPUT_RECORD));
		PeekConsoleInput(GetStdHandle(STD_INPUT_HANDLE), eventos, nEventos, &leidos);
		for(i=0;  i < nEventos; i++)
		{
			if(eventos[i].EventType == KEY_EVENT && eventos[i].Event.KeyEvent.bKeyDown)
			{
				retval=1;
			}
			free( eventos );
		}
		return(retval);
	#endif
}


/* ----------------------------------------------------------------------------
	Gets character from keyboard, does not echo to screen.
	getch reads a single character directly from the keyboard, without 
	echoing to the screen.
*/
int dgetch (void)
{
	#if defined (__DOS__)	
		union REGS r;

		r.h.ah=0;
		dint86(0x16, &r, &r);
		return(r.h.al);
	#else
		int car;
		DWORD leidos, modo;
   
		GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &modo);   
		SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), modo & !ENABLE_ECHO_INPUT & !ENABLE_PROCESSED_INPUT);
		ReadConsole(GetStdHandle(STD_INPUT_HANDLE), &car, 1, &leidos, NULL);
		SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), modo);   
		return(car);   
	#endif			
}


/* ----------------------------------------------------------------------------
	Gets character from the keyboard, echoes to screen.
	getche reads a single character from the keyboard and echoes it to the 
	current text window using direct video or BIOS.
*/
int dgetche (void)
{
	register int iKey;
	
	iKey=dgetch();
	dputch(iKey);
	return(iKey);
}


/* ----------------------------------------------------------------------------
	Deletes line in text window.
	delline deletes the line containing the cursor and moves all lines below 
	it one line up. delline operates within the currently active text window.
*/void ddelline (void)
{
}


/* ----------------------------------------------------------------------------
	Copies text from text mode screen to memory.
	gettext stores the contents of an onscreen text rectangle defined by left, 
	top, right, and bottom into the area of memory pointed to by destin.
	All coordinates are absolute screen coordinates not window-relative. The 
	upper left corner is (1,1). gettext reads the contents of the rectangle 
	into memory sequentially from left to right and top to bottom.
	Each position onscreen takes 2 bytes of memory: The first byte is the 
	character in the cell and the second is the cell's video attribute. The 
	space required for a rectangle w columns wide by h rows high is defined as 
	bytes = (h rows) x (w columns) x 2
*/
int dgettext (int piX1, int piY1, int piX2, int piY2, void *pacBuffer)
{
	#if defined (__DOS__)
		register unsigned char dfar *pPtr;
		register unsigned int iX;
		register unsigned int iY;
		
		
		dini();
		for (iY=piY1; iY<=piY2; iY++)
		{
			pPtr=doffset(piX1-1, piY1-1);
			for (iX=piX1; iX<=piX2; iX++)
			{
				*(((unsigned int *) pacBuffer))=*(((unsigned int dfar *) pPtr));
				pPtr+=2;
				((unsigned char *) pacBuffer)+=2;
			}
		}
		return(0);
	#endif
}

/* ----------------------------------------------------------------------------
	Gets text mode video information.
	gettextinfo fills in the text_info structure pointed to by r with the 
	current text video information.
*/
void dgettextinfo (struct gudtTextInfo *pudtInfo)
{
	dini();	
	memcpy(pudtInfo, &gudtTextInfo, sizeof(gudtTextInfo));
}


/* ----------------------------------------------------------------------------
	Selects high-intensity characters.
	highvideo selects high-intensity characters by setting the high-intensity 
	bit of the currently selected foreground color.
	This function does not affect any characters currently onscreen, but does 
	affect those displayed by functions (such as cprintf) that perform direct 
	video, text mode output after highvideo is called.
*/
void dhighvideo (void)
{
	#if defined (__DOS__)	
		dini();
		gudtTextInfo.attribute |= 16;
	#else
		CONSOLE_SCREEN_BUFFER_INFO udtConsole;

		dini();
		gudtTextInfo.attribute |= 16;
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &udtConsole);
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), gudtTextInfo.attribute);
   #endif		
}


/* ----------------------------------------------------------------------------
	Inserts a blank line in the text window.
	insline inserts an empty line in the text window at the cursor position 
	using the current text background color. All lines below the empty one move 
	down one line, and the bottom line scrolls off the bottom of the window.
*/
void dinsline (void)
{
}


/* ----------------------------------------------------------------------------
	Selects low-intensity characters.
	lowvideo selects low-intensity characters by clearing the high-intensity 
	bit of the currently selected foreground color.
	This function does not affect any characters currently onscreen. It affects 
	only those characters displayed by functions that perform text mode, direct 
	console output after this function is called.
*/
void dlowvideo (void)
{
	#if defined (__DOS__)	
		dini();
		gudtTextInfo.attribute &= ~16;
	#else
		CONSOLE_SCREEN_BUFFER_INFO udtConsole;

		dini();
		gudtTextInfo.attribute &= ~16;
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &udtConsole);
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), gudtTextInfo.attribute);
   #endif		
}


/* ----------------------------------------------------------------------------
	Copies text onscreen from one rectangle to another.
	movetext copies the contents of the onscreen rectangle defined by left, 
	top, right, and bottom to a new rectangle of the same dimensions. The new 
	rectangle's upper left corner is position (destleft, desttop).
	All coordinates are absolute screen coordinates. Rectangles that overlap 
	are moved correctly.
	movetext is a text mode function performing direct video output.
*/
int dmovetext (int piX1, int piY1, int piX2, int piY2, int piX, int piY)
{
	#if defined (__DOS__)	
	#else
		COORD udtCoord;
		SMALL_RECT udtRect;
		CHAR_INFO udtInfo;

		udtRect.Left=gudtTextInfo.winleft+piX1-1;
		udtRect.Top=gudtTextInfo.wintop+piY1-1;
		udtRect.Right=gudtTextInfo.winleft+piX2-1;
		udtRect.Bottom=gudtTextInfo.wintop+piX2-1;
		udtCoord.X=gudtTextInfo.winleft+piX-1;
		udtCoord.Y=gudtTextInfo.wintop+piY-1;
		udtInfo.Char.AsciiChar=' ';
		udtInfo.Attributes=gudtTextInfo.attribute;
		ScrollConsoleScreenBuffer(GetStdHandle(STD_OUTPUT_HANDLE), &udtRect, NULL, udtCoord, &udtInfo);
		return(0);
	#endif	
	
}


/* ----------------------------------------------------------------------------
	Selects normal-intensity characters.
	normvideo selects normal characters by returning the text attribute 
	(foreground and background) to the value it had when the program started.
	This function does not affect any characters currently on the screen, only 
	those displayed by functions (such as cprintf) performing direct console 
	output functions after normvideo is called.
*/
void dnormvideo (void)
{
	#if defined (__DOS__)	
		dini();
		gudtTextInfo.attribute=gudtTextInfo.normattr;
	#else
		CONSOLE_SCREEN_BUFFER_INFO udtConsole;

		dini();
		gudtTextInfo.attribute=gudtTextInfo.normattr;
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &udtConsole);
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), gudtTextInfo.attribute);
   #endif		
}


/* ----------------------------------------------------------------------------
	Copies text from memory to the text mode screen.
	puttext writes the contents of the memory area pointed to by source out to 
	the onscreen rectangle defined by left, top, right, and bottom.
	All coordinates are absolute screen coordinates, not window-relative. The 
	upper left corner is (1,1).
	puttext places the contents of a memory area into the defined rectangle 
	sequentially from left to right and top to bottom.
	Each position onscreen takes 2 bytes of memory: The first byte is the 
	character in the cell, and the second is the cell's video attribute. The 
	space required for a rectangle w columns wide by h rows high is defined as
	bytes = (h rows) x (w columns) x 2
	puttext is a text mode function performing direct video output.
*/
int dputtext (int piX1, int piY1, int piX2, int piY2, void *pacBuffer)
{
	#if defined (__DOS__)	
		register unsigned char dfar *pPtr;
		register unsigned int iX;
		register unsigned int iY;
		
		
		dini();
		for (iY=piY1; iY<=piY2; iY++)
		{
			pPtr=doffset(piX1-1, piY1-1);
			for (iX=piX1; iX<=piX2; iX++)
			{
				*(((unsigned int dfar *) pPtr))=*(((unsigned int *) pacBuffer));
				pPtr+=2;
				((unsigned char *) pacBuffer)+=2;
			}
		}
		return(0);
	#else
		int iRes, iCount;
		char *acDest;
		PCHAR_INFO acBuffer;
		COORD udtSize;
		COORD udtDest;
		SMALL_RECT udtRect;

		udtSize.X=piX2-piX1+1;
		udtSize.Y=piY2+piY1+1;
		udtDest.X=0;
		udtDest.Y=0;
		udtRect.Left=gudtTextInfo.winleft+piX1-1;
		udtRect.Top=gudtTextInfo.wintop+piY1-1;
		udtRect.Right=gudtTextInfo.winleft+piX2-1;
		udtRect.Bottom=gudtTextInfo.wintop+piY2-1;
		acDest=(char *) pacBuffer;
		acBuffer=(PCHAR_INFO) malloc(sizeof(CHAR_INFO)*udtSize.X*udtSize.Y);
		for (iCount=0; iCount<udtSize.X*udtSize.Y; iCount++)
		{
		  acBuffer[iCount].Char.AsciiChar=acDest[iCount<<1];
		  acBuffer[iCount].Attributes=(WORD) acDest[(iCount<<1)+1];
		}
		iRes=WriteConsoleOutput(GetStdHandle(STD_OUTPUT_HANDLE), acBuffer, udtSize, udtDest, &udtRect);
		free(acBuffer);
		return(iRes);
	#endif
}


/* ----------------------------------------------------------------------------
	Puts screen in text mode.
*/
void dtextmode (int piMode)
{
}


/* ----------------------------------------------------------------------------
	Selects cursor appearance.
	Sets the cursor type to
	_NOCURSOR	Turns off the cursor
	_NORMALCURSOR	Normal underscore cursor
	_SOLIDCURSOR	Solid block cursor
*/
void _dsetcursortype (int piType)
{
	#if defined (__DOS__)
		union REGS r;
		
		dini();
		/* Set cursor */
		#if (defined(__FLAT__))
			switch (piType)
			{
				case _NOCURSOR:
					r.w.ax=0x1201;
					r.w.bx=0x34;
					int386(0x10, &r, &r);
					break;
				case _SOLIDCURSOR:
					r.w.ax=0x100;
					r.w.bx=0x8;
					int386(0x10, &r, &r);
					break;
				case _NORMALCURSOR:
					r.w.ax=0x100;
					r.w.bx=0x0;
					int386(0x10, &r, &r);
					break;
			}
		#endif
	#endif

}


/* ----------------------------------------------------------------------------
	Reads a string from the console.
	cgets reads a string of characters from the console, storing the string 
	(and the string length) in the location pointed to by str.
	cgets reads characters until it encounters a carriage-return/linefeed 
	(CR/LF) combination, or until the maximum allowable number of characters 
	have been read. If cgets reads a CR/LF combination, it replaces the 
	combination with a \0 (null terminator) before storing the string.
	Before cgets is called, set str[0] to the maximum length of the string to 
	be read. On return, str[1] is set to the number of characters actually 
	read. The characters read start at str[2] and end with a null terminator. 
	Thus, str must be at least str[0] plus 2 bytes long.
*/
char * dgets (char *pacString)
{
	#if defined (__DOS__)	
	#else
		DWORD iLength;
		DWORD iRead, iMode;
   
		iLength=0;
		GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &iMode);   
		SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), iMode & !ENABLE_ECHO_INPUT);
		do
		{
			ReadConsole(GetStdHandle(STD_INPUT_HANDLE), &pacString[2+iLength], 1, &iRead, NULL);
			if (pacString[2+iLength]!='\r')
			{
				dputch(pacString[2+iLength++]);
			}
		}
		while ((iLength<pacString[0]) && (pacString[2+iLength]!='\r'));
		pacString[1]=iLength;
		pacString[2+iLength]=NULL;
		SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), iMode);   
		return(&pacString[2]);
	#endif
}



/* ----------------------------------------------------------------------------
	Scans and formats input from the console.
	cscanf scans a series of input fields one character at a time, reading 
	directly from the console. Then each field is formatted according to a 
	format specifier passed to cscanf in the format string pointed to by format.
	Finally, cscanf stores the formatted input at an address passed to it as 
	an argument following format, and echoes the input directly to the screen. 
	There must be the same number of format specifiers and addresses as there 
	are input fields. 
	Note:	For details on format specifiers, see scanf Format Specifiers.
	cscanf might stop scanning a particular field before it reaches the normal 
	end-of-field (whitespace) character, or it might terminate entirely for a 
	number of reasons. See scanf for a discussion of possible causes.
*/
int dscanf (const char *pacString, ...)
{
	register char *acAux;	
	register int iRes;
	va_list argptr;

	acAux=malloc(sizeof(int) * 512);
	acAux[0]=(sizeof(int) * 512)-4;
	dgets(acAux);
	va_start(argptr, pacString);
	dprintf("#%s#%s#\n", &acAux[2], pacString);
	iRes=sscanf(&acAux[2], pacString, argptr);
	va_end(argptr);
	free(acAux);
	return(iRes);
}


/* ----------------------------------------------------------------------------
	Reads a password.
	getpass reads a password from the system console after prompting with 
	the null-terminated string prompt and disabling the echo. A pointer is 
	returned to a null-terminated string of up to eight characters (not 
	counting the null-terminator).
*/
char * dgetpass (const char *pacPrompt)
{
	#if defined (__DOS__)	
	#else
		static char acPass[PASS_MAX+1];
		DWORD iLength;
		DWORD iRead, iMode;

		dputs(pacPrompt);
		GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &iMode);   
		SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), iMode & !ENABLE_ECHO_INPUT);
		
		iLength=0;
		do
		{
			ReadConsole(GetStdHandle(STD_INPUT_HANDLE), &acPass[iLength], 1, &iRead, NULL);
			if (acPass[iLength]!='\r')
			{
				iLength++;
			}
		}
		while ((iLength<=PASS_MAX) && (acPass[iLength]!='\r'));
		acPass[iLength]=NULL;
		SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), iMode);   
		return(acPass);
	#endif	
}


/* ----------------------------------------------------------------------------
	Pushes a character back to the keyboard buffer.
	ungetch pushes the character ch back to the console, causing ch to be the 
	next character read. The ungetch function fails if it is called more than 
	once before the next read.
*/
int dungetch (int pcChar)
{
	#if defined (__DOS__)	
	#else
		INPUT_RECORD udtInput[2];
		DWORD iWritten;
   
		udtInput[0].EventType=KEY_EVENT;
		udtInput[0].Event.KeyEvent.bKeyDown=TRUE;
		udtInput[0].Event.KeyEvent.wRepeatCount=1;
		udtInput[0].Event.KeyEvent.wVirtualKeyCode=0;
		udtInput[0].Event.KeyEvent.wVirtualScanCode=0;
		udtInput[0].Event.KeyEvent.uChar.AsciiChar=pcChar;
		udtInput[0].Event.KeyEvent.dwControlKeyState=0;
		udtInput[1].EventType=KEY_EVENT;
		udtInput[1].Event.KeyEvent.bKeyDown=FALSE;
		udtInput[1].Event.KeyEvent.wRepeatCount=1;
		udtInput[1].Event.KeyEvent.wVirtualKeyCode=0;
		udtInput[1].Event.KeyEvent.wVirtualScanCode=0;
		udtInput[1].Event.KeyEvent.uChar.AsciiChar=pcChar;
		udtInput[1].Event.KeyEvent.dwControlKeyState=0;
		if (WriteConsoleInput(GetStdHandle(STD_INPUT_HANDLE), udtInput, 2, &iWritten))
		{
			return(pcChar);
		}
		else
		{
			return(EOF);   
		}
	#endif	
}
