
#define FLEX_CHILD(_g, _s, _xj, _desc)          \
    { .desc = _desc,                            \
      .cross_justify = (_xj),                   \
      .grow = (_g),                             \
      .shrink = (_s) }

#define ACONT(_mj, _xj, _c)                     \
    { .ty = AX_NODE_CONTAINER,                  \
      .c = { .n_children = LENGTH(_c),          \
             .children = (_c),                  \
             .main_justify = (_mj),             \
             .cross_justify = (_xj) } }         \

#define CONT(_mj, _xj, ...) ACONT(                          \
        _mj, _xj,                                           \
        ((struct ax_flex_child_desc[]) { __VA_ARGS__ }))

#define RECT(_f, _w, _h)                            \
    { .ty = AX_NODE_RECTANGLE,                      \
      .r = { .fill = _f, .size = AX_DIM(_w, _h) } }

#define TEXT(_col, _str, _fnt)                  \
    { .ty = AX_NODE_TEXT,                       \
      .t = { .font = _fnt,                      \
             .text = _str,                      \
             .color = _col } }
