#ifndef _CONIOW_H_
#define _CONIOW_H_

#ifdef __cplusplus
extern "C" {
#endif

struct text_info {
    unsigned short winleft;
    unsigned short wintop;
    unsigned short winright;
    unsigned short winbottom;
    unsigned short attribute;
    unsigned short normattr;
    unsigned short currmode;
    unsigned short screenheight;
    unsigned short screenwidth;
    short curx;
    short cury;
};

#if !defined(__COLORS)
#define __COLORS

typedef enum _COLORS {
    BLACK,          /* dark colors */
    BLUE,
    GREEN,
    CYAN,
    RED,
    MAGENTA,
    BROWN,
    LIGHTGRAY,
    DARKGRAY,       /* light colors */
    LIGHTBLUE,
    LIGHTGREEN,
    LIGHTCYAN,
    LIGHTRED,
    LIGHTMAGENTA,
    YELLOW,
    WHITE
} COLORS;
#endif

#define BLINK       128 
#define PASS_MAX    8 
#define _NOCURSOR 0         
#define _SOLIDCURSOR 100    
#define _NORMALCURSOR 20    

typedef struct char_info {
    char letter;
    unsigned char attr;
} char_info;

extern int _directvideo;
extern int _wscroll;

void          clreol( void );
void          clrscr( void );
void          delline( void );
int           _conio_gettext( int __left, int __top,
                            int __right, int __bottom,
                            void *__destin);
void          gettextinfo (struct text_info *__r );
void          gotoxy(int __x, int __y );
void          highvideo( void );
void          insline( void );
void          lowvideo( void );
int           movetext( int __left, int __top,
                             int __right, int __bottom,
                             int __destleft, int __desttop );
void          normvideo( void );
int           puttext( int __left, int __top,
                            int __right, int __bottom,
                            void *__source );
void          textattr( int __newattr );
void          textbackground( int __newcolor );
void          textcolor( int __newcolor );
void          textmode( int __newmode );
void          window( int __left, int __top, int __right, int __bottom);

void          _setcursortype( int __cur_t );
char *        cgets( char *__str );
int           cprintf( const char *__format, ... );
int           cputs( const char *__str );
int           cscanf( const char *__format, ... );
int           getch( void );
int           getche( void );
char *        getpass( const char *__prompt );
int           kbhit( void );
int           putch( int __c );
int           ungetch( int __ch );
int           wherex( void );
int           wherey( void );

void          _conio_delay (int ms);
int           kbmhit( void );

#define KEY_SPECIAL     0x100
#define KEY_HOME        71
#define KEY_UP          72
#define KEY_PGUP        73
#define KEY_LEFT        75
#define KEY_CENTER      76
#define KEY_RIGHT       77
#define KEY_END         79
#define KEY_DOWN        80
#define KEY_PGDN        81
#define KEY_INSERT      82
#define KEY_DELETE      83

#define KEY_F1          59
#define KEY_F2          60
#define KEY_F3          61
#define KEY_F4          62
#define KEY_F5          63
#define KEY_F6          64
#define KEY_F7          65
#define KEY_F8          66
#define KEY_F9          67
#define KEY_F10         68

#define KEY_ESC 27

#define MOUSE_CLICK     0x200
#define MOUSE_DBLCLICK  0x201
#define MOUSE_WHEELUP   0x202
#define MOUSE_WHEELDOWN 0x203
#define WINDOW_RESIZE   0x204

int           getkbm( void );

extern int _mousex;
extern int _mousey;
extern int _mousebuttons;
extern int _controlkeystate;

#define _getch getch
#define delay _conio_delay
#ifndef _CONIO_NO_GETTEXT_
  #define gettext _conio_gettext
#endif

#include <stddef.h>
typedef struct wchar_info {
    wchar_t letter;        
    unsigned short attr;   
} wchar_info;
wchar_t *cgetws( wchar_t *__str );
int      cputws( const wchar_t *__str );
wchar_t  getwch(void);
wchar_t  putwch(wchar_t);
wchar_t  ungetwch(wchar_t);
wchar_t  getwche(void);
int cwscanf( const wchar_t *__format, ... );
int cwprintf( const wchar_t *__format, ... );
int putwtext( int __left, int __top,
                            int __right, int __bottom,
                            const wchar_info *__source );
int getwtext( int __left, int __top,
                            int __right, int __bottom,
                            wchar_info *__destin );
wchar_t ansi2unicode(char ch);
char unicode2ansi(wchar_t ch);

#ifdef __cplusplus
}
#endif

#endif /* _CONIO2_H_ */
