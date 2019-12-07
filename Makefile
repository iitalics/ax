cc		= gcc
cpp		= ${cc} -E
etags		= etags
rkt 		= racket
cc_flags	= -std=c99 -g -Wall -Werror=implicit-function-declaration
sdl_link_flags	= $(shell pkg-config -libs sdl2 SDL2_ttf)

srcs		= $(shell ${find_srcs})
gen		= _build/parser_rules.inc
test_srcs	= $(wildcard test/test_*.c)
test_gen	= _build/run_tests.inc
objs		= $(shell ${find_srcs} | ${sed_src2obj})

find_srcs	= find src -name '*.c'
sed_src2obj	= sed -e 's/src\/\(.*\)\//_build\/src__\1__/;s/$$/.o/'
sed_obj2src	= sed -e 's/_build\/src__\([^_]*\)__/src\/\1\//;s/\.o//'
sed_2dep	= sed -e 's/$$/.dep/'


# make commands

all: ax_test # ax_sdl_test

clean:
	test -d _build && rm -rf _build
	rm -f ax_test ax_sdl_test

rebuild: clean all

run_test: ./ax_test
	./$<

run_sdl_test: ./ax_sdl_test
	./$<

c: clean

r: rebuild

t: run_test

st: run_sdl_test


# executables

ax_test: ${gen} ${test_gen} ${test_srcs} ${objs} test/main.c
	@echo CC test/main.c
	@${cc} ${cc_flags} ${objs} ${test_srcs} test/main.c -o $@

ax_sdl_test: ${gen} ${objs} test/sdl_main.c
	@echo CC test/sdl_main.c
	@${cc} ${sdl_link_flags} ${cc_flags} ${objs} test/sdl_main.c -o $@


# objects and generated files

include $(wildcard _build/*.dep)

_build/src__%: obj = $@
_build/src__%: dep = $(shell echo ${obj} | ${sed_2dep})
_build/src__%: src = $(shell echo ${obj} | ${sed_obj2src})
_build/src__%: src
	@mkdir -p _build
	@${cpp} -MQ ${obj} -MM ${src} -o ${dep}
	@echo CC ${src}
	@${cc} ${cc_flags} -c -o ${obj} ${src}

_build/parser_rules.inc: scripts/rules.rkt scripts/sexp-yacc.rkt
	@mkdir -p _build
	@echo SCRIPT $<
	@${rkt} $< > $@

_build/run_tests.inc: scripts/find-tests.rkt ${test_srcs}
	@mkdir -p _build
	@echo SCRIPT $<
	@${rkt} scripts/find-tests.rkt ${test_srcs} > $@

TAGS: tags_srcs = $(shell find src test -name '*.c' -or -name '*.c' -or -name '*.h')
TAGS: ${tags_srcs}
	${etags} ${tags_srcs}

.PHONY: all clean rebuild run_test run_sdl_test c r t st
