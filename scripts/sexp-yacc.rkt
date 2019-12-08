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
(struct pd:sym terminal [name] #:transparent)

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
  [pattern {~datum INT} #:attr ctor pd:int]
  [pattern name:id #:attr ctor (λ (f) (pd:sym f (syntax-e #'name)))])

(define-splicing-syntax-class PROD
  #:description "production"
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
                                  void))]
  [pattern name:NONTERM-NAME
           #:attr pd (attribute name.sym)]
  [pattern {~seq t:TERMINAL {~optional {~seq #:op op:str}}}
           #:do [(define cgf
                   (if (attribute op)
                       (op->function (syntax-e #'op))
                       void))]
           #:attr pd ((attribute t.ctor) cgf)])

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
(define cgv:the-ctx "it->ctx")
(define cgv:the-token "tok")
(define cgv:int-value "lex->i")
(define cgv:double-value "lex->d")
(define cgv:str-value "lex->str")

(define cgv:token-lp "AX_PARSE_LPAREN")
(define cgv:token-rp "AX_PARSE_RPAREN")
(define cgv:token-int "AX_PARSE_INTEGER")
(define cgv:token-str "AX_PARSE_STRING")
(define cgv:token-sym "AX_PARSE_SYMBOL")
(define (cgv:is-symbol s)
  (format "strcmp(lex->str, ~v) == 0" (symbol->string s)))

(define (op->function stmt)
  (if (string-contains? stmt "~a")
      (λ es (apply printf stmt es))
      (λ es (display stmt))))

(define ((cg:seq . fs)) (for ([f (in-list fs)]) (f)))

(define (cg:ok) (displayln "goto ok;"))
(define (cg:syntax-error) (displayln "goto syntax_err;"))
(define (cg:impossible-state-error) (displayln "NO_SUCH_TAG(\"ax_interp.state\");"))
(define (cg:impossible-ctx-error) (displayln "NO_SUCH_TAG(\"ax_interp.ctx\");"))
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
  (printf "push_ctx(it, ~a);\n" (denote c)))
(define (cg:pop-ctx)
  (printf "pop_ctx(it);\n"))

;; ====================

(struct token [] #:transparent)
(struct tk:lp token [] #:transparent)
(struct tk:rp token [] #:transparent)
(struct tk:str token [] #:transparent)
(struct tk:int token [] #:transparent)
(struct tk:non-rp token [] #:transparent)
(struct tk:sym token [name] #:transparent)

(struct action [token next-state func] #:transparent)

(struct epsilon [next-state] #:transparent)
(struct ep:push epsilon [ctx] #:transparent)
(struct ep:pop-if epsilon [ctx] #:transparent)
(struct ep:goto epsilon [] #:transparent)

;; [hash state => (or [listof action]
;;                    [listof epsilon])
(define current-transitions (make-parameter #f))

(define (call/transitions f)
  (let ([ts (make-hash)])
    (parameterize ([current-transitions ts]) (f))
    ts))
(define-syntax-rule (with-transitions body ...)
  (call/transitions (λ () body ...)))

;; state (or action epsilon) -> void
(define (add-transition! s δ [ts (current-transitions)])
  (hash-update! ts s
                (λ (δs) (cons δ δs))
                (λ () '())))

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
  (define c=>code (make-hash))
  (define (s->code s) (hash-ref! s=>code s (λ () (hash-count s=>code))))
  (define (c->code c) (hash-ref! c=>code c (λ () (hash-count c=>code))))

  (cg:case cgv:the-state
           (for/hash ([(s acs) (in-hash ts)]
                      #:when (andmap action? acs))
             (define tok=>f
               (for/hash ([ac (in-list acs)])
                 (match-define (action tk s* f) ac)
                 (values tk (λ () (f) (cg:goto s*)))))
             (values (s->code s)
                     (λ ()
                       (cg:token-case cgv:the-token tok=>f cg:syntax-error))))
           cg:impossible-state-error)

  (for ([(s δs) (in-hash ts)])
    (cg:label s)
    (match δs
      [(list (? epsilon? eps) ...)
       (cg:case cgv:the-ctx
                (for/hash ([ep (in-list eps)]
                           #:when (ep:pop-if? ep))
                  (match-define (ep:pop-if s* c) ep)
                  (values (c->code c) (λ () (cg:pop-ctx) (cg:goto s*))))
                (or (for/or ([ep (in-list eps)])
                      (match ep
                        [(ep:push s* c)
                         (λ () (cg:push-ctx c c->code) (cg:goto s*))]
                        [(ep:goto s*)
                         (λ () (cg:goto s*))]
                        [_ #f]))
                    cg:impossible-ctx-error))]
      [_
       (cg:set-state s s->code)
       (cg:ok)]))

  (eprintf "* Generated transition table with ~a states, ~a contexts\n"
           (hash-count s=>code) (hash-count c=>code)))

;; returns start and end state
;; ---
;; rules nonterm -> state state
(define (compile-nonterm rules nt)
  (cond
    [(hash-has-key? nonterm-memo nt)
     (match-define (cons s0 s1) (hash-ref nonterm-memo nt))
     (values s0 s1)]
    [else
     (define s0 (gensym 's_start))
     (define s1 (gensym 's_end))
     (hash-set! nonterm-memo nt (cons s0 s1))
     (compile-nonterm* rules nt s0 s1)
     (values s0 s1)]))

(define nonterm-memo (make-hash))

(define (compile-nonterm* rules nt s0 s1)
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
    (define s-beg (gensym 's_lp))
    (add-transition! s0 (action (tk:lp) s-beg void))
    (for ([(hd pd) (in-hash lists)])
      (match-define (pd:list _ args rep? before after) pd)
      (define s-end (gensym 's_rp))
      (define-values [s-args loop-s0 loop-ctx] (compile-list rules args s-end after))
      (add-transition! s-beg (action (tk:sym hd) s-args before))
      (cond
        [rep?
         (define s-pop (gensym 's_pop))
         (add-transition! s-end (ep:push loop-s0 loop-ctx))
         (add-transition! loop-s0 (action (tk:rp) s-pop after))
         (add-transition! s-pop (ep:pop-if s1 loop-ctx))]
        [else
         (add-transition! s-end (action (tk:rp) s1 after))]))))

(define (terminal-token tm)
  (match tm
    [(pd:str _) (tk:str)]
    [(pd:int _) (tk:int)]
    [(pd:sym _ s) (tk:sym s)]))

(define (terminal-value tm)
  (match tm
    [(pd:str _) cgv:str-value]
    [(pd:int _) cgv:int-value]
    [(pd:sym _ _) ""]))

;; returns start state + start state of last arg, given an end state
;; ---
;; rules [listof nt-sym] state cgf -> state state
(define (compile-list rules args s1 after)
  (for/fold ([s1 s1] [last-s0 #f] [last-ctx #f])
            ([arg-name (in-list (reverse args))])
    (define nt (lookup-nonterm rules arg-name))
    (define-values [s0* s1*] (compile-nonterm rules nt))
    (define s0 (gensym 's_push))
    (define ctx (gensym 'ctx))
    (add-transition! s0 (ep:push s0* ctx))
    (add-transition! s1* (ep:pop-if s1 ctx))
    (values s0 (or last-s0 s0*) (or last-ctx ctx))))

;; rules -> state
(define (compile-rules rules)
  (define-values [s0 s1] (compile-nonterm rules (rules-start rules)))
  (add-transition! s1 (ep:goto s0))
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
