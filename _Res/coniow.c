#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include <wchar.h>
#include <stdlib.h>
#include "coniow.h"


#define MAX_BUFFER 250

static struct text_info ti={1,1,80,25,7,7,0,25,80,1,1};

int _directvideo;
int _wscroll;
int _mousex;
int _mousey;
int _mousebuttons;
int _controlkeystate;
static int ti_flag=0;
#include <string.h>
#include <malloc.h>
static int myvsscanf(
    const char  *buffer,
    const char  *format,
    va_list     argPtr
)
{
	void *a[20];
    size_t count = 0;
    const char *p;
	p= format;
    while(1)
    {
        char c = *(p++);
        if(c==0) break;
        if(c=='%' && (p[0]!='*' && p[0]!='%')) a[count++]=va_arg(argPtr, void *);
    }
    return sscanf(buffer, format, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8], a[9], 
		 a[10], a[11], a[12], a[13], a[14], a[15], a[16], a[17], a[18], a[19]);
} 
static int myvswscanf(
    const wchar_t  *buffer,
    const wchar_t  *format,
    va_list     argPtr
)
{
	void *a[20];
    size_t count = 0;
    const wchar_t *p;
	p= format;
    while(1)
    {
        wchar_t c = *(p++);
        if(c==0) break;
        if(c==L'%' && (p[0]!=L'*' && p[0]!=L'%')) a[count++]=va_arg(argPtr, void *);
    }
    return swscanf(buffer, format, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8], a[9], 
		 a[10], a[11], a[12], a[13], a[14], a[15], a[16], a[17], a[18], a[19]);
} 
static void init_ti(void)
{ 
  CONSOLE_SCREEN_BUFFER_INFO info;
  int i;
  DWORD m;
  UINT acp;
  if(ti_flag) return;
  ti_flag=1;
  for(i=0;i<2;++i)
  {
    if(GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info)) break;
    if(i) return;
    FreeConsole();
    AllocConsole();
  }
  ti.normattr = info.wAttributes;
  ti.winright  = info.dwSize.X;
  ti.winbottom = info.dwSize.Y;
  ti.screenwidth  = info.dwSize.X;
  ti.screenheight = info.dwSize.Y;
  ti.curx = info.dwCursorPosition.X+1;
  ti.cury = info.dwCursorPosition.Y+1;
  ti.attribute = info.wAttributes;
  acp=GetACP();
  if(GetConsoleOutputCP()!=acp)
  {
    SetConsoleOutputCP(acp);
    SetConsoleCP(acp);
  }
  GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &m);
  SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), m|ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT|ENABLE_ECHO_INPUT); 
}
static int in_window(int x, int y)
{ return !(x<ti.winleft || y<ti.wintop || x>ti.winright || y>ti.winbottom);
}
static void putchxyattr(int x, int y, int ch, int attr)
{
  CHAR_INFO buffer;
  SMALL_RECT r;
  COORD c={0,0},s={1,1};
  if(!in_window(x,y)) return;
  r.Left = x-1;
  r.Top = y-1;
  r.Right = x-1;
  r.Bottom = y-1;
  buffer.Char.AsciiChar = ch;
  buffer.Attributes = attr;
  WriteConsoleOutputA (GetStdHandle (STD_OUTPUT_HANDLE), &buffer, s, c, &r);
}
static void putchxyattrwh(int x, int y, int ch, int attr,int w, int h)
{
  HANDLE con;
  COORD c;
  int i;
  if(x<ti.winleft)
    w-=ti.winleft-x;
  if(y<ti.wintop)
    h-=ti.wintop-y;
  if(x+w-1>ti.winright)
    w=ti.winright-x+1;
  if(y+h-1>ti.winbottom)
    h=ti.winbottom-y+1;
  
  if(w<=0 || h<=0) return;
  con=GetStdHandle (STD_OUTPUT_HANDLE);
  for(i=0;i<h;++i)
  {
    DWORD written;
    c.X=x-1;
    c.Y=y-1+i;
    FillConsoleOutputAttribute (con,attr,w,c,&written);
    FillConsoleOutputCharacterA (con,ch,w,c,&written);
  }
}
static void putwchxyattr(int x, int y, wchar_t ch, int attr)
{
  CHAR_INFO buffer;
  SMALL_RECT r;
  COORD c={0,0},s={1,1};
  if(!in_window(x,y)) return;
  r.Left = x-1;
  r.Top = y-1;
  r.Right = x-1;
  r.Bottom = y-1;
  buffer.Char.UnicodeChar = ch;
  buffer.Attributes = attr;
  WriteConsoleOutputW(GetStdHandle (STD_OUTPUT_HANDLE), &buffer, s, c, &r);
}
static void putwchxyattrwh(int x, int y, wchar_t ch, int attr,int w, int h)
{
  HANDLE con;
  COORD c;
  int i;
  if(x<ti.winleft)
    w-=ti.winleft-x;
  if(y<ti.wintop)
    h-=ti.wintop-y;
  if(x+w-1>ti.winright)
    w=ti.winright-x+1;
  if(y+h-1>ti.winbottom)
    h=ti.winbottom-y+1;
  
  if(w<=0 || h<=0) return;
  con=GetStdHandle (STD_OUTPUT_HANDLE);
  for(i=0;i<h;++i)
  {
    DWORD written;
    c.X=x-1;
    c.Y=y-1+i;
    FillConsoleOutputAttribute (con,attr,w,c,&written);
    FillConsoleOutputCharacterW (con,ch,w,c,&written);
  }
}
#define KEYBUF_SIZE 100
static wchar_t keybuf[KEYBUF_SIZE];
static int nkeybuf=0;
static int keystate=0;

wchar_t ansi2unicode(char ch)
{
  char ansistr[2];
  wchar_t unicodestr[2];
  ansistr[0]=ch;
  ansistr[1]=0;
  unicodestr[0]=0;
  MultiByteToWideChar(CP_ACP, 0, ansistr, 1, unicodestr, 1);
  return unicodestr[0];
}

char unicode2ansi(wchar_t ch)
{
  char ansistr[2];
  wchar_t unicodestr[2];
  ansistr[0]=(char)ch;
  unicodestr[0]=ch;
  unicodestr[1]=0;
  WideCharToMultiByte(CP_ACP, 0, unicodestr, 1, ansistr, 1, NULL, NULL);
  return ansistr[0];
}

void clreol( void )
{
  init_ti();
  putchxyattrwh(ti.winleft+ti.curx-1,ti.wintop+ti.cury-1,' ',ti.attribute,ti.winright-ti.winleft+2-ti.curx,1);
  gotoxy(ti.curx, ti.cury);
}
void clrscr( void )
{
  init_ti();
  putchxyattrwh(ti.winleft,ti.wintop,' ',ti.attribute,ti.winright-ti.winleft+1,ti.winbottom-ti.wintop+1);
  gotoxy(1, 1);
}

void delline( void )
{
  COORD c;
  SMALL_RECT r;
  CHAR_INFO fc;
  init_ti();
  c.X = ti.winleft-1;
  c.Y = ti.wintop-1 + ti.cury - 1;
  r.Left = ti.winleft-1;
  r.Top = ti.wintop-1+ ti.cury;
  r.Right = ti.winright - 1;
  r.Bottom = ti.winbottom - 1;
  fc.Attributes = ti.attribute;
  fc.Char.UnicodeChar = L' ';
  ScrollConsoleScreenBufferW (GetStdHandle (STD_OUTPUT_HANDLE), &r, NULL, c, &fc);
  gotoxy (ti.curx, ti.cury);
}
void insline( void )
{
  COORD c;
  SMALL_RECT r;
  CHAR_INFO fc;
  init_ti();
  c.X = ti.winleft-1;
  c.Y = ti.wintop-1+ti.cury;
  r.Left = ti.winleft-1;
  r.Top = ti.wintop-1+ti.cury-1;
  r.Right = ti.winright - 1;
  r.Bottom = ti.winbottom - 2;
  fc.Attributes = ti.attribute;
  fc.Char.UnicodeChar = L' ';
  ScrollConsoleScreenBufferW (GetStdHandle (STD_OUTPUT_HANDLE), &r, NULL, c, &fc);
  gotoxy (ti.curx, ti.cury);
}
int _conio_gettext( int __left, int __top, int __right, int __bottom,  void *__destin)
{
  int i;
  SMALL_RECT r;
  CHAR_INFO* buffer;
  char_info *buf;
  COORD s,c={0,0};
  if(__right<__left || __bottom<__top) return 0;
  init_ti();
  r.Left = __left-1;
  r.Top = __top-1;
  r.Right = __right - 1;
  r.Bottom = __bottom - 1;
  s.X = __right - __left + 1;
  s.Y = __bottom - __top + 1;
  buffer = (CHAR_INFO*)malloc (s.X * s.Y * sizeof(CHAR_INFO));
  buf=(char_info *)__destin;
  if(ReadConsoleOutputA(GetStdHandle(STD_OUTPUT_HANDLE), buffer, s, c, &r))
  {
    for (i = 0; i < s.X * s.Y; i++)
    {
      buf[i].letter = buffer[i].Char.AsciiChar;
      buf[i].attr = (unsigned char)buffer[i].Attributes;
    }
  }
  free (buffer);
  return 0;
}
void gettextinfo (struct text_info *__r)
{ 
  init_ti();
  *__r=ti;
}
void gotoxy(int __x, int __y)
{
  COORD c;
  init_ti();
  if(!in_window(ti.winleft+__x-1,ti.wintop+__y-1)) return;
  ti.curx=__x;
  ti.cury=__y;
  c.X = ti.winleft+ti.curx-2;
  c.Y = ti.wintop+ti.cury-2;
  SetConsoleCursorPosition (GetStdHandle(STD_OUTPUT_HANDLE), c);
}
void highvideo( void )
{ textattr(ti.attribute|0x08);
}
void lowvideo( void )
{ textattr(ti.attribute&0xF7);
}
int movetext( int __left, int __top, int __right, int __bottom,
              int __destleft, int __desttop )
{
  CHAR_INFO * buffer;
  SMALL_RECT r;
  COORD c={0,0},s;
  HANDLE con;
  init_ti();
  s.X=__right - __left + 1;
  s.Y=__bottom - __top + 1;
  if(s.X<=0 || s.Y<=0) return 0;
  buffer = malloc (s.X * s.Y * sizeof(CHAR_INFO));
  con=GetStdHandle (STD_OUTPUT_HANDLE);
  r.Left = __left-1;
  r.Top = __top-1;
  r.Right = __right-1;
  r.Bottom = __bottom-1;
  if(ReadConsoleOutputW(con,buffer, s, c, &r))
    WriteConsoleOutputW(con,buffer, s, c, &r);
  free(buffer);
  return 0;
}

void normvideo( void )
{
  init_ti();
  textattr(ti.normattr);
}
int puttext( int __left, int __top, int __right, int __bottom, void *__source )
{
  int i;
  SMALL_RECT r;
  CHAR_INFO* buffer;
  char_info *buf;
  COORD s,c={0,0};
  if(__right<__left || __bottom<__top) return 0;
  init_ti();
  r.Left = __left-1;
  r.Top = __top-1;
  r.Right = __right - 1;
  r.Bottom = __bottom - 1;
  s.X = __right - __left + 1;
  s.Y = __bottom - __top + 1;
  buffer = malloc (s.X * s.Y * sizeof(CHAR_INFO));
  buf=(char_info *)__source;
  for (i = 0; i < s.X * s.Y; i++)
  {
    buffer[i].Char.AsciiChar=(unsigned char)buf[i].letter;
    buffer[i].Attributes=buf[i].attr;
  }
  WriteConsoleOutputA(GetStdHandle(STD_OUTPUT_HANDLE), buffer, s, c, &r);
  free (buffer);
  return 0;
}
void textattr( int __newattr )
{
  init_ti();
  SetConsoleTextAttribute (GetStdHandle(STD_OUTPUT_HANDLE), ti.attribute=__newattr);
}
void textbackground( int __newcolor )
{ textattr((ti.attribute&0xFF0F)|((__newcolor&0xF)<<4));
}
void textcolor( int __newcolor )
{ textattr((ti.attribute&0xFFF0)|(__newcolor&0xF));
}
void textmode( int __newmode )
{ ;
}
void window( int __left, int __top, int __right, int __bottom)
{ 
  init_ti();
  if(__left<1 || __top<1 || __right>ti.screenwidth || __bottom>ti.screenheight) return;
  ti.winleft=__left;
  ti.winright=__right;
  ti.wintop=__top;
  ti.winbottom=__bottom;
  gotoxy(1,1);
}
void _setcursortype( int __cur_t )
{
  CONSOLE_CURSOR_INFO Info;
  if (__cur_t == _NOCURSOR)
    Info.bVisible = FALSE;
  else {
    Info.bVisible = TRUE;
    Info.dwSize = __cur_t;
  }
  SetConsoleCursorInfo (GetStdHandle (STD_OUTPUT_HANDLE), &Info);
}
char *cgets( char *__str )
{
  char *str;
  int maxlen;
  int length = 0;
  int ch = 0;
  int x, y;
  if(__str==NULL) return NULL;
  str=__str+2;
  maxlen=(int)((unsigned char)__str[0])-1;
  init_ti();
  x = ti.curx;
  y = ti.cury;

  while (ch != '\r') 
  {
    ch = getch();
    if(ch==0)
      getch();
    else
    {
      switch (ch) 
      {
        case '\r':
          break;
        case '\b': /* backspace */
          if (length > 0) putchxyattr (ti.winleft+x-1 + --length, ti.wintop+y-1,' ',ti.attribute);
          gotoxy (x + length,y);
          break;
        default:
          if (length < maxlen) 
          {
            putch(ch);
            str[length++] = ch;
          }
      }
    }
  }
  __str[1]=length;
  str[length]=0;
  return str;
}
int cprintf( const char *__format, ... )
{ char buffer[MAX_BUFFER];
  int r;
  va_list ap;
  va_start(ap, __format);
  r=vsprintf(buffer, __format, ap);
  va_end(ap);
  cputs(buffer);
  return r;
}
int cputs( const char *__str )
{
  int k = 0;
  while(*__str)
  { putch(*__str);
    ++__str;
	++k;
  }
  return k;
}
int cscanf( const char *__format, ... )
{ char buffer[MAX_BUFFER+3];
  int r;
  va_list ap;
  buffer[0]=MAX_BUFFER;
  cgets(buffer);
  va_start(ap, __format);
  r=myvsscanf(buffer+2, __format, ap);
  va_end(ap);
  return r;
}
int getch( void )
{ int ch;
  if(keystate!=0)
  { ch=keystate;
    keystate=0;
    return ch;
  }
  while(1)
  { ch=getkbm();
    if((ch&0x200)==0)
      break;
  }
  if(ch&0x100)
  {
    keystate=ch&0xFF;
    return 0;
  }
  return ch;
}
int getche( void )
{ 
  int ch;
  ch=getch();
  while(ch==0)
  {
    getch();
    ch=getch();
  }
  putch(ch);
  return ch;
}
char *getpass( const char *__prompt )
{
  static char str[PASS_MAX+1];
  int length = 0;
  int ch = 0;
  int x, y;

  cputs(__prompt);
  x = ti.curx;
  y = ti.cury;

  while (ch != '\r') 
  {
    ch = getch();
    if(ch==0)
      getch();
    else
    {
      switch (ch) 
      {
        case '\r':
          break;
        case '\b': /* backspace */
          if (length > 0) putchxyattr ( ti.winleft+x-1 + --length,  ti.wintop+y-1,' ',ti.attribute);
          gotoxy(x + length,y);
          break;
        default:
          if (length < PASS_MAX) 
          {
            putch('*');
            str[length++] = ch;
          }
      }
    }
  }
  str[length]=0;
  return str;
}
int kbhit( void )
{
  HANDLE con;
  DWORD nevents=0;
  INPUT_RECORD buf; 
  init_ti();
  if(nkeybuf) return 1;
  if(keystate) return 1;
  con=GetStdHandle(STD_INPUT_HANDLE);
  while(1)
  {
    GetNumberOfConsoleInputEvents(con,&nevents);
    if(nevents==0) return 0;
    PeekConsoleInput(con,&buf,1,&nevents);
    if(buf.EventType==KEY_EVENT && buf.Event.KeyEvent.bKeyDown)
      return 1;
    ReadConsoleInput(con,&buf,1,&nevents);
    if(buf.EventType==WINDOW_BUFFER_SIZE_EVENT)
    { ti.screenwidth=buf.Event.WindowBufferSizeEvent.dwSize.X;
      ti.screenheight=buf.Event.WindowBufferSizeEvent.dwSize.Y;
      if(ti.winright>ti.screenwidth) ti.winright=ti.screenwidth;
      if(ti.winbottom>ti.screenheight) ti.winbottom=ti.screenheight;
      if(ti.winleft>ti.winright) ti.winleft=ti.winright;
      if(ti.wintop>ti.winbottom) ti.wintop=ti.winbottom;
      if(!in_window(ti.winleft+ti.curx-1,ti.wintop+ti.cury-1))
        gotoxy(1,1);
    }
  }
}
int putch(int __c)
{
  init_ti();
  switch (__c) 
  {
    case '\r':
      gotoxy(1,ti.cury);
      break;
    case '\n':
      if(ti.cury<ti.winbottom-ti.wintop+1)
        gotoxy(ti.curx,ti.cury+1);
      else
      { int x,y;
        x=ti.curx;
        y=ti.cury;
        gotoxy(1,1);
        delline();
        gotoxy(x,y);
      }
      break;
    case '\b':
      if(ti.curx > 1) gotoxy(ti.curx-1,ti.cury);
      break;
    default:
      putchxyattr(ti.winleft+ti.curx-1, ti.wintop+ti.cury-1,__c,ti.attribute);
      if(ti.curx+1>ti.winright+ti.winleft+1)
      {
        if (_wscroll)
        {

          if(ti.cury<ti.winbottom-ti.wintop+1)
            gotoxy(1,ti.cury+1);
          else 
          { int y;
            y=ti.cury;
            gotoxy(1,1);
            delline();
            gotoxy(1,y);
          }
        }
        else
          gotoxy(1,ti.cury);
      }
      else
        gotoxy(ti.curx+1,ti.cury);
  }
  return __c&0xFF;
}
int ungetch( int __ch )
{
  if(nkeybuf<KEYBUF_SIZE)
  { keybuf[nkeybuf++]=ansi2unicode(__ch&0xFF);
    return __ch;
  }
  return EOF;
}
int wherex( void )
{
  init_ti();
  return ti.curx;
}
int wherey( void )
{
  init_ti();
  return ti.cury;
}
void _conio_delay (int ms)
{
  init_ti();
  Sleep(ms);
}
int kbmhit( void )
{
  HANDLE con;
  DWORD nevents=0;
  INPUT_RECORD buf; 
  init_ti();
  if(nkeybuf) return 1;
  if(keystate) return 1;
  con=GetStdHandle(STD_INPUT_HANDLE);
  while(1)
  {
    GetNumberOfConsoleInputEvents(con,&nevents);
    if(nevents==0) return 0;
    PeekConsoleInput(con,&buf,1,&nevents);
    if(buf.EventType==KEY_EVENT && buf.Event.KeyEvent.bKeyDown)
      return 1;
    if(buf.EventType==WINDOW_BUFFER_SIZE_EVENT)
      return 1;
    if(buf.EventType==MOUSE_EVENT)
    {
      switch(buf.Event.MouseEvent.dwEventFlags)
      {
        case 0:
            if(buf.Event.MouseEvent.dwButtonState != 0)
               return 1;
            break;
        case DOUBLE_CLICK:
        case MOUSE_WHEELED:
          return 1;
        default:
          break;
      }
    }
    ReadConsoleInput(con,&buf,1,&nevents);
  }
}
int getkbm( void )
{
  HANDLE con;
  DWORD nevents=0;
  INPUT_RECORD buf; 
  init_ti();
  if(nkeybuf) 
  { 
    return keybuf[--nkeybuf];
  }
  if(keystate) keystate=0;
  con=GetStdHandle(STD_INPUT_HANDLE);
  while(1)
  {
    ReadConsoleInputA(con,&buf,1,&nevents);
    if(buf.EventType==KEY_EVENT && buf.Event.KeyEvent.bKeyDown)
    {
      _controlkeystate=buf.Event.KeyEvent.dwControlKeyState;
      if(buf.Event.KeyEvent.uChar.AsciiChar==0)
        return KEY_SPECIAL+buf.Event.KeyEvent.wVirtualScanCode;
      return buf.Event.KeyEvent.uChar.AsciiChar&0xFF;
    }
    else if(buf.EventType==WINDOW_BUFFER_SIZE_EVENT)
    {
      ti.screenwidth=buf.Event.WindowBufferSizeEvent.dwSize.X;
      ti.screenheight=buf.Event.WindowBufferSizeEvent.dwSize.Y;
      if(ti.winright>ti.screenwidth) ti.winright=ti.screenwidth;
      if(ti.winbottom>ti.screenheight) ti.winbottom=ti.screenheight;
      if(ti.winleft>ti.winright) ti.winleft=ti.winright;
      if(ti.wintop>ti.winbottom) ti.wintop=ti.winbottom;
      if(!in_window(ti.winleft+ti.curx-1,ti.wintop+ti.cury-1))
        gotoxy(1,1);
      return WINDOW_RESIZE;
    }
    else if(buf.EventType==MOUSE_EVENT)
    {
      switch(buf.Event.MouseEvent.dwEventFlags)
      {
        case 0:
          if(buf.Event.MouseEvent.dwButtonState != 0)
          { 
            _mousebuttons=buf.Event.MouseEvent.dwButtonState;
            _mousex=buf.Event.MouseEvent.dwMousePosition.X+1;
            _mousey=buf.Event.MouseEvent.dwMousePosition.Y+1;
            return MOUSE_CLICK;
          }
          break;
        case DOUBLE_CLICK:
          _mousebuttons=buf.Event.MouseEvent.dwButtonState;
          _mousex=buf.Event.MouseEvent.dwMousePosition.X+1;
          _mousey=buf.Event.MouseEvent.dwMousePosition.Y+1;
          return MOUSE_DBLCLICK;
        case MOUSE_WHEELED:
          _mousex=buf.Event.MouseEvent.dwMousePosition.X+1;
          _mousey=buf.Event.MouseEvent.dwMousePosition.Y+1;
          if((int)buf.Event.MouseEvent.dwButtonState>0)
            return MOUSE_WHEELUP;
          return MOUSE_WHEELDOWN;
        default:
          break;
      }
    }
  }
}

// Unicode
wchar_t *cgetws(wchar_t *__str )
{
 wchar_t *str;
  int maxlen;
  int length = 0;
  wchar_t ch = 0;
  int x, y;

  if(__str==NULL) return NULL;

  str=__str+2;
  maxlen=(int)(__str[0])-1;
  init_ti();
  x = ti.curx;
  y = ti.cury;

  while (ch != L'\r') 
  {
    ch = getwch();
    if(ch==0)
      getwch();
    else
    {
      switch (ch) 
      {
        case L'\r':
          break;
        case L'\b': /* backspace */
          if (length > 0) putwchxyattr ( ti.winleft+x-1 + --length,  ti.wintop+y-1,' ',ti.attribute);
          gotoxy (x + length,y);
          break;
        default:
          if (length < maxlen) 
          {
            putwch(ch);
            str[length++] = ch;
          }
      }
    }
  }
  __str[1]=length;
  str[length]=0;
  return str;
}
int cputws( const wchar_t *__str )
{
  int k = 0;
  while(*__str)
  { putwch(*__str);
    ++__str;
	++k;
  }
  return k;
}
wchar_t getwch(void)
{ int ch;
  if(keystate!=0)
  { ch=keystate;
    keystate=0;
    return ch;
  }
  while(1)
  { ch=getkbm();
    if((ch&0x200)==0)
      break;
  }
  if(ch&0x100)
  {
    keystate=ch&0xFF;
    return 0;
  }
  return ansi2unicode(ch); 
}
wchar_t  putwch(wchar_t __c)
{
  init_ti();
  switch (__c) 
  {
    case L'\r':
      gotoxy(1,ti.cury);
      break;
    case L'\n':
      if(ti.cury<ti.winbottom-ti.wintop+1)
        gotoxy(ti.curx,ti.cury+1);
      else
      { int x,y;
        x=ti.curx;
        y=ti.cury;
        gotoxy(1,1);
        delline();
        gotoxy(x,y);
      }
      break;
    case L'\b':
      if(ti.curx > 1) gotoxy(ti.curx-1,ti.cury);
      break;
    default:
      putwchxyattr( ti.winleft+ti.curx-1,  ti.wintop+ti.cury-1,__c,ti.attribute);
      if(ti.curx+1>ti.winright+ti.winleft+1)
      {
        if (_wscroll)
        {

          if(ti.cury<ti.winbottom-ti.wintop+1)
            gotoxy(1,ti.cury+1);
          else 
          { int y;
            y=ti.cury;
            gotoxy(1,1);
            delline();
            gotoxy(1,y);
          }
        }
        else
          gotoxy(1,ti.cury);
      }
      else
        gotoxy(ti.curx+1,ti.cury);
  }
  return __c;
}
wchar_t ungetwch(wchar_t __ch)
{
  if(nkeybuf<KEYBUF_SIZE)
  { keybuf[nkeybuf++]=__ch;
    return __ch;
  }
  return WEOF;
}
wchar_t  getwche(void)
{
  wchar_t ch;
  ch=getwch();
  while(ch==0)
  {
    getwch();
    ch=getwch();
  }
  putwch(ch);
  return ch;
}
int cwscanf( const wchar_t *__format, ... )
{ wchar_t buffer[MAX_BUFFER+3];
  int r;
  va_list ap;
  buffer[0]=MAX_BUFFER;
  cgetws(buffer);
  va_start(ap, __format);
  r= myvswscanf(buffer+2, __format, ap);
  va_end(ap);
  return r;
}
int cwprintf( const wchar_t *__format, ... )
{
  wchar_t buffer[MAX_BUFFER];
  va_list ap;
  int r;
  va_start(ap, __format);
  r=vswprintf(buffer, MAX_BUFFER, __format, ap);
  va_end(ap);
  cputws(buffer);
  return r;
}
int putwtext( int __left, int __top, int __right, int __bottom, const wchar_info *__source )
{
  int i;
  SMALL_RECT r;
  CHAR_INFO* buffer;
  COORD s,c={0,0};
  if(__right<__left || __bottom<__top) return 0;
  init_ti();
  r.Left = __left-1;
  r.Top = __top-1;
  r.Right = __right - 1;
  r.Bottom = __bottom - 1;
  s.X = __right - __left + 1;
  s.Y = __bottom - __top + 1;
  buffer = malloc (s.X * s.Y * sizeof(CHAR_INFO));
  for (i = 0; i < s.X * s.Y; i++)
  {
    buffer[i].Char.UnicodeChar=__source[i].letter;
    buffer[i].Attributes=__source[i].attr;
  }
  WriteConsoleOutputW(GetStdHandle(STD_OUTPUT_HANDLE), buffer, s, c, &r);
  free (buffer);
  return 0;
}
int getwtext( int __left, int __top,  int __right, int __bottom,  wchar_info *__destin )
{
  int i;
  SMALL_RECT r;
  CHAR_INFO* buffer;
  COORD s,c={0,0};
  if(__right<__left || __bottom<__top) return 0;
  init_ti();
  r.Left = __left-1;
  r.Top = __top-1;
  r.Right = __right - 1;
  r.Bottom = __bottom - 1;
  s.X = __right - __left + 1;
  s.Y = __bottom - __top + 1;
  buffer = malloc (s.X * s.Y * sizeof(CHAR_INFO));
  if(ReadConsoleOutputW(GetStdHandle(STD_OUTPUT_HANDLE), buffer, s, c, &r))
  {
    for (i = 0; i < s.X * s.Y; i++)
    {
      __destin[i].letter = buffer[i].Char.UnicodeChar;
      __destin[i].attr = buffer[i].Attributes;
    }
  }
  free (buffer);
  return 0;
}
