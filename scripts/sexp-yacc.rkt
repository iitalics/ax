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
(define (cg:pop-ctx)
  (printf "ax_interp_pop(it);\n"))

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
                       (cg:token-case cgv:the-token tok=>f cg:tok-error))))
           cg:impossible-state-error)

  (for ([(s δs) (in-hash ts)])
    (cg:label s)
    (match δs
      [(list (? epsilon? eps) ...)
       (for ([ep (in-list eps)])
         (match ep
           [(ep:goto s*) (cg:goto s*)]
           [_ (void)]))]
      [_
       (cg:set-state s s->code)
       (cg:ok)]))

  (eprintf "* Generated transition table with ~a states, ~a contexts\n"
           (hash-count s=>code) (hash-count c=>code)))

;; returns start state given an end state
;; ---
;; rules nonterm state -> state
(define (compile-nonterm rules nt s1)
  (define-values [s0 s1*] (compile-nonterm* rules nt))
  (add-transition! s1* (ep:goto s1))
  s0)

;; returns start and end state
;; ---
;; rules nonterm -> state state
(define (compile-nonterm* rules nt)
  (define s0 (gensym 's_start))
  (define s1 (gensym 's_end))
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
    (define s-end (gensym 's_rp))
    (add-transition! s0 (action (tk:lp) s-beg void))
    (add-transition! s-end (action (tk:rp) s1 void))
    (for ([(hd pd) (in-hash lists)])
      (match-define (pd:list _ args rep? before after) pd)
      (define s-args (compile-list rules args rep? s-end after))
      (add-transition! s-beg
                       (action (tk:sym hd)
                               s-args
                               before))))

  (values s0 s1))

(define (terminal-token tm)
  (match tm
    [(pd:str _) (tk:str)]
    [(pd:int _) (tk:int)]))

(define (terminal-value tm)
  (match tm
    [(pd:str _) cgv:str-value]
    [(pd:int _) cgv:int-value]))

;; returns start state given an end state
;; ---
;; rules [listof nt-sym] boolean state cgf -> state
(define (compile-list rules args rep-last? s1 after)
  (when rep-last?
    (error 'compile-list "repetitions unimplemented"))
  (for/fold ([s-next s1])
            ([arg-name (in-list (reverse args))])
    (compile-nonterm rules
                     (lookup-nonterm rules arg-name)
                     s-next)))

;; rules -> state
(define (compile-rules rules)
  (define-values [s0 s1] (compile-nonterm* rules (rules-start rules)))
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
