#include <setjmp.h>
#include "helpers.h"


char _ax_test_fail_reason[1024] = {0};
char _ax_test_fail_loc[256] = {0};

const char* _ax_true_str = "true";
const char* _ax_false_str = "false";

static int n_ran = 0, n_ok = 0;
static jmp_buf test_fail_jump;

void _ax_test_fail()
{
    longjmp(test_fail_jump, 1);
}

static bool skip_test(
    char* name,
    int argc,
    char** argv)
{
    if (argc <= 1) {
        return false;
    }
    for (size_t i = 1; i < (size_t) argc; i++) {
        for (size_t prefix_len = 0; argv[i][prefix_len]; prefix_len++) {
            if (argv[i][prefix_len] != name[prefix_len]) {
                return true;
            }
        }
        return false;
    }
    return true;
}

static void run_test(
    char* name,
    void (*func)(),
    int argc,
    char** argv)
{
    if (skip_test(name, argc, argv)) {
        return;
    }

    printf("* %s... ", name);
    fflush(stdout);
    n_ran++;

    switch (setjmp(test_fail_jump)) {
    case 0:
        func();
        printf("OK\n");
        n_ok++;
        break;

    default:
        printf("FAILED\n");
        printf("%s: %s\n", _ax_test_fail_loc, _ax_test_fail_reason);
        break;
    }
}

int main(int argc, char** argv)
{
#define RUN_TEST(_name) do {                    \
        void test_ ## _name ();                 \
        run_test(# _name,                       \
                 test_ ## _name,                \
                 argc, argv); } while(0)
    printf("----------------------\n");

#include "../_build/tests.inc"

    printf("----------------------\n"
           "Results:\n"
           "  %d ran\n"
           "  %d succeeded\n",
           n_ran, n_ok);

    if (n_ok < n_ran) {
        printf("  %d failed\n", n_ran - n_ok);
        return 1;
    }
    return 0;
#undef RUN_TEST
}
