/** @file conio2.h
 * A conio implementation for Mingw/Dev-C++.
 *
 * Written by:
 * Hongli Lai <hongli@telekabel.nl>
 * tkorrovi <tkorrovi@altavista.net> on 2002/02/26.
 * Andrew Westcott <ajwestco@users.sourceforge.net>
 * Michal Molhanec <michal@molhanec.net>
 *
 * Offered for use in the public domain without any warranty.
 */

#ifndef _CONIOWIN_H_
#define _CONIOWIN_H_

#include <conio.h>
#ifdef UNICODE
    #include <windows.h>    // we need wchar_t
#endif


#if defined (_MSC_VER)
	#define _CRT_SECURE_NO_WARNINGS
	#define putch _putch
	#define getch _getch
	#define kbhit _kbhit
#endif


#ifdef __cplusplus
	extern "C" {
#endif

/**
 * Colors which you can use in your application.
 */
typedef enum
{
    BLACK,          /**< black color */
    BLUE,           /**< blue color */
    GREEN,          /**< green color */
    CYAN,           /**< cyan color */
    RED,            /**< red color */
    MAGENTA,        /**< magenta color */
    BROWN,          /**< brown color */
    LIGHTGRAY,      /**< light gray color */
    DARKGRAY,       /**< dark gray color */
    LIGHTBLUE,      /**< light blue color */
    LIGHTGREEN,     /**< light green color */
    LIGHTCYAN,      /**< light cyan color */
    LIGHTRED,       /**< light red color */
    LIGHTMAGENTA,   /**< light magenta color */
    YELLOW,         /**< yellow color */
    WHITE           /**< white color */
} COLORS;

#define BLINK       128 /* blink bit */


enum text_modes
{
	/* DOS-compatible modes */
	LASTMODE = -1,
	BW40 = 0,
	C40,
	BW80,
	C80,
	MONO = 7,
	C4350 = 64,
	/* New Color modes */
	C40X14 = 8,
	C40X21,
	C40X28,
	C40X43,
	C40X50,
	C40X60,
	C80X14,
	C80X21,
	C80X28,
	C80X43,
	C80X50,
	C80X60,
	/* New Black & White modes */
	BW40X14,
	BW40X21,
	BW40X28,
	BW40X43,
	BW40X50,
	BW40X60,
	BW80X14,
	BW80X21,
	BW80X28,
	BW80X43,
	BW80X50,
	BW80X60,
	/* New Monochrome modes */
	MONO14,             /* Invalid VGA mode */
	MONO21,
	MONO28,
	MONO43,
	MONO50,
	MONO60,
	_ORIGMODE = 65      /* original mode at program startup */
};


#define PASS_MAX 8   /* max. no. of chars returned by getpass */


extern int _directvideo;
extern int _wscroll;



/*@{*/
/**
 * This defines enables you to use all MinGW conio.h functions without
 * underscore.
 */
#define cgets   _cgets
#define cprintf _cprintf
#define cputs   _cputs
#define cscanf  _cscanf

#ifdef UNICODE
    #define cgetws   _cgetws
    #define getwch   _getwch
    #define getwche  _getwche
    #define putwch   _putwch
    #define ungetwch _ungetwch
    #define cputws   _cputws
    #define cwprintf _cwprintf
    #define cwscanf  _cwscanf
#endif
/*@}*/

/**
 * Define alias for gettext.
 * If you want to use gettext function from some other library
 * (e.g. GNU gettext) you have to define _CONIO_NO_GETTEXT_ so you won't get
 * name conflict.
 */


/**
 * @anchor cursortypes
 * @name Cursor types
 * Predefined cursor types. */
/*@{*/
#define _NOCURSOR 0         /**< no cursor */
#define _SOLIDCURSOR 1
#define _NORMALCURSOR 2
/*@}*/

/**
 * Structure holding information about screen.
 * @see gettextinfo
 * @see inittextinfo
 */
struct text_info
{
	unsigned char winleft;
	unsigned char wintop;
	unsigned char winright;
	unsigned char winbottom;
	unsigned char attribute;
	unsigned char normattr;
	unsigned char currmode;
	unsigned char screenheight;
	unsigned char screenwidth;
	unsigned char curx;
	unsigned char cury;
};

/**
 * Structure used by gettext/puttext.
 * @see gettext
 * @see puttext
 */
struct char_info
{
	#ifdef UNICODE
		wchar_t letter;        /**< character value */
	#else
		char letter;           /**< character value */
	#endif
    unsigned short attr;   /**< attribute value */
};

/**
 * Returns information of the screen.
 * @see text_info
 */
void gettextinfo (struct text_info * info);

/**
 * Call this if you need real value of normattr attribute in the text_info
 * structure.
 * @see text_info
 */
void inittextinfo (void);

/**
 * Clears rest of the line from cursor position to the end of line without
 * moving the cursor.
 */
void clreol (void);

/**
 * Clears whole screen.
 */
void clrscr (void);

/**
 * Delete the current line (line on which is cursor) and then moves all lines
 * below one line up. Lines below the line are moved one line up.
 */
void delline (void);

/**
 * Insert blank line at the cursor position.
 * Original content of the line and content of lines below moves one line down.
 * The last line is deleted.
 */
void insline (void);

/**
 * Gets text from the screen. If you haven't defined <TT>_CONIO_NO_GETTEXT_</TT>
 * prior to including <TT>conio2.h</TT> you can use this function also under
 * the <TT>gettext</TT> name.
 * @see char_info
 * @see puttext
 * @param left Left coordinate of the rectangle, inclusive, starting from 1.
 * @param top Top coordinate of the rectangle, inclusive, starting from 1.
 * @param right Right coordinate of the rectangle, inclusive, starting from 1.
 * @param bottom Bottom coordinate of the rectangle, inclusive, starting from 1.
 * @param buf You have to pass buffer of size
 * <TT>(right - left + 1) * (bottom - top + 1) * sizeof(char_info)</TT>.
 */
void gettext (int left, int top, int right, int bottom, struct char_info * buf);

/**
 * Puts text back to the screen.
 * @see char_info
 * @see gettext
 * @param left Left coordinate of the rectangle, inclusive, starting from 1.
 * @param top Top coordinate of the rectangle, inclusive, starting from 1.
 * @param right Right coordinate of the rectangle, inclusive, starting from 1.
 * @param bottom Bottom coordinate of the rectangle, inclusive, starting from 1.
 * @param buf You have to pass buffer of size
 * <TT>(right - left + 1) * (bottom - top + 1) * sizeof(char_info)</TT>.
 */
void puttext (int left, int top, int right, int bottom, struct char_info * buf);

/**
 * Copies text.
 * @param left Left coordinate of the rectangle, inclusive, starting from 1.
 * @param top Top coordinate of the rectangle, inclusive, starting from 1.
 * @param right Right coordinate of the rectangle, inclusive, starting from 1.
 * @param bottom Bottom coordinate of the rectangle, inclusive, starting from 1.
 * @param destleft Left coordinate of the destination rectangle.
 * @param desttop Top coordinate of the destination rectangle.
 */
void movetext (int left, int top, int right, int bottom, int destleft,
              int desttop);

/**
 * Moves cursor to the specified position.
 * @param x horizontal position
 * @param y vertical position
 */
void gotoxy(int x, int y);

/**
 * Puts string at the specified position.
 * @param x horizontal position
 * @param y vertical position
 * @param str string
 */
void cputsxy (int x, int y, char * str);

/**
 * Puts char at the specified position.
 * @param x horizontal position
 * @param y vertical position
 * @param ch char
 */
void putchxy (int x, int y, char ch);

/**
 * Sets the cursor type.
 * @see @ref cursortypes
 * @param type cursor type, under Win32 it is height of the cursor in percents
 */
void _setcursortype (int type);

/**
 * Sets attribute of text.
 * @param _attr new text attribute
 */
void textattr (int _attr);

/**
 * Sets text attribute back to value it had after program start.
 * It uses text_info's normattr value.
 * @see text_info
 */
void normvideo (void);

/**
 * Sets text background color.
 * @see COLORS
 * @param color new background color
 */
void textbackground (int color);

/**
 * Sets text foreground color.
 * @see COLORS
 * @param color new foreground color
 */
void textcolor (int color);

/**
 * Reads the cursor X position.
 * @returns cursor X position
 */
int wherex (void);

/**
 * Reads the cursor Y position.
 * @returns cursor Y position
 */
int wherey (void);

/**
 * Reads password. This function behaves like cgets.
 *
 * @see cgets
 * @param prompt prompt which will be displayed to user
 * @param str string for the password. <TT>str[0]</TT> have to contain
 * length of the <TT>str</TT> - 3
 * @returns <TT>&str[2]</TT>, the password will be stored in <TT>str</TT>
 * beginning at <TT>str[2]</TT>, in <TT>str[1]</TT> will be length of the
 * string without <TT>\\0</TT>, at <TT>str[2 + str[1]]</TT> will be \\0.
 */
char * getpass (const char * prompt, char * str);

/**
 * Makes foreground colors light.
 * If the current foreground color is less than <TT>DARKGRAY</TT> adds
 * 8 to the its value making dark colors light.
 * @see COLORS
 * @see lowvideo
 */
void highvideo (void);

/**
 * Makes foreground colors dark.
 * If the current foreground color is higher than <TT>LIGHTGRAY</TT> substracts
 * 8 from its value making light colors dark.
 * @see COLORS
 * @see highvideo
 */
void lowvideo (void);

/*@{*/
/*
 * You may need to link with libmsvcr70.a or libmsvcr70d.a or libmsvcr71.a
 *  or libmsvcr71d.a if you want any of these functions.
 */
#ifdef UNICODE
_CRTIMP wchar_t * __cdecl         _cgetws(wchar_t *);
_CRTIMP unsigned short __cdecl    _getwch(void);
_CRTIMP unsigned short __cdecl    _getwche(void);
_CRTIMP unsigned short __cdecl    _putwch(wchar_t);
_CRTIMP unsigned short __cdecl    _ungetwch(unsigned short);
_CRTIMP int __cdecl               _cputws(const wchar_t *);
_CRTIMP int __cdecl               _cwprintf(const wchar_t *, ...);
_CRTIMP int __cdecl               _cwscanf(const wchar_t *, ...);
#endif
/*@}*/

/**
 * Pauses program execution for a given time.
 * @see switchbackground
 * @param ms miliseconds
 */
void delay (int ms);

/**
 * Replaces background color in the whole window. The text however
 * is left intact. Does not modify textbackground().
 * @see flashbackground
 * @param color background color
 */
void switchbackground (int color);

/**
 * Changes background color for a given time and then it restores it back.
 * You can use it for visual bell. Does not modify textbackground().
 * @see switchbackground
 * @see delay
 * @param color background color
 * @param ms miliseconds
 */
void flashbackground (int color, int ms);

/**
 * Clears the keyboard buffer.
 * To see it in effect run <TT>conio_test</TT> and try to press a key during
 * the 'Flashing...' phase.
 */
void clearkeybuf (void);

#ifdef __cplusplus
}
#endif

#endif /* _CONIO2_H_ */
