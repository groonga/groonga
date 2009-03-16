(define-module ql-test-utils
  (use srfi-13)
  (use file.util)
  (use gauche.process)
  (use gauche.selector)
  (export setup-ql teardown-ql send-ql))
(select-module ql-test-utils)

(define groonga (build-path ".." ".." "src" "groonga"))
(define groonga-process #f)
(define db-dir (build-path (temporary-directory) "groonga-ql-test"))
(define db-path (build-path db-dir "test.db"))

(define (setup-ql)
  (if (file-exists? db-dir)
    (remove-directory* db-dir))
  (make-directory* db-dir)
  (set! groonga-process (run-process (list groonga db-path)
                                   :input :pipe
                                   :output :pipe
                                   :error :pipe
                                   :wait #f))
  (set! (port-buffering (process-output groonga-process)) :modest)
  (set! (port-buffering (process-error groonga-process)) :modest))

(define (teardown-ql)
  (when groonga-process
    (close-output-port (process-input groonga-process))
    (process-wait groonga-process))
  (remove-directory* db-dir))

(define (read-ql)
  (let ((selector (make <selector>)))
    (call-with-output-string
      (lambda (result)
        (selector-add! selector
                       (process-output groonga-process)
                       (lambda (groonga-output flag)
                         (let ((output (read-block 4096 groonga-output)))
                           (unless (eof-object? output)
                             (display output result)
                             (selector-select selector 1))))
                       '(r))
        (if (zero? (selector-select selector '(1 0)))
          (error "can't read groonga output"))))))

(define (format-ql-expression expression need-write raw-input)
  (define (rec object)
    (cond ((symbol? object) (display object))
          ((pair? object)
           (display "(")
           (let loop ((elements object))
             (rec (car elements))
             (cond ((null? (cdr elements)) #f)
                   ((not (pair? (cdr elements)))
                    (display " . ")
                    (rec (cdr elements)))
                   (else
                    (display " ")
                    (loop (cdr elements)))))
           (display ")"))
          (else (write object))))
  (with-output-to-string
    (lambda ()
      (if need-write
        (display "(write "))
      (if raw-input
        (display expression)
        (rec expression))
      (if need-write
        (display ")"))
      (display "\n"))))

(define (error-message? message)
  (not (equal? message (remove-error-header message))))

(define (remove-error-header message)
  (regexp-replace #/^\*\*\* ERROR:\s*/ message ""))

(define (send-ql expression . options)
  (let-keywords options ((read-output #t)
                         (detect-error? #t)
                         (need-write #t)
                         (prepare-output identity)
                         (raw-input #f))
    (let ((formatted-expression
           (format-ql-expression expression need-write raw-input)))
      (display formatted-expression (process-input groonga-process))
      (let ((output (string-trim-right (read-ql))))
        (if read-output
          (if (and detect-error? (error-message? output))
            (error (remove-error-header output))
            (read (open-input-string (prepare-output output))))
          (prepare-output output))))))

(provide "ql-test-utils")
