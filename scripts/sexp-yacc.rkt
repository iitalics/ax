#lang racket
(provide
 (rename-out
  [module-begin #%module-begin]))

(require
 syntax/parse)


(struct produc [] #:transparent)

;; action : [cgv -> void]
(struct terminal produc [action] #:transparent)
(struct pd:str terminal [] #:transparent)
(struct pd:int terminal [] #:transparent)

;; head : symbol
;; args : [listof symbol]
;; rep? : boolean
;; before, after : [cgv -> void]
(struct pd:list produc [head args rep? before after] #:transparent)

;; name : nt-sym
;; prods : [listof (or produc nt-sym)]
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
(define (cg:goto l) (printf "goto ~a;\n" l))

;; cgv [hash cgv => cgf] cgf -> void
(define (cg:case v i=>f df)
  (printf "switch (~a) {\n" v)
  (for ([(i f) (in-hash i=>f)])
    (printf "case ~a:" i)
    (f))
  (printf "default:")
  (df)
  (printf "}\n"))

(define (cg:push-ctx c [denote values])
  (printf "ax_interp_push(it, ~a);\n" (denote c)))
(define (cg:if-pop-ctx c f [denote values])
  (printf "if (it->ctx == ~a) {\n" (denote c))
  (printf "ax_interp_pop(it);\n")
  (f)
  (printf "}\n"))

;; ====================

(struct token [] #:transparent)
(struct tk:lp token [] #:transparent)
(struct tk:rp token [] #:transparent)
(struct tk:str token [] #:transparent)
(struct tk:int token [] #:transparent)
(struct tk:non-rp token [] #:transparent)
(struct tk:sym token [name] #:transparent)

(struct action [token next-state func] #:transparent)

;; [hash state => [listof action]
(define current-transitions (make-parameter #f))

(define (call/transitions f)
  (let ([ts (make-hash)])
    (parameterize ([current-transitions ts]) (f))
    ts))
(define-syntax-rule (with-transitions body ...)
  (call/transitions (λ () body ...)))

;; state action -> void
(define (add-transition! s δ [ts (current-transitions)])
  (hash-update! ts s
                (λ (δs) (cons δ δs))
                (λ () '())))

(define (cg:goto-state s s->code)
  (cg:set-state s s->code)
  (cg:ok))

(define (cg:token-case tk tk=>f df)
  (define v=>f (make-hash))
  (define sym=>f (make-hash))
  (for ([(tk f) (in-hash tk=>f)])
    (match tk
      [#f (set! df f)] ; wildcard
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
  (cg:case tk v=>f df))

;; ts state -> void
(define (cg:transitions s0 [ts (current-transitions)])
  (define s=>code (make-hash (list (cons s0 0))))
  (define (s->code s)
    (hash-ref! s=>code s
               (λ ()
                 (hash-count s=>code))))
  (define code=>f
    (for/hash ([(s acs) (in-hash ts)]
               #:when (andmap action? acs))
      (define tok=>f
        (for/hash ([ac (in-list acs)])
          (match-define (action tk s* f) ac)
          (values tk (λ () (f) (cg:goto-state s* s->code)))))
      (values (s->code s)
              (λ () (cg:token-case cgv:the-token tok=>f cg:tok-error)))))
  (cg:case cgv:the-state code=>f cg:impossible-state-error)
  (eprintf "* Generated transition table with ~a states\n"
           (hash-count s=>code)))

;; rules nonterm state state -> void
(define (compile-nonterm rules nt s0 s1)
  (define rhs (nonterm-rhs nt rules))
  (define lists
    (for/hash ([pd (in-set rhs)]
               #:when (pd:list? pd))
      (values (pd:list-head pd) pd)))

  (for ([tm (in-set rhs)]
        #:when (terminal? tm))
    (add-transition! s0
                     (action (terminal-token tm)
                             s1
                             (λ () ((terminal-action tm)
                                    (terminal-value tm))))))

  (unless (hash-empty? lists)
    (define s-head (gensym 's))
    (add-transition! s0 (action (tk:lp) s-head void))
    (for ([(hd pd) (in-hash lists)])
      (match-define (pd:list _ args rep? before after) pd)
      (define s-tail (gensym 's))
      (add-transition! s-head (action (tk:sym hd) s-tail before))
      (compile-list rules args rep? s-tail s1 after))))

(define (terminal-token tm)
  (match tm
    [(pd:str _) (tk:str)]
    [(pd:int _) (tk:int)]))

(define (terminal-value tm)
  (match tm
    [(pd:str _) cgv:str-value]
    [(pd:int _) cgv:int-value]))

;; rules [listof nt-sym] boolean state state cgf -> void
(define (compile-list rules args rep-last? s0 s1 [after void])
  (define n-args (length args))
  (define s-nexts
    (for/fold ([s-prev s0] [ss '()] #:result (reverse ss))
              ([i (in-range n-args)])
      (if (and rep-last? (= i (sub1 n-args)))
          (values s-prev (cons s-prev ss)) ; loop instead of going to new state
          (let ([s* (gensym 'sI)]) (values s* (cons s* ss))))))

  (for ([arg-name (in-list args)]
        [s-here (in-list (cons s0 s-nexts))]
        [s-next (in-list s-nexts)])
    (compile-nonterm rules
                     (lookup-nonterm rules arg-name)
                     s-here
                     s-next))

  (add-transition! (last s-nexts) (action (tk:rp) s1 after)))

;; rules -> state
(define (compile-rules rules)
  (define s0 (gensym 's-toplevel))
  (compile-nonterm rules (rules-start rules) s0 s0)
  s0)

;; ====================

(define (parse+compile+cg-rules stx)
  (with-transitions
    (cg:transitions
     (compile-rules
      (parse-rules stx)))))

(define-syntax (module-begin stx)
  (syntax-case stx ()
    [(_ body ...)
     #'(#%plain-module-begin
        (parse+compile+cg-rules '[body ...]))]))
