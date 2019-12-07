#lang s-exp "./sexp-yacc.rkt"
#:start <top>

[<top> (log <str>)
       #:before "begin_log(it);\n"
       (die <str>)
       #:before "begin_die(it);\n"
       (set-dim <len> <len>)
       #:before "begin_dim(it);\n"
       #:after "set_dim(s, it);\n"
       (set-root <node>)
       #:after "set_root(s, it);\n"]

[<node> (rect <r-attr> ...)
        #:before "begin_node(it, AX_NODE_RECTANGLE);\n"
        (container <c-children> <c-attr> ...)
        #:before "begin_node(it, AX_NODE_CONTAINER);\n"
        (text <str> <t-attr> ...)
        #:before "begin_node(it, AX_NODE_TEXT);\nbegin_text(it);\n"]

[<r-attr> (size <len> <len>)
          #:before "begin_dim(it);\n"
          #:after "rect_set_size(it);\n"
          (fill <color>)
          #:before "begin_fill(it);\n"
          <flex-attr>]

[<c-children> (children <node> ...)
              #:before "begin_children(it);\n"
              #:after "end_children(it);\n"]

[<c-attr> (main-justify <justify>) #:before "begin_main_justify(it);\n"
          (cross-justify <justify>) #:before "begin_cross_justify(it);\n"
          (background <color>) #:before "begin_background(it);\n"
          single-line #:op "cont_set_single_line(it, true);\n"
          multi-line #:op "cont_set_single_line(it, false);\n"
          <flex-attr>]

[<t-attr> (font <str>) #:before "begin_font(it);\n"
          (color <color>) #:before "begin_text_color(it);\n"
          <flex-attr>]

[<flex-attr> (grow <int>) #:before "begin_grow(it);\n"
             (shrink <int>) #:before "begin_shrink(it);\n"
             (self-cross-justify <justify>) #:before "begin_self_justify(it);\n"]

[<str> STR #:op "string(s, it, ~a);\n"]
[<int> INT #:op "integer(s, it, ~a);\n"]
[<len> <int>]
[<color> (rgb <int> <int> <int>) #:before "begin_rgb(it);\n"
         <str>
         none #:op "color(it, AX_NULL_COLOR);\n"]

[<justify> start #:op "justify(it, AX_JUSTIFY_START);\n"
           end #:op "justify(it, AX_JUSTIFY_END);\n"
           center #:op "justify(it, AX_JUSTIFY_CENTER);\n"
           evenly #:op "justify(it, AX_JUSTIFY_EVENLY);\n"
           around #:op "justify(it, AX_JUSTIFY_AROUND);\n"
           between #:op "justify(it, AX_JUSTIFY_BETWEEN);\n"]
