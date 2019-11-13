
(define-library (srfi 142)
  (export bitwise-not
          bitwise-and   bitwise-ior 
          bitwise-xor   bitwise-eqv
          bitwise-nand  bitwise-nor 
          bitwise-andc1 bitwise-andc2
          bitwise-orc1  bitwise-orc2 
          arithmetic-shift bit-count integer-length
          bitwise-if
          bit-set? any-bit-set? every-bit-set?
          first-set-bit
          bit-field bit-field-any? bit-field-every?
          bit-field-clear bit-field-set
          bit-field-replace bit-field-replace-same
          bit-field-rotate bit-field-reverse
          copy-bit integer->list list->integer
          integer->vector vector->integer
          bits bit-swap
          bitwise-fold bitwise-for-each bitwise-unfold
          make-bitwise-generator)
  (import (chibi)
          (rename (srfi 151)
                  (bitwise-if srfi-151:bitwise-if)
                  (bits->list integer->list)
                  (list->bits list->integer)
                  (bits->vector integer->vector)
                  (vector->bits vector->integer)))
  (begin
    (define (bitwise-if mask n m)
      (srfi-151:bitwise-if mask m n))))
