C = gcc

TIME_UTIL = code/linux/time_util.c

all: link

link: build build/gen_struct.out 

build/gen_struct.out: code/gen_struct.c code/layer.h code/linux/linux_platform.h code/linux/linux_platform.c
	$(C) -O2 code/gen_struct.c -o build/gen_struct.out

build: 
	mkdir build

clean:
	rm -rf build/*
