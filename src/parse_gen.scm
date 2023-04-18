#!/usr/bin/env gsi-script

; Lexer

(define (skip-char)         (read-char) (skip-whitespace))
(define (skip-line)
  (read-char)
  (if (not (newline? (peek-char))) (skip-line)))
(define (skip-whitespace)
  (cond
    ((whitespace? (peek-char)) (skip-char))
    ((comment? (peek-char)) (skip-line))))

(define (match str)
  (define (match-loop chars str)
    (cond
      ((nil? chars) str)
      ((char=? (head chars) (peek-char))
        (match-loop (tail chars)
                    (string-append str (string (read-char)))))
      (else #f)))
  (match-loop (string->list str) ""))

(define (expect str)
  (if (not (match str))
      (error "Expected " str)))

(define (skip-empty-lines n)
  (skip-whitespace)
  (if (newline? (peek-char))
      (begin (skip-char) (skip-empty-lines (++ n)) n)
      n))

(define (quote? char)
  (and (char? char) (char=? char #\")))

(define (read-str)
  (define (str-loop str)
    (if (quote? (peek-char))
        (begin (skip-char) str)
        (str-loop (string-append str (string (read-char))))))
  (read-char)
  (str-loop ""))

(define (read-sym)
  (define (sym-loop str)
    (if (or (whitespace? (peek-char)) (newline? (peek-char)) (eof-object? (peek-char)))
        (string->symbol str)
        (sym-loop (string-append str (string (read-char))))))
  (sym-loop ""))

(define (read-num)
  (define (read-num-loop numstr)
    (if (digit? (peek-char))
        (read-num-loop (string-append numstr (string (read-char))))
        (string->number numstr)))
  (read-num-loop ""))

(define Lexer (lambda (grammar readers)
  (let* ((terms (grammar 'terminals))
         (literals (filter terms string?))
         (syms (reject terms string?)))

    (define (find-match)
      (let ((matched (find-value literals match)))
        (if (nil? matched)
            nil
            (Token matched matched))))

    (define (read-token)
      (find-value readers (lambda (reader) (reader))))

    (lambda ()
      (skip-empty-lines 0)
      (if (eof-object? (peek-char))
          (Token '$ nil)
          (let ((matched (find-match)))
            (if (nil? matched)
                (read-token)
                matched)))))))

(define Token (struct '(Token type value)))
(define end-symbol '$)
(define (end-token? token) (equal? (token 'type) end-symbol))

(define (print-token token)
  (if (string? (token 'type))
    (display (enclose (token 'type) "\""))
    (display (token 'type)))
  (if (not (nil? (token 'value)))
      (begin
        (display ": ")
        (display (token 'value)))))



; Grammar

(define Rule (struct '(Rule lhs rhs)))
(define (rhs-head rule) (if (nil? (rule 'rhs)) nil (head (rule 'rhs))))
(define (print-rule rule)
  (let ((padding (- 16 (string-length (symbol->string (rule 'lhs))))))
    (display (rule 'lhs))
    (display (repeat " " padding))
    (display "→ ")
    (if (nil? (rule 'rhs))
        (display "ε")
        (display
          (join (map (rule 'rhs) (lambda (part)
            (if (string? part)
                (enclose part "\"")
                (symbol->string part))))
            " ")))))

(define Grammar (struct '(Grammar rules terminals nonterminals) (lambda (rules)
  (let* ((nonterms (unique (map rules (lambda (r) (r 'lhs)))))
         (syms (unique (flat-map rules (lambda (r) (r 'rhs)))))
         (terms (reject syms (lambda (sym) (in-list? sym nonterms)))))
    (list rules terms nonterms)))))

(define (rules-for sym grammar) (filter (grammar 'rules) (lambda (rule) (equal? sym (rule 'lhs)))))
(define (rule-id rule grammar) (++ (index-of rule (grammar 'rules))))
(define (grammar-rule id grammar) (nth (-- id) (grammar 'rules)))
(define (terminal? sym grammar) (in-list? sym (grammar 'terminals)))
(define (nonterminal? sym grammar) (in-list? sym (grammar 'nonterminals)))
(define (top-symbol grammar) ((first (grammar 'rules)) 'lhs))
(define (symbols-in grammar) (append (grammar 'terminals) (grammar 'nonterminals)))

(define (read-grammar filename)
  (define (epsilon? char)     (and (char? char) (char=? char #\ε)))
  (define (arrow? char)       (and (char? char) (char=? char #\→)))

  (define (read-rule-part)
    (skip-whitespace)
    (if (quote? (peek-char))
        (read-str)
        (read-sym)))

  (define (read-rhs lhs)
    (define (rhs-loop rhs)
      (skip-whitespace)
      (if (or (eof-object? (peek-char)) (newline? (peek-char)))
          (Rule lhs (reverse rhs))
          (rhs-loop (pair (read-rule-part) rhs))))
    (rhs-loop '()))

  (define (read-rule line)
    (skip-whitespace)
    (let ((lhs (read-rule-part)))
      (skip-whitespace)
      (if (not (arrow? (peek-char)))
          (error "Expected \"→\" after rule left-hand-side on line" line))
      (skip-char)
      (if (epsilon? (peek-char))
        (begin
          (skip-char)
          (Rule lhs nil))
        (begin
          (read-rhs lhs)))))

  (define (loop rules line)
    (let ((line (skip-empty-lines line)))
      (if (eof-object? (peek-char))
          (reverse rules)
          (loop (pair (read-rule line) rules) line))))

  (with-input-from-file filename (lambda ()
    (let* ((rules (loop '() 1))
           (top-sym ((first rules) 'lhs))
           (top-rule (Rule 'program (list top-sym '$))))
    (Grammar (pair top-rule rules))))))

(define (print-grammar grammar)
  (each (grammar 'rules)
        (lambda (rule) (let ((i (rule-id rule grammar)))
          (display (repeat " " (- 3 (number-length i))))
          (display i)
          (display " ")
          (print-rule rule)
          (newline)))))



; Parser generator

(define Config (struct '(Config rule position)))
(define (next-sym config) (nth (config 'position) ((config 'rule) 'rhs)))
(define (config-final? config) (= (config 'position) (length ((config 'rule) 'rhs))))
(define (advance-config config)
  (if (config-final? config)
      config
      (Config (config 'rule) (++ (config 'position)))))
(define (configs-equal? config1 config2)
  (and (equal? (config1 'rule) (config2 'rule))
       (equal? (config1 'position) (config2 'position))))

(define (closure configs grammar)
  (let* ((syms (unique (map configs next-sym)))
         (syms (filter syms (lambda (sym) (nonterminal? sym grammar))))
         (rules (flat-map syms (lambda (sym) (rules-for sym grammar))))
         (new-configs (map rules (lambda (rule) (Config rule 0))))
         (new-configs (reject new-configs (lambda (config) (in-list? config configs configs-equal?)))))
    (if (empty? new-configs)
        configs
        (closure (append configs new-configs) grammar))))

(define (successor sym configs grammar)
  (closure (map configs advance-config) grammar))

(define (print-config config)
  (define (format-parts parts)
    (map parts (lambda (part)
      (if (string? part) (enclose part "\"") (symbol->string part)))))

  (let* ((rule (config 'rule))
         (padding (- 16 (string-length (symbol->string (rule 'lhs))))))
    (display (rule 'lhs))
    (display (repeat " " padding))
    (display "→ ")
    (if (nil? (rule 'rhs))
        (display "ε")
        (let* ((split (split-at (rule 'rhs) (config 'position)))
               (before (head split))
               (after (tail split)))
          (display
            (join
              (append (format-parts before)
                      (list "·")
                      (format-parts after))
              " "))))))

(define State (struct '(State id configs successors)))

(define (grammar-states grammar)
  (define (start-state grammar)
    (let* ((rules (rules-for (top-symbol grammar) grammar))
           (configs (map rules (lambda (rule) (Config rule 0))))
           (configs (closure configs grammar)))
      (State 0 configs '())))

  (define (find-state configs states)
    (find states (lambda (state)
      (and (= (length configs) (length (state 'configs)))
           (all? configs (lambda (config)
                (in-list? config (state 'configs) configs-equal?)))))))

  (define (loop states queue)
    (define (gen-successors sym-set acc)
      (let* ((sym (head sym-set))
             (configs (tail sym-set))
             (successors (first acc))
             (queue (second acc))
             (next-id (third acc))
             (state (either (find-state configs states)
                            (find-state configs queue))))
        (if (nil? state)
            (let ((state (State next-id configs '())))
              (list (keylist-set successors sym next-id)
                    (pair state queue)
                    (++ next-id)))
            (list (keylist-set successors sym (state 'id))
                  queue
                  next-id))))

      (let* ((next-id (+ (length states) (length queue)))
             (state (head queue))
             (queue (tail queue))
             (configs (reject (state 'configs) config-final?))
             (by-sym (group-by configs (lambda (config) (next-sym config))))
             (sets (map by-sym (lambda (sym-configs)
                      (let ((sym (head sym-configs)) (configs (tail sym-configs)))
                        (pair sym (successor sym configs grammar))))))
             (succ-queue (reduce sets (list '() (reverse queue) next-id) gen-successors))
             (successors (first succ-queue))
             (queue (reverse (second succ-queue)))
             (state (State (state 'id) (state 'configs) (reverse successors))))
        (if (empty? queue)
            (reverse (pair state states))
            (loop (pair state states) queue))))

  (loop '() (list (start-state grammar))))

(define (print-state state reduction)
  (display (join (list "State" (state 'id)) " "))
  (newline)
  (each (state 'configs) (lambda (config)
    (display "  ")
    (print-config config)
    (newline)))
  (if (not (nil? (state 'successors)))
    (let ((longest-sym (max-in (map (map (map (state 'successors) head) to-string) string-length))))
      (each (state 'successors) (lambda (sym-succ)
        (let* ((sym (head sym-succ))
               (sym (if (string? sym) (enclose sym "\"") (to-string sym)))
               (succ (tail sym-succ))
               (padding (- longest-sym (string-length (to-string sym)))))
          (display "    On ")
          (display sym)
          (display (repeat " " padding))
          (display " → ")
          (display succ)
          (newline))))))
  (if (not (nil? reduction))
    (begin
      (display "    Reduce ")
      (display (reduction 'id))
      (display " (")
      (display (reduction 'sym))
      (display ", ")
      (display (reduction 'num))
      (display ")")
      (newline))))

(define Reduction (struct '(Reduction id sym num) (lambda (rule grammar)
  (let ((id (rule-id rule grammar))
        (num (length (rule 'rhs))))
    (list id (rule 'lhs) num)))))

(define Parser (struct '(Parser grammar states actions reductions) (lambda (grammar)
  (define (action-table states)
    (map (sort states (lambda (s1 s2) (s1 'id) < (s2 'id)))
         (lambda (state)
            (map (symbols-in grammar) (lambda (sym)
              (pair sym (keylist-get (state 'successors) sym)))))))

  (define (reduction-table states)
    (map states (lambda (state)
      (let ((configs (filter (state 'configs) config-final?)))
        (cond
          ((nil? configs) (pair (state 'id) nil))
          ((= (length configs) 1)
            (pair (state 'id) (Reduction ((head configs) 'rule) grammar)))
          (else
            (error "Reduce conflict in" (state 'id))))))))

  (let ((states (grammar-states grammar)))
    (list grammar
          states
          (action-table states)
          (reduction-table states))))))

(define (print-parse-states parser)
  (each (parser 'states) (lambda (state)
    (let ((reduction (keylist-get (parser 'reductions) (state 'id))))
      (print-state state reduction)
      (newline)))))

(define (print-parse-syms parser)
  (define (parse-sym-name sym)
    (cond
      ((eq? sym '$) "EOF")
      ((and (string? sym) (string=? sym "$")) "EOF")
      ((and (string? sym) (string=? sym "\\n")) "NL")
      ((and (string? sym) (string=? sym "+")) "Plus")
      ((and (string? sym) (string=? sym "-")) "Minus")
      ((and (string? sym) (string=? sym "*")) "Star")
      ((and (string? sym) (string=? sym "/")) "Slash")
      ((and (string? sym) (string=? sym "(")) "LParen")
      ((and (string? sym) (string=? sym ")")) "RParen")
      ((and (string? sym) (string=? sym "->")) "Arrow")
      ((and (string? sym) (string=? sym "=")) "Equal")
      ((and (string? sym) (string=? sym "==")) "EqualEqual")
      ((and (string? sym) (string=? sym "!=")) "NotEqual")
      ((and (string? sym) (string=? sym "|")) "Bar")
      ((and (string? sym) (string=? sym "<")) "LessThan")
      ((and (string? sym) (string=? sym "<=")) "LessEqual")
      ((and (string? sym) (string=? sym ">")) "GreaterThan")
      ((and (string? sym) (string=? sym ">=")) "GreaterEqual")
      ((and (string? sym) (string=? sym ":")) "Colon")
      ((and (string? sym) (string=? sym "::")) "ColonColon")
      ((and (string? sym) (string=? sym "#[")) "HashBracket")
      ((and (string? sym) (string=? sym "[")) "LBracket")
      ((and (string? sym) (string=? sym "]")) "RBracket")
      ((and (string? sym) (string=? sym "{")) "LBrace")
      ((and (string? sym) (string=? sym "}")) "RBrace")
      ((and (string? sym) (string=? sym ",")) "Comma")
      ((and (string? sym) (string=? sym ".")) "Dot")
      (else
        (let ((str (to-string sym)))
          (string-set str 0 (char-upcase (string-ref str 0)))))))
  (display "#pragma once")
  (newline)
  (newline)
  (display "enum {")
  (newline)
  (each (symbols-in (parser 'grammar)) (lambda (sym)
    (display "  ")
    (display "ParseSym")
    (display (parse-sym-name sym))
    (display ",")
    (newline)))
  (display "};")
  (newline)
  (newline))

(define (print-parse-tables parser)
  (define (print-row row size)
    (each row (lambda (item)
      (let* ((item (if (nil? item) "NULL" (to-string item)))
             (padding (- size (string-length item))))
        (display (repeat " " padding))
        (display item)
        (display ",")))))

  (define (chunk-size items max)
    (++ (div (length items) (++ (div (length items) max)))))

  (display "#pragma once")
  (newline)
  (newline)
  (display "/*")
  (newline)
  (display "Grammar:")
  (newline)
  (print-grammar (parser 'grammar))
  (display "*/")
  (newline)
  (newline)

  (display "#define NUM_STATES ")
  (display (length (parser 'states)))
  (newline)
  (display "#define NUM_SYMS ")
  (display (length (symbols-in (parser 'grammar))))
  (newline)
  (display "#define TOP_SYMBOL ")
  (display (index-of (top-symbol (parser 'grammar)) (symbols-in (parser 'grammar))))
  (newline)
  (display "#define NUM_LITERALS ")
  (display (length (filter ((parser 'grammar) 'terminals) string?)))
  (newline)
  (newline)

  (display "char *symbol_names[] = {")
  (newline)
  (each (with-index (symbols-in (parser 'grammar))) (lambda (isym)
    (display "  [")
    (display (head isym))
    (display "] = \"")
    (display (tail isym))
    (display "\",")
    (newline)))
  (display "};")
  (newline)
  (newline)

  (display "static unused struct {char *lexeme; u8 symbol;} literals[] = {")
  (newline)
  (each (sort (filter (with-index (symbols-in (parser 'grammar))) (lambda (isym)
                      (string? (tail isym))))
              (lambda (isym1 isym2)
                (> (string-length (tail isym1)) (string-length (tail isym2)))))
        (lambda (isym)
          (display "  { ")
          (display (enclose (tail isym) "\""))
          (display ", ")
          (display (head isym))
          (display " },")
          (newline)))
  (display "};")
  (newline)
  (newline)

  (let* ((reductions (map (parser 'reductions) tail))
         (ids (map reductions
                  (lambda (reduction)
                    (if (nil? reduction) -1
                        (index-of (reduction 'sym)
                                  (symbols-in (parser 'grammar)))))))
         (lengths (map reductions
                       (lambda (reduction)
                         (if (nil? reduction) 0 (reduction 'num))))))
         ; (chunk-size (++ (div (length ids) (++ (div (length ids) 20))))))
    (display "// indexed by state")
    (newline)
    (display "static unused i8 reduction_syms[] = {")
    (newline)

    (++ (div (length ids) 16))
    (each (chunk-every ids (chunk-size ids 20)) (lambda (chunk)
      (print-row chunk 4)
      (newline)))
    (display "};")
    (newline)
    (newline)
    (display "// indexed by state")
    (newline)
    (display "static unused u8 reduction_sizes[] = {")
    (newline)
    (each (chunk-every lengths (chunk-size ids 20)) (lambda (chunk)
      (print-row chunk 4)
      (newline)))
    (display "};")
    (newline))
  (newline)

  (display "// indexed by state, symbol")
  (newline)
  (display "static unused i16 actions[][NUM_SYMS] = {")
  (newline)
  (let ((rows (map (parser 'actions)
                   (lambda (items)
                      (map items (lambda (item) (either (tail item) -1)))))))
    (each rows (lambda (row)
      (let ((chunks (chunk-every row (chunk-size row 24))))
        (display "  {")
        (print-row (head chunks) 4)
        (each (tail chunks) (lambda (chunk)
          (newline)
          (display "   ")
          (print-row chunk 4)))
        (display "},")
        (newline))))
    (display "};")
    (newline)))

(define Node (struct '(Node symbol length children)))
(define (leaf-node token) (Node 'terminal 0 token))
(define (inter-node symbol children) (Node symbol (length children) children))

(define (print-ast ast)
  (define (print-leaf node)
    (print-token (node 'children))
    (newline))

  (define (print-node node indent)
    (display (node 'symbol))
    (newline)
    (each (node 'children) (lambda (child)
      (display (repeat ". " indent))
      (loop child indent))))

  (define (loop node indent)
    (if (zero? (node 'length))
        (print-leaf node)
        (print-node node (++ indent))))

  (loop ast 0))

(define (parse file parser read-token)
  (define (action-for state sym) (keylist-get (nth state (parser 'actions)) sym))
  (define (reduction-for state) (keylist-get (parser 'reductions) state))
  (define (peek-state stack n) (head (nth n stack)))
  (define (cur-state stack) (head (head stack)))
  (define (cur-node stack) (tail (head stack)))
  (define (push state node stack) (pair (pair state node) stack))

  (define (shift state stack token)
    (let ((node (leaf-node token)))
      (loop (push state node stack) (read-token))))

  (define (reduce-nodes stack reduction)
    (Node (reduction 'sym)
          (reduction 'num)
          (reverse (map (truncate stack (reduction 'num)) tail))))

  (define (reduce reduction stack token)
    (let ((next-state (action-for (peek-state stack (reduction 'num))
                                  (reduction 'sym))))
      (cond
        ((equal? (reduction 'sym) (top-symbol (parser 'grammar)))
          (reduce-nodes stack reduction))
        ((nil? next-state)
          (error "Did not expect" (reduction 'sym) 'from 'state (cur-state stack)))
        ((= 1 (reduction 'num))
          (let ((node (cur-node stack))
                (stack (tail stack)))
            (loop (push next-state node stack) token)))
        (else
          (let ((node (reduce-nodes stack reduction))
                (stack (tail-from stack (reduction 'num))))
            (loop (push next-state node stack) token))))))

  (define (loop stack token)
    (let ((next-state (action-for (cur-state stack) (token 'type)))
          (reduction (reduction-for (cur-state stack))))
      (cond
        ((not (nil? next-state))
          (shift next-state stack token))
        ((not (nil? reduction))
          (reduce reduction stack token))
        (else (error "No action for" (token 'type) 'in 'state (head stack))))))

  (with-input-from-file file (lambda ()
    (loop (push 0 nil '()) (read-token)))))



; Script

(let* ((grammar-file (second (command-line)))
       (output-file (either (third (command-line)) "src/parse_table.h"))
       (symbols-file (either (fourth (command-line)) "src/parse_syms.h"))
       (grammar (read-grammar grammar-file))
       (parser (Parser grammar)))
  (with-output-to-file output-file (lambda () (print-parse-tables parser)))
  (with-output-to-file symbols-file (lambda () (print-parse-syms parser)))
  (print-parse-states parser))
