;; -*- coding: utf-8 -*-

(define-module test-base
  (extend test.unit.test-case)
  (use ql-test-utils)
  (use ql-assertions))
(select-module test-base)

(define (setup)
  (setup-ql))

(define (teardown)
  (teardown-ql))

(define (test-primitive)
  (assert-ql-equal 2 '(+ 1 1))
  (assert-ql-equal 'a '(car '(a b c)))
  #f)

(provide "test-base")
