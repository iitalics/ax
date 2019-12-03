#lang s-exp "./sexp-yacc.rkt"
#:start <top>

[<top> (log <str>)
       #:before "ax_interp_begin_log(it);\n"
       (set-dim <len> <len>)
       #:before "ax_interp_begin_dim(it);\n"
       #:after "ax_set_dimensions(s, it->dim);\n"
       (set-root <node>)
       #:after "ax_interp_set_root(s, it);\n"]

[<node> (rect <r-attr> ...)
        #:before "ax_interp_begin_node(it, AX_NODE_RECTANGLE);\n"
        (container <c-children> <c-attr> ...)
        #:before "ax_interp_begin_node(it, AX_NODE_CONTAINER);\n"]

[<r-attr> (size <len> <len>)
          #:before "ax_interp_begin_dim(it);\n"
          #:after "ax_interp_set_rect_size(it);\n"
          (fill <str>)
          #:before "ax_interp_begin_fill(it);\n"]

[<c-children> (children <node> ...)
              #:before "ax_interp_begin_children(it);\n"
              #:after "ax_interp_end_children(it);\n"]

[<c-attr> (main-justify <justify>) #:before "ax_interp_begin_main_justify(it);\n"
          (cross-justify <justify>) #:before "ax_interp_begin_cross_justify(it);\n"
          single-line #:op "ax_interp_single_line(it, true);\n"
          multi-line #:op "ax_interp_single_line(it, false);\n"]

[<str> STR #:op "ax_interp_string(it, ~a);\n"]
[<len> INT #:op "ax_interp_integer_len(it, ~a);\n"]

[<justify> start #:op "ax_interp_justify(it, AX_JUSTIFY_START);\n"
           end #:op "ax_interp_justify(it, AX_JUSTIFY_END);\n"
           center #:op "ax_interp_justify(it, AX_JUSTIFY_CENTER);\n"
           evenly #:op "ax_interp_justify(it, AX_JUSTIFY_EVENLY);\n"
           around #:op "ax_interp_justify(it, AX_JUSTIFY_AROUND);\n"
           between #:op "ax_interp_justify(it, AX_JUSTIFY_BETWEEN);\n"]
