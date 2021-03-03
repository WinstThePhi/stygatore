C = gcc

TIME_UTIL = code/linux/time_util.c

all: link

link: build/gen_struct.o build/time_util.o
	$(C) -g build/gen_struct.o build/time_util.o -o build/gen_struct.out

build/gen_struct.o: code/gen_struct.c code/layer.h
	$(C) -c -g ~/dev/c_template_system/code/gen_struct.c -o build/gen_struct.o

build/time_util.o: $(TIME_UTIL) code/linux/time_util.h
	$(C) -c -g $(TIME_UTIL) -o build/time_util.o

clean:
	rm -rf build/*
