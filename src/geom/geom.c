#include <string.h>

#define AX_DEFINE_TRAVERSAL_MACROS
#include "text.h"
#include "../geom.h"
#include "../tree.h"
#include "../backend.h"
#include "../utils.h"

#define MAIN(_d) (_d).w
#define CROSS(_d) (_d).h
#define MAX(_x, _y) (((_x) > (_y)) ? (_x) : (_y))
#define MIN(_x, _y) ((_x) < (_y)) ? (_x) : (_y)

static size_t n_children(struct ax_tree* tree, const struct ax_node* node)
{
    DEFINE_TRAVERSAL_LOCALS(tree, child);
    size_t n = 0;
    FOR_EACH_CHILD(node, child) {
        n++;
    }
    return n;
}

static void propagate_available_size(struct ax_tree* tr, struct ax_node* node)
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

static void container_distribute_lines(struct ax_tree* tr, struct ax_node* node)
{
    DEFINE_TRAVERSAL_LOCALS(tr, child);
    size_t* const line_count = calloc(n_children(tr, node), sizeof(size_t));
    ASSERT(line_count != NULL, "malloc ax_node_c.line_count");
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

static void container_single_line(struct ax_tree* tr, struct ax_node* node)
{
    size_t* const line_count = malloc(1 * sizeof(size_t));
    ASSERT(line_count != NULL, "malloc ax_node_c.line_count");
    line_count[0] = n_children(tr, node);
    node->c.line_count = line_count;
    node->c.n_lines = 1;
}

static void compute_hypothetical_size(struct ax_tree* tr, struct ax_node* node)
{
    struct ax_dim hypoth;
    DEFINE_TRAVERSAL_LOCALS(tr, child);
    switch (node->ty) {

    case AX_NODE_CONTAINER: {
        if (node->c.single_line) {
            container_single_line(tr, node);
        } else {
            container_distribute_lines(tr, node);
        }
        ax_length main = 0.0, cross = 0.0;
        struct ax_dim line = AX_DIM(0.0, 0.0);
#define UPDATE() do {                                            \
            cross = cross + CROSS(line);                \
            main = MAX(main, MAIN(line)); } while (0)
        size_t li, i, prev_li = 0;
        FOR_EACH_CHILD_IN_LINES(li, i, node, child) {
            if (li != prev_li) {
                prev_li = li;
                UPDATE();
                line = AX_DIM(0.0, 0.0);
            }
            MAIN(line) = MAIN(line) + MAIN(child->hypoth);
            CROSS(line) = MAX(CROSS(line), CROSS(child->hypoth));
        }
        if (i > 0) {
            UPDATE();
        }
        MAIN(hypoth) = MIN(main, MAIN(node->avail));
        CROSS(hypoth) = MIN(cross, CROSS(node->avail));
        break;
#undef UPDATE_HYPOTH
    }

    case AX_NODE_RECTANGLE:
        hypoth = node->r.size;
        break;

    case AX_NODE_TEXT: {
        size_t n_lines = 0;
        ax_length max_w = 0.0;
        struct ax_text_metrics tm;
        struct ax_text_iter ti;
        enum ax_text_elem te;
        ax__text_iter_init(&ti, node->t.text);
        ax__text_iter_set_font(&ti, node->t.font);
        ti.max_width = node->avail.w;
        do {
            te = ax__text_iter_next(&ti);
            switch (te) {
            case AX_TEXT_WORD:
                break;

            case AX_TEXT_EOL:
            case AX_TEXT_END:
                ax__measure_text(node->t.font, ti.line, &tm);
                n_lines++;
                max_w = MAX(max_w, tm.width);
                break;

            default: NO_SUCH_TAG("ax_text_elem");
            }
        } while (te != AX_TEXT_END);
        ax__text_iter_free(&ti);

        hypoth.w = max_w;
        hypoth.h = tm.text_height + tm.line_spacing * (n_lines - 1);
        break;
    }

    default: NO_SUCH_NODE_TAG();
    }
    node->hypoth = hypoth;
}

static void resolve_target_size(struct ax_tree* tr, struct ax_node* node)
{
    DEFINE_TRAVERSAL_LOCALS(tr, child);
    switch (node->ty) {

    case AX_NODE_CONTAINER: {
        // TODO: pool allocation of these temporary arrays
        struct line_calc {
            ax_length factor_sum;
            ax_length cross_size;
            ax_length free_space;
        };
        struct line_calc* const lines = calloc(node->c.n_lines, sizeof(struct line_calc));
        ASSERT(lines != NULL, "malloc line_calc array");
        size_t li, i;
        for (li = 0; li < node->c.n_lines; li++) {
            lines[li].factor_sum = 0.0;
            lines[li].cross_size = 0.0;
            lines[li].free_space = MAIN(node->target);
        }
        FOR_EACH_CHILD_IN_LINES(li, i, node, child) {
            lines[li].cross_size = MAX(lines[li].cross_size, CROSS(child->hypoth));
            lines[li].free_space -= MAIN(child->hypoth);
        }
#define FACTOR(_n)                                      \
        (did_overflow ?                                 \
         ((_n)->shrink_factor * MAIN((_n)->hypoth)) :   \
         (_n)->grow_factor)
        FOR_EACH_CHILD_IN_LINES(li, i, node, child) {
            bool did_overflow = lines[li].free_space < 0;
            lines[li].factor_sum += FACTOR(child);
        }
        FOR_EACH_CHILD_IN_LINES(li, i, node, child) {
            bool did_overflow = lines[li].free_space < 0;
            ax_length flex, factor = FACTOR(child);
            if (factor > 0) {
                flex = lines[li].free_space * FACTOR(child) / lines[li].factor_sum;
            } else {
                flex = 0.0;
            }
            MAIN(child->target) = MAIN(child->hypoth) + flex;
            CROSS(child->target) = lines[li].cross_size;
        }
        free(lines);
        break;
#undef FACTOR
    }

    case AX_NODE_RECTANGLE:
    case AX_NODE_TEXT:
        // TODO: re-calculate text? need to determine actual-height somehow
        break;

    default: NO_SUCH_NODE_TAG();
    }
}

static ax_length justify_padding(enum ax_justify j, ax_length space,
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

static void place_coords(struct ax_tree* tr, struct ax_node* node)
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
        pad_y = justify_padding(node->c.cross_justify,
                                cross_flex_space,
                                node->c.n_lines,
                                &y);
#define START_LINE() do {                                   \
            x = node->coord.x;                              \
            pad_x = justify_padding(node->c.main_justify,   \
                                    lines[li].flex_space,   \
                                    node->c.line_count[li], \
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
            justify_padding(child->cross_justify,
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

    case AX_NODE_TEXT: {
        struct ax_pos coord = node->coord;
        struct ax_node_t_line* line, *prev_line = NULL;
        ax__free_node_t_line(node->t.lines);
        node->t.lines = NULL;
        struct ax_text_metrics tm;
        ax__measure_text(node->t.font, "", &tm);
        struct ax_text_iter ti;
        enum ax_text_elem te;
        ax__text_iter_init(&ti, node->t.text);
        ax__text_iter_set_font(&ti, node->t.font);
        ti.max_width = node->target.w;
        do {
            te = ax__text_iter_next(&ti);
            switch (te) {
            case AX_TEXT_WORD:
                break;

            case AX_TEXT_EOL:
            case AX_TEXT_END: {
                line = malloc(sizeof(struct ax_node_t_line) +
                              strlen(ti.line) + 1);
                ASSERT(line != NULL, "mallox ax_node_t_line");
                line->next = NULL;
                line->coord = coord;
                strcpy(line->str, ti.line);
                coord.y += tm.line_spacing;
                if (prev_line == NULL) {
                    node->t.lines = line;
                } else {
                    prev_line->next = line;
                }
                prev_line = line;
                break;
            }

            default: NO_SUCH_TAG("ax_text_elem");
            }
        } while (te != AX_TEXT_END);
        break;
    }

    default: NO_SUCH_NODE_TAG();
    }
}


void ax__layout(struct ax_tree* tr, struct ax_geom* g)
{
    DEFINE_TRAVERSAL_LOCALS(tr, node);

    ax__root(tr)->avail = g->root_dim;
    FOR_EACH_FROM_TOP(node) {
        propagate_available_size(tr, node);
    }

    FOR_EACH_FROM_BOTTOM(node) {
        compute_hypothetical_size(tr, node);
    }

    ax__root(tr)->target = g->root_dim;
    FOR_EACH_FROM_TOP(node) {
        resolve_target_size(tr, node);
    }

    ax__root(tr)->coord = AX_POS(0.0, 0.0);
    FOR_EACH_FROM_TOP(node) {
        place_coords(tr, node);
    }
}