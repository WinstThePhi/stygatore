@echo off

if not exist build mkdir build
pushd build
cl /c /FC /nologo /Zi ..\code\win32\time_util.c
cl /c /FC /nologo /Zi ..\code\gen_struct.c

link /nologo /DEBUG time_util.obj gen_struct.obj /out:gen_struct.exe

popd