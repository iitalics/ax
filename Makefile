cc		= gcc
cpp		= ${cc} -E
ld		= ${cc}
arc		= ar cr
etags		= etags
rkt 		= racket
cc_flags	= -std=gnu99 -g -pthread -fPIC \
			-Wall -Wshadow -Wpointer-arith  -Wstrict-prototypes \
			-Wmissing-prototypes -Wfloat-equal \
			-Werror=implicit-function-declaration
cc_flags	+= -DAX_TEST_NO_STRESS_TESTS
so_flags	= -shared


srcs		= $(shell ${find_srcs})
gen		= _build/parser_rules.inc
test_srcs	= $(wildcard test/test_*.c)
test_gen	= _build/run_tests.inc
objs		= $(shell ${find_srcs} | ${sed_src2obj})

libs		= _build/lib/libaxl_SDL.so _build/lib/libaxl_fortest.so
exes		= ax_test ax_bench ax_sdl_test

find_srcs	= find src -type f -name '*.c'
sed_src2obj	= sed -e 's/src\/\(.*\)\//_build\/src__\1__/;s/$$/.o/'
sed_obj2src	= sed -e 's/_build\/src__\([^_]*\)__/src\/\1\//;s/\.o//'
sed_obj2dep	= sed -e 's/\.c\.o$$/.dep/'


# make commands

all: ${libs} ${exes}

c:
	rm -rf _build ${exes}

t: ax_test
	LD_LIBRARY_PATH=_build/lib ./$< ${args}

b: logfile := $(shell date '+_bench/log_%m%d_%H%M%S%N.txt')
b: ax_bench
	@mkdir -p _bench
	@. ./benchmark.sh ${args}

cb:
	rm -rf _bench

sdl_t: ax_sdl_test
	LD_LIBRARY_PATH=_build/lib ./$<

.PHONY: all c t sdl_t


# executables

ax_test: test/main.c ${test_gen} ${test_srcs} _build/lib/libaxl_fortest.so
	@echo "CC $<"
	@${cc} -L_build/lib -laxl_fortest \
		${cc_flags} $< ${test_srcs} -o $@

ax_bench: bench/main.c _build/lib/libaxl_fortest.so
	@echo "CC $<"
	@${cc} -L_build/lib -laxl_fortest \
		${cc_flags} $< -o $@

ax_sdl_test: test/ui_main.c _build/lib/libaxl_SDL.so
	@echo "CC $< (sdl)"
	@${cc} -L_build/lib -laxl_SDL \
		${cc_flags} test/ui_main.c -o $@


# objects and generated files

include $(wildcard _build/*.dep)

_build/src__%.o: obj = $@
_build/src__%.o: dep = $(shell echo ${obj} | ${sed_obj2dep})
_build/src__%.o: src = $(shell echo ${obj} | ${sed_obj2src})
_build/src__%.o:
	@mkdir -p $(dir $@)
	@echo "CC ${src}"
	@${cpp} -MM -MT $@ -MF ${dep} ${src}
	@${cc} ${cc_flags} -c -o ${obj} ${src}

_build/parser_rules.inc: scripts/rules.rkt scripts/sexp-yacc.rkt
	@mkdir -p $(dir $@)
	@echo "SCRIPT $<"
	@${rkt} $< > $@

_build/run_tests.inc: scripts/find-tests.rkt ${test_srcs}
	@mkdir -p $(dir $@)
	@echo "SCRIPT $<"
	@${rkt} scripts/find-tests.rkt ${test_srcs} > $@

TAGS: tags_srcs = $(shell find src test -name '*.c' -or -name '*.c' -or -name '*.h')
TAGS: ${tags_srcs}
	${etags} ${tags_srcs}


# libraries

ld_flags_SDL = $(shell pkg-config -libs -cflags sdl2 SDL2_ttf)

_build/lib/ax.a: ${gen} ${objs}
	@mkdir -p $(dir $@)
	@echo "AR $(notdir $@)"
	@${arc} $@ ${objs}

_build/backend__%.o: obj = $@
_build/backend__%.o: name = $(obj:_build/backend__%.o=%)
_build/backend__%.o: src = backend/${name}.c
_build/backend__%.o: dep = _build/backend__${name}.dep
_build/backend__%.o: backend/%.c
	@mkdir -p $(dir $@)
	@echo "CC $<"
	@${cpp} -MM -MT $@ -MF ${dep} ${src}
	@${cc} ${cc_flags} -c -o ${obj} ${src}

_build/lib/libaxl_%.so: lib = $@
_build/lib/libaxl_%.so: name = $(lib:_build/lib/libaxl_%.so=%)
_build/lib/libaxl_%.so: obj = _build/backend__${name}.o
_build/lib/libaxl_%.so: ld_flags = ${ld_flags_${name}}
_build/lib/libaxl_%.so: _build/lib/ax.a _build/backend__%.o
	@mkdir -p $(dir $@)
	@echo "LD $(notdir $@)"
	@${ld} ${so_flags} ${ld_flags} -o $@ \
		${obj} \
		-Wl,--whole-archive _build/lib/ax.a -Wl,--no-whole-archive

# please don't delete these .o files!
.PRECIOUS: $(patsubst $(wildcard src/backend.c),src/%.c,_build/backend__%.o)
