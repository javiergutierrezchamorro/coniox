#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include <bios.h>
#include <conio.h>
#include <time.h>
#include <dir.h>
#include <\tc\c\cars\conxlib.h>
ock

/* ------------------- Obtiene informaci¢n del COUNTRY -------------------- */
void pascal getcountry(struct scountry *scountry)
{
	union REGS r;
	struct SREGS s;

	r.x.ax=0x3800;
	r.x.dx=FP_OFF(scountry);
	s.ds=FP_SEG(scountry);
	int86x(0x21, &r, &r, &s);
	scountry->code=r.x.bx;
}

/* --------------------- Cambia la paleta por defecto --------------------- */
void _fastcall setpal (unsigned char e)
{
	register unsigned int i;
	unsigned char *auxpal;
	union REGS r;
	unsigned char defpal[]={ 	0x00, 0x00, 0x00, /* 00 */
														0x00, 0x00, 0x2A,	/* 01 */
														0x00, 0x2A, 0x00, /* 02 */
														0x00, 0x2A, 0x2A, /* 03 */
														0x2A, 0x00, 0x00, /* 04 */
														0x2A, 0x00, 0x2A, /* 05 */
														0x2A, 0x2A, 0x00, /* 06 */
														0x2A, 0x2A, 0x2A, /* 07 */
														0x00, 0x00, 0x15, /* 08 */
														0x00, 0x00, 0x3F,	/* 09 */
														0x00, 0x2A, 0x15, /* 10 */
														0x00, 0x2A, 0x3F, /* 11 */
														0x2A, 0x00, 0x15, /* 12 */
														0x2A, 0x00, 0x3F, /* 13 */
														0x2A, 0x2A, 0x15, /* 14 */
														0x2A, 0x2A, 0x3F};/* 15 */
	unsigned char newpal[]={ 	0x00, 0x00, 0x00, /* 00 */
														0x00, 0x00, 0x24,	/* 01 */
														0x00, 0x24, 0x04, /* 02 */
														0x00, 0x22, 0x28, /* 03 */
														0x24, 0x00, 0x00, /* 04 */
														0x28, 0x08, 0x2A, /* 05 */
														0x30, 0x30, 0x08, /* 06 */
														0x2B, 0x2B, 0x2B, /* 07 */
														0x00, 0x00, 0x1D, /* 08 */
														0x04, 0x08, 0x3C,	/* 09 */
														0x00, 0x28, 0x0E, /* 10 */
														0x00, 0x1F, 0x3F, /* 11 */
														0x28, 0x04, 0x10, /* 12 */
														0x2C, 0x08, 0x3F, /* 13 */
														0x30, 0x2A, 0x10, /* 14 */
														0x30, 0x30, 0x3F};/* 15 */
	/* Permitir parpadeo */
	r.x.ax=0x1003;
	r.h.bl=1;
	int86(0x10, &r, &r);
	if (e==0)
		auxpal=defpal;
	else
		auxpal=newpal;
	for (i=0; i<=15; i++)
  {
		r.x.ax=0x1010;
		r.x.bx=i;
    r.h.dh=*auxpal; /* Red */
		auxpal++;
		r.h.ch=*auxpal; /* Green */
		auxpal++;
		r.h.cl=*auxpal; /* Blue */
		auxpal++;
		int86(0x10, &r, &r);
	}
}

/* ----------------- Borra la pantalla con efecto de empuje ---------------- */
unsigned int _fastcall ckey (void)
{
  register unsigned int tecla;
  union REGS r;

  /* Mouse show */
  r.x.ax=1;
  int86(0x33, &r, &r);
  do
  {
    watch();
    tecla=bioskey(1);
  }
  while (tecla==0);
  tecla=bioskey(0);
  /* Mouse hidde */
  r.x.ax=2;
  int86(0x33, &r, &r);
  return(tecla);
}

/* ----------------- Borra la pantalla con efecto de empuje ---------------- */
void _fastcall empuja (void)
{
	register unsigned char i;
  union REGS r;

	/* Borra la pantalla con efecto de empuje */
	for (i=0; i!=13; i++)
	{
		/* Cuadro 1 */
    r.x.ax=0x0701;   /* 6 arriba, 7 abajo */ /* Linea a desplazar */
    r.h.bh=0x07;     /* Atributo */
    r.x.cx=0x0000;   /* Y or¡gen */ /* X or¡gen */
    r.x.dx=0x0C0D;   /* Y destino */ /* X destino */
    int86(0x10, &r, &r);
		/* Cuadro 2 */
		r.x.ax=0x0601;   /* 6 arriba, 7 abajo */ /* Linea a desplazar */
		r.h.bh=7;        /* Atributo */
		r.x.cx=0x000E;   /* Y or¡gen */ /* X or¡gen */
		r.x.dx=0x0C3F;   /* Y destino */ /* X destino */
		int86(0x10, &r, &r);
		/* Cuadro 3 */
    r.x.ax=0x0701;   /* 6 arriba, 7 abajo */ /* Linea a desplazar */
    r.h.bh=7;        /* Atributo */
    r.x.cx=0x0040;   /* Y or¡gen */ /* X or¡gen */
    r.x.dx=0x0C4F;   /* Y destino */ /* X destino */
		int86(0x10, &r, &r);
		/* Cuadro 4 */
		r.x.ax=0x0601;   /* 6 arriba, 7 abajo */ /* Linea a desplazar */
    r.h.bh=7;        /* Atributo */
    r.x.cx=0x0D00;   /* Y or¡gen */ /* X or¡gen */
    r.x.dx=0x180D;   /* Y destino */ /* X destino */
		int86(0x10, &r, &r);
		/* Cuadro 5 */
    r.x.ax=0x0701;   /* 6 arriba, 7 abajo */ /* Linea a desplazar */
    r.h.bh=7;        /* Atributo */
    r.x.cx=0x0D0E;   /* Y or¡gen */ /* X or¡gen */
    r.x.dx=0x183F;   /* Y destino */ /* X destino */
    int86(0x10, &r, &r);
		/* Cuadro 6 */
    r.x.ax=0x0601;   /* 6 arriba, 7 abajo */ /* Linea a desplazar */
		r.h.bh=7;        /* Atributo */
    r.x.cx=0x0D40;   /* Y or¡gen */ /* X or¡gen */
    r.x.dx=0x184F;   /* Y destino */ /* X destino */
    int86(0x10, &r, &r);
    delay (50);
	}
}


/* ------------ Escribe en pantalla el estado de los bloqueos ------------- */
void _fastcall watch (void)
{
	unsigned char oldx, oldy;
	struct time t;

  oldx=wherex();
	oldy=wherey();
	gettime(&t);
	gotoxy(63, 25);
	textattr(7*16+0);
	cprintf("            %2u", t.ti_hour);
	textattr(7*16+0+128);
	putch(':');
	textattr(7*16+0);
	cprintf("%02u", t.ti_min);
  gotoxy(oldx, oldy);
}

/* ------------ Escribe en pantalla el estado de los bloqueos ------------- */
void _fastcall blocks (void)
{
  register unsigned char fun;
  unsigned char oldx, oldy;

  oldx=wherex();
  oldy=wherey();
	fun=bioskey(2);		/* 0040:0017 */
  textattr(7*16+0);
	gotoxy(64, 25);
	if (fun & 128)
    cputs("INS ");
  else
    cputs("    ");
	if (fun & 64)
    cputs("CAP ");
  else
    cputs("    ");
	if (fun & 32)
    cputs("NUM ");
  else
    cputs("    ");
	if (fun & 16)
    cputs("SCR ");
  else
    cputs("    ");
  gotoxy(oldx, oldy);
}

/* ----------------------- Escribe un texto centrado ---------------------- */
void pascal cputsc (unsigned char x1, unsigned char x2, unsigned char y, char *str)
{
	register unsigned char x;

	x=(((x2-x1+1)>>1)-(strlen(str)>>1)+x1);
	gotoxy(x, y);
	cputs(str);
}

/* -------------------- Consigue una cadena por teclado ------------------- */
void pascal getstr (unsigned char x, unsigned char y, unsigned char atr, char *str, char len)
{
	register char i;
	register unsigned int tec;
	char aux[80];

	textattr(7*16+0);
  gotoxy(1, 25);
	cputs("Input a valid string                                                ");
	_setcursortype(_SOLIDCURSOR);
	memset(aux, 0, 80);
	memset(aux, 32, len+1);
	textattr(atr);
	gotoxy(x, y);
	cputs(aux);
	strcpy(aux, str);
	i=strlen(aux);
	str[i]=0;
	gotoxy(x, y);
	cputs(aux);
	do
	{
		blocks();
		tec=bioskey(0);
    textattr(atr);
		if (tec==0x0E08)
		{
			if (i>0)
			{
				cputs("\b \b");
				i--;
				aux[i]=0;
			}
		}
		else if (tec==0x011B)
		{
		}
		else if (tec==0x1C0D)
		{
			strcpy(str, aux);
			i=strlen(aux)+1;
		}
		else if (tec==0x4B00)
		{
			if (i>0)
			{
				gotoxy(wherex()-1, wherey());
				i--;
			}
		}
		else if (tec==0x4D00)
		{
			if (i<len)
			{
				gotoxy(wherex()+1, wherey());
				i++;
			}
		}
		else
		{
			if (i<len)
			{
				putch((char) tec);
				aux[i]=(char) tec;
				i++;
				aux[i]=0;
			}
		}
	}
	while ((tec!=0x1C0D) && (tec!=0x011B));
	if (tec!=0x011B)
		str[i]=0;
  _setcursortype(_NOCURSOR);
}


/* ------------------- Consigue un password por teclado ------------------- */
void pascal getpw (unsigned char x, unsigned char y, unsigned char atr, char *pass, char len)
{
	register char i;
	register unsigned int tec;

	textattr(7*16+0);
  gotoxy(1, 25);
	cputs("Type password                                                       ");
	_setcursortype(_NOCURSOR);
	memset(pass, 0, len+2);
	memset(pass, 32, len+1);
	textattr(atr);
	gotoxy(x, y);
	cputs(pass);
	pass[0]=0;
	i=0;
	gotoxy(x, y);
	do
	{
		blocks();
		tec=bioskey(0);
    textattr(atr);
		if (tec==0x0E08)
		{
			if (i>0)
			{
				cputs("\b \b");
				i--;
			}
		}
		else if ((tec==0x1C0D) || (tec==0x011B))
		{
		}
		else
		{
			if (i<len)
			{
				putch('*');
				pass[i]=(char) tec;
				i++;
			}
		}
	}
	while ((tec!=0x1C0D) && (tec!=0x011B));
	pass[i]=0;
	_setcursortype(_NOCURSOR);
}

/* ------------------------ Hace un men£ de botones ----------------------- */
unsigned char buttons (unsigned char x1, unsigned char y1, unsigned char atr0, unsigned char atr1, char menu[][11], char desc[][71], unsigned char noptions, unsigned char separation)
{
  register char x;
	register unsigned int tecla;
	char y;
	union REGS r;

	textattr(atr0);
  for (x=0; x!=noptions; x++)
  {
    gotoxy(x1+x*separation, y1);
		cputs(menu[x]);
  }
  textattr(2*16+15);
	gotoxy(x1, y1);
	cputs(menu[0]);
  textattr(7*16+0);
  gotoxy(1, 25);
  cputs(desc[0]);
	x=0;
	do
	{
		/* Mouse show */
		r.x.ax=1;
		int86(0x33, &r, &r);
		do
		{
			watch();
			tecla=bioskey(1);
			if (tecla!=0)
				tecla=bioskey(0);
			/* Get mouse */
			r.x.ax=3;
			int86(0x33, &r, &r);
			r.x.cx=(r.x.cx>>3)+1;
			r.x.dx=(r.x.dx>>3)+1;
			/* Left button */
			if ((r.x.bx==1) && (r.x.dx==y1))
			{
				y=(r.x.cx-x1)/separation;
				if ((y>=0) && (y<noptions))
				{
					tecla=0x1C0D;
					x=y;
				}
			}
			/* Right button */
			else if (r.x.bx==2)
				tecla=0x3B00;
		}
		while ((tecla!=0x4B00) && (tecla!=0x4D00) && (tecla!=0x011B) &&
					 (tecla!=0x1C0D) && (tecla!=0x3B00));
		/* Mouse hidde */
		r.x.ax=2;
		int86(0x33, &r, &r);
		textattr(atr0);
		gotoxy(x1+x*separation, y1);
		cputs(menu[x]);
		if (tecla==0x4B00)				/* Left */
			x--;
		else if (tecla==0x4D00)		/* Right */
			x++;
		if (x>noptions-1)
			x=0;
		else if (x<0)
			x=noptions-1;
		textattr(atr1);
		gotoxy(x1+x*separation, y1);
		cputs(menu[x]);
		textattr(7*16+0);
		gotoxy(1, 25);
		cputs(desc[x]);
	}
	while ((tecla!=0x1C0D) && (tecla!=0x011B) && (tecla!=0x3B00));
	if (tecla==0x011B)
		return(0);
	else if (tecla==0x3B00)
		return(255);
	else
		return(x);
}

/* ------------------------------- Hace un men£ --------------------------- */
unsigned char pascal menu (unsigned char x1, unsigned char y1, unsigned char atr0, unsigned char atr1, char menu[][21], char desc[][71], unsigned char noptions, unsigned char separation)
{
  register char y;
	register unsigned int tecla;
	char x, l;
	union REGS r;

	l=strlen(menu[0])-1;
	textattr(atr0);
  for (y=0; y!=noptions; y++)
  {
    gotoxy(x1, y1+y*separation);
		cputs(menu[y]);
  }
  textattr(atr1);
	gotoxy(x1, y1);
	cputs(menu[0]);
  textattr(7*16+0);
  gotoxy(1, 25);
  cputs(desc[0]);
	y=0;
	do
	{
		/* Mouse show */
		r.x.ax=1;
		int86(0x33, &r, &r);
		do
		{
			watch();
			tecla=bioskey(1);
			if (tecla!=0)
				tecla=bioskey(0);
			/* Get mouse */
			r.x.ax=3;
			int86(0x33, &r, &r);
			r.x.cx=(r.x.cx>>3)+1;
			r.x.dx=(r.x.dx>>3)+1;
			/* Left button */
			if ((r.x.bx==1) && (r.x.cx>=x1) && (r.x.cx<=(x1+l)))
			{
				x=(r.x.dx-y1)/separation;
        if ((x>=0) && (x<noptions))
				{
					tecla=0x1C0D;
          y=x;
				}
			}
			/* Right button */
			else if (r.x.bx==2)
				tecla=0x3B00;
		}
		while ((tecla!=0x4800) && (tecla!=0x5000) && (tecla!=0x4700) &&
					 (tecla!=0x4F00) && (tecla!=0x011B) && (tecla!=0x1C0D) &&
					 (tecla!=0x3B00));
		/* Mouse hidde */
		r.x.ax=2;
		int86(0x33, &r, &r);
		textattr(atr0);
		gotoxy(x1, y1+y*separation);
		cputs(menu[y]);
		if (tecla==0x4800)        /* Up */
			y--;
		else if (tecla==0x5000)   /* Dw */
			y++;
		else if (tecla==0x4700)   /* Org */
			y=0;
		else if (tecla==0x4F00)   /* End */
			y=noptions-1;
		if (y>noptions-1)
			y=0;
		else if (y<0)
			y=noptions-1;
		textattr(atr1);
		gotoxy(x1, y1+y*separation);
		cputs(menu[y]);
		textattr(7*16+0);
		gotoxy(1, 25);
		cputs(desc[y]);
	}
	while ((tecla!=0x1C0D) && (tecla!=0x011B) && (tecla!=0x3B00));
	if (tecla==0x011B)
		return(0);
	else if (tecla==0x3B00)
		return(255);
	else
		return(y);
}

/* ----------------------- Hace un men£ de opciones ----------------------- */
unsigned char pascal option (unsigned char x1, unsigned char y1, unsigned char atr0, unsigned char atr1, char menu[][21], char desc[][71], char *sel, unsigned char noptions, unsigned char separation, unsigned char activado)
{
	register char y;
	register unsigned int tecla;
	char x;
	union REGS r;

	textattr(atr0);
	for (y=0; y!=noptions; y++)
	{
		gotoxy(x1, y1+y*separation);
		cprintf("[%c] %s", sel[y], menu[y]);
	}
	if (activado==0)
		return(0);
	textattr(atr1);
	gotoxy(x1, y1);
	cprintf("[%c] %s", sel[0], menu[0]);
	textattr(7*16+0);
	gotoxy(1, 25);
	cputs(desc[0]);
	y=0;
	do
	{
		/* Mouse show */
		r.x.ax=1;
		int86(0x33, &r, &r);
		do
		{
			watch();
			tecla=bioskey(1);
			if (tecla!=0)
				tecla=bioskey(0);
			/* Get mouse */
			r.x.ax=3;
			int86(0x33, &r, &r);
			r.x.cx=(r.x.cx>>3)+1;
			r.x.dx=(r.x.dx>>3)+1;
			/* Left button */
			if ((r.x.bx==1) && (r.x.cx>=(x1+1)) && (r.x.cx<=(x1+3)))
			{
				x=(r.x.dx-y1)/separation;
				if ((x>=0) && (x<noptions))
				{
					y=x;
					tecla=0x5200;
				}
			}
			/* Right button */
			else if (r.x.bx==2)
				tecla=0x3B00;
		}
		while ((tecla!=0x0F00) && (tecla!=0x0F09) && (tecla!=0x3920) &&
					 (tecla!=0x5200) && (tecla!=0x011B) && (tecla!=0x1C0D) &&
					 (tecla!=0x3B00));
		/* Mouse hidde */
		r.x.ax=2;
		int86(0x33, &r, &r);
		if (tecla==0x0F00)        /* Shift-Tab */
			y--;
		else if (tecla==0x0F09)   /* Tab */
			y++;
		else if ((tecla==0x3920) || (tecla==0x5200))  /* Space/Ins */
		{
			if (sel[y]=='X')
				sel[y]=' ';
			else
				sel[y]='X';
			y++;
		}
		if (y>noptions-1)
			y=0;
		else if (y<0)
			y=noptions-1;
		textattr(atr0);
		for (x=0; x!=noptions; x++)
		{
			gotoxy(x1, y1+x*separation);
			cprintf("[%c] %s", sel[x], menu[x]);
		}
		textattr(atr1);
		gotoxy(x1, y1+y*separation);
		cprintf("[%c] %s", sel[y], menu[y]);
		textattr(7*16+0);
		gotoxy(1, 25);
		cputs(desc[y]);
	}
	while ((tecla!=0x1C0D) && (tecla!=0x011B) && (tecla!=0x3B00));
	if (tecla==0x3B00)
		return(255);
	else
		return(0);
}

/* ------------------------ Hace un men£ de botones ----------------------- */
unsigned char pascal property (unsigned char x1, unsigned char y1, unsigned char atr0, unsigned char atr1, char menu[][21], char desc[][71], char *sel, unsigned char noptions, unsigned char separation, unsigned char activado)
{
	register char y;
	register unsigned int tecla;
	char x;
	union REGS r;

	textattr(atr0);
	for (y=0; y!=noptions; y++)
	{
		gotoxy(x1, y1+y*separation);
		cprintf("(%c) %s", sel[y], menu[y]);
	}
	if (activado==0)
		return(0);
	textattr(atr1);
	gotoxy(x1, y1);
	cprintf("(%c) %s", sel[0], menu[0]);
	textattr(7*16+0);
	gotoxy(1, 25);
	cputs(desc[0]);
	y=0;
	do
	{
		/* Mouse show */
		r.x.ax=1;
		int86(0x33, &r, &r);
		do
		{
			watch();
			tecla=bioskey(1);
			if (tecla!=0)
				tecla=bioskey(0);
			/* Get mouse */
			r.x.ax=3;
			int86(0x33, &r, &r);
			r.x.cx=(r.x.cx>>3)+1;
			r.x.dx=(r.x.dx>>3)+1;
			/* Left button */
			if ((r.x.bx==1) && (r.x.cx>=(x1+1)) && (r.x.cx<=(x1+3)))
			{
				x=(r.x.dx-y1)/separation;
				if ((x>=0) && (x<noptions))
				{
					y=x;
					tecla=0x5200;
				}
			}
			/* Right button */
			else if (r.x.bx==2)
				tecla=0x3B00;
		}
		while ((tecla!=0x0F00) && (tecla!=0x0F09) && (tecla!=0x3920) &&
					 (tecla!=0x5200) && (tecla!=0x011B) && (tecla!=0x1C0D) &&
					 (tecla!=0x3B00));
		/* Mouse hidde */
		r.x.ax=2;
		int86(0x33, &r, &r);
		if (tecla==0x0F00)        /* Shift-Tab */
			y--;
		else if (tecla==0x0F09)   /* Tab */
			y++;
		else if ((tecla==0x3920) || (tecla==0x5200))  /* Space/Ins */
		{
			memset(sel, ' ', noptions);
			sel[y]='ù';
		}
		if (y>noptions-1)
			y=0;
		else if (y<0)
			y=noptions-1;
		textattr(atr0);
		for (x=0; x!=noptions; x++)
		{
			gotoxy(x1, y1+x*separation);
			cprintf("(%c) %s", sel[x], menu[x]);
		}
		textattr(atr1);
		gotoxy(x1, y1+y*separation);
		cprintf("(%c) %s", sel[y], menu[y]);
		textattr(7*16+0);
		gotoxy(1, 25);
		cputs(desc[y]);
	}
	while ((tecla!=0x1C0D) && (tecla!=0x011B) && (tecla!=0x3B00));
	if (tecla==0x3B00)
		return(255);
	else
		return(0);
}

/* -------------------------- Dibuja una ventana --------------------------- */
void pascal box (unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2, unsigned char atr, char *tipo, char *titulo, unsigned char sombra)
{
	register unsigned char x, y;
	unsigned int c1, c2;
	char l1[82], l2[82], l3[82], tmp[330];

	/* Shadow */
	if (sombra&1)
	{
		gettext(x1+2, y2+1, x2+2, y2+1, tmp);
		c2=(((x2+2)-(x1+2)+1)*((y2+1)-(y2+1)+1))<<1;
		for (c1=1; c1<=c2; c1+=2)
			tmp[c1]=8;
		puttext(x1+2, y2+1, x2+2, y2+1, tmp);
		gettext(x2+1, y1+1, x2+2, y2+1, tmp);
		c2=(((x2+2)-(x2+1)+1)*((y2+1)-(y1+1)+1))<<1;
		for (c1=1; c1<=c2; c1+=2)
			tmp[c1]=8;
		puttext(x2+1, y1+1, x2+2, y2+1, tmp);
	}
	/* No window */
	if ((sombra&4)==0)
	{
		x=(x2-x1);
		memset(&l1, 0, x+2);
		memset(&l2, 0, x+2);
		memset(&l3, 0, x+2);
		l1[0]=tipo[0];
		memset(&l1[1], tipo[2], x-1);
		l1[x]=tipo[1];
		l2[0]=tipo[4];
		memset(&l2[1], 32, x-1);
		l2[x]=tipo[4];
		l3[0]=tipo[5];
		memset(&l3[1], tipo[3], x-1);
		l3[x]=tipo[6];
		textattr(atr);
		gotoxy(x1, y1);
		cputs(l1);
		if (titulo[0]!=0)
		{
			sprintf(tmp, " %s ", titulo);
			cputsc(x1, x2, y1, tmp);
		}
		/* Close */
		if (sombra&2)
		{
			gotoxy(x1+2, y1);
			cputs("[þ]");
		}
		for (y=y1+1; y!=y2; y++)
		{
			gotoxy(x1, y);
			cputs(l2);
		}
		gotoxy(x1, y2);
		cputs(l3);
	}
}

/* ------------------- Rutina de control de errores ----------------------- */
void interrupt error (unsigned int bp, unsigned int di, unsigned int si, unsigned int ds, unsigned int es, unsigned int dx, unsigned int cx, unsigned int bx, unsigned int ax)
{
	register unsigned int err;
	unsigned char oldx, oldy;
  char *oldb;
	char butt[][11]= {"[Retry]", "[Fail]", "[Ignore]", "[Abort]"};
  char desc[][71]= {"Retry the process that have caused the error                        ",
                    "Fail the process that have caused the error                         ",
										"Ignore the ocurred error and continue execution                     ",
										"Terminate the execution of the program                              "};

	err=di & 0x00FF;
	oldb=malloc(2000);
	if (oldb==0)
		return;
	oldx=wherex();
	oldy=wherey();
  gettext(8, 10, 70, 16, oldb);
	box(8, 10, 68, 15, 4*16+15, "É»ÍÍºÈ¼" , "Error", 1);
	cputsc(8, 68, 11, "A fatal error ocurred");
  if (err==0x00)
		cputsc(8, 68, 12, "Disk is write protected");
  else if (err==0x01)
		cputsc(8, 68, 12, "Unknown unit");
  else if (err==0x02)
		cputsc(8, 68, 12, "Drive is not ready");
  else if (err==0x03)
		cputsc(8, 68, 12, "Unknown command");
  else if (err==0x04)
		cputsc(8, 68, 12, "Data error (Bad CRC)");
  else if (err==0x05)
		cputsc(8, 68, 12, "Bad request structure length");
  else if (err==0x06)
		cputsc(8, 68, 12, "Seek error");
  else if (err==0x07)
		cputsc(8, 68, 12, "Unkown media type");
  else if (err==0x08)
		cputsc(8, 68, 12, "Sector not found");
	else if (err==0x09)
		cputsc(8, 68, 12, "Printer is out of paper");
  else if (err==0x0A)
		cputsc(8, 68, 12, "Write fault");
  else if (err==0x0B)
		cputsc(8, 68, 12, "Read fault");
  else if (err==0x0C)
		cputsc(8, 68, 12, "General failure");
	err=buttons (14, 14, 4*16+15, 2*16+15, butt, desc, 4, 14);
	gotoxy(oldx, oldy);
  puttext(8, 10, 70, 16, oldb);
  free(oldb);
  if (err==0)
		ax=1;
	else if (err==1)
		ax=3;
	else if (err==2)
		ax=0;
	else if (err==3)
		ax=2;
#pragma warn -par
}
#pragma warn .par

/* -------------------------- Visualiza un texto --------------------------- */
unsigned char pascal view (unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2, unsigned char atr0, unsigned char atr1, char buffer[][61], unsigned char ultimo, char *titulo)
{
	register unsigned char z, c;
	unsigned char oldx, oldy;
	int f, ini;
	unsigned int tecla;
	char *oldb;
	union REGS r;
	char tmp[70];

	oldb=malloc(textsize(x1, y1, x2+2, y2+1));
	if (oldb==0)
		return(0);
	oldx=wherex();
	oldy=wherey();
	gettext(x1, y1, x2+2, y2+1, oldb);
	for (f=ultimo; f!=ultimo+y2-y1; f++)
		buffer[f][0]=0;
	textattr(7*16+0);
	gotoxy(1, 25);
	cputs("ENTER and ESC close the window; arrows, and pages move              ");
	box(x1, y1, x2, y2, atr0, "É»ÍÍºÈ¼" , titulo, 3);
	gotoxy(x2, y1+1);
	putch(24);
	gotoxy(x2, y2-1);
	putch(25);
	textattr(atr0);
	for (z=y1+2; z!=y2-1; z++)
	{
		gotoxy (x2, z);
		putch('°');
	}
	z=0;
	ini=0;
	do
	{
		textattr(atr0);
		gotoxy (x2, z+y1+2);
		putch('±');
		c=y1+1;
		textattr(atr1);
		for (f=ini; f!=ini+y2-y1-1; f++)
		{
			memset(tmp, 0, 61);
			memset(tmp, 32, 60);
			memcpy (tmp, &buffer[f], strlen(buffer[f]));
			gotoxy(x1+2, c);
			cputs(tmp);
			c++;
		}
		/* Mouse show */
		r.x.ax=1;
		int86(0x33, &r, &r);
		do
		{
			watch();
			tecla=bioskey(1);
			if (tecla!=0)
				tecla=bioskey(0);
			/* Get mouse */
			r.x.ax=3;
			int86(0x33, &r, &r);
			r.x.cx=(r.x.cx>>3)+1;
			r.x.dx=(r.x.dx>>3)+1;
			/* Left button */
			if (r.x.bx==1)
			{
				if (r.x.cx==x2)
				{
					if (r.x.dx==y1+1)
						tecla=0x4800;
					else if (r.x.dx==y2-1)
						tecla=0x5000;
					else if ((r.x.dx>=y1+2) && (r.x.dx<=y2-2) && (r.x.cx==x2))
					{
						ini=(r.x.dx-y1-2)*ultimo/(y2-y1-4);
						tecla=0x1312;
					}
				}
				else if ((r.x.cx==x1+3) && (r.x.dx==y1))
					tecla=0x011B;
			}
			/* Right button */
			else if (r.x.bx==2)
				tecla=0x3B00;
		}
		while ((tecla!=0x4800) && (tecla!=0x5000) && (tecla!=0x4900) &&
					 (tecla!=0x5100) && (tecla!=0x4700) && (tecla!=0x4F00) &&
					 (tecla!=0x011B) && (tecla!=0x1C0D) && (tecla!=0x1312) &&
					 (tecla!=0x3B00));
		/* Mouse hidde */
		r.x.ax=2;
		int86(0x33, &r, &r);
		if (tecla==0x4800)       /* Up */
			ini--;
		else if (tecla==0x5000)  /* Dw */
			ini++;
		else if (tecla==0x4900)  /* PgUp */
			ini-=(y2-y1)-2;
		else if (tecla==0x5100)  /* PgDw */
			ini+=(y2-y1)-2;
		else if (tecla==0x4700)  /* Org */
			ini=0;
		else if (tecla==0x4F00)  /* End */
			ini=ultimo;
		if (ini<0)
			ini=0;
		else if (ini>ultimo)
			ini=ultimo;
		textattr(atr0);
		gotoxy(x2, z+y1+2);
		putch('°');
		z=(ini*(y2-y1-4))/ultimo;
		gotoxy(x2, z+y1+2);
		putch('±');
	}
	while ((tecla!=0x011B) && (tecla!=0x1C0D));
	puttext(x1, y1, x2+2, y2+1, oldb);
	free(oldb);
	gotoxy(oldx, oldy);
	if (tecla==0x3B00)
		return(255);
	else
		return(0);
}

/* ---------------------------- Cargar la ayuda ---------------------------- */
unsigned char pascal help (unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2, unsigned char atr0, unsigned char atr1, char *nombre, char *tema, char buffer[][61])
{
	register unsigned int ultimo;
	FILE *f;
	char key[50], titulo[80], tmp[256];

  textattr(7*16+0+128);
  gotoxy(1, 25);
	cputs("Loading help...                                                     ");
	strcpy(titulo, tema);
	f=fopen(nombre, "rb");
	if (f==0)
	{
		strcpy(&buffer[0], "Help is not available.");
		ultimo=1;
		view(x1, y1, x2, y2, atr0, atr1, buffer, ultimo, "Help: Not available");
		return(0);
	}
	sprintf(key, "@%s", tema);
	/* Buscar el tema */
	do
	{
		fgets(tmp, 255, f);
		*strchr(tmp, '\r')=0;
	}
  while ((strcmp(key, tmp)!=0) && (!feof(f)));
	/* Cargar el tema */
	if (strcmp(key, tmp)==0)
	{
		ultimo=0;
		do
		{
			fgets(tmp, 255, f);
			*strchr(tmp, '\r')=0;
			memcpy(&buffer[ultimo], tmp, 61);
			ultimo++;
		}
    while ((tmp[0]!='@') && (!feof(f)));
		fclose(f);
		ultimo--;
		sprintf(tmp, "Help: %s", titulo);
    ultimo=view(x1, y1, x2, y2, atr0, atr1, buffer, ultimo, tmp);
    if (ultimo==255)
      helpindex(x1, y1, atr1, 16*2+15, nombre, buffer);
		return(1);
	}
	else
	{
		fclose(f);
		strcpy(&buffer[0], "There is no help for this topic.");
		ultimo=1;
		view(x1, y1, x2, y2, atr0, atr1, buffer, ultimo, "Help: Nothing");
		return(0);
	}
}

/* ------------------- Entorno para pillar ficheros ----------------------- */
unsigned char _fastcall select (unsigned char *sdisk, char *sdir, struct sfiles *files)
{
	register int i, j;
	unsigned char disk, ndisks, olddrive;
	unsigned char oldx, oldy;
	unsigned int ultimo, tecla;
	int fin;
	struct
	{
		unsigned sec: 5;
		unsigned min: 6;
		unsigned hour:5;
	} ff_ftime;
	struct
	{
		unsigned day: 5;
		unsigned mon: 4;
		unsigned year:7;
	} ff_fdate;
	FILE *f;
	char *oldb;
	union REGS r;
	struct ffblk ffblk;
	char mask[15];
	char tmp1[80], tmp2[80], dir[80], olddir[80];

	_setcursortype(_NOCURSOR);
	oldb=malloc(textsize(1, 2, 80, 24));
	if (oldb==0)
		return(0);
	oldx=wherex();
	oldy=wherey();
	gettext(1, 2, 80, 24, oldb);
	i=0;
	tecla=((peekb(0x40, 0x10) & 192)>>6)+1;
	for (j=0; j!=tecla; j++)
	{
		r.h.ah=8;
		r.h.dl=j;
		int86(0x13, &r, &r);
		sprintf(files[i].name, "\x10%c\x11", i+'A');
		strcpy(files[i].size, "\xF0   DRIVE   \xF0");
		files[i].date[0]=0;
		files[i].time[0]=0;
		if (r.h.bl<=2)
			strcpy(files[i].comment, "5¬ Floppy Disk");
		else if ((r.h.bl>=3) && (r.h.bl<=5))
			strcpy(files[i].comment, "3« Floppy Disk");
		else
			strcpy(files[i].comment, "Disk Drive");
		i++;
	}
	if (tecla==1)
	{
		sprintf(files[i].name, "\x10%c\x11", i+'A');
		strcpy(files[i].size, "\xF0   DRIVE   \xF0");
		files[i].date[0]=0;
		files[i].time[0]=0;
		strcpy(files[i].comment, "Virtual Floppy Disk");
		i++;
	}
	tecla=peekb(0x40, 0x75);
	for (j=0; j!=tecla; j++)
	{
		sprintf(files[i].name, "\x10%c\x11", i+'A');
		strcpy(files[i].size, "\xF0   DRIVE   \xF0");
		files[i].date[0]=0;
		files[i].time[0]=0;
		strcpy(files[i].comment, "Hard Disk");
		i++;
	}
	olddrive=getdisk();
	getcurdir(0, olddir);
	sprintf(tmp2, "\\%s", olddir);
	strcpy(olddir, tmp2);
	ndisks=i-1;
	textattr(16*3+0);
	strcpy(mask, "*.*");
	textattr(7*16+0);
	gotoxy(1, 25);
	cputs("ESC closes; ENTER accepts; SPACE and INS mark; arrows and pages move");
	select_1:
	for (i=0; i!=220; i++)
		files[i].selected=0;
	disk=getdisk()+'A';
	getcurdir(disk-'A'+1, dir);
	if (dir[0]==0)
		sprintf(tmp2, "\\");
	else
		sprintf(tmp2, "\\%s\\", dir);
	strcpy(dir, tmp2);
	sprintf(tmp2, "%c:%sDESCRIPT.ION", disk, dir);
	f=fopen(tmp2, "rb");
	sprintf(tmp2, "%c:%s%s", disk, dir, mask);
	i=ndisks+1;
	fin=findfirst(tmp2, &ffblk, FA_RDONLY|FA_HIDDEN|FA_SYSTEM|FA_DIREC|FA_ARCH);
	while (!fin)
	{
		memcpy(files[i].name, ffblk.ff_name, 13);
		if ((ffblk.ff_attrib&FA_DIREC)==FA_DIREC)
		{
			if (strcmp(files[i].name, ".")==0)
				strcpy(files[i].comment, "Current directory");
			else if (strcmp(files[i].name, "..")==0)
				strcpy(files[i].comment, "Previous directory");
			else
				memset(files[i].comment, 0, 34);
			strcpy(files[i].size, "\xF0 DIRECTORY \xF0");
			strupr(files[i].name);
		}
		else
		{
			memset(files[i].comment, 0, 34);
			select_puntua(files[i].size, ffblk.ff_fsize);
			strlwr(files[i].name);
		}
		memcpy(&ff_fdate, &ffblk.ff_fdate, 2);
		if (ff_fdate.year<20)
			sprintf(files[i].date, "%02u-%02u-%02u", ff_fdate.day, ff_fdate.mon, ff_fdate.year+80);
		else if (ff_fdate.year<120)
			sprintf(files[i].date, "%02u-%02u-%02u", ff_fdate.day, ff_fdate.mon, ff_fdate.year-20);
		else
			sprintf(files[i].date, "%02u-%02u-XX", ff_fdate.day, ff_fdate.mon);
		memcpy(&ff_ftime, &ffblk.ff_ftime, 2);
		sprintf(files[i].time, "%02u:%02u", ff_ftime.hour, ff_ftime.min);
		if (f!=0)
		{
			rewind(f);
			while (!feof(f))
			{
				fscanf(f, "%s ", tmp1);
				fgets(tmp2, 79, f);
				*strchr(tmp2, '\r')=0;
				if (stricmp(tmp1, files[i].name)==0)
				{
					memcpy(files[i].comment, tmp2, 33);
					break;
				}
			}
		}
		fin=findnext(&ffblk);
		i++;
		if (i>200)
			break;
	}
	fclose(f);
	ultimo=i-1;
	for (i=ultimo+1; i!=220; i++)
	{
		files[i].selected=0;
		files[i].name[0]=0;
		files[i].size[0]=0;
		files[i].date[0]=0;
		files[i].time[0]=0;
		files[i].comment[0]=0;
	}
	qsort((void *) files, ultimo+1, sizeof(files[0]), select_sort);
	box(2, 2, 78, 23, 0, "      ", "", 5);
	textattr(16*3+0);
	gotoxy (1, 2);  cputs("ÉÍÍÍÍÍÍÍÍÍÍÍÍÑÍÍÍÍÍÍÍÍÍÍÍÍÍÍÑÍÍÍÍÍÍÍÍÑÍÍÍÍÍÑÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ»");
	textattr(16*3+15);
	sprintf(tmp1, "[%c:%s]", disk, dir);
	cputsc(1, 78, 2, tmp1);
	gotoxy(3, 2);
	cputs("[þ]");
	textattr(16*3+0);
	gotoxy (1, 3);  cputs("º    Name    ³     Size     ³  Date  ³Time ³             Comment             º");
	gotoxy (1, 4);  cputs("ÇÄÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÅÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¶");
	gotoxy (1, 5);  cputs("º            ³              ³        ³     ³                                 \x18");
	gotoxy (1, 6);  cputs("º            ³              ³        ³     ³                                 º");
	gotoxy (1, 7);  cputs("º            ³              ³        ³     ³                                 º");
	gotoxy (1, 8);  cputs("º            ³              ³        ³     ³                                 º");
	gotoxy (1, 9);  cputs("º            ³              ³        ³     ³                                 º");
	gotoxy (1, 10); cputs("º            ³              ³        ³     ³                                 º");
	gotoxy (1, 11); cputs("º            ³              ³        ³     ³                                 º");
	gotoxy (1, 12); cputs("º            ³              ³        ³     ³                                 º");
	gotoxy (1, 13); cputs("º            ³              ³        ³     ³                                 º");
	gotoxy (1, 14); cputs("º            ³              ³        ³     ³                                 º");
	gotoxy (1, 15); cputs("º            ³              ³        ³     ³                                 º");
	gotoxy (1, 16); cputs("º            ³              ³        ³     ³                                 º");
	gotoxy (1, 17); cputs("º            ³              ³        ³     ³                                 º");
	gotoxy (1, 18); cputs("º            ³              ³        ³     ³                                 º");
	gotoxy (1, 19); cputs("º            ³              ³        ³     ³                                 º");
	gotoxy (1, 20); cputs("º            ³              ³        ³     ³                                 º");
	gotoxy (1, 21); cputs("º            ³              ³        ³     ³                                 º");
	gotoxy (1, 22); cputs("º            ³              ³        ³     ³                                 \x19");
	gotoxy (1, 23); cputs("ÈÍÍÍÍÍÍÍÍÍÍÍÍÏÍÍÍÍÍÍÍÍÍÍÍÍÍÍÏÍÍÍÍÍÍÍÍÏÍÍÍÍÍÏÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ¼");
	if (ultimo>=18)
	{
		for (i=0; i<18; i++)
		{
			gotoxy (2, 5+i);  cprintf("%-12s³%14s³%-8s³%-5s³%-33s", files[i].name, files[i].size, files[i].date, files[i].time, files[i].comment);
		}
	}
	else
	{
		for (i=0; i<=ultimo; i++)
		{
			gotoxy (2, 5+i);  cprintf("%-12s³%14s³%-8s³%-5s³%-33s", files[i].name, files[i].size, files[i].date, files[i].time, files[i].comment);
		}
	}
	textattr(2*16+15);
	gotoxy(2, 5);		cprintf("%-12s³%14s³%-8s³%-5s³%-33s", files[0].name, files[0].size, files[0].date, files[0].time, files[0].comment);
	i=0;
	j=0;
	do
	{
		/* Mouse show */
		r.x.ax=1;
		int86(0x33, &r, &r);
		do
		{
			watch();
			tecla=bioskey(1);
			if (tecla!=0)
				tecla=bioskey(0);
			/* Get mouse */
			r.x.ax=3;
			int86(0x33, &r, &r);
			r.x.cx=(r.x.cx>>3)+1;
			r.x.dx=(r.x.dx>>3)+1;
			/* Left button */
			if (r.x.bx==1)
			{
				if ((r.x.cx==4) && (r.x.dx==2))
					tecla=0x011B;
				else if ((r.x.cx==78) && (r.x.dx==22))
					tecla=0x5000;
				else if ((r.x.cx==78) && (r.x.dx==5))
					tecla=0x4800;
				else if ((r.x.cx>1) && (r.x.cx<77) &&
								 (r.x.dx>4) && (r.x.dx<23))
				{
					if (files[j].selected)
						textattr(3*16+15);
					else
						textattr(3*16+0);
					gotoxy(2, 5+i);
					cprintf("%-12s³%14s³%-8s³%-5s³%-33s", files[j].name, files[j].size, files[j].date, files[j].time, files[j].comment);
					j-=i;
					i=0;
					i=r.x.dx-5;
					j+=i;
					tecla=0x3920;
				}
			}
		}
		while ((tecla!=0x5000) && (tecla!=0x4800) && (tecla!=0x5100) &&
					 (tecla!=0x4900) && (tecla!=0x4700) && (tecla!=0x4F00) &&
					 (tecla!=0x5200) && (tecla!=0x3920) && (tecla!=0x1C0D) &&
					 (tecla!=0x011B) && (tecla!=0x03D00) && (tecla!=0x3C00) &&
					 (tecla!=0x3B00));
		/* Mouse hidde */
		r.x.ax=2;
		int86(0x33, &r, &r);
		if (files[j].selected)
			textattr(3*16+15);
		else
			textattr(3*16+0);
		gotoxy(2, 5+i); 	cprintf("%-12s³%14s³%-8s³%-5s³%-33s", files[j].name, files[j].size, files[j].date, files[j].time, files[j].comment);

		if (tecla==0x1C0D)        /* Enter */
		{
			if (strncmp(files[j].size, "\xF0   DRIVE   \xF0", 15)==0)
			{
				setdisk(files[j].name[1]-'A');
				goto select_1;
			}
			else if (strncmp(files[j].size, "\xF0 DIRECTORY \xF0", 15)==0)
			{
				chdir(files[j].name);
				goto select_1;
			}
		}
		else if (tecla==0x3C00)		/* F2 */
		{
			box (23, 12, 56, 14, 16*7+15, A_DOUBLE, "Mask", 1);
			textattr(16*7+0);
			gotoxy(24, 13);
			cputs("Enter file mask:");
			getstr(42, 13, 16*1+15, mask, 13);
			goto select_1;
		}
		else if (tecla==0x3D00)   /* F3 */
		{
/*			for (fin=0; fin!=ultimo; fin++)
				files[fin].selected=1;
			tecla=0x4700;*/
		}
		else if (tecla==0x3E00)   /* F4 */
		{
/*			for (fin=0; fin!=ultimo; fin++)
				files[fin].selected=0;
			tecla=0x4700;*/
		}
		else if (tecla==0x5000)		/* Dw */
		{
			if (j!=ultimo)
			{
				i++;
				j++;
			}
		}
		else if (tecla==0x4800)		/* Up */
		{
			if (j!=0)
			{
				i--;
				j--;
			}
		}
		else if (tecla==0x5100)		/* PgDw */
		{
			if (j<ultimo-17)
				j+=17-i;
		}
		else if (tecla==0x4900)		/* PgUp */
		{
			if (j>=17)
				j-=17-i;
		}
		else if (tecla==0x4700)		/* Org */
			j=0;
		else if (tecla==0x4F00)   /* End */
			j=ultimo-17;
		else if ((tecla==0x3920) || (tecla==0x5200))  /* Space/Ins */
		{
			if ((strcmp(files[j].size, "\xF0 DIRECTORY \xF0")!=0) &&
					(strcmp(files[j].size, "\xF0   DRIVE   \xF0")!=0) &&
					(j<=ultimo))
			{
				files[j].selected++;
				files[j].selected=files[j].selected&1;
				if (files[j].selected)
					textattr(3*16+15);
				else
					textattr(3*16+0);
				gotoxy(2, 5+i); 	cprintf("%-12s³%14s³%-8s³%-5s³%-33s", files[j].name, files[j].size, files[j].date, files[j].time, files[j].comment);
			}
			if (j<ultimo)
			{
				i++;
				j++;
			}
		}
		if (i<0)
		{
			r.x.ax=0x0701;
			r.h.bh=3*16+0;
			r.x.cx=0x0401;
			r.x.dx=0x154C;
			int86(0x10, &r, &r);
			i=0;
		}
		else if (i>17)
		{
			r.x.ax=0x0601;
			r.h.bh=3*16+0;
			r.x.cx=0x0401;
			r.x.dx=0x154C;
			int86(0x10, &r, &r);
			i=17;
		}
		if ((tecla==0x5100) || (tecla==0x4900) || (tecla==0x4700) ||
			 (tecla==0x4F00) || (tecla==0x3F00))
		{
			for (i=0; i<18; i++)
			{
				if (files[i+j].selected)
					textattr(3*16+15);
				else
					textattr(3*16+0);
				gotoxy (2, 5+i);  cprintf("%-12s³%14s³%-8s³%-5s³%-33s", files[i+j].name, files[i+j].size, files[i+j].date, files[i+j].time, files[i+j].comment);
			}
			if (tecla==0x4F00)
			{
				i=17;
				j=ultimo;
			}
			else
				i=0;
		}
		textattr(2*16+15);
		gotoxy(2, 5+i); 	cprintf("%-12s³%14s³%-8s³%-5s³%-33s", files[j].name, files[j].size, files[j].date, files[j].time, files[j].comment);
	}
	while ((tecla!=0x1C0D) && (tecla!=0x011B) && (tecla!=0x3B00));
	if (tecla==0x011B)
	{
		for (i=0; i!=200; i++)
			files[i].selected=0;
	}
	else
	{
		for (i=0; i!=ultimo; i++)
		{
			if (files[i].selected)
			{
				i=0x0AAA;
				break;
			}
		}
		if (i!=0x0AAA)
			files[j].selected=1;
	}
	*sdisk=disk;
	memcpy (sdir, dir, strlen(dir)-1);
	setdisk(olddrive);
	chdir(olddir);
	puttext(1, 2, 80, 24, oldb);
	free(oldb);
	gotoxy(oldx, oldy);
	if (tecla==0x3B00)
		return(255);
	else
		return(0);
}

/* -------------------- Ordena el select ---------------------------------- */
int select_sort(const void *a, const void *b)
{
	return(memcmp((char *)a, (char *)b, 12));
}

/* ------------------------- Puntua un long ------------------------------- */
void pascal select_puntua (char *res, long int n)
{
	register unsigned int i, j, k;
	char tmp[15];

	j=0;
	k=0;
	memset(tmp, 0, 15);
	memset(res, 0, 15);
	ltoa(n, tmp, 10);
	if (n<1000)
	{
		strcpy(res, tmp);
		return;
	}
	strrev(tmp);
	for (i=0; i!=strlen(tmp); i++)
	{
		if (j==3)
		{
			j=0;
			res[k]=',';
			k++;
		}
		res[k]=tmp[i];
		j++;
		k++;
	}
	strrev(res);
}

/* --------------------------- Funci¢n inicial ---------------------------- */
void _fastcall hello (char *nombre, unsigned char comercial)
{
	unsigned char oldx, oldy;
	FILE *f;
	char *oldb;
	void interrupt (*oldvect)();
  union REGS r;

	oldb=malloc(4000);
	if (oldb==0)
		return;
	gettext(1, 1, 80, 25, oldb);
	oldx=wherex();
	oldy=wherey();
	_setcursortype(_NOCURSOR);
	textattr(7);
	setpal(1);
	empuja();
	background(1);
	/* Mouse reset */
	r.x.ax=0;
	int86(0x33, &r, &r);
  oldvect=getvect(0x24);
	setvect(0x24, error);
	f=fopen(nombre, "wb");
	if (f==0)
		return;
	fwrite(oldb, 1, 4000, f);
	fwrite(&oldx, 1, 1, f);
	fwrite(&oldy, 1, 1, f);
	fwrite(&oldvect, 1, sizeof(oldvect), f);
	fclose(f);
	free(oldb);
  about(comercial);
}

/* --------------------------- Funci¢n final ------------------------------ */
void _fastcall bye (char *nombre, unsigned char e, unsigned char comercial)
{
	unsigned char oldx, oldy;
	FILE *f;
	char *oldb;
	void interrupt (*oldvect)();

	oldb=malloc(4000);
	if (oldb==0)
		return;
	f=fopen(nombre, "rb");
	if (f==0)
		return;
	fread(oldb, 1, 4000, f);
	fread(&oldx, 1, 1, f);
	fread(&oldy, 1, 1, f);
	fread(&oldvect, 1, sizeof(oldvect), f);
	fclose(f);
	remove(nombre);
	setvect(0x24, oldvect);
	puttext(1, 1, 80, 25, oldb);
  if (oldy<25-12)
	{
		box(1, 1, 78, 11, 0*16+14, "Ú¿ÄÄ³ÀÙ", "Thank you for using", 1);
		textattr(0*16+3);
		window(3, 2, 76, 10);
	}
	else
	{
		box(1, 2, 78, 12, 0*16+14, "Ú¿ÄÄ³ÀÙ", "Thank you for using", 1);
		textattr(0*16+3);
		window(3, 3, 76, 11);
	}
	gotoxy(1, 1);
  cputs("WIPE v4.0 for DOS\r\n");
  cputs("(c) Copyright 1994-1997 De Trans Software (D.T.S.)\r\n");
	cputs("                        Javier Gutierrez Chamorro\r\n");
	cputs("                        Av. Meridiana 336, 7 C\r\n");
	cputs("                        08027. Barcelona. [SPAIN]\r\n");
	cputs("                        Guti@bbs-ce.uab.es\r\n");
	cputs("                        http://www.geocities.com/\r\n");
	cputs("                               SiliconValley/4505\r\n");
	textattr(0*16+12);
  if (comercial)
    cputs("Registered version. Do not distribute it.");
  else
    cputs("Shareware version. Please register.");
	window(1, 1, 80, 25);
	/* rebota(); */
	if (oldy<12)
		gotoxy(oldx, 12);
	else
		gotoxy(oldx, oldy);
	_setcursortype(_NORMALCURSOR);
	textattr(7);
	/* setpal(0); */
	free(oldb);
	exit(e);
}

/* --------------- Dibuja en pantalla la ventana del about ---------------- */
void _fastcall background (unsigned char e)
{
	register unsigned int i;
	unsigned int *oldb, *olda;

	oldb=malloc(4000);
	if (oldb==0)
		return;
	olda=oldb;

	for (i=0; i!=2000-160; i++)
	{
		*olda=0x13B0;
		olda++;
	}
	puttext(1, 2, 80, 24, oldb);
	if (e==1)
	{
		textattr(7*16+0);
		gotoxy(1, 1);
		cputs("WIPE v4.0 for DOS                  (c) Copyright 1997 De Trans Software (D.T.S.)");
		olda=oldb;
		for (i=0; i!=80; i++)
		{
			*olda=0x7020;
			olda++;
		}
		puttext(1, 25, 80, 25, oldb);
	}
	free(oldb);
}

/* --------------- Dibuja en pantalla la ventana del about ---------------- */
void _fastcall about (unsigned char comercial)
{
  register unsigned int tecla;
	char x1=9, y1=5, x2=68, y2=18;

	unsigned char oldx, oldy;
	char *oldb;
  union REGS r;

	oldb=malloc(textsize(x1, y1, x2+2, y2+1));
	if (oldb==0)
		return;
	oldx=wherex();
	oldy=wherey();
  move:
	gettext(x1, y1, x2+2, y2+1, oldb);
  textattr(7*16+0);
  gotoxy(1, 25);
  cputs("ENTER and ESC close the about window                                ");
	box (x1, y1, x2, y2, 7*16+15, A_DOUBLE, "About", 3);
	textattr(7*16+0);
	cputsc(x1, x2, y1+1,	"WIPE for DOS");
	cputsc(x1, x2, y1+2,	"Version  4.0");
  cputsc(x1, x2, y1+4,  "(c) Copyright 1994-1997 De Trans Software (D.T.S.)");
	cputsc(x1, x2, y1+5, 	"                        Javier Gutierrez Chamorro ");
	cputsc(x1, x2, y1+6, 	"                        Av. Meridiana 336, 7 C    ");
	cputsc(x1, x2, y1+7, 	"                        08027. Barcelona. [SPAIN] ");
	cputsc(x1, x2, y1+8, 	"                        Guti@bbs-ce.uab.es        ");
	cputsc(x1, x2, y1+9, 	"                        http://www.geocities.com/ ");
	cputsc(x1, x2, y1+10,	"                               SiliconValley/4505 ");
	textattr(7*16+4);
  if (comercial)
		cputsc(x1, x2, y1+12, "Registered version");
  else
		cputsc(x1, x2, y1+12, "Shareware version");
	/* Mouse show */
	r.x.ax=1;
	int86(0x33, &r, &r);
  do
  {
		watch();
		tecla=bioskey(1);
    if (tecla!=0)
			tecla=bioskey(0);
    /* Get mouse */
    r.x.ax=3;
    int86(0x33, &r, &r);
    r.x.cx=(r.x.cx>>3)+1;
    r.x.dx=(r.x.dx>>3)+1;
    /* Left button */
    if (r.x.bx==1)
    {
			if ((r.x.dx==y1) && (r.x.cx==x1+3))
       tecla=0x011B;
    }
  }
	while ((tecla!=0x1C0D) && (tecla!=0x011B));
  /* Mouse hidde */
  r.x.ax=2;
  int86(0x33, &r, &r);
  gotoxy(oldx, oldy);
	puttext(x1, y1, x2+2, y2+1, oldb);
	free(oldb);
}

/* -------------------- Deja o no introducir al usuario ------------------- */
void _fastcall screensaver (void)
{
  register unsigned char i, j;
  unsigned char k;
  unsigned char oldx, oldy;
  unsigned char rep[8][5];
  char oldb[8][5][10*5*2];

  oldx=wherex();
	oldy=wherey();
	randomize();
  /* Guardar pantalla original */
  for (j=0; j!=5; j++)
  {
    for (i=0; i!=8; i++)
			gettext(i*10+1, j*5+1, i*10+10, j*5+5, oldb[i][j]);
  }
  ini:
  /* Empezar a quitar */
  memset(&rep, 0, sizeof(rep));
  for (k=0; k!=40; k++)
  {
    i=bioskey(1);
    if (i!=0)
    {
      bioskey(0);
      goto fin;
    }
    do
    {
      i=random(8);
      j=random(5);
    }
		while (rep[i][j]);
    rep[i][j]=1;
		window(i*10+1, j*5+1, i*10+10, j*5+5);
    textattr(0);
    clrscr();
    window(1, 1, 80, 25);
    delay(200);
  }
  /* Empezar a poner */
  memset(&rep, 0, sizeof(rep));
  for (k=0; k!=40; k++)
  {
    i=bioskey(1);
    if (i!=0)
    {
      bioskey(0);
      goto fin;
    }
    do
    {
      i=random(8);
      j=random(5);
    }
		while (rep[i][j]);
    rep[i][j]=1;
		puttext(i*10+1, j*5+1, i*10+10, j*5+5, oldb[i][j]);
    delay(200);
  }
  goto ini;
  fin:
  /* Restaurar pantalla original */
  for (j=0; j!=5; j++)
  {
		for (i=0; i!=8; i++)
			puttext(i*10+1, j*5+1, i*10+10, j*5+5, oldb[i][j]);
  }
  gotoxy(oldx, oldy);
}

/* -------------------- Deja o no introducir al usuario ------------------- */
void _fastcall rebota (void)
{
  register unsigned char i, j;
  char oldb[4000];

	window(1,1,80,25);
  gettext(1, 1, 80, 25, oldb);
	textattr(7);
	clrscr();

  for (j=27; j!=0; j--)
	{
    for (i=1; i!=j; i++)
		{
      puttext(1, i, 80, i+24, oldb);
      gotoxy(1, i);
			clreol();
      delay(20);
		}
    for (i=j; i!=0; i--)
		{
      puttext(1, i, 80, i+24, oldb);
      gotoxy(1, i);
			clreol();
      delay(20);
		}
	}
  puttext(1, 1, 80, 25, oldb);
}

/* -------------------- Deja o no introducir al usuario ------------------- */
unsigned char _fastcall exittodos (char *nombre)
{
  register unsigned int i;
  unsigned char oldx, oldy;
  char *oldb, *buf;
  char but1[][11]={   "[Yes]", "[No]", "[Shell]"};
  char dbut1[][71]={  "Exit to DOS and finish program                                      ",
                      "Continue running program                                            ",
                      "Exit to DOS and continue running program                            "};

  oldx=wherex();
  oldy=wherey();
  oldb=malloc(textsize(20, 10, 62, 15));
  if (oldb==0)
		return(255);
  gettext(20, 10, 62, 15, oldb);
  box(20, 10, 60, 14, 16*7+15, A_DOUBLE, "Exit", SHADOW);
	textattr(16*7+0);
  cputsc(20, 60, 11, "Do you want to exit to DOS?");
  do
  {
		i=buttons(24, 13, 7*16+0, 2*16+15, but1, dbut1, 3, 13);
		if (i==255)
    {
      buf=malloc(16384);
      if (buf==0)
				return(255);
      help (7, 3, 69, 21, 16*3+15, 16*3+0, nombre, "Exit", buf);
      free(buf);
    }
  }
  while (i==255);
  puttext(20, 10, 62, 15, oldb);
  free(oldb);
  gotoxy(oldx, oldy);
  if (i==2)
  {
		shell(3, 2, 75, 22, "");
    i=1;
	}
	return(i);
}

/* ------------ Escribe en pantalla el estado de los bloqueos ------------- */
void _fastcall shell (unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2, char *comando)
{
  register unsigned char i;

  unsigned char oldx, oldy;
  char *oldb;
	void interrupt (*oldvect)();

  oldx=wherex();
  oldy=wherey();
  oldb=malloc(textsize(x1, y1, x2+2, y2+1));
  if (oldb==0)
		return;
  gettext(x1, y1, x2+2, y2+1, oldb);
  if (comando[0]==0)
    box(x1, y1, x2, y2, 14, A_DOUBLE, "DOS Shell", SHADOW);
  else
    box(x1, y1, x2, y2, 14, A_DOUBLE, comando, SHADOW);
	window(x1+1, y1+1, x2-1, y2-1);
	textattr(15);
	clrscr();
	cputs("Type 'EXIT' to return\r\n\r\n");
	textattr(7);
  oldvect=getvect(0x29);
  setvect(0x29, shell_patch);
	i=system(comando);
	setvect(0x29, oldvect);
	if (i!=0)
	{
		textattr(15);
		cputs("Cannot execute program.");
	}
	window(1, 1, 80, 25);
	puttext(x1, y1, x2+2, y2+1, oldb);
	free(oldb);
	gotoxy(oldx, oldy);
}

/* ------------------- Rutina de control de errores ----------------------- */
void interrupt shell_patch (unsigned int bp, unsigned int di, unsigned int si, unsigned int ds, unsigned int es, unsigned int dx, unsigned int cx, unsigned int bx, unsigned int ax)
{
	register unsigned char al;

  al=ax&255;
  putch(al);
#pragma warn -par
}
#pragma warn .par

/* ------------------- Dibuja en pantalla un calendario ------------------- */
void pascal calendar (unsigned char x, unsigned char y, unsigned char atr0, unsigned char atr1, unsigned char atr2, unsigned char day, unsigned char month, unsigned int year)
{
	register unsigned char i, days, start;
	unsigned char oldx, oldy;
	unsigned int tecla;
	char *oldb;
	struct tm *tyme;
	unsigned char	jump[12]=	{ 1, 4, 4, 0, 2, 5, 0, 3, 6, 1, 4, 6 };
  union REGS r;
	unsigned char monthdays[12]=	{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	time_t curr_time;
	char *monthnameuk[12]=	{ "January", "February", "March", "April",
														"May", "June", "July", "August",
														"September", "October", "November",
														"December" };
	_setcursortype(_NOCURSOR);
	oldb=malloc(textsize(x, y, x+40, y+11));
	if (oldb==0)
		return;
	oldx=wherex();
	oldy=wherey();
	gettext(x, y, x+40, y+11, oldb);
	/* Conseguir fecha actual */
	curr_time=time(0);
	tyme=localtime(&curr_time);
	/* Si alg£n par metro es nulo, obtiene el actual */
	if (day==0)
		day=tyme->tm_mday;
	if (month==0)
		month=tyme->tm_mon;
	if ((year>=2050) || (year<=1950))
		year=0;
	else if (year>=2000)
		year-=2000;
	else
		year-=1900;
	if (year==0)
		year=tyme->tm_year;
	textattr(7*16+0);
	gotoxy(1, 25);
	cputs("ENTER and ESC close the calendar window                             ");
	textattr(atr0);
	box(x, y, x+38, y+10, 31, "É»ÍÍºÈ¼", "Calendar", 3);
	textattr(atr1);
	gotoxy(x+2, y+1);
	if (year>=2000)
		cprintf("%s-30s 20%u\r\n", monthnameuk[month], year);
	else
		cprintf("%-30s 19%u\r\n", monthnameuk[month], year);
	gotoxy(x+5, y+2);
	cputs("Sun Mon Tue Wed Thu Fri Sat");

	start=year+(year>>2)+jump[month];
	if (((year&3)==0) && (month<=1)) /* Febrero */
		start--;
	start=(start%7)+1;
	if (((year&3)==0) && (month==1))  /* Febrero */
		days=29;
	else
		days=monthdays[month];
	gotoxy(wherex(), y+4);
	for (i=1; i<=days; i++ )
	{
		gotoxy(x+2+(start<<2), wherey());
		if (i==day)
			textattr(atr1);
		else if (start==1)
			textattr(atr2);
		else
			textattr(atr0);
		cprintf("%2u", i);
		if (start==7)
		{
			cputs("\r\n");
			gotoxy(x+5, wherey());
			start=1;
		}
		else
			start++;
	}
	do
	{
		/* Mouse show */
		r.x.ax=1;
		int86(0x33, &r, &r);
		tecla=bioskey(1);
		if (tecla!=0)
			tecla=bioskey(0);
		/* Get mouse */
		r.x.ax=3;
		int86(0x33, &r, &r);
		r.x.cx=(r.x.cx>>3)+1;
		r.x.dx=(r.x.dx>>3)+1;
		/* Left button */
		if ((r.x.bx==1) && (r.x.dx==y) && (r.x.cx==x+3))
			tecla=0x011B;
	}
	while ((tecla!=0x1C0D) && (tecla!=0x011B));
	/* Mouse hidde */
	r.x.ax=2;
	int86(0x33, &r, &r);
	puttext(x, y, x+40, y+11, oldb);
	free(oldb);
	gotoxy(oldx, oldy);
}

/* --------------------- Hace una tablba de opciones ---------------------- */
unsigned char pascal tabla (unsigned char x1, unsigned char y1, unsigned char atr0, unsigned char atr1, char menu[][16], unsigned char noptions)
{
	register char y;
	register unsigned int tecla;
	char x;
	union REGS r;

	textattr(atr0);
	gotoxy(x1, y1-1);
	for (x=0; x!=noptions; x++)
	{
		if ((x&3)==0)
			gotoxy(x1, wherey()+1);
		cprintf("%-15s", menu[x]);
	}
	textattr(atr1);
	gotoxy(x1, y1);
	cputs(menu[0]);
	textattr(7*16+0);
	y=0;
	do
	{
		/* Mouse show */
		r.x.ax=1;
		int86(0x33, &r, &r);
		do
		{
			watch();
			tecla=bioskey(1);
			if (tecla!=0)
				tecla=bioskey(0);
			/* Get mouse */
			r.x.ax=3;
			int86(0x33, &r, &r);
			r.x.cx=(r.x.cx>>3)+1;
			r.x.dx=(r.x.dx>>3)+1;
			/* Left button */
			if ( (r.x.bx==1) && (r.x.cx>=x1) && (r.x.cx<=(x1+16)) && (r.x.dx>=y1)
					&& (r.x.dx<=y1+(noptions>>2)))
			{
				textattr(atr0);
				gotoxy(((y&3)<<4)+x1, (y>>2)+y1);
				cputs(menu[y]);
				y=((r.x.dx-y1)<<2)+((r.x.cx-x1)&3);
				tecla=0xFFFF;
			}
			/* Right button */
			else if (r.x.bx==2)
				tecla=0x3B00;
		}
		while ((tecla!=0x4800) && (tecla!=0x5000) && (tecla!=0x4700) &&
					 (tecla!=0x4F00) && (tecla!=0x011B) && (tecla!=0x1C0D) &&
					 (tecla!=0x3B00) && (tecla!=0x4B00) && (tecla!=0x4D00) &&
					 (tecla!=0xFFFF));
		/* Mouse hidde */
		r.x.ax=2;
		int86(0x33, &r, &r);
		textattr(atr0);
		gotoxy(((y&3)*15)+x1, (y>>2)+y1);
		cputs(menu[y]);
		if (tecla==0x4800)        /* Up */
			y-=4;
		else if (tecla==0x5000)   /* Dw */
			y+=4;
		else if (tecla==0x4700)   /* Org */
			y=0;
		else if (tecla==0x4F00)   /* End */
			y=noptions-1;
		else if (tecla==0x4B00)
			y--;
		else if (tecla==0x4D00)
			y++;
		if (y>noptions-1)
			y=0;
		else if (y<0)
			y=noptions-1;
		textattr(atr1);
		gotoxy(((y&3)*15)+x1, (y>>2)+y1);
		cputs(menu[y]);
	}
	while ((tecla!=0x1C0D) && (tecla!=0x011B) && (tecla!=0x3B00));
	if (tecla==0x011B)
		return(0);
	else if (tecla==0x3B00)
		return(255);
	else
		return(y);
}

/* --------------------- Hace una tablba de opciones ---------------------- */
unsigned char pascal helpindex (unsigned char x1, unsigned char y1, unsigned char atr0, unsigned char atr1, char *nombre, char buffer[][16])
{
	register unsigned char i;
	register unsigned int ultimo;
	unsigned char oldx, oldy;
	FILE *f;
	char *oldb;
	char tmp[256];

	oldb=malloc(4000);
	if (oldb==0)
		return(255);
	gettext(1, 1, 80, 25, oldb);
	oldx=wherex();
	oldy=wherey();
	textattr(7*16+0+128);
	gotoxy(1, 25);
	cputs("Loading help...                                                     ");
	f=fopen(nombre, "rb");
	if (f==0)
	{
		strcpy(&buffer[0], "Help is not available.");
		ultimo=1;
		view(x1, y1, x1+65, y1+20, atr0, atr0, buffer, ultimo, "Help: Not available");
		puttext(1, 1, 80, 25, oldb);
		free(oldb);
		gotoxy(oldx, oldy);
		return(255);
	}
	ultimo=0;
	do
	{
		fgets(tmp, 255, f);
    if ((tmp[0]=='@') && (strlen(tmp)>3))
		{
			*strchr(tmp, '\r')=0;
			memcpy(&buffer[ultimo], &tmp[1], 16);
			ultimo++;
		}
	}
	while (!feof(f));
	fclose(f);
	ultimo--;
  box (x1, y1, x1+62, y1+19, atr0, "É»ÍÍºÈ¼", "Help: Index", 1);
	i=tabla(x1+2, y1+1, atr0, atr1, buffer, ultimo);
	puttext(1, 1, 80, 25, oldb);
	free(oldb);
	gotoxy(oldx, oldy);
  help (x1, y1, x1+62, y1+18, 16*3+15, 16*3+0, nombre, buffer[i], buffer);
	return(i);
}
