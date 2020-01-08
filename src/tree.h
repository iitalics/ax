#pragma once
#include "base.h"
#include "utils.h"
#include "core/region.h"

struct ax_state;
struct ax_backend;
struct ax_desc;
struct ax_font;

typedef size_t node_id;

enum ax_node_type {
    AX_NODE_CONTAINER = 0,
    AX_NODE_RECTANGLE,
    AX_NODE_TEXT,
    AX_NODE__MAX
};

enum ax_justify {
    AX_JUSTIFY_START = 0,
    AX_JUSTIFY_END,
    AX_JUSTIFY_CENTER,
    AX_JUSTIFY_EVENLY,
    AX_JUSTIFY_AROUND,
    AX_JUSTIFY_BETWEEN,
    AX_JUSTIFY__MAX
};

struct ax_node_c {
    size_t n_lines;
    size_t* line_count;
    enum ax_justify main_justify;
    enum ax_justify cross_justify;
    bool single_line;
    ax_color background;
};

struct ax_rect {
    ax_color fill;
    struct ax_dim size;
};

struct ax_node_t {
    ax_color color;
    char* text;
    struct ax_font* font;
    struct ax_node_t_line* lines;
};

struct ax_node_t_line {
    struct ax_node_t_line* next;
    struct ax_pos coord;
    char* str;
};

struct ax_node {
    // properties
    enum ax_node_type ty;
    uint32_t grow_factor;
    uint32_t shrink_factor;
    enum ax_justify cross_justify;
    union {
        struct ax_node_c c;
        struct ax_rect r;
        struct ax_node_t t;
    };

    // geometry computations
    struct ax_dim avail; // TODO: infinite avail size
    struct ax_dim hypoth;
    struct ax_dim target;
    struct ax_pos coord;

    // intrusive linked list
    node_id first_child_id;
    node_id next_node_id;
};

struct ax_tree {
    struct region rgn;
    size_t count, capacity;
    struct ax_node* nodes;
};

#define NULL_ID             SIZE_MAX
#define ID_IS_NULL(_id)     ((_id) == NULL_ID)
#define NO_SUCH_NODE_TAG()  NO_SUCH_TAG("ax_node_type")

void ax__init_tree(struct ax_tree* tr);
void ax__free_tree(struct ax_tree* tr);
void ax__tree_clear(struct ax_tree* tr);

void ax__free_node(struct ax_node* node);

int ax__build_node(struct ax_state* s,     // used for ax__set_error()
                   struct ax_backend* bac, // used to load fonts
                   struct ax_tree* tr,
                   const struct ax_desc* desc,
                   node_id* out_id);

static inline
void ax__tree_drain_from(struct ax_tree* tr,
                         struct ax_tree* other)
{
    struct ax_tree tmp = *tr;
    *tr = *other;
    *other = tmp;
    ax__tree_clear(other);
}

static inline
bool ax__is_tree_empty(struct ax_tree* tree)
{
    return tree->count == 0;
}

static inline
struct ax_node* ax__node_by_id(struct ax_tree* tree, node_id id)
{
    return &tree->nodes[id];
}

static inline
struct ax_node* ax__root(struct ax_tree* tree)
{
    return ax__node_by_id(tree, 0);
}

static inline
node_id ax__new_id(struct ax_tree* tree)
{
    ASSERT(tree->count < tree->capacity, "too many nodes!");
    return tree->count++;
}

#ifdef AX_DEFINE_TRAVERSAL_MACROS

#define DEFINE_TRAVERSAL_LOCALS(_t, ...)        \
    struct ax_tree* _trav_tree = _t;            \
    node_id _trav_id;                           \
    struct ax_node* __VA_ARGS__

#define FOR_EACH_FROM_TOP(_n)                               \
    for (_trav_id = 0;                                      \
         _trav_id < (_trav_tree)->count;                    \
         _trav_id++)                                        \
        if ((_n = ax__node_by_id(_trav_tree, _trav_id)), 1)

#define FOR_EACH_FROM_BOTTOM(_n)                                \
    for (_trav_id = _trav_tree->count;                          \
         _trav_id > 0;                                          \
         _trav_id--)                                            \
        if ((_n = ax__node_by_id(_trav_tree, _trav_id - 1)), 1)

#define FOR_EACH_CHILD(_n, _c)                              \
    for (_trav_id = (_n)->first_child_id;                   \
         !ID_IS_NULL(_trav_id);                             \
         _trav_id = (_c)->next_node_id)                     \
        if ((_c = ax__node_by_id(_trav_tree, _trav_id)), 1)

#define FOR_EACH_CHILD_IN_LINES(_li, _i, _n, _c)    \
    if (_li = _i = 0, 1)                            \
        FOR_EACH_CHILD(_n, _c)                      \
            if (_i >= (_n)->c.line_count[_li] ?     \
                (_li++, _i = 1) :                   \
                _i++, 1)

#endif//AX_DEFINE_TRAVERSAL_MACROS
