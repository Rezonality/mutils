(define-library (srfi 115)
  (export regexp regexp? valid-sre? rx regexp->sre char-set->sre
          regexp-matches regexp-matches? regexp-search
          regexp-replace regexp-replace-all regexp-match->list
          regexp-fold regexp-extract regexp-split regexp-partition
          regexp-match? regexp-match-count
          regexp-match-submatch
          regexp-match-submatch-start regexp-match-submatch-end)
  (cond-expand (chicken (import (scheme base))) (else))
  (import (chibi regexp)))
