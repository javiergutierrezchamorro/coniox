#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <bios.h>
#include <time.h>
#include "dirio.h"

void main (void)
{
	unsigned int i;
	unsigned long int stop, start;
	char *acBuffer;


	dini();
	//return;

	_dsetcursortype(_NOCURSOR);
	
	start=clock();
	dwindow (5, 5, 75, 20);
	dtextcolor(15);
	dtextbackground(1);
	dclrscr();

	for (i=0; i!=50000U; i++)
	{
		dputch(254);
	}
	stop=clock();
	dtextcolor(14);
	dputs("\r\nEsto han sido 50000 car teres 254 (þ) con el dputch\r\n");
	dprintf("Ha tardado %u clocks (18.2 clocks son un segundo)\r\n", stop-start);
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
	dgetch();
	for (i=0; i!=1000; i++)
	{
		dputs("Unos cuandos dputs por aqu¡, otro por all \r\n");
	}
	stop=clock();
	dtextcolor(14);
	dputs("Esto han sido 1000 dputs\r\n");
	dprintf("Ha tardado %u clocks (18.2 clocks son un segundo)\r\n", stop-start);
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
	{
		dprintf("Linea %u del dprintf que escribo aqu¡\r\n", i);
	}
	stop=clock();
	dtextcolor(14);
	dputs("Esto han sido 1000 dprintf\r\n");
	dprintf("Ha tardado %u clocks (18.2 clocks son un segundo)\r\n", stop-start);
	dputs("---- Pulsa una tecla para continuar ----");
	dgetch();

	dwindow(1, 1, 80, 25);
	dtextattr(7);
	dclrscr();
	dwindow (5, 5, 75, 20);
	dtextcolor(15);
	dtextbackground(5);
	dclrscr();
	for (i=0; i!=50; i++)
	{
		dputs("Como puedes ver las funciones tienen scroll autom tico ");
	}
	dtextcolor(14);
	dputs("\r\nComo puedes ver las funciones tienen scroll autom tico\r\n");
	dputs("---- Pulsa una tecla para continuar ----");
	dgetch();
	acBuffer=(char *) malloc(4000);
	dgettext(1, 1, 80, 25, acBuffer);
	dwindow(1, 1, 80, 25);
	dtextattr(7);
	dclrscr();
	dgetch();
	dputtext(1, 1, 80, 25, acBuffer);
	free(acBuffer);
	dgetch();
	dwindow(1, 1, 80, 25);
	dtextattr(7);
	dclrscr();
	dend();
}
