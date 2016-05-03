;; Mode for editing files in the Latitude language

(require 'smie)

(defvar latitude-mode-map
  (let ((map (make-sparse-keymap)))
    map))

; TODO Lisp mode seems to define a lot of symbols as whitespace; change them to symbols here
(defvar latitude-mode-syntax-table
   (let ((st (make-syntax-table lisp-mode-syntax-table)))
     (modify-syntax-entry ?\; "<" st)
     (modify-syntax-entry ?\n ">" st)
     (modify-syntax-entry ?\{ "(}1n" st)
     (modify-syntax-entry ?\* ". 23n" st)
     (modify-syntax-entry ?\} "){4n" st)
     (modify-syntax-entry ?\. "." st)
     (modify-syntax-entry ?\, "." st)
     (modify-syntax-entry ?\: " " st)
     (modify-syntax-entry ?\= "_" st)
     (modify-syntax-entry ?\~ "_" st)
     (modify-syntax-entry ?\? "_" st)
     (modify-syntax-entry ?\! "_" st)
     (modify-syntax-entry ?\< "_" st)
     (modify-syntax-entry ?\> "_" st)
     (modify-syntax-entry ?\| "_" st)
     (modify-syntax-entry ?\$ "w" st)
     st))

(defvar latitude-mode-font-lock-keywords
  (list `("\\_<\\(\\sw+\\)\\s-*:=\\s-*{" (1 font-lock-function-name-face))
        `("\\_<\\([A-Z]\\sw+\\)\\s-*:=" (1 font-lock-type-face))
        `("\\_<\\(\\sw+\\)\\s-*:=" (1 font-lock-variable-name-face))
        `(,(regexp-opt '("clone" "toString" "pretty" "meta" "global" "lexical" "dynamic" "parent"
                         "here" "again" "self" "invoke" "get" "has" "put" "slot" "hold" "callCC"
                         "call" "if" "while" "ifTrue" "ifFalse" "not" "or" "and" "loop" "throw"
                         "catch" "handle" "load" "inject" "iterator" "$lexical" "$dynamic" "scope"
                         "$scope" "cons" "car" "cdr" "proc" "id" "memo" "inject" "implements" "takes"
                         "localize" "this")
                       'symbols) . font-lock-builtin-face)
        `(,(regexp-opt '("Object" "True" "False" "Nil" "Symbol" "String" "Number" "Boolean" "Method"
                         "Proc" "Stream" "SystemCall" "Cont" "Exception" "SystemError" "Array" "Kernel"
                         "Sequence" "ArgList" "Collection" "StreamError" "SystemArgError"
                         "SystemCallError" "TypeError" "SlotError" "ContError" "ParseError"
                         "BoundsError" "Mixin" "Cons" "Cell" "Lockbox" "Latchkey" "Cached"
                         "IOError" "Wildcard" "Ellipsis" "Match" "NoMatch" "LazySequence")
                       'symbols) . font-lock-constant-face)))

;; (defun latitude-mode-skip-blanks ()
;;   (forward-line -1)
;;   (while (and (not (bobp)) (looking-at-p "\n"))
;;     (forward-line -1)))

;; (defun latitude-mode-indent ()
;;   (save-excursion
;;     (beginning-of-line)
;;     (if (bobp)
;;         (indent-line-to 0)
;;       (let ((curr-indent (save-excursion
;;                            (latitude-mode-skip-blanks)
;;                            (current-indentation))))
;;         ; First, check if there is an unclosed open brace on the previous line. If so, we are
;;         ; starting a new method
;;         (

(defvar latitude-mode-smie-grammar
  (smie-prec2->grammar
   (smie-bnf->prec2
    '((id)
      (exprs (exprs "." exprs) (expr))
      (expr (id ":=" expr) (id ":" args) ("[" args "]"))
      (args (args "," args) ("(" expr ")")))
    '((assoc ".") (assoc ",")))))

(defun latitude-mode-smie-indent (kind token)
  (prin1 (format " << %S %S >> " kind token))
  (pcase (cons kind token)
    (`(:list-intro . ":") t)))

(add-to-list 'auto-mode-alist '("\\.lat\\'" . latitude-mode))

(defun latitude-mode ()
  (interactive)
  (kill-all-local-variables)
  (set (make-local-variable 'comment-start) "; ")
  (set (make-local-variable 'comment-end) "")
  (set-syntax-table latitude-mode-syntax-table)
  (use-local-map latitude-mode-map)
  (set (make-local-variable 'font-lock-defaults) '(latitude-mode-font-lock-keywords))
  ; (smie-setup latitude-mode-smie-grammar #'latitude-mode-smie-indent)
  ;;
  (setq major-mode 'latitude-mode)
  (setq mode-name "Latitude")
  ;;
  (run-hooks 'latitude-mode-hook))

(provide 'latitude-mode)
