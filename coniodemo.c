/* ----------------------------------------------------------------------------------------------------------------- */
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "coniox.h"


//#if !defined(__TURBOC__)
#if !defined(random)
	#define random(num)	(short)(((long)rand()*(num))/(RAND_MAX+1))
#endif


/* ----------------------------------------------------------------------------------------------------------------- */
void pause(void);
void init(void);
void finish(void);
void info(void);
void chars(void);
void windows(void);
void speed(void);
void inputs(void);
void movetexts(void);
void getputtexts(void);



/* ----------------------------------------------------------------------------------------------------------------- */
int main(void)
{
	init();
	info();
	chars();
	movetexts();
	windows();
	speed();
	inputs();
	getputtexts();
	finish();
	return(0);
}


/* ----------------------------------------------------------------------------------------------------------------- */
void pause(void)
{
	textcolor(YELLOW + BLINK);
	_wscroll = 0;
	cputs("---- Press any key to continue ----");
	getch();
	_wscroll = 1;
	cputs("\r\n");
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
			coniox_init(L"coniox demo");
		#else
			coniox_init("coniox demo");
		#endif
	#endif
	
	directvideo=0;

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
	strcpy(line, "CONIO DEMO v5.00 - (c) Copyright 2025 Javier Gutierrez Chamorro (Guti)");
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
	ungetch('T'); ungetch('h'); ungetch('a'); ungetch('n'); ungetch('k'); ungetch(' '); ungetch('y'); ungetch('o'); ungetch('u');
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
	#ifdef _CONIOX_H_
		cprintf("- Version: %d.%02d\r\n", coniox_version / 100, coniox_version % 100);
	#endif
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

	window(1, 1, ti.screenwidth, ti.screenheight);
	gotoxy(1, ti.screenheight);
	textattr(CYAN * 16 + BLACK);
	cputs("Press any key to continue...");

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
	window(1, 1, ti.screenwidth, ti.screenheight);
	gotoxy(1, ti.screenheight);
	textattr(CYAN * 16 + BLACK);
	cputs("                            ");
}


/* ----------------------------------------------------------------------------------------------------------------- */
void movetexts(void)
{
	int i;
	struct text_info ti;

	gettextinfo(&ti);

	for (i = 1; i < ti.screenheight; i++)
	{
		movetext(1, i, ti.screenwidth, i, 1, ti.screenheight - i + 1);
		delay(100);
	}
	window(1, 1, ti.screenwidth, ti.screenheight);
	gotoxy(1, 2);
	pause();
}



/* ----------------------------------------------------------------------------------------------------------------- */
void speed(void)
{
	unsigned int i, j;
	clock_t stop, start;
	char* background;
	char* win;
	struct text_info ti;

	gettextinfo(&ti);
	window(1, 2, ti.screenwidth, ti.screenheight - 1);
	textbackground(BLUE);
	clrscr();

	start = clock();
	textcolor(WHITE);
	clrscr();
	for (i = 0; i < 50000U; i++)
	{
		putch(254);
	}
	stop = clock();
	textcolor(YELLOW);
	cputs("\r\nThose was 50000 254 (þ) chars using putch\r\n");
	cprintf("It took %ld clocks (%d clocks are a second)\r\n", stop - start, (int) CLOCKS_PER_SEC);
	pause();

	window(1, 2, ti.screenwidth, ti.screenheight - 1);
	textbackground(BLUE);
	clrscr();

	window(5, 5, ti.screenwidth - 5, ti.screenheight - 5);
	textcolor(WHITE);
	textbackground(MAGENTA);
	clrscr();
	for (i = 0; i < 1000; i++)
	{
		cputs("All functions support automatic scrolling on screen or window ");
	}
	textcolor(YELLOW);
	cputs("\r\nAll functions support automatic scrolling on screen or window\r\n");
	pause();


	start = clock();
	window(6, 6, ti.screenwidth - 6, ti.screenheight - 6);
	textcolor(WHITE);
	textbackground(GREEN);
	clrscr();
	for (i = 0; i < 1000; i++)
	{
		cputs("Some cputs here and there\r\n");
	}
	stop = clock();
	textcolor(YELLOW);
	cputs("\r\nThose was 1000 cputs\r\n");
	cprintf("It took %ld clocks (%d clocks are a second)\r\n", stop - start, (int) CLOCKS_PER_SEC);
	pause();


	start = clock();
	window(7, 7, ti.screenwidth - 7, ti.screenheight - 7);
	textcolor(WHITE);
	textbackground(RED);
	clrscr();
	for (i = 0; i < 1000; i++)
	{
		cprintf("Line %u with cprintf writting here\r\n", i);
	}
	stop = clock();
	textcolor(YELLOW);
	cputs("\r\nThose was 1000 cprintf\r\n");
	cprintf("It took %ld clocks (%d clocks are a second)\r\n", stop - start, (int) CLOCKS_PER_SEC);
	pause();

	background = malloc(gettextsize(7, 7, ti.screenwidth - 7, ti.screenheight - 7));
	if (!background)
	{
		return;
	}
	win = malloc(gettextsize(7, 7, ti.screenwidth - 7, ti.screenheight - 7));
	if (!win)
	{
		free(background);
		return;
	}

	gettext(7, 7, ti.screenwidth - 7, ti.screenheight - 7, win);
	for (j = 1; j < 14; j++)
	{
		for (i = 7; i < 14; i++)
		{
			gettext(i, j, i + ti.screenwidth - 14, j + ti.screenheight - 14, background);
			puttext(i, j, i + ti.screenwidth - 14, j + ti.screenheight - 14, win);
			delay(100);
			puttext(i, j, i + ti.screenwidth - 14, j + ti.screenheight - 14, background);
		}
	}
	free(win);
	free(background);
	pause();
}



/* ----------------------------------------------------------------------------------------------------------------- */
void getputtexts(void)
{
	unsigned char *buf;
	int i, j, size;
	struct text_info ti;

	gettextinfo(&ti);

	size = gettextsize(1, 2, ti.screenwidth, ti.screenheight - 1);
	buf = malloc(size);
	if (!buf)
	{
		return;
	}
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
	free(buf);
}


/* ----------------------------------------------------------------------------------------------------------------- */
void inputs(void)
{
	int ch;
	char s[PRINTFBUF_SIZE + 1];
	char p[PASS_MAX + 1];
	struct text_info ti;

	gettextinfo(&ti);
	window(1, 2, ti.screenwidth, ti.screenheight - 1);
	textbackground(BLUE);
	clrscr();
	_setcursortype(_NORMALCURSOR);

	textcolor(YELLOW);
	cputs("CONIO INPUTS\r\n");

	textcolor(WHITE);
	cputs("Enter a character that will be read with echo: ");
	textcolor(LIGHTGRAY);
	ch = getche();

	textcolor(WHITE);
	cprintf("\r\nChar was '%c'\r\n\r\n", ch);
	cputs("Enter a character that will be read without echo: ");
	ch = getch();
	cprintf("\r\nChar was '%c'\r\n\r\n", ch);

	textcolor(WHITE);
	cputs("Enter a string that will be read with echo: ");
	textcolor(LIGHTGRAY);
	s[0] = PRINTFBUF_SIZE - 2;
	cgets(s);
	cprintf("\r\n%d characters read. String was '%s'\r\n\r\n", s[1], &s[2]);

	textcolor(WHITE);
	cputs("Enter a password that will be read with echo: ");
	textcolor(LIGHTGRAY);
	strcpy(p, getpass("\r\nPassword: "));
	cprintf("\r\nPassword was '%s'\r\n\r\n", p);

	_setcursortype(_NOCURSOR);
	pause();
}





void demo3(void)
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
	for (i = 0; i < 50000U; i++)
	{
		putch(254);
	}
	stop = clock();
	textcolor(YELLOW);
	cputs("\r\nThose was 50000 254 (þ) chars using putch\r\n");
	cprintf( "It took %u clocks (%u clocks are a second)\r\n", stop - start, (int) CLOCKS_PER_SEC);
	cputs("---- Press any key to continue ----");
	getch();
	
	window(1, 1, 80, 25);
	textattr(7);
	clrscr();
	window(5, 5, ti.screenwidth - 5, ti.screenheight - 5);
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
	for ( i = 0; i < 1000; i++ )
	{
		cputs("Some cputs here and there\r\n" );
	}
	stop = clock();
	textcolor(YELLOW);
	cputs("\r\nThose was 1000 cputs\r\n");
	cprintf( "It took %u clocks (%u clocks are a second)\r\n", stop - start, (int) CLOCKS_PER_SEC);
	cputs( "---- Press any key to continue ----" );
	getch();

	
	start = clock();
	window(7, 7, 77, 22);
	textcolor(WHITE);
	textbackground(RED);
	clrscr();
	for ( i = 0; i < 1000; i++ )
	{
		cprintf("Line %u with cprintf writting here\r\n", i);
	}
	stop = clock();
	textcolor(YELLOW);
	cputs("\r\nThose was 1000 cprintf\r\n");
	cprintf("It took %u clocks (%u clocks are a second)\r\n", stop - start, (int) CLOCKS_PER_SEC);
	cputs( "---- Press any key to continue ----" );
	getch();


	background = malloc(gettextsize(7, 7, 77, 22));
	if (!background)
	{
		return;
	}
	win = malloc(gettextsize(7, 7, 77, 22));
	if (!win)
	{
		free(background);
		return;
	}
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
}





/* ----------------------------------------------------------------------------------------------------------------- */
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
	window(2, 2, ti.screenwidth / 2 - 4, ti.screenheight / 2 - 4);
	textattr(1 * 16 + 15);
	clrscr();

	/* Window 2 */
	window(ti.screenwidth / 2 + 2, 2, ti.screenwidth / 2 - 4, ti.screenheight / 2 - 4);
	textattr(2 * 16 + 14);
	clrscr();

	/* Window 3 */
	window(2, ti.screenheight / 2 + 2, ti.screenwidth - 4, ti.screenheight - 4);
	textattr(3 * 16 + 15);
	clrscr();

	/* Window 4 */
	window(ti.screenwidth / 2 + 2, ti.screenheight / 2 + 2, ti.screenwidth - 4, ti.screenheight - 4);
	textattr(4 * 16 + 15);
	clrscr();

	return(0);

	i = 0;
	while (!kbhit())
	{
		/* Window 1 */
		window(2, 2, ti.screenwidth / 2 - 4, ti.screenheight / 2 - 4);
		textattr(1 * 16 + 15);
		gotoxy(x1, y1);
		cprintf("Content of WINDOW 1 %u", i);
		x1 = wherex();
		y1 = wherey();

		/* Window 2 */
		window(ti.screenwidth / 2 + 2, 2, ti.screenwidth / 2 - 4, ti.screenheight / 2 - 4);
		textattr(2 * 16 + 14);
		gotoxy(x2, y2);
		cprintf("Content of WINDOW 2 %u", i);
		x2 = wherex();
		y2 = wherey();

		/* Window 3 */
		window(2, ti.screenheight / 2 + 2, ti.screenwidth - 4, ti.screenheight - 4);
		textattr(3 * 16 + 15);
		gotoxy(x3, y3);
		cprintf("Content of WINDOW 3 %u", i);
		x3 = wherex();
		y3 = wherey();

		/* Window 4 */
		window(ti.screenwidth / 2 + 2, ti.screenheight / 2 + 2, ti.screenwidth - 4, ti.screenheight - 4);
		textattr(4 * 16 + 15);
		gotoxy(x4, y4);
		cprintf("Content of WINDOW 4 %u", i);
		x4 = wherex();
		y4 = wherey();

		i++;
	}
	getch();
	window(1, 1, ti.screenwidth, ti.screenheight);
	textattr(7);
	clrscr();
}

