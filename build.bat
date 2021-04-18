@echo off

if not exist build mkdir build
pushd build
cl /O2 /FC /nologo /Zi ..\code\gen_struct.c /link /out:gen_struct.exe
popd