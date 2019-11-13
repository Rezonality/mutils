
;;> Implementation of the SHA-2 (Secure Hash Algorithm) cryptographic
;;> hash.

(define-library (chibi crypto sha2)
  (import (scheme base))
  (export sha-224 sha-256)
  (cond-expand
   (chibi
    (include "sha2-native.scm")
    (include-shared "crypto"))
   (else
    (cond-expand
     ((library (srfi 151)) (import (srfi 151)))
     ((library (srfi 33)) (import (srfi 33)))
     (else (import (srfi 60))))
    (import (chibi bytevector))
    (include "sha2.scm"))))

;;> \procedure{(sha-224 src)}
;;>
;;> Computes SHA-224 digest of the \var{src} which can be a string,
;;> a bytevector, or a binary input port. Returns a hexadecimal string
;;> (in lowercase).

;;> \procedure{(sha-256 src)}
;;>
;;> Computes SHA-256 digest of the \var{src} which can be a string,
;;> a bytevector, or a binary input port. Returns a hexadecimal string
;;> (in lowercase).
