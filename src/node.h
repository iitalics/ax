#pragma once
#include "ax.h"
#include "utils.h"


typedef size_t node_id;

#define NULL_ID         SIZE_MAX
#define ID_IS_NULL(_id) ((_id) == NULL_ID)

struct ax_node_c {
    size_t n_lines;
    size_t* line_count;
    enum ax_justify main_justify;
    enum ax_justify cross_justify;
    bool single_line;
};

struct ax_node_t {
    ax_color color;
    char* text;
    void* font;
    struct ax_node_t_line* lines;
};

struct ax_node_t_line {
    struct ax_node_t_line* next;
    struct ax_pos coord;
    char str[0];
};

struct ax_node {
    // properties
    enum ax_node_type ty;
    uint32_t grow_factor;
    uint32_t shrink_factor;
    enum ax_justify cross_justify;
    union {
        struct ax_node_c c;
        struct ax_desc_r r;
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
    size_t count, capacity;
    struct ax_node* nodes;
};


static inline
node_id ax_new_id(struct ax_tree* tree)
{
    ASSERT(tree->count < tree->capacity, "too many nodes!");
    return tree->count++;
}

static inline
struct ax_node* ax_node_by_id(struct ax_tree* tree, node_id id)
{
    return &tree->nodes[id];
}

static inline
struct ax_node* ax_root(struct ax_tree* tree)
{
    return ax_node_by_id(tree, 0);
}

void ax__free_node_t_line(struct ax_node_t_line* line);


#define NO_SUCH_NODE_TAG() NO_SUCH_TAG("ax_node_type")

#define DEFINE_TRAVERSAL_LOCALS(_t, ...)        \
    struct ax_tree* _trav_tree = _t;            \
    node_id _trav_id;                           \
    struct ax_node* __VA_ARGS__

#define FOR_EACH_FROM_TOP(_n)                               \
    for (_trav_id = 0;                                      \
         _trav_id < (_trav_tree)->count;                    \
         _trav_id++)                                        \
        if ((_n = ax_node_by_id(_trav_tree, _trav_id)), 1)

#define FOR_EACH_FROM_BOTTOM(_n)                                \
    for (_trav_id = _trav_tree->count;                          \
         _trav_id > 0;                                          \
         _trav_id--)                                            \
        if ((_n = ax_node_by_id(_trav_tree, _trav_id - 1)), 1)

#define FOR_EACH_CHILD(_n, _c)                              \
    for (_trav_id = (_n)->first_child_id;                   \
         !ID_IS_NULL(_trav_id);                             \
         _trav_id = (_c)->next_node_id)                     \
        if ((_c = ax_node_by_id(_trav_tree, _trav_id)), 1)

#define FOR_EACH_CHILD_IN_LINES(_li, _i, _n, _c)    \
    if (_li = _i = 0, 1)                            \
        FOR_EACH_CHILD(_n, _c)                      \
            if (_i >= (_n)->c.line_count[_li] ?     \
                (_li++, _i = 1) :                   \
                _i++, 1)
