(define-module ql-assertions
  (use test.unit.assertions)
  (use ql-test-utils)
  (export-all))
(select-module ql-assertions)

(define (assert-ql-send expression)
  (assert-not-match #/^\*\*\* ERROR:/ (send-ql expression :read-output #f))
  #f)

(define (assert-ql-send-error expression)
  (assert-match #/^\*\*\* ERROR:/ (send-ql expression :read-output #f))
  #f)

(define (assert-ql-send-equal expected expression)
  (assert-equal expected (send-ql expression :read-output #f))
  #f)

(define (assert-ql-send-match expected expression)
  (assert-match expected (send-ql expression :read-output #f))
  #f)

(define (assert-ql expression)
  (assert-true (send-ql `(if ,expression #t #f)))
  #f)

(define (assert-ql-equal expected expression . options)
  (assert-equal expected (apply send-ql expression options))
  #f)

(define (assert-ql-equal-disp expected expression type)
  (assert-equal expected
                (send-ql `(disp ,expression ,type)
                         :read-output #f
                         :need-write #f))
  #f)

(define (assert-ql-equal-disp-json expected expression)
  (let ((json-expected (string-append
                        "["
                        (string-join
                         (map (lambda (values)
                                (string-append
                                 "["
                                 (string-join (map x->string values) ", ")
                                 "]"))
                              expected)
                         ", ")
                        "]")))
    (assert-ql-equal-disp json-expected expression :json))
  #f)

(define (assert-ql-equal-disp-tsv expected expression)
  (let ((tsv-expected (string-join
                       (map (lambda (values)
                              (string-join (map x->string values) "\t"))
                            expected)
                       "\n")))
    (assert-ql-equal-disp tsv-expected expression :tsv))
  #f)

(define (assert-ql-in-delta expected delta expression . options)
  (assert-in-delta expected delta (apply send-ql expression options))
  #f)

(provide "ql-assertions")
