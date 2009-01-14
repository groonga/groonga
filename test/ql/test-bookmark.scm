;; -*- coding: utf-8 -*-

(define-module test-bookmark
  (extend test.unit.test-case)
  (use ql-test-utils)
  (use ql-assertions))
(select-module test-bookmark)

(define (setup)
  (setup-ql)
  (assert-ql-send '(ptable '<items>))
  #f)

(define lisp-item-key "http://ja.wikipedia.org/wiki/LISP")
(define lisp-item-title "LISP")
(define why-lisp-item-key
  "http://www.unfindable.net/~yabuki/article/why_lisp.html")
(define why-lisp-item-title "なぜLispなのか")
(define javascript-item-key "http://d.hatena.ne.jp/brazil/20050829/1125321936")
(define javascript-item-title
  "[翻訳]JavaScript: 世界で最も誤解されたプログラミング言語")

(define (setup-items)
  (assert-ql-send `(<items> ::new ,lisp-item-key))
  (assert-ql-send `(<items> ::new ,why-lisp-item-key))
  #f)

(define (setup-terms)
  (setup-items)
  (assert-ql-send '(<items> ::def :title <text>))
  (assert-ql-send '(ptable '<terms> :ngram :normalize))
  (assert-ql-send '(<terms> ::def :item_title :as '(<items>.title ::match ())))
  (assert-ql-send `((<items> : ,lisp-item-key) :title ,lisp-item-title))
  (assert-ql-send `((<items> : ,why-lisp-item-key) :title ,why-lisp-item-title))
  #f)

(define moritan-user-key "moritan")
(define moritan-user-name "モリタン")
(define taporobo-user-key "taporobo")
(define taporobo-user-name "タポロボ")

(define (setup-users)
  (assert-ql-send '(ptable '<users>))
  (assert-ql-send '(<users> ::def :name <text>))
  (assert-ql-send `(<users> ::new ,moritan-user-key :name ,moritan-user-name))
  (assert-ql-send `(<users> ::new ,taporobo-user-key :name ,taporobo-user-name))
  #f)

(define (setup-comments)
  (setup-terms)
  (setup-users)
  (assert-ql-send '(ptable '<comments>))
  (assert-ql-send '(<comments> ::def :item <items>))
  (assert-ql-send '(<comments> ::def :author <users>))
  (assert-ql-send '(<comments> ::def :content <text>))
  (assert-ql-send '(<comments> ::def :issued <int>))
  (assert-ql-send '(<terms> ::def :comment_content
                            :as '(<comments>.content ::match ())))
  #f)

(define javascript-comment-content "JavaScript LISP")
(define (add-sample-comment)
  (assert-ql-send `(<items> ::new ,javascript-item-key
                            :title ,javascript-item-title))
  (assert-ql-send `(<comments> ::new "1"
                               :item ,javascript-item-key
                               :author ,moritan-user-key
                               :content ,javascript-comment-content
                               :issued 1187430026))
  #f)

(define (define-add-bookmark)
  (assert-ql-send
   '(define (add-bookmark item-url item-title
                          comment-author comment-content comment-issued)
      (let ((item (or (<items> : item-url)
                      (<items> ::new item-url :title item-title)))
            (id (+ (<comments> ::nrecords) 1)))
        (<comments> ::new id
                    :item item
                    :author comment-author
                    :content comment-content
                    :issued comment-issued))))
  #f)

(define cont-bookmark-key "http://practical-scheme.net/docs/cont-j.html")
(define cont-bookmark-title "なんでも継続")
(define tail-recursive-bookmark-key
  "http://d.hatena.ne.jp/higepon/20070815/1187192864")
(define tail-recursive-bookmark-title "末尾再帰")

(define moritan-cont-bookmark-comment "継続 LISP Scheme")
(define taporobo-tail-recursive-bookmark-comment "末尾再帰 Scheme LISP")
(define taporobo-cont-bookmark-comment "トランポリン LISP continuation")

(define (add-bookmarks)
  (assert-ql-send
   `(add-bookmark ,cont-bookmark-key ,cont-bookmark-title ,moritan-user-key
                  ,moritan-cont-bookmark-comment 1187568692))
  (assert-ql-send
   `(add-bookmark ,tail-recursive-bookmark-key ,tail-recursive-bookmark-title
                  ,taporobo-user-key ,taporobo-tail-recursive-bookmark-comment
                  1187568793))
  (assert-ql-send
   `(add-bookmark ,cont-bookmark-key ,cont-bookmark-title ,taporobo-user-key
                  ,taporobo-cont-bookmark-comment 1187568692))
  #f)

(define (setup-bookmarks)
  (setup-comments)
  (define-add-bookmark)
  (add-bookmarks)
  #f)

(define (teardown)
  (teardown-ql))

(define (test-nrecords)
  (assert-ql-equal 0 '(<items> ::nrecords))
  (setup-items)
  (assert-ql-equal 2 '(<items> ::nrecords))
  #f)

(define (test-get)
  (assert-ql-equal #f `(<items> : ,lisp-item-key))
  (assert-ql-send `(<items> ::new ,lisp-item-key))
  (assert-ql-equal lisp-item-key `((<items> : ,lisp-item-key) ::key))
  #f)

(define (test-full-text-search)
  (setup-terms)
  (assert-ql-equal-disp #`"[\",lisp-item-key\", \",why-lisp-item-key\"]"
                        `(<terms>.item_title : "lisp")
                        :json)
  #f)

(define (test-bookmark-relation)
  (setup-comments)
  (assert-ql-equal 0 '(<comments> ::nrecords))
  (add-sample-comment)
  (assert-ql-equal-disp moritan-user-name
                        '(list ':
                               (<terms>.comment_content : "JavaScript")
                               '(.author.name))
                        :tsv)
  #f)

(define (test-function)
  (setup-comments)
  (define-add-bookmark)
  (add-bookmarks)
  (assert-ql-equal-disp-tsv '(("3"))
                            '((<terms>.comment_content : "LISP") ::nr))
  #f)

(define (test-disp)
  (setup-bookmarks)
  (assert-ql-equal-disp-tsv `((,cont-bookmark-title
                               ,moritan-user-name
                               ,moritan-cont-bookmark-comment)
                              (,tail-recursive-bookmark-title
                               ,taporobo-user-name
                               ,taporobo-tail-recursive-bookmark-comment)
                              (,cont-bookmark-title
                               ,taporobo-user-name
                               ,taporobo-cont-bookmark-comment))
                            '`(:
                               ,(<terms>.comment_content : "LISP")
                               (.item.title .author.name .content)))
  #f)

(define (test-sort)
  (setup-bookmarks)
  (assert-ql-equal-disp-tsv `((,tail-recursive-bookmark-title
                               ,taporobo-user-name
                               ,taporobo-tail-recursive-bookmark-comment)
                              (,cont-bookmark-title
                               ,moritan-user-name
                               ,moritan-cont-bookmark-comment)
                              (,cont-bookmark-title
                               ,taporobo-user-name
                               ,taporobo-cont-bookmark-comment))
                            '`(:
                               ,((<terms>.comment_content : "LISP")
                                 ::sort :issued :desc)
                               (.item.title .author.name .content)))
  #f)

(define (test-group)
  (setup-bookmarks)
  (assert-ql-equal-disp-tsv `(("2"
                               ,cont-bookmark-key
                               ,cont-bookmark-title)
                              ("1"
                               ,tail-recursive-bookmark-key
                               ,tail-recursive-bookmark-title))
                            '`(:
                               ,((<terms>.comment_content : "LISP")
                                 ::group :item)
                               (.:nsubrecs .:key .title)))
  #f)

(define (test-union)
  (setup-bookmarks)
  (assert-ql-send '(define r1 (<terms>.comment_content : "LISP")))
  (assert-ql-send '(define r2 (<terms>.item_title : "*W1:50 LISP")))
  (assert-ql-equal-disp-tsv `(("50" ,lisp-item-title)
                              ("50" ,why-lisp-item-title)
                              ("10" ,cont-bookmark-title)
                              ("5" ,tail-recursive-bookmark-title))
                            '`(:
                               ,(((r1 ::group :item) ::union r2) ::sort ::score)
                               (.:score .title)))
  #f)

(provide "test-bookmark")
