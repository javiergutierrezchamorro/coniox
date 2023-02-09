@ECHO OFF
CLS


:WATCOM
rem *** OPENWATCOM C++ 1.9 *** 
SET WATCOM=C:\FICHEROS\OC
SET OLDPATH=%PATH%
SET PATH=%WATCOM%\BINNT;%WATCOM%\BINW;%WATCOM%\BINP;%PATH%
SET EDPATH=%WATCOM%\EDDAT
SET INCLUDE=%WATCOM%\H;%WATCOM%\MFC\INCLUDE;%WATCOM%\H\NT;INCLUDE;
SET LIB=LIB;
rem *** DOS16 ***
DEL WC16d.exe
WCL -oneatx -ohirbk -ol -ol+ -oi -ei -zp4 -0 -mc -s -ri -zm /bt=dos /l=dos /"OPTION ELIMINATE" /"OPTION VFREMOVAL" dirio.c demo.c
REN dirio.exe WC16d.exe
DEL WC32d.exe
WCL386 -oneatx -ohirbk -ol -ol+ -oi -ei -zp4 -3r -mf -s -ri -zm /bt=dos /l=dos32a /"OPTION ELIMINATE" /"OPTION VFREMOVAL" dirio.c demo.c
REN dirio.exe WC32d.exe
DEL WC32w.exe
WCL386 -oneatx -ohirbk -ol -ol+ -oi -ei -zp4 -3r -mf -s -ri -zm /bt=nt /l=nt /"OPTION ELIMINATE" /"OPTION VFREMOVAL" dirio.c demo.c
REN dirio.exe WC32w.exe
SET WATCOM=
SET PATH=%OLDPATH%
SET EDPATH=
SET INCLUDE=
SET LIB=
goto fin


:BORLAND
rem *** BORLAND C++ 5.02 ***
SET BORLAND=D:\FICHEROS\BCW\
SET OLDPATH=%PATH%
SET PATH=%BORLAND%\BIN;%PATH%
SET INCLUDE=%BORLAND%\INCLUDE;
SET LIB=%BORLAND%\LIB;
DEL BO16d.exe
bcc -1- -a2 -d -ff -k- -ms -po -RT- -O2 -I%INCLUDE% -L%LIB% dirio.c demo.c
ren dirio.exe BO16d.exe
rem SET BORLAND=
SET PATH=%OLDPATH%
SET INCLUDE=
SET LIB=


:MICROSOFT
rem *** VISUAL C++ 1.52 ***
SET MSC=D:\FICHEROS\VC\
SET OLDPATH=%PATH%
SET PATH=%MSC%\BIN;%PATH%
SET INCLUDE=%MSC%\INCLUDE;
SET LIB=%MSC%\LIB;
DEL MS16d.exe
cl /nologo /f- /Gr /Zp4 /Gy /W4 /Gf /O2 /Ox /Ob2 /D "NDEBUG" /D "_DOS" dirio.c demo.c
ren dirio.exe MS16d.exe
rem SET MSC=
SET PATH=%OLDPATH%
SET INCLUDE=
SET LIB=


:ZORTECH
rem *** DIGITALMARS C++ 8.36 ***
SET ZORTECH=D:\FICHEROS\DM
SET OLDPATH=%PATH%
SET PATH=%ZORTECH%\BIN;%PATH%
SET INCLUDE=%ZORTECH%\INCLUDE;
SET LIB=%ZORTECH%\LIB;
DEL ZC16d.exe
sc dirio.c demo.c -msd -o+time -ff -0 -a4 -Nc
ren dirio.exe ZC16d.exe
DEL ZC32d.exe
sc dirio.c demo.c x32.lib -mx -o+time -ff -3 -a4 -Nc 
ren dirio.exe ZC32d.exe
rem SET ZORTECH=
SET PATH=%OLDPATH%
SET INCLUDE=
SET LIB=


:FIN
DEL *.obj
DEL *.pch
DEL *.tds
DEL *.bak
DEL *.err
DEL *.map

:SHUTDOWN