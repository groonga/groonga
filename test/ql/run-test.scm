#!/usr/bin/env gosh

(add-load-path ".")

(use test.unit)

(define base-dir (sys-dirname *program-name*))
(for-each load
          (if (symbol-bound? 'glob)
            (glob #`",|base-dir|/**/test-*.scm")
            (append (sys-glob #`",|base-dir|/test-*.scm")
                    (sys-glob #`",|base-dir|/*/test-*.scm"))))
