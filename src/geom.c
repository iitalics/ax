#include <stdlib.h>
#include <stdio.h>
#include "ax.h"
#include "utils.h"
#include "node.h"


#define MAIN(_d) (_d).w
#define CROSS(_d) (_d).h
#define MAX(_x, _y) (((_x) > (_y)) ? (_x) : (_y))
#define MIN(_x, _y) ((_x) < (_y)) ? (_x) : (_y)


static size_t ax_n_children(struct ax_tree* tree, const struct ax_node* node)
{
    DEFINE_TRAVERSAL_LOCALS(tree, child);
    size_t n = 0;
    FOR_EACH_CHILD(node, child) {
        n++;
    }
    return n;
}

static void ax_propagate_available_size(struct ax_tree* tr, struct ax_node* node)
{
    DEFINE_TRAVERSAL_LOCALS(tr, child);
    switch (node->ty) {

    case AX_NODE_CONTAINER:
        // TODO: apply constraints on container size
        FOR_EACH_CHILD(node, child) {
            child->avail = node->avail;
        }
        break;

    case AX_NODE_RECTANGLE:
    case AX_NODE_TEXT:
        break;

    default: NO_SUCH_NODE_TAG();
    }
}

static void ax_container_distribute_lines(struct ax_tree* tr, struct ax_node* node)
{
    DEFINE_TRAVERSAL_LOCALS(tr, child);
    size_t* const line_count = calloc(ax_n_children(tr, node), sizeof(size_t));
    ax_length const avail_size = MAIN(node->avail);
    size_t i = 0;
    ax_length line_size = 0.0;
    FOR_EACH_CHILD(node, child) {
        ax_length child_size = MAIN(child->hypoth);
        if (line_count[i] > 0 && line_size + child_size > avail_size) {
            i++;
            line_size = child_size;
        } else {
            line_size += child_size;
        }
        line_count[i]++;
    }
    size_t n_lines = line_count[i] == 0 ? i : (i + 1);
    node->c.line_count = line_count;
    node->c.n_lines = n_lines;
}

static void ax_compute_hypothetical_size(struct ax_tree* tr, struct ax_node* node)
{
    struct ax_dim hypoth;
    DEFINE_TRAVERSAL_LOCALS(tr, child);
    switch (node->ty) {

    case AX_NODE_CONTAINER: {
        ax_container_distribute_lines(tr, node);
        hypoth = AX_DIM(0.0, 0.0);
        struct ax_dim line = AX_DIM(0.0, 0.0);
#define UPDATE_HYPOTH() do {                                            \
            CROSS(hypoth) = CROSS(hypoth) + CROSS(line);                \
            MAIN(hypoth) = MAX(MAIN(hypoth), MAIN(line)); } while (0)
        size_t li, i, prev_li = 0;
        FOR_EACH_CHILD_IN_LINES(li, i, node, child) {
            if (li != prev_li) {
                prev_li = li;
                UPDATE_HYPOTH();
                line = AX_DIM(0.0, 0.0);
            }
            MAIN(line) = MAIN(line) + MAIN(child->hypoth);
            CROSS(line) = MAX(CROSS(line), CROSS(child->hypoth));
        }
        if (i > 0) {
            UPDATE_HYPOTH();
        }
        break;
#undef UPDATE_HYPOTH
    }

    case AX_NODE_RECTANGLE:
        hypoth = node->r.size;
        break;

    case AX_NODE_TEXT:
        // TODO: fill in stub impl
        hypoth = AX_DIM(0.0, 0.0);
        break;

    default: NO_SUCH_NODE_TAG();
    }
    node->hypoth = hypoth;
}

static void ax_resolve_target_size(struct ax_tree* tr, struct ax_node* node)
{
    DEFINE_TRAVERSAL_LOCALS(tr, child);
    switch (node->ty) {

    case AX_NODE_CONTAINER: {
        ax_length free_space = MAIN(node->target);
        FOR_EACH_CHILD(node, child) {
            free_space -= MAIN(node->hypoth);
        }
        // TODO: pool allocation of these temporary arrays
        struct line_calc {
            ax_length factor_sum;
            ax_length cross_size;
        };
        struct line_calc* const lines = calloc(node->c.n_lines, sizeof(struct line_calc));
        ASSERT(lines != NULL, "malloc line_calc array");
        size_t li, i;
        for (li = 0; li < node->c.n_lines; li++) {
            lines[li].factor_sum = 0.0;
            lines[li].cross_size = 0.0;
        }
        FOR_EACH_CHILD_IN_LINES(li, i, node, child) {
            lines[li].factor_sum += child->grow_factor;
            lines[li].cross_size = MAX(lines[li].cross_size, CROSS(child->hypoth));
        }
        FOR_EACH_CHILD_IN_LINES(li, i, node, child) {
            ax_length flex = 0.0;
            if (child->grow_factor != 0) {
                flex = free_space * child->grow_factor / lines[li].factor_sum;
            }
            MAIN(child->target) = MAIN(child->hypoth) + flex;
            CROSS(child->target) = CROSS(child->hypoth); //lines[li].cross_size;
        }
        free(lines);
        break;
    }

    case AX_NODE_RECTANGLE:
    case AX_NODE_TEXT:
        // TODO: re-calculate text? need to determine actual-height somehow
        break;

    default: NO_SUCH_NODE_TAG();
    }
}

static ax_length ax_justify_padding(enum ax_justify j, ax_length space,
                                    size_t n_items, ax_length* start)
{
    ax_length pad, offset;
    switch (j) {
    case AX_JUSTIFY_START:
        pad = 0.0;
        offset = 0.0;
        break;
    case AX_JUSTIFY_END:
        pad = 0.0;
        offset = space;
        break;
    case AX_JUSTIFY_CENTER:
        pad = 0.0;
        offset = space / 2;
        break;
    case AX_JUSTIFY_EVENLY:
        pad = space / (n_items + 1);
        offset = pad;
        break;
    case AX_JUSTIFY_AROUND:
        pad = n_items > 0 ? space / n_items : 0.0;
        offset = pad / 2;
        break;
    case AX_JUSTIFY_BETWEEN:
        pad = n_items > 1 ? space / (n_items - 1) : 0.0;
        offset = 0.0;
        break;
    default: NO_SUCH_TAG("ax_justify");
    }
    if (start != NULL)
        *start += offset;
    return pad;
}

static void ax_place_coords(struct ax_tree* tr, struct ax_node* node)
{
    DEFINE_TRAVERSAL_LOCALS(tr, child);
    switch (node->ty) {

    case AX_NODE_CONTAINER: {
        struct line_calc {
            ax_length flex_space;
            ax_length cross_size;
        };
        struct line_calc* lines = calloc(node->c.n_lines, sizeof(struct line_calc));
        ASSERT(lines != NULL, "malloc line_calc array");
        size_t li, i;
        for (li = 0; li < node->c.n_lines; li++) {
            lines[li].flex_space = MAIN(node->target);
            lines[li].cross_size = 0.0;
        }
        FOR_EACH_CHILD_IN_LINES(li, i, node, child) {
            lines[li].flex_space -= MAIN(child->target);
            lines[li].cross_size = MAX(lines[li].cross_size, CROSS(child->target));
        }
        ax_length cross_flex_space = CROSS(node->target);
        for (li = 0; li < node->c.n_lines; li++) {
            cross_flex_space -= lines[li].cross_size;
        }
        ax_length pad_x, pad_y;
        ax_length x, y = node->coord.y;
        pad_y = ax_justify_padding(node->c.cross_justify,
                                   cross_flex_space,
                                   node->c.n_lines,
                                   &y);
#define START_LINE() do {                                       \
            x = node->coord.x;                                  \
            pad_x = ax_justify_padding(node->c.main_justify,    \
                                       lines[li].flex_space,    \
                                       node->c.line_count[li],  \
                                       &x); } while(0)
        size_t prev_li = 0;
        li = 0;
        START_LINE();
        FOR_EACH_CHILD_IN_LINES(li, i, node, child) {
            if (li != prev_li) {
                y += lines[prev_li].cross_size + pad_y;
                prev_li = li;
                START_LINE();
            }
            child->coord = AX_POS(x, y);
            ax_justify_padding(child->cross_justify,
                               lines[li].cross_size - CROSS(child->target),
                               1,
                               &child->coord.y);
            x += MAIN(child->target) + pad_x;
        }
        free(lines);
        break;
#undef START_LINE
    }

    case AX_NODE_RECTANGLE:
        break;

    case AX_NODE_TEXT:
        // TODO: fill in stub impl
        break;

    default: NO_SUCH_NODE_TAG();
    }
}


void ax__compute_geometry(struct ax_tree* tr, struct ax_aabb aabb)
{
    DEFINE_TRAVERSAL_LOCALS(tr, node);

    ax_root(tr)->avail = aabb.s;
    FOR_EACH_FROM_TOP(node) {
        ax_propagate_available_size(tr, node);
    }

    FOR_EACH_FROM_BOTTOM(node) {
        ax_compute_hypothetical_size(tr, node);
    }

    ax_root(tr)->target = aabb.s;
    FOR_EACH_FROM_TOP(node) {
        ax_resolve_target_size(tr, node);
    }

    ax_root(tr)->coord = aabb.o;
    FOR_EACH_FROM_TOP(node) {
        ax_place_coords(tr, node);
    }
}
