C = gcc

ifeq ($(shell uname -s), Linux)
TIME_UTIL = code/linux/time_util.c
else
TIME_UTIL = code/win32/time_util.c
endif

all: link

link: build/gen_struct.o build/time_util.o
	$(C) build/gen_struct.o build/time_util.o -o build/gen_struct.out

build/gen_struct.o: code/gen_struct.c code/layer.h
	$(C) -c code/gen_struct.c -o build/gen_struct.o

build/time_util.o: $(TIME_UTIL) code/linux/time_util.h
	$(C) -c $(TIME_UTIL) -o build/time_util.o

clean:
	rm -rf build/*
