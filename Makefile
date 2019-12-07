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

find_srcs	= find src -type f -name '*.c'
sed_src2obj	= sed -e 's/src\/\(.*\)\//_build\/src__\1__/;s/$$/.o/'
sed_obj2src	= sed -e 's/_build\/src__\([^_]*\)__/src\/\1\//;s/\.o//'
sed_obj2dep	= sed -e 's/\.c\.o$$/.dep/'


# make commands

all: ax_test ax_sdl_test

c:
	test -d _build && rm -rf _build
	rm -f ax_test ax_sdl_test

t: ax_test
	./$<

sdl_t: ax_sdl_test
	./$<

.PHONY: all c t sdl_t


# executables

ax_test: ${gen} ${test_gen} ${test_srcs} ${objs} test/main.c
	@echo CC "test/main.c"
	@${cc} ${cc_flags} ${objs} ${test_srcs} test/main.c -o $@

ax_sdl_test: ${gen} ${objs} backend/sdl.c test/ui_main.c
	@echo CC "test/ui_main.c (sdl)"
	@${cc} ${sdl_link_flags} ${cc_flags} ${objs} backend/sdl.c test/ui_main.c -o $@

# objects and generated files

include $(wildcard _build/*.dep)

_build/src__%.o: obj = $@
_build/src__%.o: dep = $(shell echo ${obj} | ${sed_obj2dep})
_build/src__%.o: src = $(shell echo ${obj} | ${sed_obj2src})
_build/src__%.o: ${src}
	@mkdir -p _build
	@echo CC ${src}
	@${cpp} -MM -MT ${obj} -MF ${dep} ${src}
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
