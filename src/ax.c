#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "node.h"
#include "state.h"


void ax__compute_geometry(struct ax_tree* tr, struct ax_aabb aabb);
extern void ax__redraw(struct ax_tree* tr, struct ax_drawbuf* db);
extern struct ax_drawbuf* ax__create_drawbuf();
extern void ax__free_drawbuf(struct ax_drawbuf* db);


static void ax_init_tree(struct ax_tree* tr)
{
    tr->count = 0;
    tr->capacity = 512;
    tr->nodes = malloc(sizeof(struct ax_node) * tr->capacity);
    ASSERT(tr->nodes != NULL, "malloc ax_tree.nodes");
}

static node_id ax_build_node(struct ax_tree* tr, const struct ax_desc* desc)
{
    // NOT a stack-less traversal :(

    node_id id = ax_new_id(tr);
    struct ax_node* node = ax_node_by_id(tr, id);
    node->ty = desc->ty;
    node->first_child_id = NULL_ID;
    node->next_node_id = NULL_ID;
    switch (desc->ty) {

    case AX_NODE_CONTAINER: {
        node->c.n_lines = 0;
        node->c.line_count = NULL;
        node->c.main_justify = desc->c.main_justify;
        node->c.cross_justify = desc->c.cross_justify;
        node_id prev_id = NULL_ID;
        for (size_t i = 0; i < desc->c.n_children; i++) {
            struct ax_flex_child_desc flex_child = desc->c.children[i];
            node_id child_id = ax_build_node(tr, &flex_child.desc);
            struct ax_node* child = ax_node_by_id(tr, child_id);
            child->grow_factor = flex_child.grow;
            child->shrink_factor = flex_child.shrink;
            child->cross_justify = flex_child.cross_justify;
            if (ID_IS_NULL(prev_id)) {
                ax_node_by_id(tr, id)->first_child_id = child_id;
            } else {
                ax_node_by_id(tr, prev_id)->next_node_id = child_id;
            }
            prev_id = child_id;
        }
        break;
    }

    case AX_NODE_RECTANGLE:
        node->r = desc->r;
        break;

    case AX_NODE_TEXT:
        node->t.desc = desc->t;
        char* text = malloc(strlen(desc->t.text) + 1);
        ASSERT(text != NULL, "malloc to copy ax_node_t.desc.text");
        strcpy(text, desc->t.text);
        node->t.desc.text = text;
        break;

    default: NO_SUCH_NODE_TAG();
    }
    return id;
}

static void ax_free_node_data(struct ax_node* node)
{
    switch (node->ty) {
    case AX_NODE_CONTAINER:
        if (node->c.line_count != NULL) {
            free(node->c.line_count);
        }
        break;
    case AX_NODE_TEXT:
        free((void*) node->t.desc.text);
        break;
    default:
        break;
    }
}

static void ax_reset_tree(struct ax_tree* tr)
{
    DEFINE_TRAVERSAL_LOCALS(tr, node);
    FOR_EACH_FROM_BOTTOM(node) {
        ax_free_node_data(node);
    }
    tr->count = 0;
}

static void ax_free_tree_data(struct ax_tree* tr)
{
    ax_reset_tree(tr);
    free(tr->nodes);
}

static void ax_tree_set_root(struct ax_tree* tr, const struct ax_desc* desc)
{
    node_id root_id = ax_build_node(tr, desc);
    ASSERT(root_id == 0, "root id should be 0");
}


static struct ax_desc const empty_node_desc = {
    .ty = AX_NODE_CONTAINER,
    .c = {
        .children = NULL,
        .n_children = 0,
        .main_justify = AX_JUSTIFY_START,
        .cross_justify = AX_JUSTIFY_START
    },
};

struct ax_state* ax_new_state()
{
    struct ax_state* s = malloc(sizeof(struct ax_state));
    ASSERT(s != NULL, "malloc ax_state");
    s->root_dim = AX_DIM(0.0, 0.0);
    s->draw_buf = ax__create_drawbuf();
    ax_init_tree(&s->tree);
    ax_tree_set_root(&s->tree, &empty_node_desc);
    return s;
}

void ax_destroy_state(struct ax_state* s)
{
    if (s != NULL) {
        ax__free_drawbuf(s->draw_buf);
        ax_free_tree_data(&s->tree);
        free(s);
    }
}

static void ax_invalidate(struct ax_state* s)
{
    struct ax_aabb aabb;
    aabb.o = AX_POS(0.0, 0.0);
    aabb.s = s->root_dim;
    ax__compute_geometry(&s->tree, aabb);
    ax__redraw(&s->tree, s->draw_buf);
}

void ax_set_dimensions(struct ax_state* s, struct ax_dim dim)
{
    s->root_dim = dim;
    ax_invalidate(s);
}

struct ax_dim ax_get_dimensions(struct ax_state* s)
{
    return s->root_dim;
}

void ax_set_root(struct ax_state* s, const struct ax_desc* root_desc)
{
    ax_reset_tree(&s->tree);
    ax_tree_set_root(&s->tree, root_desc);
    ax_invalidate(s);
}

const struct ax_drawbuf* ax_draw(struct ax_state* s)
{
    return s->draw_buf;
}
