@echo off
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

echo %LUA% %DYNASMDIR%/dynasm.lua %DASMFLAGS% examp1_x86.dasc 

%LUA% %DYNASMDIR%/dynasm.lua %DASMFLAGS% examp1_x86.dasc |sed "s/^# /#line /" > examp1_x86.h


echo %CC% %CFLAGS% %CPPFLAGS% dynasm-helper.cpp -DJIT=\"examp1.cpp\"
%CC% %CFLAGS% %CPPFLAGS% dynasm-helper.cpp -DJIT=\"examp1.cpp\"

%REALLINK% %LDFLAGS% /out:examp1_x86.exe dynasm-helper.obj

@echo on
