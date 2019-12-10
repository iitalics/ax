#include <string.h>

#define AX_DEFINE_TRAVERSAL_MACROS 1
#include "../tree.h"
#include "../tree/desc.h"
#include "../utils.h"
#include "../backend.h"

void ax__init_tree(struct ax_tree* tr)
{
    tr->count = 0;
    tr->capacity = 512;
    tr->nodes = malloc(sizeof(struct ax_node) * tr->capacity);
    ASSERT(tr->nodes != NULL, "malloc ax_tree.nodes");
}

void ax__free_tree(struct ax_tree* tr)
{
    ax__tree_clear(tr);
    free(tr->nodes);
}

void ax__tree_clear(struct ax_tree* tr)
{
    DEFINE_TRAVERSAL_LOCALS(tr, node);
    FOR_EACH_FROM_BOTTOM(node) {
        ax__free_node(node);
    }
    tr->count = 0;
}

void ax__free_node(struct ax_node* node)
{
    switch (node->ty) {
    case AX_NODE_CONTAINER:
        if (node->c.line_count != NULL) {
            free(node->c.line_count);
        }
        break;
    case AX_NODE_TEXT:
        ax__free_node_t_line(node->t.lines);
        ax__destroy_font(node->t.font);
        free(node->t.text);
        break;
    default:
        break;
    }
}

void ax__free_node_t_line(struct ax_node_t_line* line)
{
    struct ax_node_t_line* next;
    for (; line != NULL; line = next) {
        next = line->next;
        free(line);
    }
}


int ax__build_node(struct ax_state* s,
                   struct ax_backend* bac,
                   struct ax_tree* tr,
                   const struct ax_desc* desc,
                   node_id* out_id)
{
    // NOT a stack-less traversal :(

    node_id id = ax__new_id(tr);
    struct ax_node* node = ax__node_by_id(tr, id);
    node->ty = desc->ty;
    node->first_child_id = NULL_ID;
    node->next_node_id = NULL_ID;
    switch (desc->ty) {

    case AX_NODE_CONTAINER: {
        node->c.n_lines = 0;
        node->c.line_count = NULL;
        node->c.main_justify = desc->c.main_justify;
        node->c.cross_justify = desc->c.cross_justify;
        node->c.single_line = desc->c.single_line;
        node->c.background = desc->c.background;
        node_id prev_id = NULL_ID;
        for (const struct ax_desc* child_desc = desc->c.first_child;
             child_desc != NULL;
             child_desc = child_desc->flex_attrs.next_child)
        {
            // TODO: no recursion!
            node_id child_id;
            int r = ax__build_node(s, bac, tr, child_desc, &child_id);
            if (r != 0) {
                return r;
            }
            struct ax_node* child = ax__node_by_id(tr, child_id);
            child->grow_factor = child_desc->flex_attrs.grow;
            child->shrink_factor = child_desc->flex_attrs.shrink;
            child->cross_justify = child_desc->flex_attrs.cross_justify;
            if (ID_IS_NULL(prev_id)) {
                ax__node_by_id(tr, id)->first_child_id = child_id;
            } else {
                ax__node_by_id(tr, prev_id)->next_node_id = child_id;
            }
            prev_id = child_id;
        }
        break;
    }

    case AX_NODE_RECTANGLE:
        node->r = desc->r;
        break;

    case AX_NODE_TEXT: {
        char* text = malloc(strlen(desc->t.text) + 1);
        ASSERT(text != NULL, "malloc to copy ax_node_t.desc.text");
        strcpy(text, desc->t.text);
        struct ax_font* font = NULL;
        int r = ax__new_font(s, bac, desc->t.font_name, &font);
        if (r != 0) {
            return r;
        }
        node->t = (struct ax_node_t) {
            .color = desc->t.color,
            .text = text,
            .font = font,
            .lines = NULL,
        };
        break;
    }

    default: NO_SUCH_NODE_TAG();
    }
    *out_id = id;
    return 0;
}
