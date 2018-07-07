;;;; Copyright (c) 2018 Silvio Mayolo
;;;; See LICENSE.txt for licensing details

;; Mode for editing files in the Latitude language

(require 'smie)

(defvar latitude-mode-map
  (let ((map (make-sparse-keymap)))
    (define-key map (kbd "<backtab>") 'latitude-mode-dedent)
    map))

; TODO Lisp mode seems to define a lot of symbols as whitespace; change them to symbols here
(defvar latitude-mode-syntax-table
   (let ((st (make-syntax-table lisp-mode-syntax-table)))
     (modify-syntax-entry ?\; "<" st)
     (modify-syntax-entry ?\n ">" st)
     (modify-syntax-entry ?\{ "(}1n" st)
     (modify-syntax-entry ?\* "_ 23n" st)
     (modify-syntax-entry ?\} "){4n" st)
     (modify-syntax-entry ?\[ "(]" st)
     (modify-syntax-entry ?\] ")[" st)
     (modify-syntax-entry ?\. "." st)
     (modify-syntax-entry ?\, "." st)
     (modify-syntax-entry ?\: "." st)
     (modify-syntax-entry ?\= "_" st)
     (modify-syntax-entry ?\~ "_" st)
     (modify-syntax-entry ?\? "_" st)
     (modify-syntax-entry ?\! "_" st)
     (modify-syntax-entry ?\< "_" st)
     (modify-syntax-entry ?\> "_" st)
     (modify-syntax-entry ?\| "_" st)
     (modify-syntax-entry ?\+ "_" st)
     (modify-syntax-entry ?\$ "w" st)
     (modify-syntax-entry ?\& "w" st)
     st))

(defun latitude-mode--is-in-comment-or-string ()
  ; Checks at point
  (let ((face (get-text-property (point) 'face)))
    (unless (listp face)
      (setq face (list face)))
    (or (memq 'font-lock-comment-face face)
        (memq 'font-lock-string-face face))))

(defun latitude-mode--forward-string-check (lim)
  (save-excursion
    (let ((matched (search-forward "#\"" lim t)))
      (when matched
        (backward-char 2)
        (set-match-data (list (point) (1+ (point))))
        t))))

;; TODO Does not work over multiple lines
(defun latitude-mode--font-lock-search-string (lim start lhs rhs &optional check)
  (save-excursion
    (let ((matched (search-forward start lim t))
          (nests 0))
      (while (and matched
                  check
                  (equal (char-before (- (point) 2)) ?#))
        (setq matched (search-forward start lim t)))
      (when matched
        (while (and (>= nests 0)
                    (< (point) lim)
                    (not (eobp)))
          (let ((ch (following-char)))
            (cond
             ((equal ch ?\\) (forward-char))
             ((equal ch lhs) (setq nests (1+ nests)))
             ((equal ch rhs) (setq nests (1- nests)))
             (t))
            (forward-char)))
        (set-match-data (list (- matched 2)
                              (point)))
        t))))

(defvar latitude-mode-font-lock-keywords
  (list `(latitude-mode--forward-string-check
          . font-lock-string-face)
        `(,(lambda (lim) (latitude-mode--font-lock-search-string lim "#(" ?( ?)))
          . font-lock-string-face)
        `(,(lambda (lim) (latitude-mode--font-lock-search-string lim "#[" ?[ ?]))
          . font-lock-string-face)
        `(,(lambda (lim) (latitude-mode--font-lock-search-string lim "#{" ?{ ?}))
          . font-lock-string-face)
        `(,(lambda (lim) (latitude-mode--font-lock-search-string lim "'(" ?( ?) t))
          . font-lock-string-face)
        `("^#![^\n]*"
          . font-lock-comment-face)
        `("\\_<\\(\\(?:\\sw\\|\\s_\\)+\\)\\s-*::?=\\s-*{"
          (1 font-lock-function-name-face))
        `("\\_<\\(&?[A-Z]\\(?:\\sw\\|\\s_\\)*\\)\\_>"
          (1 font-lock-type-face))
        `("[^#]\\('[^][.,:;(){}\"\' \t\n\r`\\\\]+\\)"
          (1 font-lock-string-face))
        `("~[^][.,:;(){}\"\' \t\n\r`\\\\]+"
          . font-lock-string-face)
        `("\\_<\\(\\(?:\\sw\\|\\s_\\)+\\)\\s-*:="
          (1 font-lock-variable-name-face))
        `(,(regexp-opt '("clone" "toString" "pretty" "meta" "global" "parent"
                         "here" "again" "self" "invoke" "slot" "callCC"
                         "call" "if" "while" "ifTrue" "ifFalse" "not" "or" "and" "loop" "throw"
                         "catch" "handle" "rethrow" "inject" "iterator" "throwWith"
                         "lexical" "$dynamic" "cons" "car" "cdr" "proc" "memo"
                         "member?" "takes" "localize" "this" "resolve"
                         "protect" "thunk" "sigil" "do" "err" "then" "else" "when" "slot?"
                         "caller" "nth" "size" "length" "toBool" "catchAll" "closure" "shield"
                         "interface" "import" "importAll" "importAllSigils" "times" "upto"
                         "downto" "mod" "is?" "dup" "send" "tap" "stringify" "assign" "assignable"
                         "missing" "falsify" "println" "printf" "putln" "puts" "print" "dump"
                         "dumpObject" "printObject" "intern" "toProc" "sys" "escapable" "cond"
                         "case" "use" "breakable" "return" "break" "$break" "read" "readln"
                         "apply" "fromPackage")
                       'symbols)
          . font-lock-builtin-face)
        `(,(regexp-opt '("Object" "True" "False" "Nil" "Symbol" "String" "Number" "Boolean" "Method"
                         "Proc" "Stream" "Cont" "Exception" "Array" "Kernel"
                         "ArgList" "Collection" "Iterator"
                         "Mixin" "Cons" "Cached" "Parents" "Slots" "Process"
                         "StackFrame" "REPL" "FilePath" "FileHeader" "Chain"
                         "Conditional" "Dict")
                       'symbols)
          . font-lock-constant-face)
        `(,(regexp-opt '(":=" "::=" "=" "<-" "#'"))
          . font-lock-keyword-face)))

(defun latitude-mode-skip-blanks ()
  (forward-line -1)
  (while (and (not (bobp)) (looking-at-p "\n"))
    (forward-line -1)))

(defun latitude-mode-indent ()
  (interactive)
  (save-excursion
    (beginning-of-line)
    (if (bobp)
        (indent-line-to 0)
      (let ((prev-indent (save-excursion
                           (latitude-mode-skip-blanks)
                           (current-indentation)))
            (curr-indent (current-indentation)))
        (if (<= (+ prev-indent latitude-mode-indent) curr-indent)
            (indent-line-to 0)
          (indent-line-to (+ curr-indent latitude-mode-indent))))))
  (let ((final-indent (current-indentation)))
    (format "~S ~S" (current-column) final-indent)
    (if (< (current-column) final-indent)
        (move-to-column final-indent))))

(defun latitude-mode-dedent ()
  (interactive)
  (save-excursion
    (beginning-of-line)
    (if (bobp)
        (indent-line-to 0)
      (let ((prev-indent (save-excursion
                           (latitude-mode-skip-blanks)
                           (current-indentation)))
            (curr-indent (current-indentation)))
        (if (> latitude-mode-indent curr-indent)
            (indent-line-to (+ prev-indent latitude-mode-indent))
          (indent-line-to (- curr-indent latitude-mode-indent))))))
  (let ((final-indent (current-indentation)))
    (format "~S ~S" (current-column) final-indent)
    (if (< (current-column) final-indent)
        (move-to-column final-indent))))

(add-to-list 'auto-mode-alist '("\\.lat[s]?\\'" . latitude-mode))

(defcustom latitude-mode-indent 2
  "The number of spaces to indent one level of a code block in Latitude."
  :type 'integer
  :safe #'integerp
  :group 'latitude)

(defun latitude-mode ()
  (interactive)
  (kill-all-local-variables)
  (set (make-local-variable 'comment-start) "; ")
  (set (make-local-variable 'comment-end) "")
  (set-syntax-table latitude-mode-syntax-table)
  (use-local-map latitude-mode-map)
  (set (make-local-variable 'font-lock-defaults) '(latitude-mode-font-lock-keywords))
  (set (make-local-variable 'indent-line-function) #'latitude-mode-indent)
  ;;
  (setq major-mode 'latitude-mode)
  (setq mode-name "Latitude")
  (setq-local require-final-newline t)
  ;;
  (run-hooks 'latitude-mode-hook))

(provide 'latitude-mode)
