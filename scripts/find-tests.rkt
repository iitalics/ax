#lang racket
(define found 0)
(define files (current-command-line-arguments))
(for ([arg (in-vector files)])
  (with-input-from-file arg
    (λ ()
      (for ([line (in-lines)])
        (match (regexp-match #px"^TEST\\((.+)\\)" line)
          [(list _ name)
           (printf "RUN_TEST(~a);\n" name)
           (set! found (add1 found))]
          [_ (void)])))))

(eprintf "* Found ~a test cases in ~a files\n"
         found
         (vector-length files))
