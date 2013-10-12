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

cmd /c compile_x86.bat
cmd /c compile_x64.bat

@echo on
