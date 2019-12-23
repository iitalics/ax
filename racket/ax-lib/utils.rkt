#lang racket/base
(provide
 ; (color->rgb c) -> (list byte byte byte)
 ; c : (or "red" "green" "blue")
 color->rgb
 )

(define (color->rgb c)
  (case c
    [("red")   '(255 0 0)]
    [("green") '(0 255 0)]
    [("blue")  '(0 0 255)]))
