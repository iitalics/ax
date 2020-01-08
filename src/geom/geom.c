#include <string.h>
#define AX_DEFINE_TRAVERSAL_MACROS
#include "text.h"
#include "../geom.h"
#include "../tree.h"
#include "../backend.h"
#include "../utils.h"

void ax__init_geom(struct ax_geom* g)
{
    g->root_dim = AX_DIM(0.0, 0.0);
    ax__init_region(&g->layout_rgn);
    ax__init_region(&g->temp_rgn);
}

void ax__free_geom(struct ax_geom* g)
{
    ax__free_region(&g->temp_rgn);
    ax__free_region(&g->layout_rgn);
}

#define MAIN(_d) (_d).w
#define CROSS(_d) (_d).h
#define MAX(_x, _y) (((_x) > (_y)) ? (_x) : (_y))
#define MIN(_x, _y) ((_x) < (_y)) ? (_x) : (_y)

/* IMPORTANT NOTES ABOUT THE TWO KINDS OF REGIONS:
 * - allocate in "rgn" if the memory should persist after individual layout
 *   passes (i.e., it's used in the next pass, or it's used for drawing).
 * - allocate in "tmp_rgn" if the memory is temporary and only used for processing this
 *   single node (i.e., if you had used malloc, you would free that memory at the bottom
 *   of the function). you should clear the temp region before using it.
 */

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

static void container_distribute_lines(struct region* rgn,
                                       struct ax_tree* tr,
                                       struct ax_node* node)
{
    DEFINE_TRAVERSAL_LOCALS(tr, child);
    size_t max_n = n_children(tr, node);
    size_t* line_count = ALLOCATES(rgn, size_t, max_n);
    memset(line_count, 0, sizeof(size_t) * max_n);
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

static void container_single_line(struct region* rgn,
                                  struct ax_tree* tr,
                                  struct ax_node* node)
{
    size_t* line_count = ALLOCATES(rgn, size_t, 1);
    line_count[0] = n_children(tr, node);
    node->c.line_count = line_count;
    node->c.n_lines = 1;
}

static void compute_hypothetical_size(struct region* rgn,
                                      struct region* tmp_rgn,
                                      struct ax_tree* tr,
                                      struct ax_node* node)
{
    struct ax_dim hypoth;
    DEFINE_TRAVERSAL_LOCALS(tr, child);
    switch (node->ty) {

    case AX_NODE_CONTAINER: {
        if (node->c.single_line) {
            container_single_line(rgn, tr, node);
        } else {
            container_distribute_lines(rgn, tr, node);
        }
        ax_length main = 0.0, cross = 0.0;
        struct ax_dim line = AX_DIM(0.0, 0.0);
#define UPDATE() do {                                   \
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
        ax_length max_w = 0.0, text_h = 0.0, line_sp = 0.0;
        struct ax_text_iter ti;
        ax__region_clear(tmp_rgn);
        ax__text_iter_init(tmp_rgn, &ti, node->t.text);
        ax__text_iter_set_font(&ti, node->t.font);
        ti.max_width = node->avail.w;
        enum ax_text_elem te;
        do {
            te = ax__text_iter_next(&ti);
            switch (te) {
            case AX_TEXT_WORD:
                break;

            case AX_TEXT_EOL:
            case AX_TEXT_END: {
                struct ax_text_metrics tm;
                ax__measure_text(node->t.font, ti.line, &tm);
                max_w = MAX(max_w, tm.width);
                text_h = tm.text_height;
                line_sp = tm.line_spacing;
                n_lines++;
                break;
            }

            default: NO_SUCH_TAG("ax_text_elem");
            }
        } while (te != AX_TEXT_END);

        hypoth.w = max_w;
        hypoth.h = text_h + line_sp * (n_lines - 1);
        break;
    }

    default: NO_SUCH_NODE_TAG();
    }
    node->hypoth = hypoth;
}

static void resolve_target_size(struct region* tmp_rgn,
                                struct ax_tree* tr,
                                struct ax_node* node)
{
    DEFINE_TRAVERSAL_LOCALS(tr, child);
    switch (node->ty) {

    case AX_NODE_CONTAINER: {
        struct line_calc {
            ax_length factor_sum;
            ax_length cross_size;
            ax_length free_space;
        };
        ax__region_clear(tmp_rgn);
        struct line_calc* lines = ALLOCATES(tmp_rgn, struct line_calc, node->c.n_lines);
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

static void place_coords(struct region* rgn,
                         struct region* tmp_rgn,
                         struct ax_tree* tr,
                         struct ax_node* node)
{
    DEFINE_TRAVERSAL_LOCALS(tr, child);
    switch (node->ty) {

    case AX_NODE_CONTAINER: {
        struct line_calc {
            ax_length flex_space;
            ax_length cross_size;
        };
        ax__region_clear(tmp_rgn);
        struct line_calc* lines = ALLOCATES(tmp_rgn, struct line_calc, node->c.n_lines);
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
        break;
#undef START_LINE
    }

    case AX_NODE_RECTANGLE:
        break;

    case AX_NODE_TEXT: {
        struct ax_pos coord = node->coord;
        struct ax_node_t_line* first_line = NULL, *prev_line = NULL;
        struct ax_text_metrics tm; // TODO: helper function "ax__measure_line_spacing(font)"
        ax__measure_text(node->t.font, "", &tm);
        struct ax_text_iter ti;
        ax__region_clear(tmp_rgn);
        ax__text_iter_init(tmp_rgn, &ti, node->t.text);
        ax__text_iter_set_font(&ti, node->t.font);
        ti.max_width = node->target.w;
        enum ax_text_elem te;
        do {
            te = ax__text_iter_next(&ti);
            switch (te) {
            case AX_TEXT_WORD:
                break;

            case AX_TEXT_EOL:
            case AX_TEXT_END: {
                struct ax_node_t_line* line = ALLOCATE(rgn, struct ax_node_t_line);
                line->next = NULL;
                line->coord = coord;
                line->str = ax__strdup(rgn, ti.line);
                coord.y += tm.line_spacing;
                if (first_line == NULL) {
                    first_line = line;
                } else {
                    prev_line->next = line;
                }
                prev_line = line;
                break;
            }

            default: NO_SUCH_TAG("ax_text_elem");
            }
        } while (te != AX_TEXT_END);
        node->t.lines = first_line;
        break;
    }

    default: NO_SUCH_NODE_TAG();
    }
}


void ax__layout(struct ax_tree* tr, struct ax_geom* g)
{
    if (ax__is_tree_empty(tr)) {
        return;
    }

    DEFINE_TRAVERSAL_LOCALS(tr, node);

    ax__region_clear(&g->layout_rgn);

    ax__root(tr)->avail = g->root_dim;
    FOR_EACH_FROM_TOP(node) {
        propagate_available_size(tr, node);
    }

    FOR_EACH_FROM_BOTTOM(node) {
        compute_hypothetical_size(&g->layout_rgn, &g->temp_rgn, tr, node);
    }

    ax__root(tr)->target = g->root_dim;
    FOR_EACH_FROM_TOP(node) {
        resolve_target_size(&g->temp_rgn, tr, node);
    }

    ax__root(tr)->coord = AX_POS(0.0, 0.0);
    FOR_EACH_FROM_TOP(node) {
        place_coords(&g->layout_rgn, &g->temp_rgn, tr, node);
    }
}
