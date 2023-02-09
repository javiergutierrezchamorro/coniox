#include <bios.h>
#include <time.h>
#include <conio.h>
#include "dirio.h"


void main (void)
{
	unsigned int i;
	unsigned long int stop, start;

	dini();

	dwindow(1, 1, 80, 25);
	dtextattr(7);
	dclrscr();
	start=clock();
	dwindow (5, 5, 75, 20);
	dtextcolor(15);
	dtextbackground(1);
	dclrscr();
	for (i=0; i!=50000U; i++)
		dputch(254);
	stop=clock();
	dtextcolor(14);
	dputs("\r\nEsto han sido 50000 car teres 254 (þ) con el dputch\r\n");
	dprintf("Ha tardado %u clocks (18.2 clocks son un segundo\r\n", stop-start);
	dputs("---- Pulsa una tecla para continuar ----");
	dgetch();

	dwindow(1, 1, 80, 25);
	dtextattr(7);
	dclrscr();
	start=clock();
	dwindow (5, 5, 75, 20);
	dtextcolor(15);
	dtextbackground(2);
	dclrscr();
	for (i=0; i!=1000; i++)
		dputs("Unos cuandos dputs por aqu¡, otro por all \r\n");
	stop=clock();
	dtextcolor(14);
	dputs("Esto han sido 1000 dputs\r\n");
	dprintf("Ha tardado %u clocks (18.2 clocks son un segundo\r\n", stop-start);
	dputs("---- Pulsa una tecla para continuar ----");
	dgetch();

	dwindow(1, 1, 80, 25);
	dtextattr(7);
	dclrscr();
	start=clock();
	dwindow (5, 5, 75, 20);
	dtextcolor(15);
	dtextbackground(4);
	dclrscr();
	for (i=0; i!=1000; i++)
		dprintf("Linea %u del dprintf que escribo aqu¡\r\n", i);
	stop=clock();
	dtextcolor(14);
	dputs("Esto han sido 1000 dprintf\r\n");
	dprintf("Ha tardado %u clocks (18.2 clocks son un segundo\r\n", stop-start);
	dputs("---- Pulsa una tecla para continuar ----");
	dgetch();

	dwindow(1, 1, 80, 25);
	dtextattr(7);
	dclrscr();
	dwindow (5, 5, 75, 20);
	dtextcolor(15);
	dtextbackground(5);
	dclrscr();
	for (i=0; i!=25; i++)
		dputs("Como puedes ver las funciones tienen scroll autom tico ");
	dtextcolor(14);
	dputs("\r\nComo puedes ver las funciones tienen scroll autom tico\r\n");
	dputs("---- Pulsa una tecla para continuar ----");
	dgetch();

	window(1, 1, 80, 25);
	textattr(7);
	clrscr();
	start=clock();
	window (5, 5, 75, 20);
	textcolor(15);
	textbackground(1);
	clrscr();
	for (i=0; i!=50000U; i++)
		putch(254);
	stop=clock();
	textcolor(14);
	cputs("\r\nEsto han sido 50000 car teres 254 (þ) con el putch\r\n");
	cprintf("Ha tardado %u clocks (18.2 clocks son un segundo\r\n", stop-start);
	cputs("---- Pulsa una tecla para continuar ----");
	getch();

	window(1, 1, 80, 25);
	textattr(7);
	clrscr();
	start=clock();
	window (5, 5, 75, 20);
	textcolor(15);
	textbackground(2);
	clrscr();
	for (i=0; i!=1000; i++)
		cputs("Unos cuandos cputs por aqu¡, otro por all \r\n");
	stop=clock();
	textcolor(14);
	cputs("Esto han sido 1000 cputs\r\n");
	cprintf("Ha tardado %u clocks (18.2 clocks son un segundo\r\n", stop-start);
	cputs("---- Pulsa una tecla para continuar ----");
	getch();

	window(1, 1, 80, 25);
	textattr(7);
	clrscr();
	start=clock();
	window (5, 5, 75, 20);
	textcolor(15);
	textbackground(4);
	clrscr();
	for (i=0; i!=1000; i++)
		cprintf("Linea %u del cprintf que escribo aqu¡\r\n", i);
	stop=clock();
	textcolor(14);
	cputs("Esto han sido 1000 cprintf\r\n");
	cprintf("Ha tardado %u clocks (18.2 clocks son un segundo\r\n", stop-start);
	cputs("---- Pulsa una tecla para continuar ----");
	getch();

	dwindow(1, 1, 80, 25);
	dtextattr(7);
	dclrscr();
	_setcursortype(_NORMALCURSOR);
}
