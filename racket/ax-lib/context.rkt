#lang racket/base
(provide
 ; (make-context) -> context
 ; (shutdown [cx]) -> void
 ; (shutdown? [cx]) -> void
 ; cx : context
 context?
 make-context
 current-context
 shutdown
 shutdown?

 ; (call/ax f) -> A
 ; f : [-> A]
 call/ax

 ; (with-ax body ...+)
 with-ax

 ; (send-value v) -> void
 ; v : any
 send-value

 ; (close-evt) -> evt
 close-evt
 )

(require
 racket/match
 ffi/unsafe
 ffi/unsafe/define
 ffi/unsafe/schedule
 (only-in "./utils.rkt" with-bracket))

(module+ test
  (require rackunit))

;; ---------------------------------------------------------------------------------------
;; FFI

(define build-dir "/home/milo/Prog/ax-attempt/_build")
(define libaxl (ffi-lib (build-path build-dir "lib/libaxl_SDL")))

(define-ffi-definer define-axffi libaxl)
(define _ax_state (_cpointer 'ax_state))

(define-axffi ax_new_state (_fun -> _ax_state))
(define-axffi ax_destroy_state (_fun _ax_state -> _void))

(define-axffi ax_write_start (_fun _ax_state -> _void))
(define-axffi ax_write_chunk (_fun _ax_state _pointer _size -> _int))
(define-axffi ax_write_end (_fun _ax_state -> _int))
(define-axffi ax_get_error (_fun _ax_state -> _bytes/nul-terminated))
(define-axffi ax_poll_event (_fun _ax_state -> _stdbool))
(define-axffi ax_poll_event_fd (_fun _ax_state -> _int))
(define-axffi ax_read_close_event (_fun _ax_state -> _void))

;; ---------------------------------------------------------------------------------------
;; Errors

(struct exn:fail:ax exn:fail [])

(define (get-ax-error ax-st who)
  (define msg (ax_get_error ax-st))
  (exn:fail:ax (format "~a: ~a" who msg)
               (current-continuation-marks)))

;; ---------------------------------------------------------------------------------------
;; Basic state management

(struct context [(ax-st #:mutable)])
(define current-context (make-parameter (context #f)))

(define (make-context)
  (context (ax_new_state)))

(define (shutdown? [cx (current-context)])
  (match-define (context ax-st) cx)
  (not ax-st))

(define (shutdown [cx (current-context)])
  (match-define (context ax-st) cx)
  (when ax-st
    (ax_destroy_state ax-st)
    (set-context-ax-st! cx #f)))

(define (ax-state who [cx (current-context)])
  (or (context-ax-st cx)
      (error who "ax context shutdown")))

(define (call/ax f)
  (with-bracket [cx (make-context) shutdown]
    (parameterize ([current-context cx])
      (f))))

(define-syntax-rule (with-ax body ...)
  (call/ax (λ () body ...)))

(module+ test
  (check-true (shutdown?))
  (with-ax
    (check-false (shutdown?))
    (check-not-false (ax-state 'test)))
  (let ([cx (with-ax (current-context))])
    (check-true (shutdown? cx))
    (check-exn #px"shutdown" (λ () (ax-state 'test cx)))))

;; ---------------------------------------------------------------------------------------
;; Sending objects

(define (send-value val [cx (current-context)])
  (define ax-st (ax-state 'send-value cx))
  (define (on-write bs s-off e-off no-block? enable-brk?)
    (define len (- e-off s-off))
    (ax_write_chunk ax-st (ptr-add bs s-off) len)
    len)
  (ax_write_start ax-st)
  (with-bracket [port (make-output-port "ax:send-value" always-evt on-write void)
                      close-output-port]
    (write val port))
  (unless (zero? (ax_write_end ax-st))
    (raise (get-ax-error ax-st 'send-value))))

(module+ test
  (with-ax
    (send-value '(log "Hi from Racket!"))
    (check-exn #px"Bye from Racket"
               (λ () (send-value '(die "Bye from Racket!"))))
    (check-exn #px"syntax error"
               (λ () (send-value '(bleh))))))

;; ---------------------------------------------------------------------------------------
;; Receiving events

(struct event-avail [ax-st]
  #:property prop:evt
  (unsafe-poller
   (λ (self wup)
     (match-define (event-avail ax-st) self)
     (cond
       [(ax_poll_event ax-st)
        (values '() #f)]
       [else
        (when wup
          (unsafe-poll-ctx-fd-wakeup wup
                                     (ax_poll_event_fd ax-st)
                                     'read))
        (values #f self)]))))

(define (close-evt [cx (current-context)])
  (define ax-st (ax-state 'close-evt cx))
  (letrec ([evt (handle-evt (event-avail ax-st)
                            (λ ()
                              (ax_read_close_event ax-st)
                              evt))])
    evt))
