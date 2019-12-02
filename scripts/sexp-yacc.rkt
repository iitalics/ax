#lang racket/base
(provide
 (rename-out
  [module-begin #%module-begin]))

(require
 syntax/parse
 racket/string
 racket/set
 racket/match
 racket/port
 (for-syntax racket/base))

(struct produc [] #:transparent)

;; action : [cgv -> void]
(struct pd:term produc [action] #:transparent)
(struct pd:str pd:term [] #:transparent)
(struct pd:int pd:term [] #:transparent)

;; head : symbol
;; args : [listof symbol]
;; before, after : [cgv -> void]
(struct pd:list produc [head args before after] #:transparent)

;; name : nt-sym
;; prods : [listof (or prod nt-sym)]
(struct nonterm [name prods] #:transparent)

;; nonterms : [listof nonterm]
;; start-name : nt-sym
(struct rules [nonterms start-name] #:transparent)

;; ====================

(define-syntax-class NONTERM-NAME
  #:description "nonterminal name"
  [pattern x:id
           #:do [(define s (symbol->string (syntax-e #'x)))]
           #:when (regexp-match? "^<[-a-zA-Z_]+>$" s)
           #:attr sym (string->symbol (substring s 1 (- (string-length s) 1)))])

(define-splicing-syntax-class TERMINAL
  #:description "terminal"
  [pattern {~datum STR} #:attr ctor pd:str]
  [pattern {~datum INT} #:attr ctor pd:int])

(define-splicing-syntax-class PROD
  #:description "production"
  [pattern {~seq t:TERMINAL {~optional {~seq #:op op:str}}}
           #:do [(define cgf
                   (if (attribute op)
                       (op->function (syntax-e #'op))
                       void))]
           #:attr pd ((attribute t.ctor) cgf)]
  [pattern name:NONTERM-NAME
           #:attr pd (attribute name.sym)]
  [pattern {~seq (head:id arg:NONTERM-NAME ...)
                 {~optional {~seq #:before bef:str}}
                 {~optional {~seq #:after aft:str}}}
           #:attr pd (pd:list (syntax-e #'head)
                              (attribute arg.sym)
                              (if (attribute bef)
                                  (op->function (syntax-e #'bef))
                                  void)
                              (if (attribute aft)
                                  (op->function (syntax-e #'aft))
                                  void))])

(define-syntax-class NONTERM
  #:description "nonterminal"
  [pattern [name:NONTERM-NAME {~optional {~and kw-start #:start}}
                              p:PROD ...]
           #:attr start? (attribute kw-start)
           #:attr nt (nonterm (attribute name.sym)
                              (attribute p.pd))])

(define (parse-rules stx)
  (syntax-parse stx
    [(#:start start:NONTERM-NAME n:NONTERM ...)
     (rules (attribute n.nt)
            (attribute start.sym))]))

;; ====================

;; rules nt-sym -> nonterm
(define (lookup-nonterm rules nt-sym)
  (or (for/first ([nt (in-list (rules-nonterms rules))]
                  #:when (eq? (nonterm-name nt) nt-sym))
        nt)
      (error 'lookup-nonterm (format "no such nonterm: ~a" nt-sym))))

;; nonterm rules -> [listof produc]
(define (nonterm-rhs nt rules)
  (for/fold ([pds (set)])
            ([pd/name (in-list (nonterm-prods nt))])
    (if (symbol? pd/name)
        (set-union (nonterm-rhs (lookup-nonterm rules pd/name) rules)
                   pds)
        (set-add pds pd/name))))

;; rules -> nonterm
(define (rules-start rules)
  (lookup-nonterm rules (rules-start-name rules)))

;; ====================

(define cgv:the-state "it->state")
(define cgv:the-token "p")
(define cgv:int-value "pr->i")
(define cgv:double-value "pr->d")
(define cgv:str-value "pr->str")

(define cgv:token-lp "AX_PARSE_LPAREN")
(define cgv:token-rp "AX_PARSE_RPAREN")
(define cgv:token-int "AX_PARSE_INTEGER")
(define cgv:token-str "AX_PARSE_STRING")
(define cgv:token-sym "AX_PARSE_SYMBOL")
(define (cgv:is-symbol s)
  (format "strcmp(pr->str, ~v) == 0" (symbol->string s)))

(define (op->function stmt)
  (if (string-contains? stmt "~a")
      (λ es (apply printf stmt es))
      (λ es (display stmt))))

(define (cg:ok)
  (displayln "return 0;"))

(define (cg:tok-error)
  (displayln "return ax_interp_generic_err(it);"))

(define (cg:impossible-state-error)
  (displayln "NO_SUCH_TAG(\"ax_interp.state\");"))

(define (cg:set-state s [denote values])
  (printf "it->state = ~a;\n" (denote s)))

;; cgv [hash cgv => cgf] cgf -> void
(define (cg:case v i=>f df)
  (printf "switch (~a) {\n" v)
  (for ([(i f) (in-hash i=>f)])
    (printf "case ~a:" i)
    (f))
  (printf "default:")
  (df)
  (printf "}\n"))

;; ====================

(struct action [] #:transparent)
(struct ac:do action [func ac-rest] #:transparent)
(struct ac:goto action [state] #:transparent)

(struct token [] #:transparent)
(struct tk:lp token [] #:transparent)
(struct tk:rp token [] #:transparent)
(struct tk:str token [] #:transparent)
(struct tk:int token [] #:transparent)
(struct tk:sym token [name] #:transparent)

(struct dom [state tok] #:transparent)

(define (add-transition! ts s tok ac)
  (hash-set! ts (dom s tok) ac))

(define (cg:action ac s->code)
  (match ac
    [(ac:do f ac*) (f) (cg:action ac* s->code)]
    [(ac:goto s) (cg:set-state s s->code)]))

(define (cg:token-case tk=>f df)
  (define v=>f (make-hash))
  (define sym=>f (make-hash))
  (for ([(tk f) (in-hash tk=>f)])
    (match tk
      [(tk:lp) (hash-set! v=>f cgv:token-lp f)]
      [(tk:rp) (hash-set! v=>f cgv:token-rp f)]
      [(tk:str) (hash-set! v=>f cgv:token-str f)]
      [(tk:int) (hash-set! v=>f cgv:token-int f)]
      [(tk:sym s) (hash-set! sym=>f s f)]))
  (unless (hash-empty? sym=>f)
    (define (f*)
      (for ([(sym f) (in-hash sym=>f)])
        (printf "if (~a) {\n" (cgv:is-symbol sym))
        (f)
        (printf "}\n"))
      (df))
    (hash-set! v=>f cgv:token-sym f*))
  (cg:case cgv:the-token v=>f df))

;; ts state -> void
(define (cg:transitions ts s0)
  (define s=>code (make-hash (list (cons s0 0))))
  (define (s->code s)
    (hash-ref! s=>code s
               (λ ()
                 (hash-count s=>code))))
  (define s=>tok=>ac (make-hash))
  (for ([(d ac) (in-hash ts)])
    (match-define (dom s tok) d)
    (hash-update! s=>tok=>ac s
                  (λ (tok=>ac)
                    (hash-set tok=>ac tok ac))
                  make-immutable-hash))
  (cg:case cgv:the-state
           (for/hash ([(s tok=>ac) (in-hash s=>tok=>ac)])
             (define (f)
               (cg:token-case (for/hash ([(tok ac) (in-hash tok=>ac)])
                                (values tok
                                        (λ ()
                                          (cg:action ac s->code)
                                          (cg:ok))))
                              cg:tok-error))
             (values (s->code s) f))
           cg:impossible-state-error))

;; transitions rules nt-sym state state -> void
(define (compile-nonterm/name ts rules nt-sym s0 s1)
  (compile-nonterm ts rules (lookup-nonterm rules nt-sym) s0 s1))

;; transitions rules nonterm state state -> void
(define (compile-nonterm ts rules nt s0 s1)
  (define lists (make-hash))

  (for ([pd (in-set (nonterm-rhs nt rules))])
    (match pd
      [(pd:str op)
       (add-transition! ts s0 (tk:str)
                        (ac:do (λ () (op cgv:str-value))
                               (ac:goto s1)))]
      [(pd:int op)
       (add-transition! ts s0 (tk:int)
                        (ac:do (λ () (op cgv:int-value))
                               (ac:goto s1)))]

      [(pd:list hd _ _ _)
       (hash-set! lists hd pd)]))

  (unless (hash-empty? lists)
    (define s-head (gensym 's))
    (add-transition! ts s0 (tk:lp) (ac:goto s-head))
    (for ([(hd pd) (in-hash lists)])
      (match-define (pd:list _ args before after) pd)
      (define s-tail (gensym 's))
      (add-transition! ts s-head (tk:sym hd) (ac:do before (ac:goto s-tail)))
      (compile-list ts rules args s-tail s1 after))))

;; transitions rules [listof nt-sym] state state cgf -> void
(define (compile-list ts rules args s0 s1 [after void])
  (define s* (for/fold ([s s0]) ([arg (in-list args)])
               (let ([s* (gensym 's)])
                 (compile-nonterm/name ts rules arg s s*)
                 s*)))
  (add-transition! ts s* (tk:rp) (ac:do after (ac:goto s1))))

;; ====================

(define (compile+cg-rules rules)
  (define s0 (gensym 's))
  (define ts (make-hash))
  (compile-nonterm ts rules (rules-start rules) s0 s0)
  (cg:transitions ts s0))

(define-syntax (module-begin stx)
  (syntax-case stx ()
    [(_ body ...)
     #'(#%plain-module-begin
        (compile+cg-rules
         (parse-rules '[body ...])))]))
