#include <stdlib.h>
#include <time.h>

#include "coniox.h"

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

int main(void)
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
	
	for (i = 0; i < ti.screenheight * ti.screenwidth; i++)
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

