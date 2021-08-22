C = gcc

all: link

link: build build/gen_struct.out 

build/gen_struct.out: code/gen_struct.c code/gen_struct.h code/layer.h
	$(C) -g code/gen_struct.c -o build/gen_struct.out

build: 
	mkdir build

clean:
	rm -rf build/*
