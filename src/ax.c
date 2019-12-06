#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "node.h"
#include "desc.h"
#include "state.h"


extern void* ax__create_font(const char* name);
extern void ax__destroy_font(void* font);
extern void ax__compute_geometry(struct ax_tree* tr, struct ax_aabb aabb);
extern void ax__redraw(struct ax_tree* tr, struct ax_drawbuf* db);
extern struct ax_drawbuf* ax__create_drawbuf();
extern void ax__free_drawbuf(struct ax_drawbuf* db);


static struct ax_desc* ax_reverse_flex_children(struct ax_desc* init_desc)
{
    struct ax_desc* first, *last, *next;
    for (first = NULL, last = init_desc; last != NULL; last = next)
    {
        next = last->flex_attrs.next_child;
        last->flex_attrs.next_child = first;
        first = last;
    }
    return first;
}

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
        node->c.single_line = desc->c.single_line;
        node->c.background = desc->c.background;
        node_id prev_id = NULL_ID;
        for (const struct ax_desc* child_desc = desc->c.first_child;
             child_desc != NULL;
             child_desc = child_desc->flex_attrs.next_child)
        {
            node_id child_id = ax_build_node(tr, child_desc);
            struct ax_node* child = ax_node_by_id(tr, child_id);
            child->grow_factor = child_desc->flex_attrs.grow;
            child->shrink_factor = child_desc->flex_attrs.shrink;
            child->cross_justify = child_desc->flex_attrs.cross_justify;
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

    case AX_NODE_TEXT: {
        char* text = malloc(strlen(desc->t.text) + 1);
        ASSERT(text != NULL, "malloc to copy ax_node_t.desc.text");
        strcpy(text, desc->t.text);
        void* font = ax__create_font(desc->t.font_name);
        ASSERT(font != NULL, "create font");
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
    .parent = NULL,
    .flex_attrs = {
        .grow = 0, .shrink = 1, .cross_justify = AX_JUSTIFY_START,
        .next_child = NULL,
    },
    .c = {
        .first_child = NULL,
        .main_justify = AX_JUSTIFY_START,
        .cross_justify = AX_JUSTIFY_START
    },
};

static void ax_interp_init(struct ax_interp* it);
static void ax_interp_free(struct ax_interp* it);
static int ax_interp(struct ax_state* s,
                     struct ax_interp* it,
                     struct ax_parser* pr, enum ax_parse p);

struct ax_state* ax_new_state()
{
    struct ax_state* s = malloc(sizeof(struct ax_state));
    ASSERT(s != NULL, "malloc ax_state");
    s->root_dim = AX_DIM(0.0, 0.0);
    s->draw_buf = ax__create_drawbuf();
    ax_interp_init(&s->interp);
    ax__parser_init(&s->parser);
    ax_init_tree(&s->tree);
    ax_tree_set_root(&s->tree, &empty_node_desc);
    return s;
}

void ax_destroy_state(struct ax_state* s)
{
    if (s != NULL) {
        ax__parser_free(&s->parser);
        ax_interp_free(&s->interp);
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

static void ax_set_dimensions(struct ax_state* s, struct ax_dim dim)
{
    s->root_dim = dim;
    ax_invalidate(s);
}

static void ax_set_root(struct ax_state* s, const struct ax_desc* root_desc)
{
    ax_reset_tree(&s->tree);
    ax_tree_set_root(&s->tree, root_desc);
    ax_invalidate(s);
}

const struct ax_drawbuf* ax_draw(struct ax_state* s)
{
    return s->draw_buf;
}


const char* ax_get_error(struct ax_state* s)
{
    if (s->interp.err_msg != NULL) {
        return s->interp.err_msg;
    } else {
        return "(no error)";
    }
}


void ax_read_start(struct ax_state* s)
{
    ax_interp_free(&s->interp);
    ax_interp_init(&s->interp);
    ax__parser_eof(&s->parser);
}

int ax_read_chunk(struct ax_state* s, const char* input)
{
    char* acc = (char*) input;
    char const* end = input + strlen(input);
    while (acc < end) {
        enum ax_parse p = ax__parser_feed(&s->parser, acc, &acc);
        if (p != AX_PARSE_NOTHING) {
            int r = ax_interp(s, &s->interp, &s->parser, p);
            if (r != 0) {
                return r;
            }
        }
    }
    return 0;
}

int ax_read_end(struct ax_state* s)
{
    enum ax_parse p = ax__parser_eof(&s->parser);
    return ax_interp(s, &s->interp, &s->parser, p);
}


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

static void ax_interp_init(struct ax_interp* it)
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

static void ax_interp_free(struct ax_interp* it)
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

static void ax_interp_set_root(struct ax_state* s, struct ax_interp* it)
{
    ax_set_root(s, it->desc);
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
    parent->c.first_child = ax_reverse_flex_children(it->desc);
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
            ax_interp_color(it, ax_rgb_color(it->col.rgb));
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

static int ax_interp(struct ax_state* s,
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

#include "../_build/parser_rules.inc"
}
