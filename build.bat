@echo off

if not exist build mkdir build
pushd build
cl /c ..\code\win32\time_util.c
cl /c ..\code\gen_struct.c

link time_util.obj gen_struct.obj /out:gen_struct.exe

popd