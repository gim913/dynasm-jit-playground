call "C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\vcvarsall.bat" amd64
@echo off
set PLAT=x64

pushd src

%LUA% %DYNASMDIR%/dynasm.lua %DASMFLAGS% examp1_%PLAT%.dasc |sed "s/^# /#line /" > examp1_%PLAT%.h
%LUA% %DYNASMDIR%/dynasm.lua %DASMFLAGS% examp2_%PLAT%.dasc |sed "s/^# /#line /" > examp2_%PLAT%.h
%LUA% %DYNASMDIR%/dynasm.lua %DASMFLAGS% gram_%PLAT%.dasc |sed "s/^# /#line /" > gram_%PLAT%.h

%CC% %CFLAGS% %CPPFLAGS% dynasm-helper.cpp /Fo%OBJDIR% 
%CC% %CFLAGS% %CPPFLAGS% examp1.cpp /Fo%OBJDIR% 
%CC% %CFLAGS% %CPPFLAGS% examp2.cpp /Fo%OBJDIR% 
%CC% %CFLAGS% %CPPFLAGS% gRamMachine.cpp /Fo%OBJDIR% 

%REALLINK% %LDFLAGS% /out:../bin/examp1_%PLAT%.exe %OBJDIR%dynasm-helper.obj %OBJDIR%examp1.obj
%REALLINK% %LDFLAGS% /out:../bin/examp2_%PLAT%.exe %OBJDIR%dynasm-helper.obj %OBJDIR%examp2.obj
%REALLINK% %LDFLAGS% /out:../bin/gramMachine_%PLAT%.exe %OBJDIR%dynasm-helper.obj %OBJDIR%gRamMachine.obj 

popd src

@echo on
