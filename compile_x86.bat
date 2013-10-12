call "C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\vcvarsall.bat" x86
@echo off
echo %Platform%
set DYNASMDIR=/projects/LuaJIT-2.0.2/dynasm
set DASMFLAGS=-D WIN -D JIT -D FFI
set LUA=/projects/LuaJIT-2.0.2/src/luajit.exe
set CFLAGS=/nologo /c /EHsc /MD /O2 /W3 /I%DYNASMDIR% /Tp
set LDFLAGS=/nologo
set CC=cl.exe
rem stupid link issue
set LINK=
set REALLINK=link.exe
set MT=mt.exe
set PLAT=x86

echo %LUA% %DYNASMDIR%/dynasm.lua %DASMFLAGS% examp1_%PLAT%.dasc 

%LUA% %DYNASMDIR%/dynasm.lua %DASMFLAGS% examp1_%PLAT%.dasc |sed "s/^# /#line /" > examp1_%PLAT%.h


echo %CC% %CFLAGS% %CPPFLAGS% dynasm-helper.cpp -DJIT=\"examp1.cpp\"
%CC% %CFLAGS% %CPPFLAGS% dynasm-helper.cpp -DJIT=\"examp1.cpp\"

%REALLINK% %LDFLAGS% /out:examp1_%PLAT%.exe dynasm-helper.obj

@echo on
