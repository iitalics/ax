#define AX_DEFINE_TRAVERSAL_MACROS
#include "../tree.h"
#include "../draw.h"
#include "../utils.h"

void ax__init_draw_buf(struct ax_draw_buf* db)
{
    ASSERT(db != NULL, "malloc ax_draw_buf");
    db->len = 0;
    db->cap = 4;
    db->data = malloc(sizeof(struct ax_draw) * db->cap);
    ASSERT(db->data != NULL, "malloc ax_draw_buf.data");
}

void ax__free_draw_buf(struct ax_draw_buf* db)
{
    free(db->data);
}

static struct ax_draw* draw_buf_ins(struct ax_draw_buf* db)
{
    if (db->len >= db->cap) {
        db->cap *= 2;
        db->data = realloc(db->data, db->cap * sizeof(struct ax_draw));
        ASSERT(db->data != NULL, "realloc ax_draw_buf.data");
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
