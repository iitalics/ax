#lang racket
(provide
 (rename-out
  [module-begin #%module-begin]))

(require
 syntax/parse)


(struct produc [] #:transparent)

;; action : [cgv -> void]
(struct pd:term produc [action] #:transparent)
(struct pd:str pd:term [] #:transparent)
(struct pd:int pd:term [] #:transparent)

;; head : symbol
;; args : [listof symbol]
;; rep? : boolean
;; before, after : [cgv -> void]
(struct pd:list produc [head args rep? before after] #:transparent)

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
  [pattern {~seq (head:id arg:NONTERM-NAME ...
                          {~optional {~and ooo {~datum ...}}})
                 {~optional {~seq #:before bef:str}}
                 {~optional {~seq #:after aft:str}}}
           #:attr pd (pd:list (syntax-e #'head)
                              (attribute arg.sym)
                              (and (attribute ooo) #t)
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

(define ((cg:seq . fs)) (for ([f (in-list fs)]) (f)))

(define (cg:ok) (displayln "return 0;"))
(define (cg:tok-error) (displayln "return ax_interp_generic_err(it);"))
(define (cg:impossible-state-error) (displayln "NO_SUCH_TAG(\"ax_interp.state\");"))
(define (cg:set-state s [denote values]) (printf "it->state = ~a;\n" (denote s)))
(define (cg:label l) (printf "~a:" l))
(define (cg:goto l) (printf "goto ~a;" l))

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

(struct token [] #:transparent)
(struct tk:lp token [] #:transparent)
(struct tk:rp token [] #:transparent)
(struct tk:str token [] #:transparent)
(struct tk:int token [] #:transparent)
(struct tk:non-rp token [] #:transparent)
(struct tk:sym token [name] #:transparent)

(struct dom [state tok] #:transparent)

(struct action [] #:transparent)
(struct ac:do action [func ac-rest] #:transparent)
(struct ac:goto action [state] #:transparent)
(struct ac:retry action [state] #:transparent)

;; [hash dom => action]
(define current-transitions (make-parameter #f))

(define (call/transitions f)
  (let ([ts (make-hash)])
    (parameterize ([current-transitions ts]) (f))
    ts))
(define-syntax-rule (with-transitions body ...)
  (call/transitions (λ () body ...)))

(define (add-transition! s tok ac [ts (current-transitions)])
  (hash-set! ts (dom s tok) ac))

(define (cg:action ac s->code retry-l)
  (match ac
    [(ac:do f ac*) (f) (cg:action ac* s->code retry-l)]
    [(ac:goto s) (cg:set-state s s->code)]
    [(ac:retry s) (cg:seq (cg:set-state s s->code)
                          (cg:goto retry-l))]))

(define (cg:token-case tk=>f df)
  (define v=>f (make-hash))
  (define sym=>f (make-hash))
  (for ([(tk f) (in-hash tk=>f)])
    (match tk
      [(tk:lp) (hash-set! v=>f cgv:token-lp f)]
      [(tk:rp) (hash-set! v=>f cgv:token-rp f)]
      [(tk:str) (hash-set! v=>f cgv:token-str f)]
      [(tk:int) (hash-set! v=>f cgv:token-int f)]
      [(tk:non-rp)
       (for ([v (in-list (list cgv:token-lp cgv:token-str cgv:token-int))])
         (hash-set! v=>f v f))]
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
(define (cg:transitions #:init s0 [ts (current-transitions)])
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
  (define retry-l (gensym 'retry))
  (define code=>f
    (for/hash ([(s tok=>ac) (in-hash s=>tok=>ac)])
      (define (f)
        (cg:token-case (for/hash ([(tok ac) (in-hash tok=>ac)])
                         (values tok
                                 (λ ()
                                   (cg:action ac s->code retry-l)
                                   (cg:ok))))
                       cg:tok-error))
      (values (s->code s) f)))
  (cg:label retry-l)
  (cg:case cgv:the-state
           code=>f
           cg:impossible-state-error))

;; rules nt-sym state state -> void
(define (compile-nonterm/name rules nt-sym s0 s1)
  (compile-nonterm rules (lookup-nonterm rules nt-sym) s0 s1))

;; rules nonterm state state -> void
(define (compile-nonterm rules nt s0 s1)
  (define lists (make-hash))

  (for ([pd (in-set (nonterm-rhs nt rules))])
    (match pd
      [(pd:str op)
       (add-transition! s0 (tk:str)
                        (ac:do (λ () (op cgv:str-value))
                               (ac:goto s1)))]
      [(pd:int op)
       (add-transition! s0 (tk:int)
                        (ac:do (λ () (op cgv:int-value))
                               (ac:goto s1)))]

      [(pd:list hd _ _ _ _)
       (hash-set! lists hd pd)]))

  (unless (hash-empty? lists)
    (define s-head (gensym 's))
    (add-transition! s0 (tk:lp) (ac:goto s-head))
    (for ([(hd pd) (in-hash lists)])
      (match-define (pd:list _ args rep? before after) pd)
      (define s-tail (gensym 's))
      (add-transition! s-head (tk:sym hd) (ac:do before (ac:goto s-tail)))
      (compile-list rules args rep? s-tail s1 after))))

;; transitions rules [listof nt-sym] boolean state state cgf -> void
(define (compile-list rules args rep-last? s0 s1 [after void])

  (define n-args (length args))
  (define s-tos (build-list n-args (λ (i) (gensym 'sI))))
  (define s-froms (cons s0 s-tos))

  (for ([arg (in-list args)]
        [s-from (in-list s-froms)]
        [s-to (in-list s-tos)])
    (compile-nonterm/name rules arg s-from s-to))

  (define s* (last s-tos))
  (when rep-last?
    (define s** (list-ref s-froms (sub1 n-args)))
    (add-transition! s* (tk:non-rp) (ac:retry s**)))
  (add-transition! s* (tk:rp) (ac:do after (ac:goto s1))))

;; ====================

(define (compile+cg-rules rules)
  (with-transitions
    (define s0 (gensym 's))
    (compile-nonterm rules (rules-start rules) s0 s0)
    (cg:transitions #:init s0)))

(define-syntax (module-begin stx)
  (syntax-case stx ()
    [(_ body ...)
     #'(#%plain-module-begin
        (compile+cg-rules
         (parse-rules '[body ...])))]))
