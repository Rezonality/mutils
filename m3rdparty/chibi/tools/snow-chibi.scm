#!/usr/bin/env chibi-scheme

;; This code was written by Alex Shinn in 2013 and placed in the
;; Public Domain.  All warranties are disclaimed.

(import (scheme base)
        (scheme process-context)
        (chibi snow commands)
        (chibi snow interface)
        (chibi app)
        (chibi config)
        (chibi pathname)
        (chibi process))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; (define repo-spec
;;   '((repository
;;      (conf
;;       (sibling
;;        (conf
;;         (name string)
;;         (url string)))
;;       (package
;;        (conf
;;         (name (list (or symbol integer)))
;;         (url string)
;;         (size integer)
;;         (checksums (alist symbol string))
;;         (signature (alist symbol string))
;;         (library
;;          (conf
;;           (name (list (or symbol integer)))
;;           (path string)
;;           (depends
;;            (list (list (or symbol integer string
;;                            (list (member < > <= >=) string)))))
;;           (provides (list (list (or symbol string))))
;;           (platforms (list (or symbol (list symbol))))
;;           (features (list symbol))
;;           (authors (list string))
;;           (maintainers (list string))
;;           (description string)
;;           (created string)
;;           (updated string)
;;           (version string)
;;           (licenses
;;            (list
;;             (or (member gpl2 gpl3 lgpl mit bsd artistic apache public-domain)
;;                 (list 'license
;;                       (conf (name string)
;;                             (url string)
;;                             (checksums (alist symbol string)))))))))))))))

(define conf-spec
  ;; name type aliases doc
  '((verbose? boolean (#\v "verbose") "print additional informative messages")
    (always-yes? boolean (#\y "always-yes") "answer all questions with yes")
    (always-no? boolean (#\n "always-no") "answer all questions with no")
    (require-signature? boolean ("require-sig" "require-signature")
                        "require signature on installation")
    (ignore-signature? boolean ("ignore-sig" "ignore-signature")
                       "don't verify package signatures")
    (ignore-digest? boolean ("ignore-digest") "don't verify package checksums")
    (skip-digest? boolean ("skip-digest") "don't provide digests without rsa")
    (skip-version-checks? boolean ("skip-version-checks")
                          "don't verify implementation versions")
    (sign-uploads? boolean ("sign-uploads") "sign with the rsa key if present")
    (host string "base uri of snow repository")
    (repository-uri (list string) ("repo") "uris or paths of snow repositories")
    (local-root-repository dirname "repository cache dir for root")
    (local-user-repository dirname "repository cache dir for non-root users")
    (update-strategy symbol
                     "when to refresh repo: always, never, cache or confirm")
    (install-prefix string "prefix directory for installation")
    (install-source-dir dirname "directory to install library source in")
    (install-library-dir dirname "directory to install shared libraries in")
    (install-binary-dir dirname "directory to install programs in")
    (install-data-dir dirname "directory to install data files in")
    (library-extension string "the extension to use for library files")
    (library-separator string "the separator to use for library components")
    (library-path (list string) "the path to search for local libraries")
    (installer symbol "name of installer to use")
    (builder symbol "name of builder to use")
    (program-builder symbol "name of program builder to use")
    (implementations (list symbol) ("impls") "impls to install for, or 'all'")
    (program-implementation symbol "impl to install programs for")
    (scheme-script string "shell command to use for running scheme scripts")
    (scheme-program-command string "shell command to use for running scheme programs")
    (chibi-path filename "path to chibi-scheme executable")
    (cc string "path to c compiler")
    (cflags string "flags for c compiler")
    (use-curl? boolean ("use-curl") "use curl for file uploads")
    (sexp? boolean ("sexp") "output information in sexp format")
    ))

(define (conf-default-path name)
  (or (get-environment-variable "SNOW_CHIBI_CONFIG")
      (make-path (or (get-environment-variable "HOME") ".")
                 (string-append "." name)
                 "config.scm")))

(define search-spec '())
(define show-spec '())
(define install-spec
  '((skip-tests? boolean ("skip-tests") "don't run tests even if present")
    (show-tests? boolean ("show-tests") "show test output even on success")
    (install-tests? boolean ("install-tests") "install test-only libraries")
    (auto-upgrade-dependencies?
     boolean ("auto-upgrade-dependencies")
     "upgrade install dependencies when newer versions are available")
    (use-sudo? symbol ("use-sudo") "always, never, or as-needed (default)")))
(define upgrade-spec
  install-spec)
(define remove-spec '())
(define status-spec '())
(define gen-key-spec
  '((bits integer)
    (validity-period string)
    (name string)
    (library-prefix (list symbol))
    (email string)
    (gen-rsa-key? boolean ("gen-rsa-key"))
    (gen-key-in-process? boolean ("gen-key-in-process"))))
(define reg-key-spec
  '((uri string)
    (email string)))
(define sign-spec
  '((output filename #\o)
    (digest symbol #\d)
    (email string)))
(define verify-spec
  '())
(define package-spec
  '((name sexp)
    (programs (list existing-filename))
    (data-files (list sexp))
    (authors (list string))
    (maintainers (list string))
    (recursive? boolean (#\r "recursive") "...")
    (version string)
    (version-file existing-filename)
    (license symbol)
    (doc existing-filename)
    (doc-from-scribble boolean)
    (description string)
    (test existing-filename)
    (test-library sexp)
    (sig-file existing-filename)
    (output filename)
    (output-dir dirname)
    ))
(define upload-spec
  `((uri string)
    ,@package-spec))
(define index-spec
  '())
(define update-spec
  '())
(define implementations-spec
  '())

(define app-spec
  `(snow
    "Snow package management"
    (@ ,conf-spec)
    (begin: ,(lambda (cfg) (restore-history cfg)))
    (end: ,(lambda (cfg) (save-history cfg)))
    (or
     (search
      "search for packages"
      (@ ,search-spec) (,command/search terms ...))
     (show
      "show package descriptions"
      (@ ,show-spec) (,command/show names ...))
     (install
      "install packages"
      (@ ,install-spec) (,command/install names ...))
     (upgrade
      "upgrade installed packages"
      (@ ,upgrade-spec) (,command/upgrade names ...))
     (remove
      "remove packages"
      (@ ,remove-spec) (,command/remove names ...))
     (status
      "print package status"
      (@ ,status-spec) (,command/status names ...))
     (package
      "create a package"
      (@ ,package-spec) (,command/package files ...))
     (gen-key
      "create an RSA key pair"
      (@ ,gen-key-spec) (,command/gen-key))
     (reg-key
      "register an RSA key pair"
      (@ ,reg-key-spec) (,command/reg-key))
     (sign
      "sign a package"
      (@ ,sign-spec) (,command/sign file))
     (verify
      "verify a signature"
      (@ ,verify-spec) (,command/verify file))
     (upload
      "upload a package to a remote repository"
      (@ ,upload-spec) (,command/upload files ...))
     (index
      "add a package to a local repository file"
      (@ ,index-spec) (,command/index files ...))
     (update
      "force an update of available package status"
      (@ ,update-spec) (,command/update))
     (implementations
      "print currently available scheme implementations"
      (@ ,implementations-spec) (,command/implementations))
     (help
      "print help"
      (,app-help-command args ...))
     )))

(run-application app-spec
                 (command-line)
                 (conf-load (conf-default-path "snow")))
