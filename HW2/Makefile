.PHONY: all build clean


all: build

testcode: build
	gcc test_code.c -o test_code

sample1:
	gcc -o sample -Wall -g sample.c
	nm -D sample
sample_big:
	gcc sample.c -o sample2 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
	nm -D sample2
build:
	gcc -g sandbox.c -o sandbox
	gcc -o sandbox.so -shared -fPIC injected_library.c -ldl

clean:
	rm -f sandbox
	rm -f sandbox.so
