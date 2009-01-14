;; -*- coding: utf-8 -*-

(define-module test-disp
  (extend test.unit.test-case)
  (use ql-test-utils)
  (use ql-assertions))
(select-module test-disp)

(define key "record")
(define int-value -12345)
(define uint-value 12345)
(define int64-value (expt 2 50))
(define float-value 29.29)
(define short-text-value "short text")
(define text-value "text")
(define long-text-value "long text")
(define time-value '(987654321 . 12345))

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
  (assert-ql-send
   `(define record (<test> ::new ,key
                           :int ,int-value
                           :uint ,uint-value
                           :int64 ,int64-value
                           :float ,float-value
                           :short-text ,short-text-value
                           :text ,text-value
                           :long-text ,long-text-value
                           :time ,#`"#:<,(car time-value).,(cdr time-value)>")))
  #f)

(define (teardown)
  (teardown-ql))

(define (test-json)
  (assert-ql-equal-disp-json `((,(format #f "~s" key)
                                ,int-value
                                ,uint-value
                                ,int64-value
                                ,float-value
                                ,(format #f "~s" short-text-value)
                                ,(format #f "~s" text-value)
                                ,(format #f "~s" long-text-value)
                                ,(format #f "~d.~6,'0d"
                                         (car time-value)
                                         (cdr time-value))))
                             '`(: ,(<test> ::scan #t)
                                  (.:key .int .uint .int64 .float
                                         .short-text .text .long-text .time)))
  #f)


(define json-expected `(,(format #f "~s" key)
                        ,int-value
                        ,uint-value
                        ,int64-value
                        ,float-value
                        ,(format #f "~s" short-text-value)
                        ,(format #f "~s" text-value)
                        ,(format #f "~s" long-text-value)
                        ,(format #f "~d.~6,'0d"
                                 (car time-value)
                                 (cdr time-value))))
(define json-empty-expected '(0 0 0 0.0
                              "\"\"" "\"\"" "\"\""
                              "0.0"))

(define (test-json)
  (assert-ql-equal-disp-json `(,json-expected)
                             '`(: ,(<test> ::scan #t)
                                  (.:key .int .uint .int64 .float
                                         .short-text .text .long-text .time)))
  #f)

(define (test-json-limit-and-offset)
  (assert-ql-send '(<test> ::new "record2"))
  (assert-ql-send '(<test> ::new "record3"))
  (assert-ql-equal-disp-json `(,json-expected
                               ("\"record2\"" ,@json-empty-expected)
                               ("\"record3\"" ,@json-empty-expected))
                             '`(: ,(<test> ::scan #t)
                                  (.:key .int .uint .int64 .float
                                         .short-text .text .long-text .time)))
  (assert-ql-equal-disp-json `(("\"record2\"" ,@json-empty-expected)
                               ("\"record3\"" ,@json-empty-expected))
                             '`(: ,(<test> ::scan #t)
                                  (.:key .int .uint .int64 .float
                                         .short-text .text .long-text .time)
                                  1))
  (assert-ql-equal-disp-json `(("\"record2\"" ,@json-empty-expected))
                             '`(: ,(<test> ::scan #t)
                                  (.:key .int .uint .int64 .float
                                         .short-text .text .long-text .time)
                                  1 1))
  #f)

(define (test-tsv)
  (assert-ql-equal-disp-tsv `((,key
                               ,int-value
                               ,uint-value
                               ,int64-value
                               ,float-value
                               ,short-text-value
                               ,text-value
                               ,long-text-value
                               ,(format #f "~d.~6,'0d"
                                        (car time-value)
                                        (cdr time-value))))
                            '`(: ,(<test> ::scan #t)
                                 (.:key .int .uint .int64 .float
                                        .short-text .text .long-text .time)))
  #f)

(provide "test-disp")
