c_srcs = $(wildcard src/*.c)
c_hdr_srcs = $(wildcard src/*.h)
c_objs = $(c_srcs:src/%=_build/%.o)

cc = gcc
c_flags = -Wall -std=c99 -g

c_sdl_test_srcs  = test/sdl_test.c
c_sdl_test_flags = ${c_flags} $(shell pkg-config -libs sdl2 SDL2_ttf)

c_test_srcs  = $(wildcard test/main.c test/test_*.c)
c_test_gen   = _build/tests.inc
c_test_flags = ${c_flags}

etags = etags
tags_srcs = ${c_srcs} ${c_hdr_srcs} ${c_test_srcs}

py3 = python3


all: ax_test ax_sdl_test

clean:
	test -d _build && rm -rf _build
	rm -f ax_test ax_sdl_test

ax_sdl_test: ${c_sdl_test_srcs} ${c_objs}
	${cc} ${c_sdl_test_flags} ${c_objs} ${c_sdl_test_srcs} -o $@

run_sdl_test: ax_sdl_test
	./ax_sdl_test

_build/tests.inc: _build ${c_test_srcs}
	${py3} scripts/find_tests.py test $@

ax_test: ${c_test_srcs} ${c_test_gen} ${c_objs}
	${cc} ${c_test_flags} ${c_objs} ${c_test_srcs} -o $@

run_test: ax_test
	./ax_test

TAGS: ${tags_srcs}
	${etags} ${tags_srcs}

_build:
	mkdir _build

_build/%.c.o: src/%.c _build
	${cc} ${c_flags} -c -o $@ $<

.PHONY: all clean run_test run_sdl_test
