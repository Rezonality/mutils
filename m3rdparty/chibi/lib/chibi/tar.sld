
(define-library (chibi tar)
  (import (scheme base) (scheme file) (scheme time) (srfi 1) (scheme write)
          (chibi string) (chibi binary-record) (chibi pathname)
          (chibi filesystem))
  (cond-expand
   ((library (srfi 151)) (import (srfi 151)))
   ((library (srfi 33)) (import (srfi 33)))
   (else (import (srfi 60))))
  (cond-expand
   (chibi
    (import (chibi system)))
   (chicken
    (import posix)
    (begin
      (define (user-name x) (if (pair? x) (car x) "nobody"))
      (define (group-name x) (if (pair? x) (car x) "nobody")))))
  (export
   ;; basic
   tar make-tar tar? read-tar write-tar
   ;; utilities
   tar-safe? tar-files tar-fold tar-extract tar-extract-file tar-create
   ;; accessors
   tar-path tar-path-prefix tar-mode tar-uid tar-gid
   tar-owner tar-group tar-size
   tar-time tar-type tar-link-name
   tar-path-set! tar-mode-set! tar-uid-set! tar-gid-set!
   tar-owner-set! tar-group-set! tar-size-set!
   tar-time-set! tar-type-set! tar-link-name-set!
   tar-device-major tar-device-major-set!
   tar-device-minor tar-device-minor-set!
   tar-ustar tar-ustar-set!)
  (include "tar.scm"))
