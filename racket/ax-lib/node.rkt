#lang racket/base
(provide
 node?
 (struct-out nd:container)
 (struct-out nd:rect))

(require
 racket/match
 "./utils.rkt")

(module+ test
  (require rackunit racket/port)
  (provide (all-defined-out)))

;; --------------------
;; Nodes

(struct node [] #:transparent
  #:methods gen:custom-write
  [(define (write-proc nd port _mode)
     (write-node nd port))])

;; (nd:container children) -> node
;; children : [listof node]
;; main-justify : justify
(struct nd:container node [children main-justify] #:transparent)

;; (nd:rect fill w h) -> node
;; fill : color
;; w, h : positive
(struct nd:rect node [fill w h] #:transparent)

;; (write-node nd [port]) -> void
;; nd : node
;; port : output-port
(define (write-node nd [port (current-output-port)])
  (write (match nd
           [(nd:container nds mj)
            `(container (children ,@nds)
                        (main-justify ,mj))]
           [(nd:rect fill w h)
            `(rect (fill (rgb ,@(color->rgb fill)))
                   (size ,w ,h))])
         port))

;; ---------------------------------------------------------------------------------------

(module+ test
  (define nd-ex1 (nd:rect "red" 80 30))
  (define nd-ex2 (nd:rect "blue" 80 40))
  (define nd-ex3 (nd:container (list nd-ex1 nd-ex2) 'start))
  (define nd-ex4 (nd:container (for/list ([i (in-range 100)])
                                 (nd:rect "red"
                                          (+ 30 (* i 2))
                                          (+ 20 (* i 5))))
                               'between))

  (define nd-ex3/string
    (with-output-to-string (λ () (write nd-ex3))))

  (check-equal? (with-input-from-string nd-ex3/string read)
                `(container (children (rect (fill (rgb 255 0 0))
                                            (size 80 30))
                                      (rect (fill (rgb 0 0 255))
                                            (size 80 40)))
                            (main-justify start))))
