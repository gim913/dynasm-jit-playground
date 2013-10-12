call "C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\vcvarsall.bat" x86
@echo off
set PLAT=x86

%LUA% %DYNASMDIR%/dynasm.lua %DASMFLAGS% examp1_%PLAT%.dasc |sed "s/^# /#line /" > examp1_%PLAT%.h
%LUA% %DYNASMDIR%/dynasm.lua %DASMFLAGS% examp2_%PLAT%.dasc |sed "s/^# /#line /" > examp2_%PLAT%.h

%CC% %CFLAGS% %CPPFLAGS% dynasm-helper.cpp
%CC% %CFLAGS% %CPPFLAGS% examp1.cpp
%CC% %CFLAGS% %CPPFLAGS% examp2.cpp

%REALLINK% %LDFLAGS% /out:examp1_%PLAT%.exe dynasm-helper.obj examp1.obj
%REALLINK% %LDFLAGS% /out:examp1_%PLAT%.exe dynasm-helper.obj examp2.obj

@echo on
