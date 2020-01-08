#include "helpers.h"
#include "../src/base.h"
#include "../src/core/region.h"
#include "../src/core/growable.h"

struct list {
    int elem;
    struct list* next;
};

static struct list* from_array(struct region* rgn, int* a, size_t n)
{
    struct list* la = NULL;
    for (size_t i = 0; i < n; i++) {
        struct list* l = ALLOCATE(rgn, struct list);
        l->elem = a[n - i - 1];
        l->next = la;
        la = l;
    }
    return la;
}

static struct list const* ith(struct list const* l, size_t i)
{
    for (size_t j = 0; j < i; j++) {
        l = l->next;
    }
    return l;
}

static struct list* reverse(struct region* rgn, struct list const* l1)
{
    struct list* l_rev = NULL;
    while (l1 != NULL) {
        struct list* l = ALLOCATE(rgn, struct list);
        l->elem = l1->elem;
        l->next = l_rev;
        l_rev = l;
        l1 = l1->next;
    }
    return l_rev;
}

TEST(rgn_list_reverse)
{
    struct region rgn[1];
    ax__init_region(rgn);
    int a[3] = { 1, 2, 3 };
    struct list* l1 = from_array(rgn, a, 3);
    struct list* l2 = reverse(rgn, l1);
    CHECK_IEQ(ith(l2, 0)->elem, 3);
    CHECK_IEQ(ith(l2, 1)->elem, 2);
    CHECK_IEQ(ith(l2, 2)->elem, 1);
    CHECK_NULL(ith(l2, 3));
    ax__free_region(rgn);
}

TEST(rgn_lots_of_small)
{
    struct region rgn[1];
    ax__init_region(rgn);
    for (size_t i = 0; i < 5000; i++) {
        (void) ALLOCATE(rgn, int);
    }
    ax__region_clear(rgn);
    for (size_t i = 0; i < 10000; i++) {
        (void) ALLOCATE(rgn, int);
    }
    ax__free_region(rgn);
}

TEST(rgn_big)
{
    struct big {
        int stuff[2048];
    };
    struct region rgn[1];
    ax__init_region(rgn);
    for (int i = 0; i < 5; i++) {
        (void) ALLOCATE(rgn, struct big);
    }
    ax__region_clear(rgn);
    for (int i = 0; i < 5; i++) {
        (void) ALLOCATE(rgn, struct big);
    }
    ax__free_region(rgn);
}

TEST(grow_structs)
{
    struct growable g;
    ax__init_growable(&g, DEFAULT_CAPACITY);
    for (int i = 0; i < 100; i++) { PUSH(&g, &AX_DIM(i, i * 0.5)); }
    struct ax_dim* dims = g.data;
    for (int i = 0; i < 100; i++) { CHECK_DIMEQ(dims[i], AX_DIM(i, i * 0.5)); }
    ax__growable_clear(&g);
    CHECK_TRUE(ax__is_growable_empty(&g));
    for (int i = 0; i < 50; i++) { PUSH(&g, &AX_DIM(i, i * 0.3)); }
    dims = g.data;
    for (int i = 0; i < 50; i++) { CHECK_DIMEQ(dims[i], AX_DIM(i, i * 0.3)); }
    ax__free_growable(&g);
}

TEST(grow_string)
{
    struct growable g;
    ax__init_growable(&g, DEFAULT_CAPACITY);
    ax__growable_clear_str(&g);
    CHECK_STREQ((char*) g.data, "");
    ax__growable_push_str(&g, "hello");
    CHECK_STREQ((char*) g.data, "hello");
    ax__growable_push_char(&g, ',');
    CHECK_STREQ((char*) g.data, "hello,");
    ax__growable_push_str(&g, " world");
    CHECK_STREQ((char*) g.data, "hello, world");
    ax__free_growable(&g);
}
