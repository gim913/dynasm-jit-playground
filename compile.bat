@echo off
set DYNASMDIR=/projects/LuaJIT-2.0.2/dynasm
set DASMFLAGS=-D WIN -D JIT -D FFI
set LUA=/projects/LuaJIT-2.0.2/src/luajit.exe
set OBJDIR=../obj/
set CFLAGS=/nologo /c /Zi /EHsc /MD /O2 /W3 /I%DYNASMDIR% /Tp
set LDFLAGS=/nologo /DEBUG
set CC=cl.exe
rem stupid link issue
set LINK=
set REALLINK=link.exe
set MT=mt.exe

mkdir bin
mkdir obj

cmd /c compile_x86.bat
cmd /c compile_x64.bat

@echo on
