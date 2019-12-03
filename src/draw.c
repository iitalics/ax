#include "node.h"


struct ax_drawbuf* ax__create_drawbuf()
{
    struct ax_drawbuf* db = malloc(sizeof(struct ax_drawbuf));
    ASSERT(db != NULL, "malloc ax_drawbuf");
    db->len = 0;
    db->cap = 4;
    db->data = malloc(sizeof(struct ax_draw) * db->cap);
    ASSERT(db->data != NULL, "malloc ax_drawbuf.data");
    return db;
}

void ax__free_drawbuf(struct ax_drawbuf* db)
{
    if (db != NULL) {
        free(db->data);
        free(db);
    }
}

static struct ax_draw* ax_drawbuf_ins(struct ax_drawbuf* db)
{
    if (db->len >= db->cap) {
        db->cap *= 2;
        db->data = realloc(db->data, db->cap * sizeof(struct ax_draw));
        ASSERT(db->data != NULL, "realloc ax_drawbuf.data");
    }
    return &db->data[db->len++];
}

static void ax_redraw_(struct ax_node* node, struct ax_drawbuf* db)
{
    switch (node->ty) {
    case AX_NODE_CONTAINER:
        if (!AX_COLOR_IS_NULL(node->c.background)) {
            struct ax_draw* d = ax_drawbuf_ins(db);
            d->ty = AX_DRAW_RECT;
            d->r.fill = node->c.background;
            d->r.bounds.o = node->coord;
            d->r.bounds.s = node->target;
        }
        break;

    case AX_NODE_RECTANGLE: {
        struct ax_draw* d = ax_drawbuf_ins(db);
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
            struct ax_draw* d = ax_drawbuf_ins(db);
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


void ax__redraw(struct ax_tree* tr, struct ax_drawbuf* db)
{
    DEFINE_TRAVERSAL_LOCALS(tr, node);
    db->len = 0;
    FOR_EACH_FROM_TOP(node) {
        ax_redraw_(node, db);
    }
}


bool ax_color_rgb(ax_color color, uint8_t* out_rgb)
{
    out_rgb[0] = (color & 0xff0000) >> 16;
    out_rgb[1] = (color & 0x00ff00) >> 8;
    out_rgb[2] = (color & 0x0000ff) >> 0;
    return !AX_COLOR_IS_NULL(color);
}

ax_color ax_rgb_color(uint8_t* rgb)
{
    if (rgb == NULL) {
        return AX_NULL_COLOR;
    } else {
        return (uint32_t) rgb[0] * 0x010000 +
            (uint32_t) rgb[1] * 0x000100 +
            (uint32_t) rgb[2];
    }
}
