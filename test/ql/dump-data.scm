(define (select recs expr)
  (reverse
   (letrec ((func (lambda (r x)
                    (if x
                        (let ((y (expr x)))
                          (func (if y (cons y r) r) (recs ::+ x)))
                        r))))
     (func () (recs ::)))))

(define (ptables)
  (select <db> (lambda (x)
                 (let ((s (x ::schema)))
                   (and (pair? s) (eq? 'ptable (car s)) x)))))

(define (slots class)
  (select <db> (lambda (x)
                 (let ((s (x ::schema)))
                   (and (pair? s) (eq? class (car s)) (car (cdr (cdr s))))))))

(define (slot-exps class)
  (select <db> (lambda (x)
                 (let ((s (x ::schema)))
                   (and (pair? s) (eq? class (car s)) (list () (car (cdr (cdr s)))))))))

(define (dump-table class)
  (if (< 0 class.:nrecords)
      (begin
        (write `(,class ::load ,@(slots class)))
        (newline)
        (disp `(: ,class (.:key ,@(slot-exps class)) 0 0) :tsv)
        (newline))))

(define (dump-db)
  (letrec ((func (lambda (x)
                   (if (pair? x)
                       (begin (dump-table (car x)) (func (cdr x)))))))
    (func (ptables))))

(dump-db)
