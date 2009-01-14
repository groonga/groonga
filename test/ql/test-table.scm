;; -*- coding: utf-8 -*-

(define-module test-table
  (extend test.unit.test-case)
  (use ql-test-utils)
  (use ql-assertions))
(select-module test-table)

(define (setup)
  (setup-ql))

(define (teardown)
  (teardown-ql))

(define (test-nrecords)
  (assert-ql '(ptable '<test>))
  (assert-ql-equal 0 '(<test> ::nrecords))
  (assert-ql '(<test> ::new "record1"))
  (assert-ql '(<test> ::new "record2"))
  (assert-ql-equal 2 '(<test> ::nrecords))
  #f)

(define (test-select)
  (assert-ql '(ptable '<test>))
  (assert-ql-equal #f '(<test> : "record1"))
  (assert-ql '(<test> ::new "record1"))
  (assert-ql '(<test> ::new "record2"))
  (assert-ql-send-match #/^#p<\d+>/ '(<test> : "record1"))
  (assert-ql '(eqv? (<test> : "record1")
                    (<test> : "record1")))
  #f)

(define (test-iterator)
  (assert-ql '(ptable '<test>))
  (assert-ql '(<test> ::new "record1"))
  (assert-ql '(<test> ::new "record2"))

  (assert-ql-equal "record1" "(<test> : \"record1\").:key" :raw-input #t)
  (assert-ql-equal "record2" "(<test> : \"record2\").:key" :raw-input #t)

  (assert-ql '(define iterator (<test> ::)))
  (assert-ql-equal "record1" '|iterator.:key|)
  (assert-ql '(<test> ::+ iterator))
  (assert-ql-equal "record2" '|iterator.:key|)
  #f)

(provide "test-table")
