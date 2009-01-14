;; -*- coding: utf-8 -*-

(define-module test-slot
  (extend test.unit.test-case)
  (use ql-test-utils)
  (use ql-assertions))
(select-module test-slot)

(define (setup)
  (setup-ql)
  (assert-ql-send '(ptable '<test>))
  (assert-ql-send '(<test> ::def :int <int>))
  (assert-ql-send '(<test> ::def :uint <uint>))
  (assert-ql-send '(<test> ::def :int64 <int64>))
  (assert-ql-send '(<test> ::def :float <float>))
  (assert-ql-send '(<test> ::def :short-text <shorttext>))
  (assert-ql-send '(<test> ::def :text <text>))
  (assert-ql-send '(<test> ::def :long-text <longtext>))
  (assert-ql-send '(<test> ::def :time <time>))
  (assert-ql-send '(define record (<test> ::new "record")))
  #f)

(define (teardown)
  (teardown-ql))

(define (assert-accessor-equal expected key value . options)
  (apply assert-ql-equal expected `(record ,key ,value) options)
  (apply assert-ql-equal expected `(record ,key) options)
  #f)

(define (assert-accessor-equal-time expected key value)
  (define (groonga-time-format->float-format string)
    (regexp-replace #/^#:<([^.]+)\.([^.]+)>$/ string "(\\1 . \\2)"))
  (assert-accessor-equal expected key value
                         :prepare-output groonga-time-format->float-format)
  #f)

(define (assert-accessor-in-delta expected delta key value . options)
  (apply assert-ql-in-delta expected delta `(record ,key ,value) options)
  (apply assert-ql-in-delta expected delta `(record ,key) options)
  #f)

(define max-int (- (expt 2 31) 1))
(define min-int (- (expt 2 31)))
(define (test-slot-int)
  (assert-accessor-equal -1 :int -1)
  (assert-accessor-equal 0 :int 0)

  (assert-accessor-equal max-int :int max-int)
  (assert-accessor-equal min-int :int min-int)

  (assert-accessor-equal min-int :int (+ max-int 1)) ; is it OK?
  #f)

(define max-uint (- (expt 2 32) 1))
(define min-uint 0)
(define (test-slot-uint)
  (assert-accessor-equal max-int :uint max-int)
  (assert-accessor-equal 0 :uint 0)

  (assert-accessor-equal max-uint :uint max-uint)
  (assert-accessor-equal min-uint :uint min-uint)

  (assert-accessor-equal min-uint :uint (+ max-uint 1)) ; is it OK?
  (assert-accessor-equal max-uint :uint -1) ; is it OK?
  #f)

(define max-int64 (- (expt 2 63) 1))
(define min-int64 (- (expt 2 63)))
(define (test-slot-int64)
  (assert-accessor-equal -1 :int64 -1)
  (assert-accessor-equal 0 :int64 0)

  (assert-accessor-equal max-int64 :int64 max-int64)
  (assert-accessor-equal min-int64 :int64 min-int64)

  (assert-accessor-equal min-int64 :int64 (+ max-int64 1)) ; is it OK?
  #f)

(define large-float (* max-int64 2.0))
(define small-float (* min-int64 2.0))
(define (test-slot-float)
  (assert-accessor-in-delta 0 0.0001 :float 0)

  (assert-accessor-in-delta large-float (/ large-float 1000)
                            :float large-float)
  (assert-accessor-in-delta small-float (abs (/ small-float 1000))
                            :float small-float)
  #f)

(define (test-slot-short-text)
  (assert-accessor-equal "" :short-text "")
  (assert-accessor-equal "short text" :short-text "short text")
  ;; should test max length text
  #f)

(define (test-slot-text)
  (assert-accessor-equal "" :text "")
  (assert-accessor-equal "text" :text "text")
  ;; should test max length text
  #f)

(define (test-slot-long-text)
  (assert-accessor-equal "" :long-text "")
  (assert-accessor-equal "long text" :long-text "long text")
  ;; should test max length text
  #f)

(define (test-slot-time)
  (assert-accessor-equal-time '(0 . 0) :time 0)
  (assert-accessor-equal-time '(100 . 10) :time "#:<100.10>")
  (assert-accessor-equal-time `(,max-int . ,max-int)
                              :time #`"#:<,|max-int|.,|max-int|>")
  (assert-accessor-equal-time `(,min-int . ,min-int)
                              :time #`"#:<,|min-int|.,|min-int|>")

  (assert-ql-send-error `(record :time ,#`"#:<,(+ max-int 1).,|max-int|>"))
  (assert-ql-send-error `(record :time ,#`"#:<,|max-int|.,(+ max-int 1)>"))
  (assert-ql-send-error `(record :time ,#`"#:<,(- min-int 1).,|min-int|>"))
  (assert-ql-send-error `(record :time ,#`"#:<,|min-int|.,(- min-int 1)>"))
  #f)

(provide "test-slot")
