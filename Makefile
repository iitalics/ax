cc		= gcc
cpp		= ${cc} -E
ld		= ${cc}
etags		= etags
rkt 		= racket
cc_flags	= -std=c99 -g -Wall -Werror=implicit-function-declaration -fPIC

srcs		= $(shell ${find_srcs})
gen		= _build/parser_rules.inc
test_srcs	= $(wildcard test/test_*.c)
test_gen	= _build/run_tests.inc
objs		= $(shell ${find_srcs} | ${sed_src2obj})

libs		= _build/lib/libaxl.so _build/lib/libaxl_SDL.so
test_exes	= ax_test ax_sdl_test

find_srcs	= find src -type f -name '*.c'
sed_src2obj	= sed -e 's/src\/\(.*\)\//_build\/src__\1__/;s/$$/.o/'
sed_obj2src	= sed -e 's/_build\/src__\([^_]*\)__/src\/\1\//;s/\.o//'
sed_obj2dep	= sed -e 's/\.c\.o$$/.dep/'


# make commands

all: ${libs} ${test_exes}

c:
	rm -rf _build ${test_exes}

t: ax_test
	LD_LIBRARY_PATH=_build/lib ./$<

sdl_t: ax_sdl_test
	LD_LIBRARY_PATH=_build/lib ./$<

.PHONY: all c t sdl_t


# executables

ax_test: ${test_gen} ${test_srcs} test/main.c _build/lib/libaxl.so
	@echo CC "test/main.c"
	@${cc} -L_build/lib -laxl ${cc_flags} ${test_srcs} test/main.c -o $@

ax_sdl_test: test/ui_main.c _build/lib/libaxl.so _build/lib/libaxl_SDL.so
	@echo CC "test/ui_main.c (sdl)"
	@${cc} -L_build/lib -laxl -laxl_SDL \
		${cc_flags} test/ui_main.c -o $@


# objects and generated files

include $(wildcard _build/*.dep)

_build/src__%.o: obj = $@
_build/src__%.o: dep = $(shell echo ${obj} | ${sed_obj2dep})
_build/src__%.o: src = $(shell echo ${obj} | ${sed_obj2src})
_build/src__%.o: ${src}
	@mkdir -p $(dir $@)
	@echo CC ${src}
	@${cpp} -MM -MT ${obj} -MF ${dep} ${src}
	@${cc} ${cc_flags} -c -o ${obj} ${src}

_build/parser_rules.inc: scripts/rules.rkt scripts/sexp-yacc.rkt
	@mkdir -p $(dir $@)
	@echo SCRIPT $<
	@${rkt} $< > $@

_build/run_tests.inc: scripts/find-tests.rkt ${test_srcs}
	@mkdir -p $(dir $@)
	@echo SCRIPT $<
	@${rkt} scripts/find-tests.rkt ${test_srcs} > $@

TAGS: tags_srcs = $(shell find src test -name '*.c' -or -name '*.c' -or -name '*.h')
TAGS: ${tags_srcs}
	${etags} ${tags_srcs}


# libraries

_build/lib/libaxl.so: ${gen} ${objs}
	@mkdir -p $(dir $@)
	@echo "LD $(notdir $@)"
	@${ld} -shared -o $@ ${objs}

_build/lib/libaxl_SDL.so: backend/sdl.c
	@mkdir -p $(dir $@)
	@echo "LD $(notdir $@)"
	@${ld} -shared $(shell pkg-config -libs -cflags sdl2 SDL2_ttf) -o $@ $<
