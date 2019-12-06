#include "interp.h"
#include "../core.h"
#include "../tree.h"
#include "../tree/desc.h"

enum ax_interp_mode {
    M_NONE = 0,
    M_LOG,
    M_TEXT,
    M_FONT,
    M_FILL,
    M_TEXT_COLOR,
    M_BACKGROUND,
    M_DIM_W,
    M_DIM_H,
    M_MAIN_JUSTIFY,
    M_CROSS_JUSTIFY,
    M_SELF_JUSTIFY,
    M_GROW,
    M_SHRINK,
    M__MAX,
};

void ax__init_interp(struct ax_interp* it)
{
    it->state = 0;
    it->ctx = -1;
    it->ctx_sp = 0;

    it->mode = M_NONE;
    it->err_msg = NULL;

    it->desc = NULL;
    it->parent_desc = NULL;
}

static void ax_interp_reset_node(struct ax_interp* it)
{
    // TODO: free it->desc, it->parent_desc
    it->desc = NULL;
    it->parent_desc = NULL;
}

void ax__free_interp(struct ax_interp* it)
{
    ax_interp_reset_node(it);
    free(it->err_msg);
}

/* static void ax_interp_log_stack(struct ax_interp* it)
{
    printf("[LOG] stack: ");
    for (size_t i = 0; i < it->ctx_sp; i++) {
        printf("%d, ", it->ctx_stack[i]);
    }
    printf("%d\n", it->ctx);
} */

static void ax_interp_push(struct ax_interp* it, int new_ctx)
{
    ASSERT(it->ctx_sp < LENGTH(it->ctx_stack), "stack overflow");
    it->ctx_stack[it->ctx_sp++] = it->ctx;
    it->ctx = new_ctx;
    //printf("[LOG] push %d\n", new_ctx);
    //ax_interp_log_stack(it);
}

static void ax_interp_pop(struct ax_interp* it)
{
    ASSERT(it->ctx_sp > 0, "stack underflow");
    it->ctx = it->ctx_stack[--it->ctx_sp];
    //printf("[LOG] pop\n");
    //ax_interp_log_stack(it);
}

static void ax_interp_begin_node(struct ax_interp* it, enum ax_node_type ty)
{
    struct ax_desc* desc = malloc(sizeof(struct ax_desc));
    ASSERT(desc != NULL, "malloc ax_desc");
    // TODO: better way to make a default initialized node desc
    desc->ty = ty;
    switch (ty) {
    case AX_NODE_CONTAINER:
        desc->c = (struct ax_desc_c) {
            .first_child = NULL,
            .main_justify = AX_JUSTIFY_START,
            .cross_justify = AX_JUSTIFY_START,
            .single_line = false,
            .background = AX_NULL_COLOR,
        };
        break;
    case AX_NODE_RECTANGLE:
        desc->r = (struct ax_rect) {
            .fill = 0x000000,
            .size = AX_DIM(0.0, 0.0),
        };
        break;
    case AX_NODE_TEXT:
        desc->t = (struct ax_desc_t) {
            .color = 0x000000,
            .font_name = "size:10",
            .text = "blah",
        };
        break;
    default: NO_SUCH_NODE_TAG();
    }
    desc->flex_attrs = (struct ax_flex_child_attrs) {
        .grow = 0,
        .shrink = 1,
        .cross_justify = AX_JUSTIFY_START,
        .next_child = it->desc,
    };
    desc->parent = it->parent_desc;
    it->desc = desc;
}

static void ax_interp_set_dim(struct ax_state* s, struct ax_interp* it)
{
    ax__set_dim(s, it->dim);
}

static void ax_interp_set_root(struct ax_state* s, struct ax_interp* it)
{
    ax__set_root(s, it->desc);
    ax_interp_reset_node(it);
}

static void ax_interp_set_rect_size(struct ax_interp* it)
{
    it->desc->r.size = it->dim;
}

static void ax_interp_begin_children(struct ax_interp* it)
{
    it->parent_desc = it->desc;
    it->desc = NULL;
}

static void ax_interp_end_children(struct ax_interp* it)
{
    struct ax_desc* parent = it->parent_desc;
    parent->c.first_child = ax_desc_reverse_flex_children(it->desc);
    it->parent_desc = parent->parent;
    it->desc = parent;
}

static void ax_interp_begin_log(struct ax_interp* it) { it->mode = M_LOG; }
static void ax_interp_begin_dim(struct ax_interp* it) { it->mode = M_DIM_W; }
static void ax_interp_begin_fill(struct ax_interp* it) { it->mode = M_FILL; }
static void ax_interp_begin_main_justify(struct ax_interp* it) { it->mode = M_MAIN_JUSTIFY; }
static void ax_interp_begin_cross_justify(struct ax_interp* it) { it->mode = M_CROSS_JUSTIFY; }
static void ax_interp_begin_self_justify(struct ax_interp* it) { it->mode = M_SELF_JUSTIFY; }
static void ax_interp_begin_grow(struct ax_interp* it) { it->mode = M_GROW; }
static void ax_interp_begin_shrink(struct ax_interp* it) { it->mode = M_SHRINK; }
static void ax_interp_begin_text(struct ax_interp* it) { it->mode = M_TEXT; }
static void ax_interp_begin_font(struct ax_interp* it) { it->mode = M_FONT; }
static void ax_interp_begin_text_color(struct ax_interp* it) { it->mode = M_TEXT_COLOR; }
static void ax_interp_begin_rgb(struct ax_interp* it) { it->col.rgb_idx = 0; }
static void ax_interp_begin_background(struct ax_interp* it) { it->mode = M_BACKGROUND; }

static void ax_interp_color(struct ax_interp* it, ax_color col)
{
    switch (it->mode) {
    case M_FILL:
        it->desc->r.fill = col;
        break;
    case M_TEXT_COLOR:
        it->desc->t.color = col;
        break;
    case M_BACKGROUND:
        it->desc->c.background = col;
        break;
    default: break;
    }
}

static void ax_interp_string(struct ax_interp* it, const char* str)
{
    switch (it->mode) {
    case M_LOG:
        printf("[LOG] %s\n", str);
        break;
    case M_TEXT:
        it->desc->t.text = malloc(strlen(str) + 1);
        strcpy((char*) it->desc->t.text, str);
        break;
    case M_FONT:
        it->desc->t.font_name = malloc(strlen(str) + 1);
        strcpy((char*) it->desc->t.font_name, str);
        break;
    default: break;
    }
}

static void ax_interp_integer(struct ax_interp* it, long v)
{
    switch (it->mode) {
    case M_DIM_W:
        it->dim.w = v;
        it->mode = M_DIM_H;
        break;
    case M_DIM_H:
        it->dim.h = v;
        it->mode = M_NONE;
        //printf("[LOG] dim: %.2fx%.2f\n", it->dim.w, it->dim.h);
        break;
    case M_GROW:
        it->desc->flex_attrs.grow = v;
        break;
    case M_SHRINK:
        it->desc->flex_attrs.shrink = v;
        break;

    case M_FILL:
    case M_TEXT_COLOR:
    case M_BACKGROUND:
        // (rgb ...) form
        it->col.rgb[it->col.rgb_idx++] = v < 0 ? 0 : v > 255 ? 255 : v;
        if (it->col.rgb_idx >= 3) {
            ax_interp_color(it, ax_color_from_rgb(it->col.rgb));
            it->mode = M_NONE;
        }
        break;

    default: break;
    }
}

static void ax_interp_justify(struct ax_interp* it, enum ax_justify just)
{
    switch (it->mode) {
    case M_MAIN_JUSTIFY:
        it->desc->c.main_justify = just;
        break;
    case M_CROSS_JUSTIFY:
        it->desc->c.cross_justify = just;
        break;
    case M_SELF_JUSTIFY:
        it->desc->flex_attrs.cross_justify = just;
        break;
    default: break;
    }
}

static void ax_interp_color_string(struct ax_interp* it, const char* str)
{
    ax_color col = strtol(str, NULL, 16);
    ax_interp_color(it, col);
}

static void ax_interp_single_line(struct ax_interp* it, bool s)
{
    it->desc->c.single_line = s;
}

static int ax_interp_generic_err(struct ax_interp* it)
{
    const char* ctx;
    switch (it->state) {
        // TODO: generate state names
    default: ctx = "???";
    }
#define FMT "invalid syntax, expected %s"
    size_t len = strlen(FMT) - 2 + strlen(ctx);
    it->err_msg = malloc(len + 1);
    sprintf(it->err_msg, FMT, ctx);
    return 1;
#undef FMT
}

static int ax_interp_parse_err(struct ax_interp* it, struct ax_parser* pr)
{
#define FMT "parse erorr: %s"
    size_t len = strlen(FMT) - 2 + strlen(pr->str);
    it->err_msg = malloc(len + 1);
    sprintf(it->err_msg, FMT, pr->str);
    return 1;
#undef FMT
}

int ax__interp(struct ax_state* s,
                     struct ax_interp* it,
                     struct ax_parser* pr, enum ax_parse p)
{
    if (it->err_msg != NULL) {
        return 1;
    }
    if (p == AX_PARSE_NOTHING) {
        return 0;
    }
    if (p == AX_PARSE_ERROR) {
        return ax_interp_parse_err(it, pr);
    }

#include "../../_build/parser_rules.inc"
}
