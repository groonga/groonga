(define (show-schema)
  (letrec ((func (lambda (x)
                   (if x
                       (let ((s (x ::schema)))
                         (cond ((pair? s) (display (x ::schema)) (newline)))
                         (func (<db> ::+ x)))))))
    (func (<db> ::))))

(show-schema)
