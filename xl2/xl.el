;;; xl.el --- Basic mode for XL

;; Copyright (C) 2003, 2004, 2005  Free Software Foundation, Inc.

;; Author: Christophe de Dinechin (christophe@dinechin.org)
;; Based on: Python mode by Dave Love <fx@gnu.org>
;; Maintainer: FSF
;; Created: July 2005
;; Keywords: languages
;; WARNING: This file works only on Emacs 22, it fails with 21.x

;; This file is part of GNU Emacs.

;; This file is free software; you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation; either version 2, or (at your option)
;; any later version.

;; This file is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.

;; You should have received a copy of the GNU General Public License
;; along with GNU Emacs; see the file COPYING.  If not, write to
;; the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
;; Boston, MA 02111-1307, USA.

;;; Commentary:

;; Major mode for editing programs written in XL.

;; Successive TABs cycle between possible indentations for the line.
;; There is symbol completion using lookup in XL.

;;; Code:

;; It's messy to autoload the relevant comint functions so that comint
;; is only required when inferior XL is used.
(require 'comint)
(eval-when-compile
  (require 'compile)
  (autoload 'info-lookup-maybe-add-help "info-look"))
(autoload 'compilation-start "compile")

(defgroup xl nil
  "Editing mode for XL programs"
  :group 'languages
  :version "22.1"
  :link '(emacs-commentary-link "xl"))

;;;###autoload
(add-to-list 'interpreter-mode-alist '("xl" . xl-mode))
;;;###autoload
(add-to-list 'auto-mode-alist '("\\.xl\\'" . xl-mode))
;;;###autoload
(add-to-list 'auto-mode-alist '("\\.xs\\'" . xl-mode))

;;;; Font lock
(defvar xl-keywords
  '("generic" "is" "import" "using" "use"
    "in" "out" "variable" "constant" "var" "const"
    "return" "other" "require" "ensure" "written" "yield"
    "try" "catch" "retry" "raise"
    "while" "until" "for" "do" "loop" "exit" "restart"
    "translate" "when" "where" "with" "case"
    "if" "then" "else" "not" "and" "or" "xor" "nil")
  "List of words highlighted as 'keywords' in XL mode")

(defvar xl-warnings
  '("raise")
  "List of words prefixing a 'warnings' in XL mode")

(defvar xl-types
  '("boolean" "integer" "character" "real" "text")
  "List of words highlighted as 'types' in XL mode")

(defvar xl-functors
  '("function" "procedure" "to" "translation" "iterator")
  "List of words declaring functions in XL mode")

(defvar xl-type-declarators
  '("type" "class")
  "List of words declaring types in XL mode")

(defvar xl-module-declarators
  '("module" "record")
  "List of words declaring modules or records in XL mode")

(defvar xl-outdent-words
  '("else")
  "List of words declaring modules or records in XL mode")

(defvar xl-quotes
  '(("<<" ">>"))
  "List of character sequences used as quotes in XL mode")

(defvar xl-comments
  '(("//")
    ("/*" "*/"))
  "List of character sequences used as comments in XL mode")


(defun xl-separators-regexp (sep-list)
  "Return a regexp matching the separator list"
  (cons 'or (mapcar '(lambda(x)
                       (if (cadr x) 
                           (list 'and
                                 (car x)
                                 '(0+ (or printing "\f" "\n"))
                                 (cadr x))
                         (list 'and
                               (car x)
                               '(0+ (or printing))
                               '(or "\n" "\f"))))
                    sep-list)))
  
;; Strictly speaking, XL has no keywords, but colorizing special words
;; that have standard uses in normal XL programs still makes sense.
(defvar xl-font-lock-keywords

  ;; Strings
  `((,(rx (group (eval (xl-separators-regexp xl-quotes))))
     (1 font-lock-string-face))

    ;; Comments
    (,(rx (group (eval (xl-separators-regexp xl-comments))))
     (1 font-lock-comment-face))
    
    ;; Pragmas
    (,(rx (group (and "{" (0+ blank) (0+ word))))
     (1 font-lock-preprocessor-face))
    (,(rx (group "}"))
     (1 font-lock-preprocessor-face))
    
    ;; Keywords
    (,(rx (group (and word-start
                      (eval (cons 'or xl-keywords))
                      word-end)))
     (1 font-lock-keyword-face))
    
    ;; Warnings (raise X)
    (,(rx (and word-start
               (group (eval (cons 'or xl-warnings)))
               (1+ space) (group (and (1+ word)
                                      (0+ (and "." (1+ word)))))))
     (1 font-lock-keyword-face) (2 font-lock-warning-face))
    
    ;; Constants
    (,(rx (and word-start
               (group (1+ (char digit "_"))
                     "#"
                     (1+ word)
                     (optional "." (0+ word))
                     (optional "#")
                     (optional (and (char "eE")
                                    (optional (char "+-"))
                                    (1+ (char digit "_")))))
              word-end))
    (1 font-lock-constant-face))
   (,(rx (and word-start
              (group (1+ (char digit "_"))
                     (optional "." (0+ (char digit "_")))
                     (optional (and (char "eE")
                                    (optional (char "+-"))
                                    (1+ (char digit "_")))))
              word-end))
    (1 font-lock-constant-face))              

   ;; Predefined types
   (,(rx (and word-start
              (group (eval (cons 'or xl-types)))
              word-end))
    (1 font-lock-type-face))

   ;; Executable types (procedures)
   (,(rx (and word-start
              (group (eval (cons 'or xl-functors)))
              (1+ space) (group (and (1+ word)
                                     (0+ (and "." (1+ word)))))))
    (1 font-lock-keyword-face) (2 font-lock-function-name-face))

   ;; Type declarations
   (,(rx (and word-start
              (group (eval (cons 'or xl-type-declarators)))
              (1+ space) (group (and (1+ word)
                                     (0+ (and "." (1+ word)))))))
    (1 font-lock-keyword-face) (2 font-lock-type-face))

   ;; Module declarations (we use the preprocessor face for these)
   (,(rx (and word-start
              (group (eval (cons 'or xl-module-declarators)))
              (1+ space) (group (and (1+ word)
                                     (0+ (and "." (1+ word)))))))
    (1 font-lock-keyword-face) (2 font-lock-preprocessor-face))

   ;; Import directives
   (,(rx (and word-start
              (group "import")
              (1+ space) (group (1+ word))
              (0+ space) "=" (0+ space)
              (group (and (1+ word))
                     (0+ (and "." (1+ word))))))
    (1 font-lock-keyword-face)
    (2 font-lock-variable-name-face)
    (3 font-lock-preprocessor-face))
   (,(rx (and word-start
              (group "import")
              (1+ space)
              (group (and (1+ word))
                     (0+ (and "." (1+ word))))))
    (1 font-lock-keyword-face)
    (2 font-lock-preprocessor-face))

   ;; Declaration
   (,(rx (and word-start
              (group (1+ word))
              (0+ space) ":" (0+ space)
              (group (and (1+ word))
                     (0+ (and "." (1+ word))))))
    (1 font-lock-variable-name-face)
    (2 font-lock-type-face))

   ;; Assignment
   (,(rx (and word-start
              (group (1+ word))
              (0+ space) (or ":=" "+=" "-=" "*=" "/=") (0+ space)))
    (1 font-lock-variable-name-face)) ))


;; Recognize XL text separators
(defconst xl-font-lock-syntactic-keywords
  `(
    (,(rx (and "[[" (0+ anything) "]]")))
    (,(rx (and (group (syntax string-quote)) anything (backref 1)))) ))

;;;; Keymap and syntax

(defvar xl-mode-map
  (let ((map (make-sparse-keymap)))
    ;; Mostly taken from xl-mode.el.
    (define-key map "\177" 'xl-backspace)
    (define-key map [(shift tab)] 'xl-unindent-for-tab)
    (define-key map [(control tab)] 'xl-shift-right)
    (define-key map [(control shift tab)] 'xl-shift-left)
    (define-key map "\C-c<" 'xl-shift-left)
    (define-key map "\C-c>" 'xl-shift-right)

    (define-key map "\C-c4" 'xl-bwc1)
    (define-key map "\C-c7" 'xl-bwc2)
    (define-key map "\C-c6" 'xl-fwc1)
    (define-key map "\C-c9" 'xl-fwc2)
    (define-key map "\C-c1" 'xl-beginning-of-statement)
    (define-key map "\C-c3" 'xl-end-of-statement)

    (define-key map "\C-c\C-k" 'xl-mark-block)
    (define-key map "\C-c\C-n" 'xl-next-statement)
    (define-key map "\C-c\C-p" 'xl-previous-statement)
    (define-key map "\C-c\C-u" 'xl-beginning-of-block)
    (define-key map "\C-c\C-f" 'xl-describe-symbol)
    (define-key map "\C-c\C-w" 'xl-check)
    (define-key map "\C-c\C-v" 'xl-check) ; a la sgml-mode
    (define-key map "\C-c\C-s" 'xl-send-string)
    (define-key map [?\C-\M-x] 'xl-send-defun)
    (define-key map "\C-c\C-r" 'xl-send-region)
    (define-key map "\C-c\M-r" 'xl-send-region-and-go)
    (define-key map "\C-c\C-c" 'xl-send-buffer)
    (define-key map "\C-c\C-z" 'xl-switch-to-xl)
    (define-key map "\C-c\C-m" 'xl-load-file)
    (define-key map "\C-c\C-l" 'xl-load-file) ; a la cmuscheme
    (substitute-key-definition 'complete-symbol 'xl-complete-symbol
			       map global-map)
    ;; Fixme: Add :help to menu.
    (easy-menu-define xl-menu map "XL Mode menu"
      '("XL"
	["Shift region left" xl-shift-left :active mark-active]
	["Shift region right" xl-shift-right :active mark-active]
	"-"
	["Mark block" xl-mark-block]
	["Mark def/class" mark-defun
	 :help "Mark innermost definition around point"]
	"-"
	["Start of block" xl-beginning-of-block]
	["End of block" xl-end-of-block]
	["Start of definition" beginning-of-defun
	 :help "Go to start of innermost definition around point"]
	["End of definition" end-of-defun
	 :help "Go to end of innermost definition around point"]
	["Previous statement" xl-previous-statement]
	["Next statement" xl-next-statement]
	"-"
	["Start interpreter" run-xl
	 :help "Run `inferior' XL in separate buffer"]
	["Import/reload file" xl-load-file
	 :help "Load into inferior XL session"]
	["Eval buffer" xl-send-buffer
	 :help "Evaluate buffer en bloc in inferior XL session"]
	["Eval region" xl-send-region :active mark-active
	 :help "Evaluate region en bloc in inferior XL session"]
	["Eval def/class" xl-send-defun
	 :help "Evaluate current definition in inferior XL session"]
	["Switch to interpreter" xl-switch-to-xl
	 :help "Switch to inferior XL buffer"]
	["Check file" xl-check :help "Run pychecker"]
	["Debugger" pdb :help "Run pdb under GUD"]
	"-"
	["Help on symbol" xl-describe-symbol
	 :help "Use pydoc on symbol at point"]))
    map))

(defvar xl-mode-syntax-table
  (let ((table (make-syntax-table)))
    ;; Give punctuation syntax to ASCII that normally has symbol
    ;; syntax or has word syntax and isn't a letter.
    (modify-syntax-entry ?%  "." table)
    (modify-syntax-entry ?\" "\"" table)
    (modify-syntax-entry ?\' "\"" table)

    (modify-syntax-entry ?:  "." table)
    (modify-syntax-entry ?\; "." table)
    (modify-syntax-entry ?&  "." table)
    (modify-syntax-entry ?\|  "." table)
    (modify-syntax-entry ?+  "." table)
    (modify-syntax-entry ?-  "." table)
    (modify-syntax-entry ?=  "." table)
    (modify-syntax-entry ?<  "." table)
    (modify-syntax-entry ?>  "." table)
    (modify-syntax-entry ?$ "." table)
    (modify-syntax-entry ?\[ "." table)
    (modify-syntax-entry ?\] "." table)
    (modify-syntax-entry ?\{ "." table)
    (modify-syntax-entry ?\} "." table)
    (modify-syntax-entry ?. "." table)
    (modify-syntax-entry ?\\ "." table)

    ;; a single slash is punctuation, but a double slash starts a comment
    (modify-syntax-entry ?/  ". 124b" table)
    (modify-syntax-entry ?*  ". 23"   table)
    ;; and \f and \n end a comment
    (modify-syntax-entry ?\f  "> b" table)
    (modify-syntax-entry ?\n "> b"  table)
    ;; Give CR the same syntax as newline, for selective-display
    (modify-syntax-entry ?\^m "> b" table)

    ;; # is a paired delimiter in 16#FFFE#, but we treat it as symbol
    (modify-syntax-entry ?#  "." table)
  
    ;; define what belongs in Ada symbols
    (modify-syntax-entry ?_ "_" table)

    ;; define parentheses to match
    (modify-syntax-entry ?\( "()" table)
    (modify-syntax-entry ?\) ")(" table)
    table))

;;;; Utility stuff

(defsubst xl-in-string/comment ()
  "Return non-nil if point is in a XL literal (a comment or string)."
  (syntax-ppss-context (syntax-ppss)))

(defun xl-skip-comments/blanks (&optional backward)
  "Skip comments and blank lines.
BACKWARD non-nil means go backwards, otherwise go forwards.
Doesn't move out of comments -- should be outside or at end of line."
  (forward-comment (if backward most-negative-fixnum most-positive-fixnum)))

(defun xl-skip-comments/blanks2 (&optional backward)
  "Same as above"
  (let ((regexp (rx (1+ (or (eval (xl-separators-regexp xl-comments))
                            (1+ (char blank ?\r ?\f)))))))
    (if backward
        (re-search-backward regexp (point-min) t)
      (re-search-forward regexp (point-max) t))))

(defun xl-fwc1 () "" (interactive) (xl-skip-comments/blanks))
(defun xl-bwc1 () "" (interactive) (xl-skip-comments/blanks t))
(defun xl-fwc2 () "" (interactive) (xl-skip-comments/blanks2))
(defun xl-bwc2 () "" (interactive) (xl-skip-comments/blanks2 t))

(defun xl-continuation-line-p ()
  "Return non-nil if current line continues a previous one.
The criteria are that the previous line ends in a punctuation (except semicolon)
or that the bracket/paren nesting depth is nonzero."
  (or (and (save-excursion
             (xl-skip-comments/blanks t)
             (and (re-search-backward (rx graph) (point-min) t)
                  (looking-at (rx (syntax punctuation)))
                  (not (looking-at (rx (or ";" ")" "]" "}"))))))
           (not (syntax-ppss-context (syntax-ppss))))
      (/= 0 (syntax-ppss-depth
	     (save-excursion	  ; syntax-ppss with arg changes point
	       (syntax-ppss (line-beginning-position)))))))

(defun xl-comment-line-p (&optional arg)
  "Return non-nil iff current line has only a comment.
With optional arg, return non-nil iff current line is empty or only a comment."
  (save-excursion
    (end-of-line)
    (when (or arg (eq 'comment (syntax-ppss-context (syntax-ppss))))
      (back-to-indentation)
      (looking-at (rx (or (syntax comment-start)
                          (eval (xl-separators-regexp xl-comments))
                          line-end))))))

(defun xl-beginning-of-string ()
  "Go to beginning of string around point.
Do nothing if not in string."
  (let ((state (syntax-ppss)))
    (when (eq 'string (syntax-ppss-context state))
      (goto-char (nth 8 state)))))

(defun xl-open-block-statement-p ()
  "Return non-nil if statement at point opens a block.
In XL, a statement opens a block if next line is more indented"
  (let ((indent (current-indentation)))
    (if (eobp) nil
      (if (xl-outdent-p) t
        (save-excursion
          (beginning-of-line)
          (forward-line)
          (if (xl-comment-line-p) (xl-skip-comments/blanks))
          (< indent (current-indentation)))))))

(defun xl-close-block-statement-p ()
  "Return non-nil if current line is a statement closing a block.
The criteria is that the previous line has a lower indent"
  (let ((indent (current-indentation)))
    (if (bobp) nil
      (save-excursion
        (forward-line -1)
        (while (and (not (bobp)) (xl-comment-line-p))
          (forward-line -1))
        (< indent (current-indentation))))))

(defun xl-outdent-p ()
  "Return non-nil if current line should outdent a level."
  (save-excursion
    (back-to-indentation)
    (looking-at (rx (eval (cons 'or xl-outdent-words))))))

;;;; Indentation.

(defcustom xl-indent 4
  "*Number of columns for a unit of indentation in XL mode.
See also `\\[xl-guess-indent]'"
  :group 'xl
  :type 'integer)

(defcustom xl-guess-indent t
  "*Non-nil means XL mode guesses `xl-indent' for the buffer."
  :type 'boolean
  :group 'xl)

(defcustom xl-indent-string-contents t
  "*Non-nil means indent contents of multi-line strings together.
This means indent them the same as the preceding non-blank line.
Otherwise indent them to column zero."
  :type '(choice (const :tag "Align with preceding" t)
		 (const :tag "Indent to column 0" nil))
  :group 'xl)

(defcustom xl-honour-comment-indentation nil
  "Non-nil means indent relative to preceding comment line.
Only do this for comments where the leading comment character is followed
by space.  This doesn't apply to comment lines, which are always indented
in lines with preceding comments."
  :type 'boolean
  :group 'xl)

(defcustom xl-continuation-offset 4
  "*Number of columns of additional indentation for continuation lines.
Continuation lines follow a line terminated by an operator other than semicolon"
  :group 'xl
  :type 'integer)

(defun xl-guess-indent ()
  "Guess step for indentation of current buffer.
Set `xl-indent' locally to the value guessed."
  (interactive)
  (save-excursion
    (save-restriction
      (widen)
      (goto-char (point-min))
      (let (done indent)
	(while (and (not done) (not (eobp)))
	  (when (and (re-search-forward (rx (and ?: (0+ space)
						 (or (syntax comment-start)
						     line-end)))
					nil 'move)
		     (xl-open-block-statement-p))
	    (save-excursion
	      (xl-beginning-of-statement)
	      (let ((initial (current-indentation)))
		(if (zerop (xl-next-statement))
		    (setq indent (- (current-indentation) initial)))
		(if (and (>= indent 2) (<= indent 8)) ; sanity check
		    (setq done t))))))
	(when done
	  (when (/= indent (default-value 'xl-indent))
	    (set (make-local-variable 'xl-indent) indent)
	    (unless (= tab-width xl-indent)
	      (setq indent-tabs-mode nil)))
	  indent)))))

(defun xl-calculate-indentation ()
  "Calculate XL indentation for line at point."
  (save-excursion
    (beginning-of-line)
    (let ((syntax (syntax-ppss))
	  start)
      (cond
       ((eq 'string (syntax-ppss-context syntax)) ; multi-line string
	(if (not xl-indent-string-contents)
	    0
	  (save-excursion
	    ;; Find indentation of preceding non-blank line within string.
	    (setq start (nth 8 syntax))
	    (forward-line -1)
	    (while (and (< start (point)) (looking-at "\\s-*$"))
	      (forward-line -1))
	    (current-indentation))))
       ((xl-continuation-line-p)
	(let ((point (point))
	      (open-start (cadr syntax)))
	  (if open-start
	      ;; Inside bracketed expression.
	      (progn
		(goto-char (1+ open-start))
		;; Look for first item in list (preceding point) and
		;; align with it, if found.
		(if (let ((parse-sexp-ignore-comments t))
                      (condition-case ()
                          (progn (forward-sexp)
                                 (backward-sexp)
                                 (< (point) point))
                        (error nil)))
		    (current-column)
		  ;; Otherwise indent relative to statement start, one
		  ;; level per bracketing level.
		  (goto-char (1+ open-start))
		  (xl-beginning-of-statement)
		  (+ (current-indentation) (* (car syntax) xl-indent))))
	    ;; Otherwise backslash-continued.
	    (forward-line -1)
	    (if (xl-continuation-line-p)
		;; We're past first continuation line.  Align with
		;; previous line.
		(current-indentation)
	      ;; First continuation line.  Indent one step, with an
	      ;; extra one if statement opens a block.
	      (save-excursion
		(xl-beginning-of-statement)
		(+ (current-indentation) xl-continuation-offset
		   (if (xl-open-block-statement-p)
		       xl-indent
		     0)))))))
       ((bobp) 
        0)
       (t (let ((point (point)))
	    (if xl-honour-comment-indentation
		;; Back over whitespace, newlines, non-indentable comments.
		(catch 'done
		  (while t
		    (if (cond ((bobp))
			      ;; not at comment start
			      ((not (forward-comment -1))
			       (xl-beginning-of-statement)
			       t)
			      ;; trailing comment
			      ((/= (current-column) (current-indentation))
			       (xl-beginning-of-statement)
			       t)
			      ;; indentable comment like xl-mode.el
			      ((and (looking-at (rx (and (syntax comment-start)
							 (or space line-end))))
				    (/= 0 (current-column)))))
			(throw 'done t))))
	      ;; Else back over all comments.
	      (xl-skip-comments/blanks t)
	      (xl-beginning-of-statement))
	    ;; don't lose on bogus outdent
	    (max 0 (+ (current-indentation)
		      (or (cond ((xl-open-block-statement-p)
				 xl-indent)
				((xl-outdent-p)
				 xl-indent))
			  (progn (goto-char point)
                                 (if (xl-outdent-p)
				     (- xl-indent)))
			  0)))))))))

;; (defun xl-comment-indent ()
;;   "`comment-indent-function' for XL."
;;   ;; If previous non-blank line was a comment, use its indentation.
;;   ;; FIXME: This seems unnecessary since the default code delegates to
;;   ;; indent-according-to-mode.  --Stef
;;   (unless (bobp)
;;     (save-excursion
;;       (forward-comment -1)
;;       (if (eq ?# (char-after)) (current-column)))))

;;;; Cycling through the possible indentations with successive TABs.

;; These don't need to be buffer-local since they're only relevant
;; during a cycle.

;; Alist of possible indentations and start of statement they would close.
(defvar xl-indent-list nil
  "Internal use.")
;; Length of the above
(defvar xl-indent-list-length nil
  "Internal use.")
;; Current index into the alist.
(defvar xl-indent-index nil
  "Internal use.")

(defun xl-initial-text ()
  "Text of line following indentation and ignoring any trailing comment."
  (buffer-substring (+ (line-beginning-position) (current-indentation))
		    (save-excursion
		      (end-of-line)
		      (forward-comment -1)
		      (point))))

(defun xl-indentation-levels ()
  "Return a list of possible indentations for this line.
Includes the default indentation, one extra indent,
and those which would close all enclosing blocks.
Assumes the line has already been indented per
`xl-indent-line'.  Elements of the list are actually pairs:
\(INDENTATION . TEXT), where TEXT is the initial text of the
corresponding block opening (or nil)."
  (let ((levels (list (cons (current-indentation) nil)))
        (unindents '()))
    ;; Only one possibility if we are in a continuation line.
    (unless (xl-continuation-line-p)
      (progn
        (save-excursion
          (while (xl-beginning-of-block)
            (push (cons (current-indentation) (xl-initial-text)) unindents)))
        (setq levels (append (reverse unindents) levels unindents levels))
        (push (cons (+ (current-indentation) xl-indent) t)
              levels)))
    levels))

;; This is basically what `xl-indent-line' would be if we didn't
;; do the cycling.
(defun xl-indent-line-1 ()
  "Subroutine of `xl-indent-line'."
  (let ((target (xl-calculate-indentation))
	(pos (- (point-max) (point))))
    (if (= target (current-indentation))
	(if (< (current-column) (current-indentation))
	    (back-to-indentation))
      (beginning-of-line)
      (delete-horizontal-space)
      (indent-to target)
      (if (> (- (point-max) pos) (point))
	  (goto-char (- (point-max) pos))))))

(defun xl-unindent-for-tab ()
  "Indent for tab with a -1 argument"
  (interactive)
  (indent-for-tab-command))

(defun xl-indent-line ()
  "Indent current line as XL code.
When invoked via `indent-for-tab-command', cycle through possible
indentations for current line.  The cycle is broken by a command different
from `indent-for-tab-command', i.e. successive TABs do the cycling."
  (interactive)
  ;; Don't do extra work if invoked via `indent-region', for instance.
  (if (not (or (eq this-command 'indent-for-tab-command)
               (eq this-command 'xl-unindent-for-tab)))
      (xl-indent-line-1)
    (if (or (eq last-command 'indent-for-tab-command)
            (eq last-command 'xl-unindent-for-tab))
	(if (= 1 xl-indent-list-length)
	    (message "Sole indentation")
	  (progn (setq xl-indent-index
                       (mod 
                        (if (eq this-command 'xl-unindent-for-tab)
                            (1- xl-indent-index)
                          (1+ xl-indent-index))
                        xl-indent-list-length))
		 (beginning-of-line)
		 (delete-horizontal-space)
		 (indent-to (car (nth xl-indent-index xl-indent-list)))
                 (let ((text (cdr (nth xl-indent-index
                                       xl-indent-list))))

                   (if text
                       (cond
                        ((eq text t) (message "Indenting"))
                        ((stringp text) (message "Closes: %s" text)))
                     (message "Current indentation")))))
      (xl-indent-line-1)
      (setq xl-indent-list (xl-indentation-levels)
	    xl-indent-list-length (length xl-indent-list)
	    xl-indent-index (1- xl-indent-list-length)))))

;; Fixme: Define an indent-region-function.  It should probably leave
;; lines alone if the indentation is already at one of the allowed
;; levels.  Otherwise, M-C-\ typically keeps indenting more deeply
;; down a function.

;;;; Movement.

(defun xl-beginning-of-defun ()
  "`beginning-of-defun-function' for XL.
Finds beginning of innermost nested class or method definition.
Returns the name of the definition found at the end, or nil if reached
start of buffer."
  (let ((ci (current-indentation))
	(def-re (rx (and line-start (0+ space) (eval (cons 'or xl-functors))
			 (1+ space)
			 (group (1+ (or word (syntax symbol)))))))
	found lep def-line)
    (if (xl-comment-line-p)
	(setq ci most-positive-fixnum))
    (while (and (not (bobp)) (not found))
      ;; Treat bol at beginning of function as outside function so
      ;; that successive C-M-a makes progress backwards.
      (setq def-line (looking-at def-re))
      (unless (bolp) (end-of-line))
      (setq lep (line-end-position))
      (if (and (re-search-backward def-re nil 'move)
	       ;; Must be less indented or matching top level, or
	       ;; equally indented if we started on a definition line.
	       (let ((in (current-indentation)))
                 (or (and (zerop ci) (zerop in))
                     (= lep (line-end-position)) ; on initial line
                     (and def-line (= in ci))
                     (< in ci)))
	       (not (xl-in-string/comment)))
	  (setq found t)))
    (back-to-indentation)))

(defun xl-end-of-defun ()
  "`end-of-defun-function' for XL.
Finds end of innermost nested class or method definition."
  (xl-beginning-of-defun)
  (xl-end-of-block))

(defun xl-beginning-of-statement ()
  "Go to start of current statement.
Accounts for continuation lines and multi-line strings."
  (interactive)
  (beginning-of-line)
  (xl-beginning-of-string)
  (while (xl-continuation-line-p)
    (beginning-of-line)
    (forward-line -1)))

(defun xl-end-of-statement ()
  "Go to the end of the current statement and return point.
Usually this is the beginning of the next line, but if there is a continuation,
we need to skip additional lines."
  (interactive)
  (end-of-line)
  (xl-skip-comments/blanks)
  (while (xl-continuation-line-p)
    (end-of-line)
    (xl-skip-comments/blanks))
  (point))

(defun xl-previous-statement (&optional count)
  "Go to start of previous statement.
With argument COUNT, do it COUNT times.  Stop at beginning of buffer.
Return count of statements left to move."
  (interactive "p")
  (unless count (setq count 1))
  (if (< count 0)
      (xl-next-statement (- count))
    (xl-beginning-of-statement)
    (while (and (> count 0) (not (bobp)))
      (xl-skip-comments/blanks t)
      (xl-beginning-of-statement)
      (unless (bobp) (setq count (1- count))))
    (back-to-indentation)
    count))

(defun xl-next-statement (&optional count)
  "Go to start of next statement.
With argument COUNT, do it COUNT times.  Stop at end of buffer.
Return count of statements left to move."
  (interactive "p")
  (unless count (setq count 1))
  (if (< count 0)
      (xl-previous-statement (- count))
    (beginning-of-line)
    (while (and (> count 0) (not (eobp)))
      (xl-end-of-statement)
      (xl-skip-comments/blanks)
      (setq count (1- count)))
    count))

(defun xl-beginning-of-block (&optional arg)
  "Go to start of current block.
With numeric arg, do it that many times.  If ARG is negative, call
`xl-end-of-block' instead.
If point is on the first line of a block, use its outer block.
If current statement is in column zero, don't move and return nil.
Otherwise return non-nil."
  (interactive "p")
  (unless arg (setq arg 1))
  (cond
   ((zerop arg))
   ((< arg 0) (xl-end-of-block (- arg)))
   (t
    (let ((point (point)))
      (if (xl-comment-line-p)
	  (xl-skip-comments/blanks t))
      (xl-beginning-of-statement)
      (let ((ci (current-indentation)))
	(if (zerop ci)
	    (not (goto-char point))	; return nil
	  ;; Look upwards for less indented statement.
	  (if (catch 'done
		(while (zerop (forward-line -1))
		  (when (and (< (current-indentation) ci)
			     (not (xl-comment-line-p t))
			     ;; Move to beginning to save effort in case
			     ;; this is in string.
			     (progn (xl-beginning-of-statement) t)
			     (xl-open-block-statement-p))
		    (beginning-of-line)
		    (throw 'done t)))
		(not (goto-char point))) ; Failed -- return nil
	      (xl-beginning-of-block (1- arg)))))))))

(defun xl-end-of-block (&optional arg)
  "Go to end of current block.
With numeric arg, do it that many times.  If ARG is negative, call
`xl-beginning-of-block' instead.
If current statement is in column zero and doesn't open a block, don't
move and return nil.  Otherwise return t."
  (interactive "p")
  (unless arg (setq arg 1))
  (if (< arg 0)
      (xl-beginning-of-block (- arg)))
  (while (and (> arg 0)
	      (let* ((point (point))
		     (_ (if (xl-comment-line-p)
			    (xl-skip-comments/blanks t)))
		     (ci (current-indentation))
		     (open (xl-open-block-statement-p)))
		(if (and (zerop ci) (not open))
		    (not (goto-char point))
		  (catch 'done
		    (while (zerop (xl-next-statement))
		      (when (or (and open (<= (current-indentation) ci))
				(< (current-indentation) ci))
			(xl-skip-comments/blanks t)
			(beginning-of-line 2)
			(throw 'done t)))
		    (not (goto-char point))))))
    (setq arg (1- arg)))
  (zerop arg))

;;;; Imenu.

(defvar xl-recursing)
(defun xl-imenu-create-index ()
  "`imenu-create-index-function' for XL.

Makes nested Imenu menus from nested `class' and `def' statements.
The nested menus are headed by an item referencing the outer
definition; it has a space prepended to the name so that it sorts
first with `imenu--sort-by-name' (though, unfortunately, sub-menus
precede it)."
  (unless (boundp 'xl-recursing)		; dynamically bound below
    (goto-char (point-min)))		; normal call from Imenu
  (let (index-alist			; accumulated value to return
	name)
    (while (re-search-forward
	    (rx (and line-start (0+ space) ; leading space
		     (or (group "def") (group "class"))	    ; type
		     (1+ space) (group (1+ (or word ?_))))) ; name
	    nil t)
      (unless (xl-in-string/comment)
	(let ((pos (match-beginning 0))
	      (name (match-string-no-properties 3)))
	  (if (match-beginning 2)	; def or class?
	      (setq name (concat "class " name)))
	  (save-restriction
	    (narrow-to-defun)
	    (let* ((xl-recursing t)
		   (sublist (xl-imenu-create-index)))
	      (if sublist
		  (progn (push (cons (concat " " name) pos) sublist)
			 (push (cons name sublist) index-alist))
		(push (cons name pos) index-alist)))))))
    (nreverse index-alist)))

;;;; `Electric' commands.

(defun xl-backspace (arg)
  "Maybe delete a level of indentation on the current line.
If not at the end of line's indentation, or on a comment line, just call
`backward-delete-char-untabify'.  With ARG, repeat that many times."
  (interactive "*p")
  (if (or (/= (current-indentation) (current-column))
	  (bolp)
	  (xl-continuation-line-p))
      (backward-delete-char-untabify arg)
    (let ((indent 0))
      (save-excursion
	(while (and (> arg 0) (xl-beginning-of-block))
	  (setq arg (1- arg)))
	(when (zerop arg)
	  (setq indent (current-indentation))
	  (message "Closes %s" (xl-initial-text))))
      (delete-horizontal-space)
      (indent-to indent))))
(put 'xl-backspace 'delete-selection 'supersede)

;;;; pychecker

(defcustom xl-check-command "pychecker --stdlib"
  "*Command used to check a XL file."
  :type 'string
  :group 'xl)

(defvar xl-saved-check-command nil
  "Internal use.")

;; After `sgml-validate-command'.
(defun xl-check (command)
  "Check a XL file (default current buffer's file).
Runs COMMAND, a shell command, as if by `compile'.
See `xl-check-command' for the default."
  (interactive
   (list (read-string "Checker command: "
		      (or xl-saved-check-command
			  (concat xl-check-command " "
				  (let ((name (buffer-file-name)))
				    (if name
					(file-name-nondirectory name))))))))
  (setq xl-saved-check-command command)
  (save-some-buffers (not compilation-ask-about-save) nil)
  (let ((compilation-error-regexp-alist
	 (cons '("(\\([^,]+\\), line \\([0-9]+\\))" 1 2)
	       compilation-error-regexp-alist)))
    (compilation-start command)))

;;;; Inferior mode stuff (following cmuscheme).

;; Fixme: Make sure we can work with IXL.

(defcustom xl-xl-command "xl"
  "*Shell command to run XL interpreter.
Any arguments can't contain whitespace.
Note that IXL may not work properly; it must at least be used with the
`-cl' flag, i.e. use `ixl -cl'."
  :group 'xl
  :type 'string)

(defcustom xl-jython-command "jython"
  "*Shell command to run Jython interpreter.
Any arguments can't contain whitespace."
  :group 'xl
  :type 'string)

(defvar xl-command xl-xl-command
  "Actual command used to run XL.
May be `xl-xl-command' or `xl-jython-command'.
Additional arguments are added when the command is used by `run-xl'
et al.")

(defvar xl-buffer nil
  "The current xl process buffer."
  ;; Fixme: a single process is currently assumed, so that this doc
  ;; is misleading.

;;   "*The current xl process buffer.
;; To run multiple XL processes, start the first with \\[run-xl].
;; It will be in a buffer named *XL*.  Rename that with
;; \\[rename-buffer].  Now start a new process with \\[run-xl].  It
;; will be in a new buffer, named *XL*.  Switch between the different
;; process buffers with \\[switch-to-buffer].

;; Commands that send text from source buffers to XL processes have
;; to choose a process to send to.  This is determined by global variable
;; `xl-buffer'.  Suppose you have three inferior XLs running:
;;     Buffer	Process
;;     foo		xl
;;     bar		xl<2>
;;     *XL*    xl<3>
;; If you do a \\[xl-send-region-and-go] command on some XL source
;; code, what process does it go to?

;; - In a process buffer (foo, bar, or *XL*), send it to that process.
;; - In some other buffer (e.g. a source file), send it to the process
;;   attached to `xl-buffer'.
;; Process selection is done by function `xl-proc'.

;; Whenever \\[run-xl] starts a new process, it resets `xl-buffer'
;; to be the new process's buffer.  If you only run one process, this will
;; do the right thing.  If you run multiple processes, you can change
;; `xl-buffer' to another process buffer with \\[set-variable]."
  )

(defconst xl-compilation-regexp-alist
  ;; FIXME: maybe these should move to compilation-error-regexp-alist-alist.
  `((,(rx (and line-start (1+ (any " \t")) "File \""
	       (group (1+ (not (any "\"<")))) ; avoid `<stdin>' &c
	       "\", line " (group (1+ digit))))
     1 2)
    (,(rx (and " in file " (group (1+ not-newline)) " on line "
	       (group (1+ digit))))
     1 2))
  "`compilation-error-regexp-alist' for inferior XL.")

(defvar inferior-xl-mode-map
  (let ((map (make-sparse-keymap)))
    ;; This will inherit from comint-mode-map.
    (define-key map "\C-c\C-l" 'xl-load-file)
    (define-key map "\C-c\C-v" 'xl-check)
    ;; Note that we _can_ still use these commands which send to the
    ;; XL process even at the prompt iff we have a normal prompt,
    ;; i.e. '>>> ' and not '... '.  See the comment before
    ;; xl-send-region.  Fixme: uncomment these if we address that.

    ;; (define-key map [(meta ?\t)] 'xl-complete-symbol)
    ;; (define-key map "\C-c\C-f" 'xl-describe-symbol)
    map))

;; Fixme: This should inherit some stuff from xl-mode, but I'm not
;; sure how much: at least some keybindings, like C-c C-f; syntax?;
;; font-locking, e.g. for triple-quoted strings?
(define-derived-mode inferior-xl-mode comint-mode "Inferior XL"
  "Major mode for interacting with an inferior XL process.
A XL process can be started with \\[run-xl].

Hooks `comint-mode-hook' and `inferior-xl-mode-hook' are run in
that order.

You can send text to the inferior XL process from other buffers containing
XL source.
 * `xl-switch-to-xl' switches the current buffer to the XL
    process buffer.
 * `xl-send-region' sends the current region to the XL process.
 * `xl-send-region-and-go' switches to the XL process buffer
    after sending the text.
For running multiple processes in multiple buffers, see `xl-buffer'.

\\{inferior-xl-mode-map}"
  :group 'xl
  (set-syntax-table xl-mode-syntax-table)
  (setq mode-line-process '(":%s"))
  (set (make-local-variable 'comint-input-filter) 'xl-input-filter)
  (add-hook 'comint-preoutput-filter-functions #'xl-preoutput-filter
	    nil t)
  ;; Still required by `comint-redirect-send-command', for instance
  ;; (and we need to match things like `>>> ... >>> '):
  (set (make-local-variable 'comint-prompt-regexp)
       (rx (and line-start (1+ (and (repeat 3 (any ">.")) ?\ )))))
  (set (make-local-variable 'compilation-error-regexp-alist)
       xl-compilation-regexp-alist)
  (compilation-shell-minor-mode 1))

(defcustom inferior-xl-filter-regexp "\\`\\s-*\\S-?\\S-?\\s-*\\'"
  "*Input matching this regexp is not saved on the history list.
Default ignores all inputs of 0, 1, or 2 non-blank characters."
  :type 'regexp
  :group 'xl)

(defun xl-input-filter (str)
  "`comint-input-filter' function for inferior XL.
Don't save anything for STR matching `inferior-xl-filter-regexp'."
  (not (string-match inferior-xl-filter-regexp str)))

;; Fixme: Loses with quoted whitespace.
(defun xl-args-to-list (string)
  (let ((where (string-match "[ \t]" string)))
    (cond ((null where) (list string))
	  ((not (= where 0))
	   (cons (substring string 0 where)
		 (xl-args-to-list (substring string (+ 1 where)))))
	  (t (let ((pos (string-match "[^ \t]" string)))
	       (if pos (xl-args-to-list (substring string pos))))))))

(defvar xl-preoutput-result nil
  "Data from last `_emacs_out' line seen by the preoutput filter.")

(defvar xl-preoutput-continuation nil
  "If non-nil, funcall this when `xl-preoutput-filter' sees `_emacs_ok'.")

(defvar xl-preoutput-leftover nil)

;; Using this stops us getting lines in the buffer like
;; >>> ... ... >>>
;; Also look for (and delete) an `_emacs_ok' string and call
;; `xl-preoutput-continuation' if we get it.
(defun xl-preoutput-filter (s)
  "`comint-preoutput-filter-functions' function: ignore prompts not at bol."
  (when xl-preoutput-leftover
    (setq s (concat xl-preoutput-leftover s))
    (setq xl-preoutput-leftover nil))
  (cond ((and (string-match (rx (and string-start (repeat 3 (any ".>"))
                                     " " string-end))
                            s)
              (/= (let ((inhibit-field-text-motion t))
                    (line-beginning-position))
                  (point)))
         "")
        ((string= s "_emacs_ok\n")
         (when xl-preoutput-continuation
           (funcall xl-preoutput-continuation)
           (setq xl-preoutput-continuation nil))
         "")
        ((string-match "_emacs_out \\(.*\\)\n" s)
         (setq xl-preoutput-result (match-string 1 s))
         "")
	((string-match ".*\n" s)
	 s)
	((or (eq t (compare-strings s nil nil "_emacs_ok\n" nil (length s)))
	     (let ((end (min (length "_emacs_out ") (length s))))
	       (eq t (compare-strings s nil end "_emacs_out " nil end))))
	 (setq xl-preoutput-leftover s)
	 "")
        (t s)))

;;;###autoload
(defun run-xl (&optional cmd noshow)
  "Run an inferior XL process, input and output via buffer *XL*.
CMD is the XL command to run.  NOSHOW non-nil means don't show the
buffer automatically.
If there is a process already running in `*XL*', switch to
that buffer.  Interactively, a prefix arg allows you to edit the initial
command line (default is `xl-command'); `-i' etc.  args will be added
to this as appropriate.  Runs the hook `inferior-xl-mode-hook'
\(after the `comint-mode-hook' is run).
\(Type \\[describe-mode] in the process buffer for a list of commands.)"
  (interactive (list (if current-prefix-arg
			 (read-string "Run XL: " xl-command)
		       xl-command)))
  (unless cmd (setq cmd xl-xl-command))
  (setq xl-command cmd)
  ;; Fixme: Consider making `xl-buffer' buffer-local as a buffer
  ;; (not a name) in XL buffers from which `run-xl' &c is
  ;; invoked.  Would support multiple processes better.
  (unless (comint-check-proc xl-buffer)
    (let* ((cmdlist (append (xl-args-to-list cmd) '("-i")))
	   (path (getenv "XLPATH"))
	   (process-environment		; to import emacs.py
	    (cons (concat "XLPATH=" data-directory
			  (if path (concat ":" path)))
		  process-environment)))
      (set-buffer (apply 'make-comint "XL" (car cmdlist) nil
			 (cdr cmdlist)))
      (setq xl-buffer (buffer-name)))
    (inferior-xl-mode)
    ;; Load function defintions we need.
    ;; Before the preoutput function was used, this was done via -c in
    ;; cmdlist, but that loses the banner and doesn't run the startup
    ;; file.  The code might be inline here, but there's enough that it
    ;; seems worth putting in a separate file, and it's probably cleaner
    ;; to put it in a module.
    (xl-send-string "import emacs"))
  (unless noshow (pop-to-buffer xl-buffer)))

;; Fixme: We typically lose if the inferior isn't in the normal REPL,
;; e.g. prompt is `help> '.  Probably raise an error if the form of
;; the prompt is unexpected; actually, it needs to be `>>> ', not
;; `... ', i.e. we're not inputting a block &c.  However, this may not
;; be the place to do it, e.g. we might actually want to send commands
;; having set up such a state.

(defun xl-send-command (command)
  "Like `xl-send-string' but resets `compilation-minor-mode'."
  (goto-char (point-max))
  (let ((end (marker-position (process-mark (xl-proc)))))
    (compilation-forget-errors)
    (xl-send-string command)
    (set-marker compilation-parsing-end end)
    (setq compilation-last-buffer (current-buffer))))

(defun xl-send-region (start end)
  "Send the region to the inferior XL process."
  ;; The region is evaluated from a temporary file.  This avoids
  ;; problems with blank lines, which have different semantics
  ;; interactively and in files.  It also saves the inferior process
  ;; buffer filling up with interpreter prompts.  We need a XL
  ;; function to remove the temporary file when it has been evaluated
  ;; (though we could probably do it in Lisp with a Comint output
  ;; filter).  This function also catches exceptions and truncates
  ;; tracebacks not to mention the frame of the function itself.
  ;;
  ;; The compilation-minor-mode parsing takes care of relating the
  ;; reference to the temporary file to the source.
  ;;
  ;; Fixme: Write a `coding' header to the temp file if the region is
  ;; non-ASCII.
  (interactive "r")
  (let* ((f (make-temp-file "py"))
	 (command (format "emacs.eexecfile(%S)" f))
	 (orig-start (copy-marker start)))
    (when (save-excursion
	    (goto-char start)
	    (/= 0 (current-indentation))) ; need dummy block
      (save-excursion
	(goto-char orig-start)
	;; Wrong if we had indented code at buffer start.
	(set-marker orig-start (line-beginning-position 0)))
      (write-region "if True:\n" nil f nil 'nomsg))
    (write-region start end f t 'nomsg)
    (with-current-buffer (process-buffer (xl-proc))	;Runs xl if needed.
      (xl-send-command command)
      ;; Tell compile.el to redirect error locations in file `f' to
      ;; positions past marker `orig-start'.  It has to be done *after*
      ;; xl-send-command's call to compilation-forget-errors.
      (compilation-fake-loc orig-start f))))

(defun xl-send-string (string)
  "Evaluate STRING in inferior XL process."
  (interactive "sXL command: ")
  (comint-send-string (xl-proc) string)
  (comint-send-string (xl-proc) "\n\n"))

(defun xl-send-buffer ()
  "Send the current buffer to the inferior XL process."
  (interactive)
  (xl-send-region (point-min) (point-max)))

;; Fixme: Try to define the function or class within the relevant
;; module, not just at top level.
(defun xl-send-defun ()
  "Send the current defun (class or method) to the inferior XL process."
  (interactive)
  (save-excursion (xl-send-region (progn (beginning-of-defun) (point))
				      (progn (end-of-defun) (point)))))

(defun xl-switch-to-xl (eob-p)
  "Switch to the XL process buffer.
With prefix arg, position cursor at end of buffer."
  (interactive "P")
  (pop-to-buffer (process-buffer (xl-proc))) ;Runs xl if needed.
  (when eob-p
    (push-mark)
    (goto-char (point-max))))

(defun xl-send-region-and-go (start end)
  "Send the region to the inferior XL process.
Then switch to the process buffer."
  (interactive "r")
  (xl-send-region start end)
  (xl-switch-to-xl t))

(defcustom xl-source-modes '(xl-mode jython-mode)
  "*Used to determine if a buffer contains XL source code.
If it's loaded into a buffer that is in one of these major modes, it's
considered a XL source file by `xl-load-file'.
Used by these commands to determine defaults."
  :type '(repeat function)
  :group 'xl)

(defvar xl-prev-dir/file nil
  "Caches (directory . file) pair used in the last `xl-load-file' command.
Used for determining the default in the next one.")

(defun xl-load-file (file-name)
  "Load a XL file FILE-NAME into the inferior XL process.
If the file has extension `.py' import or reload it as a module.
Treating it as a module keeps the global namespace clean, provides
function location information for debugging, and supports users of
module-qualified names."
  (interactive (comint-get-source "Load XL file: " xl-prev-dir/file
				  xl-source-modes
				  t))	; because execfile needs exact name
  (comint-check-source file-name)     ; Check to see if buffer needs saving.
  (setq xl-prev-dir/file (cons (file-name-directory file-name)
				   (file-name-nondirectory file-name)))
  (with-current-buffer (process-buffer (xl-proc)) ;Runs xl if needed.
    ;; Fixme: I'm not convinced by this logic from xl-mode.el.
    (xl-send-command
     (if (string-match "\\.py\\'" file-name)
	 (let ((module (file-name-sans-extension
			(file-name-nondirectory file-name))))
	   (format "emacs.eimport(%S,%S)"
		   module (file-name-directory file-name)))
       (format "execfile(%S)" file-name)))
    (message "%s loaded" file-name)))

;; Fixme: If we need to start the process, wait until we've got the OK
;; from the startup.
(defun xl-proc ()
  "Return the current XL process.
See variable `xl-buffer'.  Starts a new process if necessary."
  (or (if xl-buffer
	  (get-buffer-process (if (eq major-mode 'inferior-xl-mode)
				  (current-buffer)
				xl-buffer)))
      (progn (run-xl nil t)
	     (xl-proc))))

;;;; Context-sensitive help.

(defconst xl-dotty-syntax-table
  (let ((table (make-syntax-table)))
    (set-char-table-parent table xl-mode-syntax-table)
    (modify-syntax-entry ?. "_" table)
    table)
  "Syntax table giving `.' symbol syntax.
Otherwise inherits from `xl-mode-syntax-table'.")

(defvar view-return-to-alist)
(eval-when-compile (autoload 'help-buffer "help-fns"))

;; Fixme: Should this actually be used instead of info-look, i.e. be
;; bound to C-h S?  Can we use other pydoc stuff before xl 2.2?
(defun xl-describe-symbol (symbol)
  "Get help on SYMBOL using `help'.
Interactively, prompt for symbol.

Symbol may be anything recognized by the interpreter's `help' command --
e.g. `CALLS' -- not just variables in scope.
This only works for XL version 2.2 or newer since earlier interpreters
don't support `help'."
  ;; Note that we do this in the inferior process, not a separate one, to
  ;; ensure the environment is appropriate.
  (interactive
   (let ((symbol (with-syntax-table xl-dotty-syntax-table
		   (current-word)))
	 (enable-recursive-minibuffers t))
     (list (read-string (if symbol
			    (format "Describe symbol (default %s): " symbol)
			  "Describe symbol: ")
			nil nil symbol))))
  (if (equal symbol "") (error "No symbol"))
  (let* ((func `(lambda ()
		  (comint-redirect-send-command (format "emacs.ehelp(%S)\n"
							,symbol)
						"*Help*" nil))))
    ;; Ensure we have a suitable help buffer.
    ;; Fixme: Maybe process `Related help topics' a la help xrefs and
    ;; allow C-c C-f in help buffer.
    (let ((temp-buffer-show-hook	; avoid xref stuff
	   (lambda ()
	     (toggle-read-only 1)
	     (setq view-return-to-alist
		   (list (cons (selected-window) help-return-method))))))
      (help-setup-xref (list 'xl-describe-symbol symbol) (interactive-p))
      (with-output-to-temp-buffer (help-buffer)
	(with-current-buffer standard-output
	  (set (make-local-variable 'comint-redirect-subvert-readonly) t)
	  (print-help-return-message))))
    (if (and xl-buffer (get-buffer xl-buffer))
	(with-current-buffer xl-buffer
	  (funcall func))
      (setq xl-preoutput-continuation func)
      (run-xl nil t))))

(add-to-list 'debug-ignored-errors "^No symbol")

(defun xl-send-receive (string)
  "Send STRING to inferior XL (if any) and return result.
The result is what follows `_emacs_out' in the output (or nil)."
  (let ((proc (xl-proc)))
    (xl-send-string string)
    (setq xl-preoutput-result nil)
    (while (progn
	     (accept-process-output proc 5)
	     xl-preoutput-leftover))
    xl-preoutput-result))

;; Fixme: try to make it work with point in the arglist.  Also, is
;; there anything reasonable we can do with random methods?
;; (Currently only works with functions.)
(defun xl-eldoc-function ()
  "`eldoc-print-current-symbol-info' for XL.
Only works when point is in a function name, not its arglist, for instance.
Assumes an inferior XL is running."
  (let ((symbol (with-syntax-table xl-dotty-syntax-table
		  (current-word))))
    (when symbol
      (xl-send-receive (format "emacs.eargs(%S)" symbol)))))

;;;; Info-look functionality.

(defun xl-after-info-look ()
  "Set up info-look for XL.
Used with `eval-after-load'."
  (let* ((version (let ((s (shell-command-to-string (concat xl-command
							    " -V"))))
		    (string-match "^XL \\([0-9]+\\.[0-9]+\\>\\)" s)
		    (match-string 1 s)))
	 ;; Whether info files have a XL version suffix, e.g. in Debian.
	 (versioned
	  (with-temp-buffer
	    (with-no-warnings (Info-mode))
	    (condition-case ()
		;; Don't use `info' because it would pop-up a *info* buffer.
		(with-no-warnings
		 (Info-goto-node (format "(xl%s-lib)Miscellaneous Index"
					 version))
		 t)
	      (error nil)))))
    (info-lookup-maybe-add-help
     :mode 'xl-mode
     :regexp "[[:alnum:]_]+"
     :doc-spec
     ;; Fixme: Can this reasonably be made specific to indices with
     ;; different rules?  Is the order of indices optimal?
     ;; (Miscellaneous in -ref first prefers lookup of keywords, for
     ;; instance.)
     (if versioned
	 ;; The empty prefix just gets us highlighted terms.
	 `((,(concat "(xl" version "-ref)Miscellaneous Index") nil "")
	   (,(concat "(xl" version "-ref)Module Index" nil ""))
	   (,(concat "(xl" version "-ref)Function-Method-Variable Index"
		     nil ""))
	   (,(concat "(xl" version "-ref)Class-Exception-Object Index"
		     nil ""))
	   (,(concat "(xl" version "-lib)Module Index" nil ""))
	   (,(concat "(xl" version "-lib)Class-Exception-Object Index"
		     nil ""))
	   (,(concat "(xl" version "-lib)Function-Method-Variable Index"
		     nil ""))
	   (,(concat "(xl" version "-lib)Miscellaneous Index" nil "")))
       '(("(xl-ref)Miscellaneous Index" nil "")
	 ("(xl-ref)Module Index" nil "")
	 ("(xl-ref)Function-Method-Variable Index" nil "")
	 ("(xl-ref)Class-Exception-Object Index" nil "")
	 ("(xl-lib)Module Index" nil "")
	 ("(xl-lib)Class-Exception-Object Index" nil "")
	 ("(xl-lib)Function-Method-Variable Index" nil "")
	 ("(xl-lib)Miscellaneous Index" nil ""))))))
(eval-after-load "info-look" '(xl-after-info-look))

;;;; Miscellancy.

(defcustom xl-jython-packages '("java" "javax" "org" "com")
  "Packages implying `jython-mode'.
If these are imported near the beginning of the buffer, `xl-mode'
actually punts to `jython-mode'."
  :type '(repeat string)
  :group 'xl)

;; Called from `xl-mode', this causes a recursive call of the
;; mode.  See logic there to break out of the recursion.
(defun xl-maybe-jython ()
  "Invoke `jython-mode' if the buffer appears to contain Jython code.
The criterion is either a match for `jython-mode' via
`interpreter-mode-alist' or an import of a module from the list
`xl-jython-packages'."
  ;; The logic is taken from xl-mode.el.
  (save-excursion
    (save-restriction
      (widen)
      (goto-char (point-min))
      (let ((interpreter (if (looking-at auto-mode-interpreter-regexp)
			     (match-string 2))))
	(if (and interpreter (eq 'jython-mode
				 (cdr (assoc (file-name-nondirectory
					      interpreter)
					     interpreter-mode-alist))))
	    (jython-mode)
	  (if (catch 'done
		(while (re-search-forward
			(rx (and line-start (or "import" "from") (1+ space)
				 (group (1+ (not (any " \t\n."))))))
			(+ (point-min) 10000) ; Probably not worth customizing.
			t)
		  (if (member (match-string 1) xl-jython-packages)
		      (throw 'done t))))
	      (jython-mode)))))))

(defun xl-fill-paragraph (&optional justify)
  "`fill-paragraph-function' handling comments and multi-line strings.
If any of the current line is a comment, fill the comment or the
paragraph of it that point is in, preserving the comment's
indentation and initial comment characters.  Similarly if the end
of the current line is in or at the end of a multi-line string.
Otherwise, do nothing."
  (interactive "P")
  (or (fill-comment-paragraph justify)
      ;; The `paragraph-start' and `paragraph-separate' variables
      ;; don't allow us to delimit the last paragraph in a multi-line
      ;; string properly, so narrow to the string and then fill around
      ;; (the end of) the current line.
      (save-excursion
	(end-of-line)
	(let* ((syntax (syntax-ppss))
	       (orig (point))
	       (start (nth 8 syntax))
	       end)
	  (cond ((eq t (nth 3 syntax))	    ; in fenced string
		 (goto-char (nth 8 syntax)) ; string start
		 (condition-case ()	    ; for unbalanced quotes
		     (progn (forward-sexp)
			    (setq end (point)))
		   (error (setq end (point-max)))))
		((re-search-backward "\\s|\\s-*\\=" nil t) ; end of fenced
							   ; string
		 (forward-char)
		 (setq end (point))
		 (condition-case ()
		     (progn (backward-sexp)
			    (setq start (point)))
		   (error nil))))
	  (when end
	    (save-restriction
	      (narrow-to-region start end)
	      (goto-char orig)
	      (fill-paragraph justify))))))
      t)

(defun xl-shift-left (start end &optional count)
  "Shift lines in region COUNT (the prefix arg) columns to the left.
COUNT defaults to `xl-indent'.  If region isn't active, just shift
current line.  The region shifted includes the lines in which START and
END lie.  It is an error if any lines in the region are indented less than
COUNT columns."
  (interactive (if (/= (mark) (point))
		   (list (region-beginning) (region-end) current-prefix-arg)
		 (list (point) (point) current-prefix-arg)))
  (if count
      (setq count (prefix-numeric-value count))
    (setq count xl-indent))
  (when (> count 0)
    (save-excursion
      (goto-char start)
      (while (< (point) end)
	(if (and (< (current-indentation) count)
		 (not (looking-at "[ \t]*$")))
	    (error "Can't shift all lines enough"))
	(forward-line))
      (indent-rigidly start end (- count)))))

(add-to-list 'debug-ignored-errors "^Can't shift all lines enough")

(defun xl-shift-right (start end &optional count)
  "Shift lines in region COUNT (the prefix arg) columns to the right.
COUNT defaults to `xl-indent'.  If region isn't active, just shift
current line.  The region shifted includes the lines in which START and
END lie."
  (interactive (if mark-active
		   (list (region-beginning) (region-end) current-prefix-arg)
		 (list (point) (point) current-prefix-arg)))
  (if count
      (setq count (prefix-numeric-value count))
    (setq count xl-indent))
  (indent-rigidly start end count))

(defun xl-outline-level ()
  "`outline-level' function for XL mode.
The level is the number of `xl-indent' steps of indentation
of current line."
  (/ (current-indentation) xl-indent))

;; Fixme: Consider top-level assignments, imports, &c.
(defun xl-current-defun ()
  "`add-log-current-defun-function' for XL."
  (save-excursion
    ;; Move up the tree of nested `class' and `def' blocks until we
    ;; get to zero indentation, accumulating the defined names.
    (let ((start t)
	  accum)
      (while (or start (> (current-indentation) 0))
	(setq start nil)
	(xl-beginning-of-block)
	(end-of-line)
	(beginning-of-defun)
	(if (looking-at (rx (and (0+ space) (or "def" "class") (1+ space)
				 (group (1+ (or word (syntax symbol))))
				 ;; Greediness makes this unnecessary?  --Stef
				 symbol-end)))
	    (push (match-string 1) accum)))
      (if accum (mapconcat 'identity accum ".")))))

(defun xl-mark-block ()
  "Mark the block around point.
Uses `xl-beginning-of-block', `xl-end-of-block'."
  (interactive)
  (push-mark)
  (xl-beginning-of-block)
  (push-mark (point) nil t)
  (xl-end-of-block)
  (exchange-point-and-mark))

;;;; Completion.

(defun xl-symbol-completions (symbol)
  "Return a list of completions of the string SYMBOL from XL process.
The list is sorted."
  (when symbol
    (let ((completions
	   (condition-case ()
	       (car (read-from-string (xl-send-receive
				       (format "emacs.complete(%S)" symbol))))
	     (error nil))))
      (sort
       ;; We can get duplicates from the above -- don't know why.
       (delete-dups completions)
       #'string<))))

(defun xl-partial-symbol ()
  "Return the partial symbol before point (for completion)."
  (let ((end (point))
	(start (save-excursion
		 (and (re-search-backward
		       (rx (and (or buffer-start (regexp "[^[:alnum:]._]"))
				(group (1+ (regexp "[[:alnum:]._]")))
				point))
		       nil t)
		      (match-beginning 1)))))
    (if start (buffer-substring-no-properties start end))))

;; Fixme: We should have an abstraction of this sort of thing in the
;; core.
(defun xl-complete-symbol ()
  "Perform completion on the XL symbol preceding point.
Repeating the command scrolls the completion window."
  (interactive)
  (let ((window (get-buffer-window "*Completions*")))
    (if (and (eq last-command this-command)
	     window (window-live-p window) (window-buffer window)
	     (buffer-name (window-buffer window)))
	(with-current-buffer (window-buffer window)
	  (if (pos-visible-in-window-p (point-max) window)
	      (set-window-start window (point-min))
	    (save-selected-window
	      (select-window window)
	      (scroll-up))))
      ;; Do completion.
      (let* ((end (point))
	     (symbol (xl-partial-symbol))
	     (completions (xl-symbol-completions symbol))
	     (completion (if completions
			     (try-completion symbol completions))))
	(when symbol
	  (cond ((eq completion t))
		((null completion)
		 (message "Can't find completion for \"%s\"" symbol)
		 (ding))
		((not (string= symbol completion))
		 (delete-region (- end (length symbol)) end)
		 (insert completion))
		(t
		 (message "Making completion list...")
		 (with-output-to-temp-buffer "*Completions*"
		   (display-completion-list completions))
		 (message "Making completion list...%s" "done"))))))))

(eval-when-compile (require 'hippie-exp))

(defun xl-try-complete (old)
  "Completion function for XL for use with `hippie-expand'."
  (when (eq major-mode 'xl-mode)	; though we only add it locally
    (unless old
      (let ((symbol (xl-partial-symbol)))
	(he-init-string (- (point) (length symbol)) (point))
	(if (not (he-string-member he-search-string he-tried-table))
	    (push he-search-string he-tried-table))
	(setq he-expand-list
	      (and symbol (xl-symbol-completions symbol)))))
    (while (and he-expand-list
		(he-string-member (car he-expand-list) he-tried-table))
      (pop he-expand-list))
    (if he-expand-list
	(progn
	  (he-substitute-string (pop he-expand-list))
	  t)
      (if old (he-reset-string))
      nil)))

;;;; Modes.

(defvar outline-heading-end-regexp)
(defvar eldoc-documentation-function)

;;;###autoload
(define-derived-mode xl-mode fundamental-mode "XL"
  "Major mode for editing XL files.
Turns on Font Lock mode unconditionally since it is required for correct
parsing of the source.
See also `jython-mode', which is actually invoked if the buffer appears to
contain Jython code.  See also `run-xl' and associated XL mode
commands for running XL under Emacs.

The Emacs commands which work with `defun's, e.g. \\[beginning-of-defun], deal
with nested `def' and `class' blocks.  They take the innermost one as
current without distinguishing method and class definitions.  Used multiple
times, they move over others at the same indentation level until they reach
the end of definitions at that level, when they move up a level.
\\<xl-mode-map>
Colon is electric: it outdents the line if appropriate, e.g. for
an else statement.  \\[xl-backspace] at the beginning of an indented statement
deletes a level of indentation to close the current block; otherwise it
deletes a charcter backward.  TAB indents the current line relative to
the preceding code.  Successive TABs, with no intervening command, cycle
through the possibilities for indentation on the basis of enclosing blocks.

\\[fill-paragraph] fills comments and multiline strings appropriately, but has no
effect outside them.

Supports Eldoc mode (only for functions, using a XL process),
Info-Look and Imenu.  In Outline minor mode, `class' and `def'
lines count as headers.

\\{xl-mode-map}"
  :group 'xl
  (set (make-local-variable 'font-lock-defaults)
       '(xl-font-lock-keywords nil nil ((?_ . "w")) nil
                               (font-lock-syntactic-keywords
                                . xl-font-lock-syntactic-keywords)
				   ;; This probably isn't worth it.
				   ;; (font-lock-syntactic-face-function
				   ;;  . xl-font-lock-syntactic-face-function)
				   ))
  (set (make-local-variable 'parse-sexp-lookup-properties) t)
  (set (make-local-variable 'comment-start) "// ")
  ;;(set (make-local-variable 'comment-indent-function) #'xl-comment-indent)
  (set (make-local-variable 'indent-line-function) #'xl-indent-line)
  (set (make-local-variable 'paragraph-start) "\\s-*$")
  (set (make-local-variable 'fill-paragraph-function) 'xl-fill-paragraph)
  (set (make-local-variable 'require-final-newline) mode-require-final-newline)
  (set (make-local-variable 'add-log-current-defun-function)
       #'xl-current-defun)
  ;; Fixme: Generalize to do all blocks?
  (set (make-local-variable 'outline-regexp)
       "\\s-*\\(procedure\\|function\\|to\\|module\\)\\>")
  (set (make-local-variable 'outline-heading-end-regexp) ":\\s-*\n")
  (set (make-local-variable 'outline-level) #'xl-outline-level)
  (set (make-local-variable 'open-paren-in-column-0-is-defun-start) nil)
  (make-local-variable 'xl-saved-check-command)
  (set (make-local-variable 'beginning-of-defun-function)
       'xl-beginning-of-defun)
  (set (make-local-variable 'end-of-defun-function) 'xl-end-of-defun)
  (setq imenu-create-index-function #'xl-imenu-create-index)
  (set (make-local-variable 'eldoc-documentation-function)
       #'xl-eldoc-function)
  (add-hook 'eldoc-mode-hook
	    '(lambda () (run-xl 0 t)) nil t) ; need it running
  (if (featurep 'hippie-exp)
      (set (make-local-variable 'hippie-expand-try-functions-list)
	   (cons 'xl-try-complete hippie-expand-try-functions-list)))
  (unless font-lock-mode (font-lock-mode 1))
  (when xl-guess-indent (xl-guess-indent))
  (set (make-local-variable 'xl-command) xl-xl-command)
  (unless (boundp 'xl-mode-running)	; kill the recursion from jython-mode
    (let ((xl-mode-running t))
      (xl-maybe-jython))))

(custom-add-option 'xl-mode-hook 'imenu-add-menubar-index)
(custom-add-option 'xl-mode-hook
		   '(lambda ()
		      "Turn on Indent Tabs mode."
		      (set (make-local-variable 'indent-tabs-mode) t)))
(custom-add-option 'xl-mode-hook 'turn-on-eldoc-mode)

;;;###autoload
(define-derived-mode jython-mode xl-mode  "Jython"
  "Major mode for editing Jython files.
Like `xl-mode', but sets up parameters for Jython subprocesses.
Runs `jython-mode-hook' after `xl-mode-hook'."
  :group 'xl
  (set (make-local-variable 'xl-command) xl-jython-command))

(provide 'xl)
;; arch-tag: 6fce1d99-a704-4de9-ba19-c6e4912b0554
;;; xl.el ends here
