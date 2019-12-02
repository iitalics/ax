#lang s-exp "./sexp-yacc.rkt"
#:start <top>

[<top> (log <text>)
       #:before "ax_interp_begin_log(it);\n"
       (set-dim <len> <len>)
       #:before "ax_interp_begin_dim(it);\n"
       #:after "ax_set_dimensions(s, it->dim);\n"
       (set-root <node>)
       #:after "ax_interp_set_root(s, it);\n"
       ]

[<node> (rect <r-attr>)
        #:before "ax_interp_begin_node(it, AX_NODE_RECTANGLE);\n"]

[<r-attr> (size <len> <len>)
          #:before "ax_interp_begin_dim(it);\n"
          #:after "ax_interp_set_rect_size(it);\n"]

[<text> STR #:op "ax_interp_text(it, ~a);\n"]
[<len> INT #:op "ax_interp_integer_len(it, ~a);\n"]
