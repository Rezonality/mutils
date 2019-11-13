
(define-library (chibi iset base)
  (cond-expand
   (chibi (import (chibi) (srfi 9)))
   (else (import (scheme base))))
  (cond-expand
   ((library (srfi 151)) (import (srfi 151)))
   ((library (srfi 33)) (import (srfi 33)))
   (else (import (srfi 60))))
  (include "base.scm")
  (cond-expand  ;; workaround for #1342
   (chicken (begin (define Integer-Set #f)))
   (else))
  (export
   %make-iset make-iset iset? iset-contains? Integer-Set
   iset-start iset-end iset-bits iset-left iset-right
   iset-start-set! iset-end-set! iset-bits-set! iset-left-set! iset-right-set!))
