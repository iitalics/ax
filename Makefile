c_srcs = $(wildcard src/*.c)
c_hdr_srcs = $(wildcard src/*.h)
c_objs = $(c_srcs:src/%=_build/%.o)
gen = _build/parser_rules.inc

cc = gcc
cpp = ${cc} -E
c_flags = -Wall -std=c99 -g

c_sdl_test_srcs  = test/sdl_test.c
c_sdl_test_flags = ${c_flags} $(shell pkg-config -libs sdl2 SDL2_ttf)

c_test_srcs  = $(wildcard test/main.c test/test_*.c)
c_test_flags = ${c_flags}
test_gen   = _build/tests.inc

etags = etags
tags_srcs = ${c_srcs} ${c_hdr_srcs} ${c_test_srcs}

py3 = python3
rkt = racket

all: ax_test ax_sdl_test

clean:
	test -d _build && rm -rf _build
	rm -f ax_test ax_sdl_test

rebuild: clean all

ax_sdl_test: ${c_sdl_test_srcs} ${gen} ${c_objs}
	${cc} ${c_sdl_test_flags} ${c_objs} ${c_sdl_test_srcs} -o $@

run_sdl_test: ax_sdl_test
	./ax_sdl_test

ax_test: ${c_test_srcs} ${test_gen} ${gen} ${c_objs}
	${cc} ${c_test_flags} ${c_objs} ${c_test_srcs} -o $@

run_test: ax_test
	./ax_test ${test_args}

TAGS: ${tags_srcs}
	${etags} ${tags_srcs}

_build:
	mkdir _build

include $(wildcard _build/*.dep)

_build/%.c.o: src/%.c
	@mkdir -p _build
	@${cpp} -MQ $@ -MM $< -o $(<:src/%.c=_build/%.c.dep)
	${cc} ${c_flags} -c -o $@ $<

_build/tests.inc: ${c_test_srcs}
	@mkdir -p _build
	${py3} scripts/find_tests.py test $@

_build/parser_rules.inc: scripts/rules.rkt scripts/sexp-yacc.rkt
	@mkdir -p _build
	${rkt} $< > $@

.PHONY: all clean rebuild run_test run_sdl_test


c: clean

re: rebuild

t: run_test

.PHONY: c re t
