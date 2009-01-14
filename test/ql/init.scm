(define (map proc list)
    (if (pair? list)
        (cons (proc (car list)) (map proc (cdr list)))))
