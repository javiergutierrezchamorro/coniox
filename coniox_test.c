/* ----------------------------------------------------------------------------------------------------------------- */
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "coniox.h"


#if !defined(__TURBOC__)
	#define random(num)	(int)(((long)rand()*(num))/(RAND_MAX+1))
#endif


/* ----------------------------------------------------------------------------------------------------------------- */
int pause(void);
void init(void);
void finish(void);
void info(void);
void chars(void);
void windows(void);
void getputtexts(void);



/* ----------------------------------------------------------------------------------------------------------------- */
int main(void)
{
	init();
	info();
	//chars();
	//windows();
	getputtexts();
	finish();
	return(0);
}


/* ----------------------------------------------------------------------------------------------------------------- */
int pause(void)
{
	int c;
	textcolor(YELLOW);
	_wscroll = 0;
	cputs("Press any key to continue or [ESC] to exit");
	_wscroll = 1;
	c = getch();
	cputs("\r\n");
	return(c);
}


/* ----------------------------------------------------------------------------------------------------------------- */
void init(void)
{
	struct text_info ti;
	int i;
	char line[255];
	
	//Optional: Set window caption
	#ifdef _CONIOX_H_
		#if UNICODE
			coniox_init(L"coniox Test");
		#else
			coniox_init("coniox Test");
		#endif
	#endif

	//textmode(C4350);
	_setcursortype(_NOCURSOR);
	textattr(BLUE*16+WHITE);
	clrscr();
	
	gettextinfo(&ti);
	
	/* Title bar */
	memset(line, 0, sizeof(line));
	memset(line, ' ', ti.screenwidth);
	textattr(CYAN*16+BLACK);
	gotoxy(1, 1);
	cputs(line);
	
	/* Status bar */
	gotoxy(1, ti.screenheight);
	_wscroll = 0;
	cputs(line);
	_wscroll = 1;
	
	/* Title text */
	strcpy(line, "CONIO DEMO v5.00 - (c) Copyright 2022 Javier Gutierrez Chamorro (Guti)");
	i = (int) strlen(line);
	gotoxy((ti.screenwidth - i) >> 1, 1);
	cputs(line);
	
}

/* ----------------------------------------------------------------------------------------------------------------- */
void finish(void)
{
	textmode(LASTMODE);
	textattr(7);
	clrscr();
}


/* ----------------------------------------------------------------------------------------------------------------- */
void info(void)
{
	struct text_info ti;
	
	gettextinfo(&ti);
	window(1, 2, ti.screenwidth, ti.screenheight - 1);
	textbackground(BLUE);
	clrscr();
	
	textcolor(YELLOW);
	cputs("CONIO INFORMATION\r\n");
	textcolor(WHITE);
	cprintf("- winleft: %d\r\n", ti.winleft);
	cprintf("- wintop: %d\r\n", ti.wintop);
	cprintf("- winright: %d\r\n", ti.winright);
	cprintf("- winbottom: %d\r\n", ti.winbottom);
	cprintf("- attribute: %d\r\n", ti.attribute);
	cprintf("- normattr: %d\r\n", ti.normattr);
	cprintf("- currmode: %d\r\n", ti.currmode);
	cprintf("- screenheight: %d\r\n", ti.screenheight);
	cprintf("- screenwidth: %d\r\n", ti.screenwidth);
	cprintf("- curx: %d\r\n", ti.curx);
	cprintf("- cury: %d\r\n", ti.cury);
	cprintf("- directvideo: %d\r\n", directvideo);
	cprintf("- _wscroll: %d\r\n", _wscroll);
	pause();
}


/* ----------------------------------------------------------------------------------------------------------------- */
void chars(void)
{
	int i;
	struct text_info ti;
	
	gettextinfo(&ti);
	window(1, 2, ti.screenwidth, ti.screenheight - 1);
	textbackground(BLUE);
	clrscr();
	
	textcolor(YELLOW);
	cputs("WRITE CHARS\r\n");

	/* ToDo: Calculate with window size */
	textcolor(WHITE);
	for (i = 0; i < 1000; i++)
	{
		putch((i & 63) + 'A');
	}
	cputs("\r\n");
	pause();

	textcolor(WHITE);
	for (i = 0; i < 30000; i++)
	{
		putch((i & 63) + 'A');
	}
	cputs("\r\n");
	pause();
}


/* ----------------------------------------------------------------------------------------------------------------- */
void windows(void)
{
	int i;
	int x1, y1, x2, y2;
	struct text_info ti;

	gettextinfo(&ti);

	while (!kbhit())
	{
		x1 = random(ti.screenwidth - 1) + 1;
		y1 = random(ti.screenheight - 3) + 2;
		x2 = random(ti.screenwidth - 1) + 1;
		y2 = random(ti.screenheight - 3) + 2;
		if (x1 > x2)
		{
			i = x2;
			x2 = x1;
			x1 = i;
		}
		if (y1 > y2)
		{
			i = y2;
			y2 = y1;
			y1 = i;
		}


		window(1, 2, ti.screenwidth, ti.screenheight - 1);
		textbackground(BLUE);
		clrscr();
		
		window(x1, y1, x2, y2);
		textcolor(x1 & 15);
		textbackground(y1 & 15);
		clrscr();
		
		for(x1 = 1; x1 < 100; x1++)
		{
			cprintf("Text output with automatic scrolling %d ", x1);
		}
	}
	getch();
}


/* ----------------------------------------------------------------------------------------------------------------- */
void getputtexts(void)
{
	unsigned char *buf;
	int i, j, size;
	struct text_info ti;

	gettextinfo(&ti);
	size = ti.screenwidth * (ti.screenheight - 2) * 2;
	buf = malloc(size);
	/*
	buf[0] = 'A';
	buf[1] = 7;
	buf[2] = 'B';
	buf[3] = 15;
	puttext(1, 1, 2, 1, buf);
	gettext(1, 1, 2, 1, buf);
	*/
	gotoxy(1, 1);
	textattr(7);
	cputs("ABCD");
	gettext(1, 2, ti.screenwidth, ti.screenheight - 1, buf);
	window(1, 1, ti.screenwidth, ti.screenheight);
	textattr(7);
	clrscr();
	puttext(1, 2, ti.screenwidth, ti.screenheight - 1, buf);
	for (j = 0; j < 255; j++)
	{
		for (i = 0; i < size; i+=2)
		{
			if (buf[i] > 0)
			{
				buf[i]--;
			}
			else
			{
				buf[i+1] = 0;
			}
		}
		puttext(1, 2, ti.screenwidth, ti.screenheight - 1, buf);
	}
	if (buf)
	{
		free(buf);
	}
}




int demo2(void)
{
	int x1 = 1, y1 = 1, x2 = 1, y2 = 1, x3 = 1, y3 = 1, x4 = 1, y4 = 1;
	int i;
	struct text_info ti;
	
	
	//Optional: Set window caption
	#ifdef _CONIOX_H_
		#if UNICODE
			coniox_init(L"coniox Test");
		#else
			coniox_init("coniox Test");
		#endif
	#endif
	
	directvideo = 1;
	textattr(7);
	clrscr();
	
	gettextinfo(&ti);

	/* Window 1 */
	window (2, 2, ti.screenwidth / 2 - 4, ti.screenheight / 2 - 4);
	textattr(1*16+15);
	clrscr();

	/* Window 2 */
	window (ti.screenwidth / 2 + 2, 2, ti.screenwidth / 2 - 4, ti.screenheight / 2 - 4);
	textattr(2*16+14);
	clrscr();
	
	/* Window 3 */
	window (2, ti.screenheight / 2 + 2, ti.screenwidth - 4, ti.screenheight - 4);
	textattr(3*16+15);
	clrscr();

	/* Window 4 */
	window (ti.screenwidth / 2 + 2, ti.screenheight / 2 + 2, ti.screenwidth - 4, ti.screenheight - 4);
	textattr(4*16+15);
	clrscr();
	
	return(0);
	
	i = 0;
	while(!kbhit())
	{
		/* Window 1 */
		window (2, 2, ti.screenwidth / 2 - 4, ti.screenheight / 2 - 4);
		textattr(1*16+15);
		gotoxy(x1, y1);
		cprintf("Content of WINDOW 1 %u", i);
		x1=wherex();
		y1=wherey();
		
		/* Window 2 */
		window (ti.screenwidth / 2 + 2, 2, ti.screenwidth / 2 - 4, ti.screenheight / 2 - 4);
		textattr(2*16+14);
		gotoxy(x2, y2);
		cprintf("Content of WINDOW 2 %u", i);
		x2=wherex();
		y2=wherey();

		/* Window 3 */
		window (2, ti.screenheight / 2 + 2, ti.screenwidth - 4, ti.screenheight - 4);
		textattr(3*16+15);
		gotoxy(x3, y3);
		cprintf("Content of WINDOW 3 %u", i);
		x3=wherex();
		y3=wherey();
		
		/* Window 4 */
		window (ti.screenwidth / 2 + 2, ti.screenheight / 2 + 2, ti.screenwidth - 4, ti.screenheight - 4);
		textattr(4*16+15);
		gotoxy(x4, y4);
		cprintf("Content of WINDOW 4 %u", i);
		x4=wherex();
		y4=wherey();
		
		i++;
	}
	getch();
	window(1, 1, ti.screenwidth, ti.screenheight);
	textattr(7);
	clrscr();
}

int demo3(void)
{
	unsigned int i, j;
	unsigned long int stop, start;
	char *background;
	char *win;
	struct text_info ti;


	//Optional: Set window caption
	#ifdef _CONIOX_H_
		#if UNICODE
			coniox_init(L"coniox Test");
		#else
			coniox_init("coniox Test");
		#endif
	#endif

	directvideo=1;

	textmode(C80);
	
	clrscr();
	gettextinfo(&ti);
	cprintf("Screen width: %d\r\n", ti.screenwidth);
	cprintf("Screen height: %d\r\n", ti.screenheight);
	getch();
	
	_setcursortype(_NOCURSOR);
	
	for (i = 0; i < (unsigned int) ti.screenheight * ti.screenwidth; i++)
	{
		putch(254);
	}
	getch();

	_wscroll = 0;
	for (i = 0; i< 80; i++)
	{
		putch(254);
	}
	_wscroll = 1;
	getch();

	start = clock();
	window(5, 5, 75, 20);
	textcolor(15);
	textbackground(BLUE);
	clrscr();
	for (i = 0; i != 50000U; i++)
	{
		putch(254);
	}
	stop = clock();
	textcolor(YELLOW);
	cputs("\r\nThose was 50000 254 (þ) chars using putch\r\n");
	cprintf( "It took %u clocks (%u clocks are a second)\r\n", stop - start, CLOCKS_PER_SEC);
	cputs( "---- Press any key to continue ----" );
	getch();
	
	window(1, 1, 80, 25);
	textattr(7);
	clrscr();
	window (5, 5, 75, 20);
	textcolor(15);
	textbackground(5);
	clrscr();
	for (i=0; i<1000; i++)
	{
		cputs("All functions support automatic scrolling on screen or window ");
	}
	textcolor(14);
	cputs("\r\nAll functions support automatic scrolling on screen or window\r\n");
	cputs( "---- Press any key to continue ----" );
	getch();
	


	start = clock();
	window(6, 6, 76, 21);
	textcolor(WHITE);
	textbackground(GREEN);
	clrscr();
	for ( i = 0; i != 1000; i++ )
	{
		cputs("Some cputs here and there\r\n" );
	}
	stop = clock();
	textcolor(YELLOW);
	cputs("\r\nThose was 1000 cputs\r\n");
	cprintf( "It took %u clocks (%u clocks are a second)\r\n", stop - start, CLOCKS_PER_SEC );
	cputs( "---- Press any key to continue ----" );
	getch();

	
	start = clock();
	window(7, 7, 77, 22);
	textcolor(WHITE);
	textbackground(RED);
	clrscr();
	for ( i = 0; i != 1000; i++ )
	{
		cprintf("Line %u with cprintf writting here\r\n", i);
	}
	stop = clock();
	textcolor(YELLOW);
	cputs("\r\nThose was 1000 cprintf\r\n");
	cprintf("It took %u clocks (%u clocks are a second)\r\n", stop - start, CLOCKS_PER_SEC);
	cputs( "---- Press any key to continue ----" );
	getch();


	background = malloc(8000);
	win = malloc(8000);

	gettext(7, 7, 77, 22, win);
	for (j = 1; j < 10; j++)
	{
		for (i = 1; i < 10; i++)
		{
			gettext(i, j, i + 70, j + 15, background);
			puttext(i, j, i + 70, j + 15, win);
			delay(100);
			puttext(i, j, i + 70, j + 15, background);
		}
	}
	free(win);
	free(background);
	getch();

	//window(1, 1, ti.screenwidth, ti.screenheight);
	window(1, 1, 80, 25);
	textattr(LIGHTGRAY);
	clrscr();
	_setcursortype(_NORMALCURSOR);
	getch();
	
	return(0);
}

