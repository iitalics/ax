#lang racket
(require
 "./context.rkt"
 "./node.rkt"
 "./utils.rkt")

; heartbeat thread to demonstrate that we aren't blocking the racket VM
(define (doki)
  (unless (with-handlers ([exn:break? exn?])
            (displayln ";; doki")
            (sleep 1)
            #f)
    (doki)))

(define root
  (nd:container
   (list (nd:rect "red" 60 60)
         (nd:rect "green" 80 80)
         (nd:rect "blue" 100 100))
   'between))

(module+ main
  (with-bracket [t (thread doki) break-thread]
    (with-ax
      (send-value '(init))
      (send-value `(set-root ,root))
      (sync (close-evt))
      (displayln "nice try.")
      (sync (close-evt))
      (displayln "bye."))))
