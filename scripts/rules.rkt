#lang s-exp "./sexp-yacc.rkt"
#:start <top>
[<top> <log-cmd>]
[<log-cmd> (log <log-arg>)]
[<log-arg> STR #:op "ax_interp_log(~a);\n"]
