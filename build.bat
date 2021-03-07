@echo off

if not exist build mkdir build
pushd build
cl /FC /nologo /O2 ..\code\gen_struct.c /link /out:gen_struct.exe
popd