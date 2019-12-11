#include "interp.h"
#include "../core.h"
#include "../tree.h"
#include "../tree/desc.h"

enum ax_interp_mode {
    M_LOG,
    M_DIE,
    M_TEXT,
    M_FONT,
    M_FILL,
    M_TEXT_COLOR,
    M_BACKGROUND,
    M_WIN_SIZE,
    M_RECT_SIZE,
    M_MAIN_JUSTIFY,
    M_CROSS_JUSTIFY,
    M_SELF_JUSTIFY,
    M_GROW,
    M_SHRINK,
    M__MAX,
};

void ax__init_interp(struct ax_interp* it)
{
    it->err = 0;
    it->err_msg = NULL;

    it->state = 0;
    it->ctx = -1;
    it->ctx_sp = 0;

    it->desc = NULL;
    it->parent_desc = NULL;
}

static void reset_desc(struct ax_interp* it)
{
    // TODO: free it->desc, it->parent_desc
    it->desc = NULL;
    it->parent_desc = NULL;
}

void ax__free_interp(struct ax_interp* it)
{
    reset_desc(it);
    free(it->err_msg);
}

/* static void log_stack(struct ax_interp* it)
{
    printf("[LOG] stack: ");
    for (size_t i = 0; i < it->ctx_sp; i++) {
        printf("%d, ", it->ctx_stack[i]);
    }
    printf("%d\n", it->ctx);
} */

static void push_ctx(struct ax_interp* it, int new_ctx)
{
    ASSERT(it->ctx_sp < LENGTH(it->ctx_stack), "stack overflow");
    it->ctx_stack[it->ctx_sp++] = it->ctx;
    it->ctx = new_ctx;
    //printf("[LOG] push %d\n", new_ctx);
    //ax_interp_log_stack(it);
}

static void pop_ctx(struct ax_interp* it)
{
    ASSERT(it->ctx_sp > 0, "stack underflow");
    it->ctx = it->ctx_stack[--it->ctx_sp];
    //printf("[LOG] pop\n");
    //ax_interp_log_stack(it);
}

static void end_init(struct ax_state* s, struct ax_interp* it)
{
    if (ax__is_backend_initialized(s)) {
#define STR "backend already initialized"
        it->err_msg = malloc(strlen(STR) + 1);
        strcpy(it->err_msg, STR);
        it->err = 1;
        return;
#undef STR
    }
    ax__initialize_backend(s);
}

static void begin_node(struct ax_interp* it, enum ax_node_type ty)
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

static void set_root(struct ax_state* s, struct ax_interp* it)
{
    struct ax_backend* bac = s->backend;
    if (!ax__is_backend_initialized(s)) {
#define STR "backend not initialized"
        it->err_msg = malloc(strlen(STR) + 1);
        strcpy(it->err_msg, STR);
        it->err = 1;
        goto cleanup1;
#undef STR
    }

    struct ax_tree tree;
    ax__init_tree(&tree);
    node_id root;
    int r = ax__build_node(s, bac, &tree, it->desc, &root);
    if (r != 0) {
        it->err = r;
        goto cleanup2;
    }
    ax__set_tree(s, &tree);
    ASSERT(ax__is_tree_empty(&tree), "tree should be empty now-");

cleanup2:
    ax__free_tree(&tree);
cleanup1:
    reset_desc(it);
}

static void begin_children(struct ax_interp* it)
{
    it->parent_desc = it->desc;
    it->desc = NULL;
}

static void end_children(struct ax_interp* it)
{
    struct ax_desc* parent = it->parent_desc;
    parent->c.first_child = ax_desc_reverse_flex_children(it->desc);
    it->parent_desc = parent->parent;
    it->desc = parent;
}

static void begin_win_size(struct ax_interp* it) { it->mode = M_WIN_SIZE; it->i = 0; }
static void begin_rect_size(struct ax_interp* it) { it->mode = M_RECT_SIZE; it->i = 0; }
static void begin_log(struct ax_interp* it) { it->mode = M_LOG; }
static void begin_die(struct ax_interp* it) { it->mode = M_DIE; }
static void begin_fill(struct ax_interp* it) { it->mode = M_FILL; }
static void begin_main_justify(struct ax_interp* it) { it->mode = M_MAIN_JUSTIFY; }
static void begin_cross_justify(struct ax_interp* it) { it->mode = M_CROSS_JUSTIFY; }
static void begin_self_justify(struct ax_interp* it) { it->mode = M_SELF_JUSTIFY; }
static void begin_grow(struct ax_interp* it) { it->mode = M_GROW; }
static void begin_shrink(struct ax_interp* it) { it->mode = M_SHRINK; }
static void begin_text(struct ax_interp* it) { it->mode = M_TEXT; }
static void begin_font(struct ax_interp* it) { it->mode = M_FONT; }
static void begin_text_color(struct ax_interp* it) { it->mode = M_TEXT_COLOR; }
static void begin_rgb(struct ax_interp* it) { it->i = 0; }
static void begin_background(struct ax_interp* it) { it->mode = M_BACKGROUND; }

static void color(struct ax_interp* it, ax_color col)
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

static void string(struct ax_state* s, struct ax_interp* it, const char* str)
{
    (void) s;
    switch (it->mode) {
    case M_LOG:
        printf("[LOG] %s\n", str);
        break;
    case M_DIE:
        it->err_msg = malloc(strlen(str) + 1);
        strcpy((char*) it->err_msg, str);
        it->err = 1;
        break;
    case M_TEXT:
        it->desc->t.text = malloc(strlen(str) + 1);
        strcpy((char*) it->desc->t.text, str);
        break;
    case M_FONT:
        it->desc->t.font_name = malloc(strlen(str) + 1);
        strcpy((char*) it->desc->t.font_name, str);
        break;
    case M_FILL:
    case M_TEXT_COLOR:
    case M_BACKGROUND: {
        ax_color col = strtol(str, NULL, 16);
        color(it, col);
        break;
    }
    default: break;
    }
}

static void dim(struct ax_state* s, struct ax_interp* it, struct ax_dim d)
{
    switch (it->mode) {
    case M_WIN_SIZE:
        ax__config_win_size(s, d);
        break;
    case M_RECT_SIZE:
        it->desc->r.size = d;
        break;
    default: break;
    }
}

static void integer(struct ax_state* s, struct ax_interp* it, long v)
{
    switch (it->mode) {

    case M_WIN_SIZE:
    case M_RECT_SIZE:
        switch (it->i++) {
        case 0:
            it->dim.w = v;
            break;
        case 1:
            it->dim.h = v;
            dim(s, it, it->dim);
            break;
        default: break;
        }
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
        it->rgb[it->i++] = v < 0 ? 0 : v > 255 ? 255 : v;
        if (it->i >= 3) {
            color(it, ax_color_from_rgb(it->rgb));
        }
        break;

    default: break;
    }
}

static void justify(struct ax_interp* it, enum ax_justify just)
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

static void cont_set_single_line(struct ax_interp* it, bool s)
{
    it->desc->c.single_line = s;
}

void ax__interp(struct ax_state* s,
                struct ax_interp* it,
                struct ax_lexer* lex, enum ax_parse tok)
{
    ASSERT(it->err == 0, "error bit should not be set");
    ASSERT(tok != AX_PARSE_NOTHING, "table doesn't handle NOTHING tokens");

    if (tok == AX_PARSE_ERROR) {
        goto token_err;
    }

#include "../../_build/parser_rules.inc"

ok: return;

syntax_err: {
        const char* ctx;
        switch (it->state) {
            // TODO: generate state names
        default: ctx = "???";
        }
#define FMT "syntax error: expected %s"
        size_t len = strlen(FMT) - 2 + strlen(ctx);
        it->err_msg = malloc(len + 1);
        sprintf(it->err_msg, FMT, ctx);
        it->err = 1;
        return;
#undef FMT
    }

token_err: {
#define FMT "syntax error: %s"
        size_t len = strlen(FMT) - 2 + strlen(lex->str);
        it->err_msg = malloc(len + 1);
        sprintf(it->err_msg, FMT, lex->str);
        it->err = 1;
        return;
#undef FMT
    }
}
