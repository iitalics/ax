#lang racket/base
(provide
 justify?

 ; (color->rgb c) -> (list byte byte byte)
 ; c : (or "red" "green" "blue")
 color->rgb

 ; (call/bracket rsrc destroy f) -> A
 ; rsrc    : R
 ; destroy : [R -> void]
 ; f       : [R -> A]
 call/bracket

 ; (with-bracket [id expr destroy-fn]
 ;   body ...+)
 with-bracket
 )

(module+ test
  (require
   rackunit
   racket/port))

(define (justify? x)
  (memq x '(start end center evenly around between)))

(define (color->rgb c)
  (case c
    [("red")   '(255 0 0)]
    [("green") '(0 255 0)]
    [("blue")  '(0 0 255)]))

(define (call/bracket rsrc destroy f)
  (define-values [err result]
    (with-handlers ([exn? (λ (e) (values e #f))])
      (values #f (f rsrc))))
  (destroy rsrc)
  (if err (raise err) result))

(define-syntax-rule (with-bracket [x create destroy-fn]
                      body ...)
  (call/bracket create
                destroy-fn
                (λ (x) body ...)))

(module+ test

  (define (destroy x) (printf "destroy ~a\n" x))

  (check-equal? (with-output-to-string
                  (λ ()
                    (with-bracket [x "thing" destroy]
                      (printf "hello ~a\n" x))))
                "hello thing\ndestroy thing\n")

  (check-equal? (with-output-to-string
                  (λ ()
                    (with-handlers ([exn? void])
                      (with-bracket [x "thing" destroy]
                        (error 't "!!!!")))))
                "destroy thing\n")

  (check-exn #px"!!!!"
             (λ ()
               (with-output-to-string
                 (λ ()
                   (with-bracket [x "thing" destroy]
                     (error 't "!!!!"))))))

  )
