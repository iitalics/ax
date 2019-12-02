#lang s-exp "./sexp-yacc.rkt"
#:start <top>

[<top> (log <text>)
       #:before "ax_interp_begin_log(it);\n"
       (set-dim <len> <len>)
       #:before "ax_interp_begin_dim(it);\n"
       #:after "ax_set_dimensions(s, it->dim);\n"
       ;; (set-root <node>)
       ;; #:before "ax_interp_begin_root_node(it);\n"
       ;; #:after "ax_interp_set_root(s, it);\n"
       ]

;; [<node> (rect <r-attr>)
;;         #:before "ax_interp_begin_rect(it);\n"]
;; [<r-attr> (size <len> <len>)
;;           #:before "ax_interp_begin_size(it);\n"
;;           #:after "ax_interp_set_rect_size(it);\n"]

[<text> STR #:op "ax_interp_text(it, ~a);\n"]
[<len> INT #:op "ax_interp_integer_len(it, ~a);\n"]
