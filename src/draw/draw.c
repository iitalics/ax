#include <string.h>
#define AX_DEFINE_TRAVERSAL_MACROS
#include "../tree.h"
#include "../draw.h"
#include "../utils.h"

void ax__init_draw_buf(struct ax_draw_buf* db)
{
    ax__init_region(&db->cur_rgn);
    ax__init_region(&db->swap_rgn);

    db->data = ALLOCATES(&db->cur_rgn, struct ax_draw, db->cap = 4);
    db->len = 0;
}

void ax__free_draw_buf(struct ax_draw_buf* db)
{
    ax__free_region(&db->swap_rgn);
    ax__free_region(&db->cur_rgn);
}

static struct ax_draw* draw_buf_ins(struct ax_draw_buf* db)
{
    if (db->len >= db->cap) {
        size_t new_cap = db->cap * 2;
        struct ax_draw* new_data = ALLOCATES(&db->swap_rgn, struct ax_draw, new_cap);
        memcpy(new_data, db->data, sizeof(struct ax_draw) * db->cap);
        db->data = new_data;
        db->cap = new_cap;
        ax__swap_regions(&db->cur_rgn, &db->swap_rgn);
        ax__region_clear(&db->swap_rgn);
    }
    return &db->data[db->len++];
}

static void redraw_(struct ax_node* node, struct ax_draw_buf* db)
{
    switch (node->ty) {
    case AX_NODE_CONTAINER:
        if (!AX_COLOR_IS_NULL(node->c.background)) {
            struct ax_draw* d = draw_buf_ins(db);
            d->ty = AX_DRAW_RECT;
            d->r.fill = node->c.background;
            d->r.bounds.o = node->coord;
            d->r.bounds.s = node->target;
        }
        break;

    case AX_NODE_RECTANGLE: {
        struct ax_draw* d = draw_buf_ins(db);
        d->ty = AX_DRAW_RECT;
        d->r.fill = node->r.fill;
        d->r.bounds.o = node->coord;
        d->r.bounds.s = node->r.size;
        break;
    }

    case AX_NODE_TEXT:
        for (struct ax_node_t_line* line = node->t.lines;
             line != NULL;
             line = line->next)
        {
            struct ax_draw* d = draw_buf_ins(db);
            d->ty = AX_DRAW_TEXT;
            d->t.color = node->t.color;
            d->t.font = node->t.font;
            d->t.text = line->str;
            d->t.pos = line->coord;
        }
        break;

    default: NO_SUCH_NODE_TAG();
    }
}

void ax__redraw(struct ax_tree* tr, struct ax_draw_buf* db)
{
    if (ax__is_tree_empty(tr)) {
        return;
    }

    DEFINE_TRAVERSAL_LOCALS(tr, node);
    db->len = 0;
    FOR_EACH_FROM_TOP(node) {
        redraw_(node, db);
    }
}
