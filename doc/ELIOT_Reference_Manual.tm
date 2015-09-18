<TeXmacs|1.0.7.14>

<style|article>

<\body>
  \;

  <doc-data|<doc-title|The ELIOT Programming
  Language>|<\doc-author-data|<author-name|Christophe de
  Dinechin>|<\author-address>
    Taodyne SAS, 1300 Route des Cretes, Sophia Antipolis, France
  </author-address>>
    \;
  </doc-author-data>>

  <section|Introduction>

  ELIOT is a dynamic language derived from XL and based on meta-programming,
  i.e. programs that manipulate programs. It is designed to enable a natural
  syntax similar to imperative languages descending from Algol (e.g. C,
  Pascal, Ada, Modula), while preserving the power and expressiveness of
  homoiconic languages descending from Lisp (i.e. languages where programs
  are data).<strong|> But more importantlly, XL is designed from the ground
  up to be <em|extensible>. XL lets you extend the core language to suit your
  own needs. It should be as easy to add new language constructs to XL as it
  is to add functions or classes in existing programming languages.

  <\warning>
    This document describes the language XL as it should ultimately be
    implemented. The current ELIOT implementations are currently incomplete.
    An attempt will be made to indicate in this document features that are
    not yet available with a warning. See also
    Section<nbsp><reference|state-of-implementation>.
  </warning>

  <subsection|Design objectives>

  ELIOT is intended to make it easy to write simple<strong|> programs, yet
  impose no upper limit, since you can extend the language to suit your own
  needs. The goal is to make it as easy and robust to extend the language
  with new features or concepts as it is to add new functions or classes in
  more traditional programming languages.

  This design is in response to the following observation: programmers have
  to deal with exponentially-growing program complexity. The reason is that
  the complexity of programs indirectly follows Moore's law, since users want
  to fully benefit from the capabilities of new hardware generations. But our
  brains do not follow a similar exponential law, so we need increasingly
  sophisticated tools to bridge the gap with higher and higher levels of
  abstraction.

  Over time, this lead to a never ending succession of <em|programming
  paradigms><index|programming paradigm>, each one intended to make the next
  generation of hardware accessible to programmers. For example,
  object-oriented programming was primarily fueled by the demands of
  graphical user interfaces.

  The unfortunate side effect of this continuous change in programming
  paradigms is that code designed with an old approach quickly becomes
  obsolete as a new programming model emerges. For example, even if C++ is
  nominally compatible with C, the core development model is so incompatible
  that C++ replicates core functionality of C in a completely different way
  (memory allocation, I/Os, containers, sorts, etc).

  The purpose of ELIOT is to allow the language to grow naturally over time,
  under programmers' control. ELIOT<index|Extensible language and
  runtime><index|ELIOT (eXtensible Language and Runtime> actually stands for
  ``eXtensible<\footnote>
    In prehistoric versions of XL, the X stood for ``eXperimental''
  </footnote> Language and Runtime''. The long term vision is a language made
  both more powerful and easier to use thanks to a large number of
  community-developed and field-tested language extensions.

  <subsection|Keeping it simple>

  In order to keep programs easy to write and read, the ELIOT syntax and
  semantics are very simple. As Saint-Exupery once said, perfection is
  achieved, not when there's nothing to add, but when there's nothing left to
  take away.

  And indeed, the ELIOT syntax will seem very natural to most programmers,
  except for what it's missing: ELIOT makes little use of parentheses and
  other punctuation characters. Instead, the syntax is based on indentation.
  There was a conscious design decision to keep only symbols that had an
  active role in the meaning of the program, as opposed to a purely syntactic
  role. ELIOT programs look a little like pseudo-code, except that they can
  be compiled and run.

  This simplicity translates into the internal representations of programs,
  which makes meta-programming not just possible, but easy. Any ELIOT
  program<strong|> or data can be represented with just 8 data types:
  integer<index|integer type>, real<index|real type>, text<index|text type>,
  name<index|name type>, infix<index|infix type>, prefix<index|prefix type>,
  posfix<index|postfix type> and block<index|block type>. For example, the
  internal representation for <verbatim|3 * sin X> is an infix <verbatim|*>
  node with two children, the left child of <verbatim|*> being integer
  <verbatim|3>, and the right child of <verbatim|*> being a prefix with the
  name <verbatim|sin> on the left and the name <verbatim|X> on the right.
  Therefore, these basic types define an <em|abstract syntax
  tree><index|abstract syntax tree> (AST)<index|AST (abstract syntax tree)>.

  The data structure representing programs is simple enough to make
  meta-programming practical and easy. Meta-programming is the ability for a
  program to deal with programs. In the case of ELIOT, meta-programming is
  the key to language extensibility. Instead of wishing you had this or that
  feature in the language, you can simply add it. Better yet, the process of
  extending the language is so simple that you can now consider language
  notations or compilation techniques that are useful only in a particular
  context. In short, with ELIOT, creating your own <em|domain-specific
  languages><index|domain-specific language> (DSLs<index|DSL (domain-specific
  language)>) is just part of normal, everyday programming.

  ASTs are also central to understanding how ELIOT programs are executed.
  Conceptually, an ELIOT program is a transformation of ASTs following a
  number of tree rewrite rules. In other words, there is no real difference
  in ELIOT between meta-programming and normal program execution, as both are
  represented by tranformations on ASTs. The entire ELIOT semantics boils
  down to this: incrementally rewriting abstract syntax trees.

  <subsection|Examples>

  The key characteristics of ELIOT outlined above are best illustrated with a
  few short examples, going from simple programming to more advanced
  functional-style programming<index|functional programming> to simple
  meta-programming<index|meta-programming>.

  Figure<nbsp><reference|factorial> illustrates the definition of the
  factorial function:

  <big-figure|<\verbatim>
    // Declaration of the factorial notation

    0! -\<gtr\> 1

    N! -\<gtr\> N * (N-1)!
  </verbatim>|Declaration of the factorial function><label|factorial>

  Figure<nbsp><reference|map-reduce-filter> illustrates operations usually
  known as <em|map><index|map>, <em|reduce><index|reduce> and
  <em|filter><index|filter>. These operations are characteristic of a
  programming paradigm called <em|functional programming><index|functional
  programming>, because they take functions as arguments. In ELIOT, <em|map>,
  <em|reduce> and <em|filter> operations can all use an infix <verbatim|with>
  notation with slightly different forms for the parameters.
  Section<nbsp><reference|list-operations> describes these operations in more
  details.

  <big-figure|<\verbatim>
    // Map: Computing the factorial of the first 10 integers

    // The result is 1, 1, 2, 6, 24, 120, 720, 5040, 40320, 362880

    (N-\<gtr\>N!) with 0..9

    \;

    // Reduce: Compute the sum of the first 5 factorials, i.e. 409114

    (X,Y -\<gtr\> X+Y) with (N-\<gtr\>N!) with 0..5

    \;

    // Filter: Displaying the factorials that are multiples of 3

    // The result is <verbatim|6, 24, 120, 720, 5040, 40320, 362880>

    (N when N mod 3 = 0) with (N-\<gtr\>N!) with 0..9
  </verbatim>|Map, reduce and filter><label|map-reduce-filter>

  Figure<nbsp><reference|if-then> illustrates the ELIOT <strong|>definition
  of the <verbatim|if>-<verbatim|then>-<verbatim|else>
  statement<subindex|if-then-else|statement>, which will serve as our first
  introduction to meta-programming. Here, we tell the compiler how to
  transform a particular form of the source code (the if-then-else
  statement). Note how this transformation uses the same <verbatim|-\<gtr\>>
  notation we used to declare a factorial function in
  Figure<nbsp><reference|factorial>. This shows how, in ELIOT,
  meta-programming integrates transparently with regular programming.

  <big-figure|<\verbatim>
    // Declaration of if-then-else

    if true then TrueClause else FalseClause \ \ -\<gtr\> TrueClause

    if false then TrueClause else FalseClause \ -\<gtr\> FalseClause
  </verbatim>|Declaration of if-then-else><label|if-then>

  The next sections will clarify how these operations work.

  <subsection|Concept programming><label|concept-programming>

  <index|concept><em|Concept programming> is the underlying design philosophy
  behind ELIOT. The core idea is very simple:

  <dfn|Programming is the art of transforming ideas (i.e. <em|concepts> that
  belong to <em|concept space><index|concept space>) into artifacts such as
  programs or data structure (i.e. <em|code> that belongs to <em|code
  space><index|code space>).>

  <paragraph|From concept to code: a lossy conversion>Concepts and code do
  not exist in the same context, do not obey the same rules, and are
  generally hard to compare. However, experience shows that it is generally a
  good idea to make the code look and feel as close to the concept it
  represents as possible. Unfortunately, doing so is incredibly difficult in
  practice, in large part because computers and code are limiting in their
  ability to represent arbitrary concepts.

  We are quite good at building abstractions that bridge the gap, for example
  integer data types and arithmetic that mimic mathematical integers and
  arithmetic. But then we tend to forget these are only abstractions, and get
  caught when they do not behave like the real thing, for example when an
  <verbatim|int> overflows or wraps around, something that real integers
  never do.

  The key takeaway is that the conversion of concept to code is necessarily
  <em|lossy>. Minimizing the loss remains a worthy goal, but doing so is
  difficult. By drawing our attention to the conversion process itself,
  concept programming gives us new and useful tools to solve old problems.

  <paragraph|Pseudo-metrics>Among the tools brought by concept programming is
  a set of <em|pseudo-metrics> <index|pseudo-metric>allowing us to better
  evaluate the code we create. These are called pseudo-metrics because they
  apply to things that in all fairness cannot really be measured, like the
  distance between concepts in our brains and code in the computer. At the
  same time, they are easy to understand and use, and allow us to identify
  and solve problems that are otherwise hard to pinpoint.

  Key pseudo-metrics in concept programming include:

  <\enumerate>
    <item><em|Syntactic noise><index|syntactic noise> is a discrepancy
    between the appearance of the code and the usual or desired notation for
    the associated concept. For example, the usual mathematical operation
    <math|1+2> is ideally represented in the code by <verbatim|1+2>.
    Notations such as <verbatim|(+ 1 2)> or <verbatim|add(1,2)>, by contrast,
    introduce a little bit of syntactic noise.

    <item><em|Semantic noise><index|semantic noise> is a discrepancy between
    the meaning of the code and the usual or desired notation for the
    associated concept. For example, when one needs to consider if computing
    <verbatim|X+1> possibly overflows, runs out of memory, throws an
    exception or takes an unpredictable amount of time to compute, then a
    little bit of semantic noise appears.

    <item><em|Bandwith><index|bandwidth> is the fraction of the concept space
    that is covered by a given code. The larger the bandwidth, the more
    general the code is. For example, the mathematical <em|minimum> concept
    includes the ability to compare almost anything provided there is an
    order relation (which may be total or partial); it applies to functions,
    to sets, to series, and so on. So it's fair to say that the following C
    function is very narrow band:

    <\cpp-code>
      <verbatim|int min(int x, int y) { return x\<less\>y?x:y; }>
    </cpp-code>

    <item>The <em|signal-noise ratio><index|signal-noise ratio> is the
    fraction of the code that is actually useful to solve the problem from
    concept space, as opposed to code that is there only because of
    code-space considerations. In the same <verbatim|min> example given
    above, semi-colons or curly braces have little to do with the problem at
    hand: they are noise rather than signal.
  </enumerate>

  An amusing observation about this choice of terminology is that just like
  in engineering, noise cannot ever be completely eliminated, though many
  techniques exist to reduce it; and just like in art<index|art>, what is
  noise<index|noise> to one person may be music<index|music> to another.

  <paragraph|Influence on ELIOT>ELIOT is the first programming language
  designed specifically with concept programming in mind. As a result, it is
  also the first programming language that explicitly attempts to optimize
  the pseudo-metrics listed above.

  <subsection|State of the implementation><label|state-of-implementation>

  The current implementation of the language is available as an open-source
  program, in particular at <hlink|http://xlr.sourceforge.net|http://xlr.sourceforge.net>.
  A few details of the implementation are given in
  Section<nbsp><reference|implementation-notes>.

  There are currently three wildly different implementations in one program:

  <\itemize>
    <item>An interpreted mode, corresponding to optimization level 0, which
    proved so slow that it has not been maintained in a while and is a state
    of serous disrepair.

    <item>The standard mode, which generates machine code with LLVM and runs
    acceptably fast on simple programs. This implementation is used by
    <hlink|Tao Presentations|http://www.taodyne.com>, and has consequently
    been more field tested for relatively large real-time interactive
    applications. It has, however, serious limitations with respect to the
    language.

    <item>The optimized mode, which generates much better machine code with
    LLVM (practically on a par with optimized C for simple examples) by using
    techniques such as type inference. This optimized mode was supposed to
    take over, and as a result, many language constructs where implemented
    only in that optimized mode (for example, guards). Unfortunately,
    bringing it up to par with the standard mode proved more difficult than
    planned, which means that optimized mode cannot yet support Tao
    Presentations.
  </itemize>

  The net result of this development history is that the same compiler
  supports slightly different features depending on the optimization level.
  Best effort attempts will be made to indicate which features are impacted
  with Warnings in this document.

  <\section>
    Syntax
  </section>

  ELIOT source text is encoded using UTF-8. Source code is parsed into a an
  abstract syntax tree<index|abstract syntax tree> format known as
  <em|XL0><index|XL0 (abstract syntax tree for ELIOT)>. XL0 trees consist of
  four literal node types (integer, real, text and symbol) and four
  structured node types (prefix, postfix, infix and block). Note that line
  breaks normally parse as infix operators, and that indentation normally
  parses as blocks.

  The precedence of operators is given by the <verbatim|xl.syntax>
  configuration file<index|xl.syntax>. It can also be changed dynamically in
  the source code using the <verbatim|syntax> statements. This is detailed in
  Section<nbsp><reference|precedence>. Both methods to define syntax are
  called <em|syntax configuration><index|syntax configuration>.

  The rest of this document will occasionally refer to <em|normal
  ELIOT><index|normal ELIOT> for defaults settings such as the default syntax
  configuration, as shipped with the standard ELIOT distribution.

  <subsection|Spaces and indentation>

  Spaces and tabs are generally not significant, but may be required to
  separate operator or name symbols. For example, there is no difference
  between <verbatim|A B> (one space) and <verbatim|A \ \ \ B> (four spaces),
  but both are different from <verbatim|AB> (zero space).

  Spaces and tabs are significant at the beginning of lines. ELIOT will use
  them to determine the level of indentation from which it derives program
  structures (off-side rule<index|off-side rule>), as illustrated in
  Figure<nbsp><reference|off-side-rule>. Both space<index|spaces (for
  indentation)> or tabs<index|tabs (for indentation)> can be used for
  indentation<index|indentation>, but cannot be mixed for indentation in a
  single source file. In other words, if the first indented line uses spaces,
  all other indentation must be done using spaces, and similarly for tabs.

  <big-figure|<\verbatim>
    if A \<less\> 3 then

    \ \ \ \ write "A is too small"

    else

    \ \ \ \ write "A is too big"
  </verbatim>|Off-side rule: Using indentation to mark program structure.>
  <label|off-side-rule>

  <subsection|Comments and spaces>

  <em|Comments><index|comments> are section of the source text which are
  typically used for documentation purpose and play no role in the execution
  of the program. Comments begin with a comment separator, and finish with a
  comment terminator.

  Comments in normal ELIOT are similar to C++ comments: they begin with
  <verbatim|/*> and finish with <verbatim|*/>, or they begin with
  <verbatim|//> and finish at the end of line. This is illustrated in
  Figure<nbsp><reference|comments>.

  <big-figure|<\verbatim>
    // This is a single-line comment

    /* This particular comment

    \ \ \ can be placed on multiple lines */
  </verbatim>|Single-line and multi-line comments><label|comments>

  While comments play no actual role in the execution of a normal ELIOT
  program, they are actually recorded as attachments in XL0. It is possible
  for some special code to access or otherwise use these comments. For
  example, a documentation generator can read comments and use them to
  construct documentation automatically.

  <subsection|Literals><label|literals>

  Four literal node<index|literal node types> types represent atomic values,
  i.e. values which cannot be decomposed into smaller units from an ELIOT
  point of view. They are:

  <\enumerate-numeric>
    <item>Integer constants

    <item>Real constants

    <item>Text literals

    <item>Symbols and names
  </enumerate-numeric>

  <subsubsection|Integer constants>

  Integer constants<index|integer constant><\footnote>
    At the moment, XL uses the largest native integer type on the machine
    (generally 64-bit) in its internal representations. The scanner detects
    overflow in integer constants.
  </footnote> such as <verbatim|123> consist of one or more digits
  (<verbatim|0123456789>) interpreted as unsigned radix-10 values. Note that
  <verbatim|-3> is not an integer literal but a prefix <verbatim|-> preceding
  the integer literal. The constant is defined by the longest possible
  sequence of digits.

  Integer constants can be expressed in any radix<subindex|radix|in integer
  numbers> between 2 and 36. Such constants begin with a radix-10 integer
  specifying the radix, followed by a hash sign <verbatim|#><index|hash sign
  (as a radix delimiter)>, followed by valid digits in the given radix. For
  instance, <verbatim|2#1001> represents the same integer constant as
  <verbatim|9>. If the radix is larger than 10, letters are used to represent
  digits following <verbatim|9>. For example, <verbatim|255> can be
  represented in hexadecimal as <verbatim|16#FF>.

  The underscore character<subindex|underscore|as digit separator>
  <verbatim|_> can be used to separate digits, but do not change the value
  being represented. For example <verbatim|1_000_000> is a more legible way
  to write <verbatim|1000000>, and <verbatim|16#FFFF_FFFF> is the same as
  <verbatim|16#FFFFFFFF>. Underscore characters can only separate digits,
  i.e. <verbatim|1__3>, <verbatim|_3> or <verbatim|3_> are not valid integer
  constants.

  <big-figure|<\verbatim>
    12

    1_000_000

    16#FFFF_FFFF

    2#1001_1001_1001_1001
  </verbatim>|Valid integer constants>

  <subsubsection|Real constants>

  Real constants such as <verbatim|3.14> consist of one or more digits
  (<verbatim|0123456789>), followed by a dot <verbatim|.><subindex|dot|as
  decimal separator> followed by one or more digits (<verbatim|0123456789>).
  Note that there must be at least one digit after the dot, i.e.
  <verbatim|1.> is not a valid real constant, but <verbatim|1.0> is.

  Real constants can have a radix<subindex|radix|in real numbers> and use
  underscores<subindex|underscore|as digit separator> to separate digits like
  integer constants. For example <verbatim|2#1.1> is the same as
  <verbatim|1.5> and <verbatim|3.141_592_653> is an approximation of
  <math|\<pi\>>.

  A real constant can have an exponent<index|exponent (for real constants)>,
  which consists of an optional hash sign <verbatim|#>, followed by the
  character <verbatim|e> or <verbatim|E>, followed by optional plus
  <verbatim|+> or minus <verbatim|-> sign, followed by one or more decimal
  digits <verbatim|0123456789>. For example, <verbatim|1.0e-3> is the same as
  <verbatim|0.001> and <verbatim|1.0E3> is the same as <verbatim|1000.0>. The
  exponent value is always given in radix-10, and indicates a power of the
  given radix. For example, <verbatim|2#1.0e3> represents <math|2<rsup|3>>,
  in other words it is the same as <verbatim|8.0>.

  The hash sign in the exponent is required for any radix greater than 14,
  since in that case the character <verbatim|e> or <verbatim|E> is also a
  valid digit. For instance, <verbatim|16#1.0E1> is approximately the same as
  <verbatim|1.05493>, whereas <verbatim|16#1.0#E1> is the same as
  <verbatim|16.0>.

  <\big-figure>
    <\verbatim>
      1.0

      3.1415_9265_3589_7932

      2#1.0000_0001#e-128
    </verbatim>
  </big-figure|Valid real constants>

  <subsubsection|Text literals>

  Text<index|text literals> is any valid UTF-8<index|UTF-8> sequence of
  printable or space characters surrounded by text delimiters, such as
  <verbatim|"Hello<nbsp>Möndé">. Except for line-terminating
  characters<index|line-terminating characters>, the behavior when a text
  sequence contains control characters<index|control characters> or invalid
  UTF-8 sequences is unspecified. However, implementations are encouraged to
  preserve the contents of such sequences.

  The base text delimiters<index|text delimiters> are the single
  quote<index|quote><index|single quote> <verbatim|'> and the double quote
  <verbatim|"><index|double quote>. They can be used to enclose any text that
  doesn't contain a line-terminating character. The same delimiter must be
  used at the beginning and at the end of the text. For example,
  <verbatim|"Shouldn't break"> is a valid text surrounded by double quotes,
  and <verbatim|'He said "Hi"'> is a valid text surrounded by single quotes.

  In text surrounded by base delimiters, the delimiter can be inserted by
  doubling it. For instance, except for the delimiter, <verbatim|'Shouldn''t
  break'> and <verbatim|"He said ""Hi"""> are equivalent to the two previous
  examples.

  Other text delimiters can be specified, which can be used to delimit text
  that may include line breaks. Such text is called <em|long text><index|long
  text>. With the default configuration, long text can be delimited with
  <verbatim|\<less\>\<less\>> and <verbatim|\<gtr\>\<gtr\>>.

  \;

  <big-figure|<\verbatim>
    "Hello World"

    'Où Toto élabora ce plan çi'

    \<less\>\<less\>This text spans

    multiple lines\<gtr\>\<gtr\>
  </verbatim>|Valid text constants>

  When long text contains multiple lines of text,
  indentation<index|indentation (in long text)> is ignored up to the
  indentation level of the first character in the long text.
  Figure<nbsp><reference|long-text-indent> illustrates how long text indent
  is eliminated from the text being read.

  <\big-figure>
    <block*|<tformat|<table|<row|<cell|<strong|Source
    code>>|<cell|<strong|Resulting text>>>|<row|<cell| <\verbatim>
      \<less\>\<less\> Long text can

      \ \ \ contain indentation

      \ or not,

      \ \ \ \ \ it's up to you\<gtr\>\<gtr\>
    </verbatim>>|<cell| <\verbatim>
      Long text can

      contain indentation

      or not,

      \ \ it's up to you
    </verbatim>>>>>>

    \;
  </big-figure|Long text and indentation><label|long-text-indent>

  The text delimiters are not part of the value of text literals<index|value
  (of text literals)>. Therefore, text delimiters are ignored when comparing
  texts.

  <\warning>
    The current implementation may not treat indentation of long text as
    indicated above.
  </warning>

  <subsubsection|Name and operator symbols<index|symbols>>

  Names<index|name> begin with an alphabetic character
  <verbatim|A>..<verbatim|Z> or <verbatim|a>..<verbatim|z> or any non-ASCII
  UTF-8 character, followed by the longuest possible sequence of alphabetic
  characters, digits or underscores. Two consecutive underscore characters
  are not allowed. Thus, <verbatim|Marylin_Monroe>, <verbatim|élaböràtion> or
  <verbatim|j1> are valid ELIOT names, whereas <verbatim|A-1>, <verbatim|1cm>
  or <verbatim|A__2> are not.

  Operator symbols, or <em|operators><index|operator symbols>, begin with an
  ASCII punctuation character<\footnote>
    Non-ASCII punctuation characters or digits are considered as alphabetic.
  </footnote> which does not act as a special delimiter for text, comments or
  blocks. For example, <verbatim|+> or <verbatim|-\<gtr\>> are operator
  symbols. An operator includes more than one punctuation character only if
  it has been declared in the syntax (typically in the syntax configuration
  file). For example, unless the symbol <verbatim|%,> (percent character
  followed by comma character) has been declared in the syntax,
  <verbatim|3%,4%> will contain two operator symbols <verbatim|%> and
  <strong|,> instead of a single <verbatim|%,> operator.

  A special name, the empty name, exists only as a child of empty blocks such
  as <verbatim|()>.

  After parsing, operator and name symbols are treated identically. During
  parsing, they are treated identically except in the <em|expression versus
  statement rule><index|expression vs. statement> explained in
  Section<nbsp><reference|expression-vs-statement-section>.

  <\big-figure>
    <\verbatim>
      x

      <\verbatim>
        X12_after_transformation

        <math|\<alpha\>>_times_<math|\<pi\>>

        +

        --\<gtr\>

        \<less\>\<less\>\<less\>\<gtr\>\<gtr\>\<gtr\>
      </verbatim>
    </verbatim>
  </big-figure|Examples of valid operator and name symbols>

  <subsection|Structured nodes>

  Four structured node types<index|structured node types> represent
  combinations of nodes. They are:

  <\enumerate>
    <item>Infix nodes<index|infix>, representing operations such as
    <verbatim|A+B> or <verbatim|A and B>, where the operator is between its
    two operands.

    <item>Prefix nodes<index|prefix>, representing operations such as
    <verbatim|+3> or <verbatim|sin x>, where the operator is before its
    operand.

    <item>Postfix nodes<index|postfix>, representing operations such as
    <verbatim|3%> or <verbatim|3 cm>, where the operator is after its
    operand.

    <item>Blocks<index|block>, representing grouping such as <verbatim|(A+B)>
    or <verbatim|{lathe;rinse;repeat}>, where the operators surround their
    operand.
  </enumerate>

  Infix, prefix and postfix nodes have two children nodes<index|child node>.
  Blocks have a single child node.

  <subsubsection|Infix nodes>

  An infix node<index|infix> has two children, one on the left, one on the
  right, separated by a name or operator symbol.

  Infix nodes are used to separate statements with semi-colons <verbatim|;>
  or line breaks (referred to as <verbatim|NEWLINE> in the syntax
  configuration).

  <subsubsection|Prefix and postfix nodes>

  Prefix<index|prefix> and postfix<index|postfix> nodes have two children,
  one on the left, one on the right, without any separator between them. The
  only difference between prefix and postfix nodes is in what is considered
  the ``operation'' and what is considered the ``operand'<index|operand (in
  prefix and postfix)>'. For a prefix node, the operation is on the left and
  the operand on the right, whereas for a postfix node, the operation is on
  the right and the operand on the left.

  Prefix nodes are used for functions. The default for a name or operator
  symbol that is not explicitly declared in the
  <verbatim|xl.syntax><index|xl.syntax> file or configured is to be treated
  as a prefix function, i.e. to be given a common function
  precedence<index|function precedence> referred to as <verbatim|FUNCTION> in
  the syntax configuration. For example, <verbatim|sin> in the expression
  <verbatim|sin x> is treated as a function<index|function>.

  <subsubsection|Block nodes>

  Block<index|block> nodes have one child bracketed by two
  delimiters<index|block delimiters>.

  Normal ELIOT recognizes the following pairs as block delimiters:

  <\itemize>
    <item>Parentheses, as in <verbatim|(A)>

    <item>Brackets, as in <verbatim|[A]>

    <item>Curly braces, as in <verbatim|{A}>

    <item>Indentation, as shown surrounding the <verbatim|write> statements
    in Figure<nbsp><reference|off-side-rule>. The delimiters for indentation
    are referred to as <verbatim|INDENT> and <verbatim|UNINDENT> in the
    syntax configuration.
  </itemize>

  <subsection|Parsing rules>

  <index|parsing>The ELIOT parser only needs a small number of rules to parse
  any ELIOT source code into XL0:

  <\enumerate>
    <item>Precedence<index|precedence>

    <item>Associativity<index|associativity>

    <item>Infix versus prefix versus postfix<index|infix vs. prefix vs.
    postfix>

    <item>Expression versus statement<index|expression vs. statement>
  </enumerate>

  These rules are detailed below.

  <subsubsection|Precedence>

  <index|precedence>Infix, prefix, postfix and block symbols are ranked
  according to their <em|precedence>, represented as a non-negative integer.
  The precedence is specified by the syntax configuration, either in the
  syntax configuration file, <verbatim|xl.syntax><index|xl.syntax>, or
  through <verbatim|syntax> statements<index|syntax statement> in the source
  code. This is detailed in Section<nbsp><reference|precedence>.

  Symbols with higher precedence associate before symbols with lower
  precedence. For instance, if the symbol <verbatim|*> has infix precedence
  value <verbatim|300> and symbol <verbatim|+> has infix precedence value
  <verbatim|290>, then the expression <verbatim|2+3*5> will parse as an infix
  <verbatim|+> whose right child is an infix <verbatim|*>.

  The same symbol may receive a different precedence as an infix, as a prefix
  and as a postfix operator. For example, if the precedence of <verbatim|->
  as an infix is <verbatim|290> and the precedence of <verbatim|-> as a
  prefix is <verbatim|390>, then the expression <verbatim|3 - -5> will parse
  as an infix <verbatim|-> with a prefix <verbatim|-> as a right child.

  The precedence associated to blocks is used to define the precedence of the
  resulting expression. This precedence given to entire expressions is used
  primarily in the <em|expression versus statement> rule described in
  Section<nbsp><reference|expression-vs-statement-section>.

  <subsubsection|Associativity>

  <index|associativity>Infix operators can associate to their left or to
  their right.

  The addition operator is traditionally left-associative, meaning that in
  <verbatim|A+B+C>, <verbatim|A> and <verbatim|B> associate before
  <verbatim|C>. As a result, the outer infix <verbatim|+> node in
  <verbatim|A+B+C> has an infix <verbatim|+> node as its left child, with
  <verbatim|A> and <verbatim|B> as children, and <verbatim|C> as its right
  child.

  Conversely, the semi-colon in ELIOT is right-associative, meaning that
  <verbatim|A;B;C> is an infix node with an infix as the right child and
  <verbatim|A> as the left child.

  Operators with left and right associativity cannot have the same
  precedence, as this would lead to ambiguity. To enforce that rule, ELIOT
  arbitrarily gives an even precedence to left-associative operators, and an
  odd precedence to right-associative operators. For example, the precedence
  of <verbatim|+> in the default configuration is <verbatim|290>
  (left-associative), whereas the precedence of <verbatim|^> is
  <verbatim|395> (right-associative).

  <subsubsection|Infix versus Prefix versus Postfix>

  <index|infix vs. prefix vs. postfix>During parsing<index|parsing>, ELIOT
  needs to resolve ambiguities<index|parsing ambiguities> between infix and
  prefix symbols. For example, in <verbatim|-A + B>, the minus sign
  <verbatim|-> is a prefix, whereas the plus sign <verbatim|+> is an infix.
  Similarly, in <verbatim|A and not B>, the <verbatim|and> word is infix,
  whereas the <verbatim|not> word is prefix. The problem is therefore exactly
  similar for names and operator symbols.

  ELIOT resolves this ambiguity as follows<\footnote>
    All the examples given are in normal XL, i.e. based on the default
    <verbatim|xl.syntax> configuration file.
  </footnote>:

  <\itemize>
    <item>The first symbol in a statement or in a block is a prefix:
    <verbatim|and> in <verbatim|(and x)> is a prefix.

    <item>A symbol on the right of an infix symbol is a prefix:
    <verbatim|and> in <verbatim|A+and B> is a prefix.

    <item>Otherwise, if the symbol has an infix precedence but no prefix
    precedence, then it is interpreted as an infix: <verbatim|and> in
    <verbatim|A and B> is an infix.

    <item>If the symbol has both an infix precedence and a prefix precedence,
    and either a space following it, or no space preceding it, then it is an
    infix: the minus sign <verbatim|-> in <verbatim|A - B> is an infix, but
    the same character is a prefix in <verbatim|A -B>.

    <item>Otherwise, if the symbol has a postfix precedence, then it is a
    postfix: <verbatim|%> in <verbatim|3%> is a postfix.

    <item>Otherwise, the symbol is a prefix: <verbatim|sin> in
    <verbatim|write sin x> is a prefix.
  </itemize>

  In the first, second and last case, a symbol may be identified as a prefix
  without being given an explicit precedence. Such symbols are called
  <em|default prefix><index|default prefix (precedence)>. They receive a
  particular precedence known as <em|function precedence><index|function
  precedence>, identified by <verbatim|FUNCTION> in the syntax
  configuration<index|syntax configuration>.

  <subsubsection|Expression versus statement><label|expression-vs-statement-section>

  <index|expression vs. statement>Another ambiguity<index|parsing
  ambiguities> is related to the way humans read text. In <verbatim|write sin
  x, sin y>, most humans will read this as a <verbatim|write> instruction
  taking two arguments. This is however not entirely logical: if
  <verbatim|write> takes two arguments, then why shouldn't <verbatim|sin>
  also take two arguments? In other words, why should this example parse as
  <verbatim|write(sin(x),sin(y))> and not as <verbatim|write(sin(x,sin(y)))>?

  The reason is that we tend to make a distinction between
  <em|statements><index|statement> and <em|expressions><index|expression (as
  opposed to statement)>. This is not a distinction that is very relevant to
  computers, but one that exists in most natural languages, which distinguish
  whole sentences as opposed to subject or complement<index|subject and
  complement>.

  ELIOT resolves the ambiguity by implementing a similar distinction. The
  boundary is a particular infix precedence, called <em|statement
  precedence><index|statement precedence>, denoted as <verbatim|STATEMENT> in
  the syntax configuration. Intuitively, infix operators with a lower
  precedence separate statements, whereas infix operators with a higher
  precedence separate expressions. For example, the semi-colon <verbatim|;>
  or <verbatim|else> separate statements, whereas <verbatim|+> or
  <verbatim|and> separate expressions.

  More precisely:

  <\itemize>
    <item>If a block's precedence is less than statement precedence, its
    content begins as an expression, otherwise it begins as a statement:
    <verbatim|3> in <verbatim|(3)> is an expression, <verbatim|write> in
    <verbatim|{write}> is a statement.

    <item>Right after an infix symbol with a precedence lower than statement
    precedence, we are in a statement, otherwise we are in an expression. The
    name <verbatim|B> in <verbatim|A+B> is an expression, but it is a
    statement in <verbatim|A;B>.

    <item>A similar rule applies after prefix nodes: <verbatim|{optimize}
    write A,B> gives two arguments to <verbatim|write>, whereas in
    <verbatim|(x-\<gtr\>x+1) sin x,y> the <verbatim|sin> function only
    receives a single argument.

    <item>A default prefix begins a statement if it's a name, an expression
    if it's a symbol: the name <verbatim|write> in <verbatim|write X> begins
    a statement, the symbol <verbatim|+> in <verbatim|+3> begins an
    expression.
  </itemize>

  In practice, there is no need to worry too much about these rules, since
  normal ELIOT ensures that most text parses as one would expect from daily
  use of English or mathematical notations.

  <subsection|Syntax configuration><label|precedence>

  <index|syntax configuration>The default ELIOT syntax configuration file,
  named <verbatim|xl.syntax><index|xl.syntax>, looks like
  Figure<nbsp><reference|syntax-file> and specifies the standard
  operators<index|operators><index|standard operators> and their
  precedence<index|precedence>.

  <\big-figure>
    <\verbatim>
      INFIX

      \ \ \ \ \ \ \ \ 11 \ \ \ \ \ NEWLINE

      \ \ \ \ \ \ \ \ 21 \ \ \ \ \ -\<gtr\> =\<gtr\>

      \ \ \ \ \ \ \ \ 25 \ \ \ \ \ as

      \ \ \ \ \ \ \ \ 31 \ \ \ \ \ else into

      \ \ \ \ \ \ \ \ 40 \ \ \ \ \ loop while until

      \ \ \ \ \ \ \ \ 50 \ \ \ \ \ then require ensure

      \ \ \ \ \ \ \ \ 61 \ \ \ \ \ ;

      \ \ \ \ \ \ \ \ 75 \ \ \ \ \ with

      \ \ \ \ \ \ \ \ 85 \ \ \ \ \ := += -= *= /= ^= \|= &=

      \ \ \ \ \ \ \ \ 100 \ \ \ \ STATEMENT

      \ \ \ \ \ \ \ \ 110 \ \ \ \ is\ 

      \ \ \ \ \ \ \ \ 120 \ \ \ \ written

      \ \ \ \ \ \ \ \ 130 \ \ \ \ where

      \ \ \ \ \ \ \ \ 200 \ \ \ \ DEFAULT

      \ \ \ \ \ \ \ \ 211 \ \ \ \ when

      \ \ \ \ \ \ \ \ 231 \ \ \ \ ,

      \ \ \ \ \ \ \ \ 240 \ \ \ \ return\ 

      \ \ \ \ \ \ \ \ 250 \ \ \ \ and or xor

      \ \ \ \ \ \ \ \ 260 \ \ \ \ in at contains

      \ \ \ \ \ \ \ \ 271 \ \ \ \ of to

      \ \ \ \ \ \ \ \ 280 \ \ \ \ .. by

      \ \ \ \ \ \ \ \ 290 \ \ \ \ = \<less\> \<gtr\> \<less\>= \<gtr\>=
      \<less\>\<gtr\>

      \ \ \ \ \ \ \ \ 300 \ \ \ \ & \|

      \ \ \ \ \ \ \ \ 310 \ \ \ \ + -

      \ \ \ \ \ \ \ \ 320 \ \ \ \ * / mod rem

      \ \ \ \ \ \ \ \ 381 \ \ \ \ ^

      \ \ \ \ \ \ \ \ 500 \ \ \ \ .

      \ \ \ \ \ \ \ \ 600 \ \ \ \ :

      \;

      PREFIX

      \ \ \ \ \ \ \ \ 30 \ \ \ \ \ data

      \ \ \ \ \ \ \ \ 40 \ \ \ \ \ loop while until

      \ \ \ \ \ \ \ \ 50 \ \ \ \ \ property constraint

      \ \ \ \ \ \ \ \ 121 \ \ \ \ case if return yield transform

      \ \ \ \ \ \ \ \ 350 \ \ \ \ not in out constant variable const var

      \ \ \ \ \ \ \ \ 360 \ \ \ \ ! ~

      \ \ \ \ \ \ \ \ 370 \ \ \ \ - + * /

      \ \ \ \ \ \ \ \ 401 \ \ \ \ FUNCTION

      \ \ \ \ \ \ \ \ 410 \ \ \ \ function procedure to type iterator

      \ \ \ \ \ \ \ \ 420 \ \ \ \ ++ --

      \ \ \ \ \ \ \ \ 430 \ \ \ \ &

      \;

      POSTFIX

      \ \ \ \ \ \ \ \ 400 \ \ \ \ ! ? % cm inch mm pt px

      \ \ \ \ \ \ \ \ 420 \ \ \ \ ++ --

      \;

      BLOCK

      \ \ \ \ \ \ \ \ 5 \ \ \ \ \ \ INDENT UNINDENT

      \ \ \ \ \ \ \ \ 25 \ \ \ \ \ '{' '}'

      \ \ \ \ \ \ \ \ 500 \ \ \ \ '(' ')' '[' ']'

      \;

      TEXT

      \ \ \ \ \ \ \ \ "\<less\>\<less\>" "\<gtr\>\<gtr\>"

      \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ 

      COMMENT

      \ \ \ \ \ \ \ \ "//" NEWLINE

      \ \ \ \ \ \ \ \ "/*" "*/"

      \;

      SYNTAX "C"

      \ \ \ \ \ \ \ \ extern ;
    </verbatim>
  </big-figure|Default syntax configuration file><label|syntax-file>

  Syntax information can also be provided in the source code using the
  <verbatim|syntax> name<index|syntax statement> followed by a block, as
  illustrated in Figure<nbsp><reference|source-syntax>.

  <big-figure|<\verbatim>
    // Declare infix 'weight' operator

    syntax (INFIX 350 weight)

    Obj weight W -\<gtr\> Obj = W

    \;

    // Declare postfix 'apples' and 'kg'

    syntax

    \ \ \ \ POSTFIX 390 apples kg

    X kg -\<gtr\> X * 1000

    N apples -\<gtr\> N * 0.250 kg

    \;

    // Combine the notations declared above

    if 6 apples weight 1.5 kg then

    \ \ \ \ write "Success!"
  </verbatim>|Use of the <verbatim|syntax> specification in a source
  file><label|source-syntax>

  As a general stylistic rule, it is recommended to use restraint when
  introducing new operators using <verbatim|syntax> statements<index|syntax
  statement>, as this can easily confuse a reader who is not familiar with
  the new notation. On the other hand, there are cases where good use of new
  and well-chosen operators will render the code much more readable and easy
  to maintain.

  <paragraph|Format of syntax configuration>Spaces and
  indentation<index|indentation> are not significant in a syntax
  configuration file. Lexical elements are identical to those of ELIOT, as
  detailed in Section<nbsp><reference|literals>. The significant elements are
  integer constants, names, symbols and text. Integer constants are
  interpreted as the precedence of names and symbols that follow them. Name
  and symbols can be given either with lexical names and symbols, or with
  text.

  A few names are reserved for use as keywords in the syntax configuration
  file:

  <\itemize>
    <item><verbatim|INFIX> begins a section declaring infix symbols and
    precedence. In this section:

    <\itemize>
      <item><verbatim|NEWLINE> identifies line break characters in the source
      code

      <item><verbatim|STATEMENT> identifies the precedence of
      statements<index|statement precedence>

      <item><verbatim|DEFAULT> identifies the precedence for symbols not
      otherwise given a precedence. This precedence should be unique in the
      syntax confguration, i.e. no other symbol should be given the
      <verbatim|DEFAULT> precedence.<index|default precedence>
    </itemize>

    <item><verbatim|PREFIX> begins a section declaring prefix symbols and
    precedence. In this section:

    <\itemize>
      <item><verbatim|FUNCTION >identifies the precedence for default prefix
      symbols, i.e. symbols identified as prefix that are not otherwise given
      a precedence. This precedence should be unique, i.e. no other symbol
      shoud be given the <verbatim|FUNCTION> precedence.<index|function
      precedence>
    </itemize>

    <item><verbatim|POSTFIX> begins a section declaring postfix symbols and
    precedence.

    <item><verbatim|BLOCK> begins a section declaring block
    delimiters<index|block delimiters> and precedence. In this section:

    <\itemize>
      <item><verbatim|INDENT> and <verbatim|UNINDENT> are used to mark
      indentation and unindentation.<index|indentation>
    </itemize>

    <item><verbatim|TEXT> begins a section declaring delimiters for long
    text.<index|text delimiters>

    <item><verbatim|COMMENT> begins a section declaring delimiters for
    comments. In this section:

    <\itemize>
      <item><verbatim|NEWLINE> identifies line breaks
    </itemize>

    <item><verbatim|SYNTAX> begins a section declaring external syntax
    files<index|external syntax file>. In normal ELIOT, a file
    <verbatim|C.syntax><index|C.syntax file> is used to define the
    precedences for any text between <verbatim|extern> and <verbatim|;>
    symbols. This is used to import C symbols<index|C symbols> using an
    approximation of the syntax of the C language, as described in
    Section<nbsp><reference|C-library>. The <verbatim|C.syntax> configuration
    file is shown in Figure<nbsp><reference|C-syntax-file>.
  </itemize>

  <\big-figure>
    <\verbatim>
      INFIX

      \ \ \ \ \ \ \ \ 41 \ \ \ \ ,

      \;

      PREFIX

      \ \ \ \ \ \ \ \ 30 \ \ \ \ \ extern ...

      \ \ \ \ \ \ \ \ 400 \ \ \ \ FUNCTION

      \ \ \ \ \ \ \ \ 450 \ \ \ \ short long unsigned signed

      \;

      POSTFIX

      \ \ \ \ \ \ \ \ 100 \ \ \ \ *

      \;

      BLOCK

      \ \ \ \ \ \ \ \ 500 \ \ \ \ '(' ')' '[' ']'

      \;

      COMMENT

      \ \ \ \ \ \ \ \ "//" NEWLINE

      \ \ \ \ \ \ \ \ "/*" "*/"
    </verbatim>
  </big-figure|C syntax configuration file><label|C-syntax-file>

  <section|Language semantics>

  <index|semantics>The semantics of ELIOT is based entirely on the rewrite of
  XL0 abstract syntax trees. Tree rewrite operations define the
  execution<index|execution (of programs)> of ELIOT programs, also called
  <em|evaluation><index|evaluation>.

  <\subsection>
    Tree rewrite operators
  </subsection>

  <label|tree-rewrite-operators><index|tree rewrite>There is a very small set
  of tree rewrite operators<index|tree rewrite operators> that are given
  special meaning in ELIOT and treated specially by the ELIOT compiler:

  <\itemize>
    <item><em|Rewrite declarations><index|rewrite declarations> are used to
    declare operations. They roughly play the role of functions, operator or
    macro declarations in other programming languages. A rewrite declaration
    takes the general form <verbatim|Pattern-\<gtr\>Implementation><index|pattern><index|implementation>
    and indicates that any tree matching <verbatim|Pattern> should be
    rewritten as <verbatim|Implementation>.

    <\big-figure>
      <\verbatim>
        0! -\<gtr\> 1

        N! -\<gtr\> N*(N-1)!

        3! // Computes 6
      </verbatim>
    </big-figure|Example of rewrite declaration>

    <item><em|Data declarations><index|data declarations> identify data
    structures in the program. Data structures are nothing more than trees
    that need no further rewrite. A data declaration takes the general form
    of <verbatim|data Pattern>. Any tree matching <verbatim|Pattern> will not
    be rewritten further.

    <big-figure|<\verbatim>
      data complex(x, y)

      complex(3,5) // Will stay as is
    </verbatim>|Example of data declaration>

    <item><em|Type declarations<index|type declarations>> define the type of
    variables. Type declarations take the general form of an infix colon
    operator <verbatim|Name:Type>, with the name of the variable on the left,
    and the type of the variable on the right.

    <big-figure|<\verbatim>
      data person

      \ \ \ \ first:text

      \ \ \ \ last:text

      \ \ \ \ age:integer

      person

      \ \ \ \ "John"

      \ \ \ \ "Smith"

      \ \ \ \ 33
    </verbatim>|Example of data declarations containing type declarations>

    <item><em|Guards><index|guard (in a rewrite declaration)> limit the
    validity of rewrite or data declarations. They use an infix
    <verbatim|when> with a boolean expression on the right of
    <verbatim|when>, i.e. a form like <verbatim|Declaration when Condition>.

    <big-figure|<\verbatim>
      syracuse X:integer when X mod 2 = 0 -\<gtr\> X/2

      syracuse X:integer \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ -\<gtr\> 3*X+1
    </verbatim>|Example of guard to build the Syracuse suite>

    <item><em|Assignment><index|assignment> change the value associated to a
    binding<index|binding>. Assignments take the form <verbatim|Reference :=
    Value>, where <verbatim|Reference> identifies the binding to change.

    <big-figure|<verbatim|Zero := 0>|Example of assignment>

    <item><em|Sequence operators><index|sequence><index|sequence operator>
    indicate the order in which computations must be
    performed<index|evaluation order>. ELIOT has two infix sequence
    operators, the semi-colon <verbatim|;> and the new-line
    <verbatim|NEWLINE>.

    <big-figure|<\verbatim>
      write "Hello"; writeln " World"

      emit_loud_beep
    </verbatim>|Example of sequence>

    <item><em|Index operators><index|index operator> perform particular kinds
    of tree rewrites similar in usage to ``structures'' or ``arrays'' in
    other programming languages. The notations <verbatim|Reference.Field> and
    <verbatim|Reference[Index]> are used to refer to individual elements in a
    data structure. These are only convenience notations for specific kinds
    of tree rewrites, see Section<nbsp><reference|index-operators>.

    <big-figure|<\verbatim>
      A[3] := 5

      A.ref_count := A.ref_count + 1
    </verbatim>|Examples of index operators>
  </itemize>

  <\warning>
    The current implentation has a number of deficiencies with respect to
    guards, assignment and index operators. These are described in the
    corresponding sections below.
  </warning>

  <subsubsection|Rewrite declarations>

  <subindex|declaration|of rewrites><index|rewrite declaration>The infix
  <verbatim|-\<gtr\>> operator declares a tree rewrite.
  Figure<nbsp><reference|if-then-else> repeats the code in
  Figure<nbsp><reference|if-then> illustrating how rewrite declarations can
  be used to define the traditional <verbatim|if>-<verbatim|then>-<verbatim|else>
  statement<subindex|if-then-else|statement>.

  <big-figure|<\verbatim>
    if true then TrueClause else FalseClause \ \ \ -\<gtr\> TrueClause

    if false then TrueClause else FalseClause \ \ -\<gtr\> FalseClause
  </verbatim>|Examples of tree rewrites><label|if-then-else>

  The tree on the left of the <verbatim|-\<gtr\>> operator is called the
  <em|pattern><index|pattern>. The tree on the right is called the
  <em|implementation> of the pattern. The rewrite declaration indicates that
  a tree that matches the pattern should be rewritten using the
  implementation.

  The pattern contains <em|constant><index|constant> and
  <em|variable><index|variable> symbols and names:

  <\itemize>
    <item>Infix symbols and names are constant, like <verbatim|+> in
    <verbatim|A+B>.

    <item>Block-delimiting symbols and names are constant, like <verbatim|[>
    and <verbatim|]> in <verbatim|[A]>.

    <item>A name on the left of a prefix is a constant, like <verbatim|sin>
    in <verbatim|sin X>.

    <item>A name on the right of a postfix is a constant, like <verbatim|cm>
    in <verbatim|X cm>.

    <item>A name alone on the left of a rewrite is a constant, like
    <verbatim|X> in <verbatim|X-\<gtr\>0>.

    <item>Operators are constant, like <verbatim|+> in <verbatim|X and +Y>.

    <item>All other names are variable.
  </itemize>

  Figure<nbsp><reference|if-then-else-colorized> highlight in blue italic all
  variable symbols in the declarations of
  Figure<nbsp><reference|if-then-else>.

  <big-figure|<\verbatim>
    if <with|color|blue|<em|true>> then <with|color|blue|<em|TrueClause>>
    else <with|color|blue|<em|FalseClause>> \ \ \ -\<gtr\> TrueClause

    if <em|<with|color|blue|false>> then <em|<with|color|blue|TrueClause>>
    else <em|<with|color|blue|FalseClause>> \ \ -\<gtr\> FalseClause
  </verbatim>|Constants vs. Variable symbols><label|if-then-else-colorized>

  Constant symbol and names<index|constant symbols> form the structure of the
  pattern, whereas variable names form the parts of the pattern which can
  match other trees. The names are called <em|parameters><index|parameter>
  and the tree they match are called <em|arguments><index|argument>.

  For example, to match the pattern in Figure<nbsp><reference|if-then-else>,
  the <verbatim|if>, <verbatim|then> and <verbatim|else> words must match
  exactly, but <verbatim|TrueClause> may match any tree, like for example
  <verbatim|write "Hello">. <verbatim|TrueClause> is a parameter, and
  <verbatim|write "Hello"> would be the matching argument.

  Note that there is a special case for a name as the pattern of a rewrite. A
  rewrite like <verbatim|X-\<gtr\>0> binds <verbatim|X> to value
  <verbatim|0>, i.e. <verbatim|X> is a constant that must match in the tree
  being evaluated.

  It is however possible to create a rewrite with a variable on the left by
  using a type declaration. For example, the rewrite
  <verbatim|X:real-\<gtr\>X+1> does not declare the variable <verbatim|X>,
  but an <em|anonymous function><index|anonymous function><\footnote>
    In functional programming, these are often called <em|lambda
    functions><index|lambda function>.
  </footnote> that increments its input. Such rewrites are somewhat special,
  in particular because they are not visible to their implementation so as to
  avoid infinite recursion if their return type is identical to their input
  type.

  An expression may use declarations that follow it in the same sequence.
  Declarations are visible to prior elements in the sequence and need not be
  evaluated, as shown in Figure<nbsp><reference|out-of-order-declarations>,
  which computes <verbatim|4>. More generally, rewrites in a sequence belong
  to the context<index|context> for the entire sequence (contexts are defined
  in Section<nbsp><reference|binding>).

  <strong|>

  <big-figure|<\verbatim>
    foo 3

    foo N -\<gtr\> N + 1
  </verbatim>|Declarations are visible to the entire sequence containing
  them><label|out-of-order-declarations>

  <subsubsection|Data declaration>

  <subindex|declaration|of data><index|data declaration>The <verbatim|data>
  prefix declares tree structures that need not be rewritten further. For
  instance, Figure<nbsp><reference|comma-separated-list-data-declaration>
  declares that <verbatim|1,3,4> should not be evaluated further, because it
  is made of infix <verbatim|,> trees which are declared as <verbatim|data>.

  <\big-figure>
    <\verbatim>
      data a,b
    </verbatim>
  </big-figure|Declaring a comma-separated
  list><label|comma-separated-list-data-declaration>

  The tree following a data declaration is a pattern, with constant and
  variable symbols like for rewrite declarations. Data declarations only
  limit the rewrite of the tree specified by the pattern, but not the
  evaluation of pattern variables<subindex|evaluation|data declaration
  arguments>. In other words, pattern variables are evaluated normally, as
  specified in Section<nbsp><reference|evaluation>.

  For instance, in Figure<nbsp><reference|complex-type>, the names
  <verbatim|x> and <verbatim|y> are variable, but the name <verbatim|complex>
  is constant because it is a prefix. Using integer addition as defined in
  normal ELIOT, <verbatim|complex(3+4, 5+6)> will evaluate as
  <verbatim|complex(7,11)> but no further<\footnote>
    Evaluation is caused by the need to check the parameter types, i.e.
    verify that <verbatim|3+4> is actually an <verbatim|integer>.
  </footnote>.

  <big-figure|<verbatim|data complex(x:integer,y:integer)>|Declaring a
  <verbatim|complex> data type><label|complex-type>

  \ The declaration in Figure<nbsp><reference|complex-type> can be
  interpreted as declaring a <verbatim|complex> data type. There is, however,
  a better way to describe data types in ELIOT, which is detailed in
  Section<nbsp><reference|type-definition>.

  The word <verbatim|self><index|self> can be used to build data forms:
  <verbatim|data X> is equivalent to <verbatim|X-\<gtr\>self>.

  <\warning>
    The <verbatim|self> keyword is not implemented yet.
  </warning>

  <subsubsection|Type declaration>

  <subindex|declaration|of types>An <em|type declaration><index|type
  declaration> is an infix colon <verbatim|:> operator in a rewrite or data
  pattern with a name on the left and a type on the right. It indicates that
  the named parameter on the left has the type indicated on the right. A
  <em|return type declaration><index|return type declaration> is an infix
  <verbatim|as> in a rewrite pattern with a pattern on the left and a type on
  the right. It specifies the value that will be returned by the
  implementation of the rewrite. Section<nbsp><reference|types> explains how
  types are defined.

  Figure<nbsp><reference|type-declaration> shows examples of type
  declarations. To match the pattern for <verbatim|polynom>, the arguments
  corresponding to parameters <verbatim|X> and <verbatim|Z> must be
  <verbatim|real>, whereas the argument corresponding to parameter
  <verbatim|N> must be <verbatim|integer>. The value returned by
  <verbatim|polynom> will belong to <verbatim|real>.

  <big-figure|<\verbatim>
    <\verbatim>
      polynom X:real, Z:real, N:integer as real -\<gtr\> (X-Z)^N
    </verbatim>
  </verbatim>|Simple type declarations><label|type-declaration>

  The type declarations filter which rewrites can be selected to evaluate a
  particular tree. This enables <em|overloading><index|overloading>, i.e. the
  ability to have multiple functions or operators with a similar structure,
  but different types for the parameters. Return type declarations, on the
  other hand, plays no role in the selection of candidates<\footnote>
    Ada is one of the few programming languages that have overloading based
    on return types.
  </footnote>. If there is a return type declaration and the implementation
  does not actually return the declared return type, a type error expression
  of the form <verbatim|type_error ExpectedType, ActualValue> will attempt to
  correct the problem.

  A type declaration can also be placed on the left of an assignment, see
  Section<nbsp><reference|assignment>.<subindex|type declaration|in
  assignment><subindex|assignment|to type declaration>

  <subsubsection|Assignment><label|assignment>

  The assignment<index|assignment> operator <verbatim|:=><index|:=> binds the
  reference on its left to the value of the tree on its right. The tree on
  the right is evaluated prior to the assignment<subindex|evaluation|in
  assignment>.

  An assignment is valid even if the reference on the left of <verbatim|:=>
  had not previously been bound. In that case, it creates a new
  binding<subindex|binding|in assignment> in the current context. This is
  shown in Figure<nbsp><reference|creating-a-new-binding>.

  <\big-figure>
    <\verbatim>
      // Assigns to locally created X

      assigns_to_local -\<gtr\> X := 1
    </verbatim>
  </big-figure|Creating a new binding<label|creating-a-new-binding>>

  On the other hand, if there is an existing binding, the assignment replaces
  the corresponding bound value. This is shown in
  Figure<nbsp><reference|nonlocal-assignment>:

  <\big-figure>
    <\verbatim>
      // Assigns to global X defined below

      assigns_to_global -\<gtr\> X := 1

      X -\<gtr\> 0
    </verbatim>
  </big-figure|Assignment to existing binding<label|nonlocal-assignment>>

  <\warning>
    In the current state of the standard implementation, assigning to an
    existing rewrite must respect the type and overwrites the value in place.
    For example, if there is a declaration like <verbatim|X-\<gtr\>0>, you
    may assign <verbatim|X:=1> and then <verbatim|X> will be replaced with
    <verbatim|1>. But you will not be able to assign <verbatim|X:="Hello">.
    Furthermore, it is currently only possible to assign scalar types, i.e.
    integer, real and text values. You cannot assign an arbitrary tree to a
    rewrite.
  </warning>

  <paragraph|Local variables>If the left side of an assignment is a type
  declaration<subindex|assignment|to type declaration><subindex|type
  declaration|in assignment>, that assignment creates a new
  binding<subindex|binding|local scope> in the local scope<index|local
  scope><subindex|scope|local>, as illustrated in
  Figure<nbsp><reference|assign-to-new-local>. That binding has a return type
  declaration<subindex|return type declaration|in
  assignment><subindex|binding|with return type declaration> associated with
  it, so that later assignments to that same name will only succeed if the
  type of the assigned value matches the previously declared type. This is
  shown in Figure<nbsp><reference|assign-to-new-local>:

  <big-figure|<\verbatim>
    // Global X

    X := 0

    \;

    // Assign to local X

    assigns_new -\<gtr\> X:integer := 1
  </verbatim>|Assigning to new local variable <label|assign-to-new-local>>

  <\warning>
    Assigning to a new local may not work in the current implementation.
  </warning>

  <paragraph|Assigning to references>If the left side of an assignment is a
  reference, then the assignment will apply to the referred value, as shown
  in Figure<nbsp><reference|assign-to-reference-example>. This may either
  modify the referred value if a binding already exists, or create a new
  binding in the context being referred to if no binding exists.

  <big-figure|<\verbatim>
    Data -\<gtr\>

    \ \ \ \ 0 -\<gtr\> 3

    \ \ \ \ 1 -\<gtr\> 2

    Data.0 := 4 // replaces 3

    Data.2 := 5 // Creates new binding 2-\<gtr\>5 in Data
  </verbatim>|Assignment to references>

  <label|assign-to-reference-example>An assignment can also assign to the
  following special references<subindex|expression|allowed on left of
  assignment> (see Section<nbsp><reference|tree-operations>):

  <\itemize>
    <item><verbatim|left X>, <verbatim|right X> when <verbatim|X> is an
    infix, prefix or postfix

    <item><verbatim|child X> when <verbatim|X> is a block

    <item><verbatim|symbol X> when <verbatim|X> is a name or infix and the
    assigned value is a text

    <item><verbatim|opening X> and <verbatim|closing X> when <verbatim|X> is
    a block or text and the assigned value is a text
  </itemize>

  <\warning>
    Assignment to references, and in particular to portions of a tree, is
    mostly broken and does not work in the current implementation, whether
    standard or optimized.
  </warning>

  <paragraph|Assigning to parameters>Assigning to a reference is particularly
  useful for parameters<subindex|assignment|to parameter>. In some cases,
  parameters may be bound without being evaluated (see
  Section<nbsp><reference|lazy-evaluation>). This means that the parameter is
  bound to a reference. In that case, assigning to the parameter will assign
  to the reference, making it possible to implement assignment-like
  operations, as illustrated in Figure<nbsp><reference|assign-to-parameter>.

  <big-figure|<\verbatim>
    A : integer := 5

    A+=3

    \;

    // Effectively assign to A

    X+=Y -\<gtr\> X:=X+Y
  </verbatim>|Assigning to parameter>

  <label|assign-to-parameter>In that example, the context for evaluating the
  implementation <verbatim|X:=X+Y> will contain a binding for <verbatim|X> in
  the form <verbatim|X-\<gtr\>(A-\<gtr\>5).A>, where <verbatim|(A-\<gtr\>5)>
  is the original execution context. The expression <verbatim|(A-\<gtr\>5).A>
  means that we evaluate <verbatim|A> in the context that existed at the
  point where expression <verbatim|A+=3> was evaluated. Therefore, assigning
  to <verbatim|X> will affect the existing binding,, resulting in the updated
  binding <verbatim|A-\<gtr\>8> in the original context.

  <paragraph|Assignments as expressions>Using an assignment in an
  expression<subindex|assignment|in expression><subindex|expression|assignment
  as expression> is equivalent to using the value bound to the variable after
  the assignment. For instance, <verbatim|sin(x:=f(0))> is equivalent to
  <verbatim|x:=f(0)> followed by <verbatim|sin(x)>.

  <subsubsection|Guards>

  <index|guard><index|when infix operator>The infix <verbatim|when> operator
  in a rewrite or data pattern introduces a <em|guard>, i.e. a boolean
  condition that must be true for the pattern to apply.

  Figure<nbsp><reference|guard> shows an improved definition of the factorial
  function which only applies for non-negative values. This set of rewrites
  is ignored for a negative <verbatim|integer> value.

  <big-figure|<\verbatim>
    0! \ \ \ \ \ \ \ \ \ \ \ -\<gtr\> 1

    N! when N \<gtr\> 0 -\<gtr\> N * (N-1)!
  </verbatim>|Guard limit the validity of operations><label|guard>

  A form where the guard cannot be evaluated or evaluates to anything but the
  value <verbatim|true> is not selected. For example, if we try to evaluate
  <verbatim|'ABC'!> the condition <verbatim|N\<gtr\>0> is equivalent to
  <verbatim|'ABC'\<gtr\>0>, which cannot be evaluated unless you added
  specific declarations. Therefore, the rewrite for <verbatim|N!> does not
  apply.

  <\warning>
    Guards are only implemented in optimized mode, which is not fully
    functional yet.
  </warning>

  <subsubsection|Sequences>

  <index|sequence>The infix line-break <verbatim|NEWLINE> and semi-colon
  <verbatim|;> operators are used to introduce a sequence between statements.
  They ensure that the left node is evaluated entirely before the evaluation
  of the right node begins<subindex|sequence|evaluation
  order><subindex|evaluation|order>.

  Figure<nbsp><reference|sequence> for instance guarantees that the code will
  first <verbatim|write> "A", then <verbatim|write "B">, then write the
  result of the sum <verbatim|f(100)+f(200)>. However, the implementation is
  entirely free to compute <verbatim|f(100)> and <verbatim|f(200)> in any
  order, including in parallel.

  <big-figure|<\verbatim>
    write "A"; write "B"

    write f(100)+f(200)
  </verbatim>|Code writing <verbatim|A>, then <verbatim|B>, then
  <verbatim|f(100)+f(200)>><label|sequence>

  Items in a sequence can be <em|declarations><index|declaration> or
  <em|statements><index|statement>. Declarations include rewrite
  declarations, data declarations, type declarations and assignments to a
  type declaration. All other items in a sequence are statements.

  <subsubsection|Index operators><label|index-operators>

  <index|index operator>The notation <verbatim|A[B]> and
  <verbatim|A.B><subindex|dot|as index operator> are used as index operators,
  i.e. to refer to individual items in a collection. The <verbatim|A[B]>
  notation is intended to represent array<subindex|array|index>
  indexing<index|array index><subindex|index|array> operations, whereas the
  <verbatim|A.B> notation is intended to represent field indexing
  operations<index|field index><subindex|index|field>.

  For example, consider the declarations in
  Figure<nbsp><reference|structured-data>.

  <big-figure|<\verbatim>
    MyData -\<gtr\>

    \ \ \ \ Name \ -\<gtr\> "Name of my data"

    \ \ \ \ Value -\<gtr\> 3.45

    \ \ \ \ 1 -\<gtr\> "First"

    \ \ \ \ 2 -\<gtr\> "Second"

    \ \ \ \ 3 -\<gtr\> "Third"
  </verbatim>|Structured data><label|structured-data>

  In that case, the expression <verbatim|MyData.Name> results in the value
  <verbatim|"Name of my data">. The expression <verbatim|MyData[1]> results
  in the value <verbatim|"First">.

  The two index operators differ when their right operand is a name. The
  notation <verbatim|A.B> evaluates <verbatim|B> in the context of
  <verbatim|A>, whereas <verbatim|A[B]> first evaluates <verbatim|B> in the
  current context, and then applies <verbatim|A> to it (it is actually
  nothing more than a regular tree rewrite). Therefore, the notation
  <verbatim|MyData.Value> returns the value <verbatim|3.45>, whereas the
  value of <verbatim|MyData[Value]> will evaluate <verbatim|Value> in the
  current context, and then apply <verbatim|MyData> to the result. For
  example, if we had <verbatim|Value-\<gtr\>3> in the current context, then
  <verbatim|MyData[Value]> would evaluate to <verbatim|"Third">.

  <\warning>
    Index operators are only partially implemented. They work for simple
    examples, but may fail for more complex use cases. In particular, it is
    not currently possible to update a context by writing to an indexed
    value.
  </warning>

  <paragraph|Comparison with C>Users familiar with languages such as C may be
  somewhat disconcerted by ELIOT's index operators. The following points are
  critical for properly understanding them:

  <\itemize>
    <item><subindex|array|as function>Arrays, structures and functions are
    all represented the same way. The entity called <verbatim|MyData> can be
    interpreted as an array in <verbatim|MyData[3]>, as a structure in
    <verbatim|MyData.Name>, or as a function if one writes <verbatim|MyData
    3>. In reality, there is no difference between <verbatim|MyData[3]> and
    <verbatim|MyData 3>: the former simply passes a block as an argument,
    i.e. it is exactly equivalent to <verbatim|MyData(3)>,
    <verbatim|MyData{3}>. Writing <verbatim|MyData[3]> is only a way to
    document an intent to use <verbatim|MyData> as an array, but does not
    change the implementation.

    <item>Data structures can be extended on the fly. For example, it is
    permitted to assign something to a non-existent binding in
    <verbatim|MyData>, e.g. by writing <verbatim|MyData[4]:=3>. The ability
    to add ``fields'' to a data structure on the fly makes it easier to
    extend existing code.

    <item>Data structures can include other kinds of rewrites, for example
    ``functions'', enabling object-oriented data structures. This is
    demonstrated in Section<nbsp><reference|object-oriented-programming>.

    <item>Since the notation <verbatim|A.B> simply evaluates <verbatim|B> in
    the context of <verbatim|A>, the value of <verbatim|MyData.4> is...
    <verbatim|4>: there is no rewrite for <verbatim|4> in <verbatim|MyData>,
    therefore it evaluates a itself.
  </itemize>

  <subsubsection|C interface>

  A <em|C interface><index|C interface> is is a rewrite where the
  implementation is a prefix of two names, the first one being <verbatim|C>
  and the second one being the name of a C function. A C interface can also
  be specified using a special <verbatim|extern> syntax<index|extern syntax>.
  The name of the C function can also be specified as text if it does not
  obey ELIOT naming rules, e.g. to interface to a function named
  <verbatim|_foobar_>.

  Figure<nbsp><reference|C-interface> shows two ways of making the
  <verbatim|sin> function of the C standard library available to an ELIOT
  program. The first one uses an ELIOT-style rewrite, whereas the second one
  uses a C-style syntax:

  <big-figure|<\verbatim>
    sin X:real as real -\<gtr\> C sin

    extern double sin(double);
  </verbatim>|Creating an interface for a C function>

  <label|C-interface>The C-like syntax used for <verbatim|extern> declaration
  is defined by the file <verbatim|C.syntax><index|C.syntax>, and applies for
  anything between delimiters <verbatim|extern> and <verbatim|;> as indicated
  in the <verbatim|xl.syntax> file<subindex|xl.syntax|connexion to
  C.syntax><subindex|C.syntax|connexion to xl.syntax>. While extremely
  simplistic relative to the real C syntax, it is sufficient to import most
  functions.

  Table<nbsp><reference|C-types-conversion> shows which types can be used in
  a C interface and what C type they map to:

  <big-table|<block*|<tformat|<table|<row|<cell|<strong|ELIOT
  type>>|<cell|<strong|C type>>>|<row|<cell|<verbatim|integer>>|<cell|<verbatim|int>>>|<row|<cell|<verbatim|real>>|<cell|<verbatim|double>>>|<row|<cell|<verbatim|text>>|<cell|<verbatim|const
  char *>>>|<row|<cell|<verbatim|tree>>|<cell|<verbatim|Tree
  *>>>|<row|<cell|<verbatim|infix>>|<cell|<verbatim|Infix
  *>>>|<row|<cell|<verbatim|prefix>>|<cell|<verbatim|Prefix
  *>>>|<row|<cell|<verbatim|postfix>>|<cell|<verbatim|Postfix
  *>>>|<row|<cell|<verbatim|block>>|<cell|<verbatim|Block
  *>>>|<row|<cell|<verbatim|name>>|<cell|<verbatim|Name
  *>>>|<row|<cell|<verbatim|boolean>>|<cell|<verbatim|bool>>>>>>|Type
  correspondances in a C interface><label|C-types-conversion>

  <\warning>
    The C interface syntax is only available in optimized mode.
  </warning>

  <subsubsection|Machine Interface>

  A <em|machine interface><index|machine interface> is a rewrite where the
  implementation is a prefix of two names, the first one being
  <verbatim|opcode><index|opcode>. Figure<nbsp><reference|opcode-declaration>
  shows how a specific tree rewrite can be connected to the generation of
  machine-level opcodes:

  <big-figure|<verbatim|X:integer+Y:integer as integer -\<gtr\> opcode
  Add>|Generating machine code using opcode
  declarations><label|opcode-declaration>

  Machine-level opcodes are provided by the LLVM library
  (<hlink|http://llvm.org|http://llvm.org>). Opcodes available to ELIOT
  programs are described in Section<nbsp><reference|machine-interface>.

  <\warning>
    The machine-level interface is only available in optimized mode.
  </warning>

  <subsection|Binding References to Values><label|binding>

  A <em|rewrite declaration> of the form <verbatim|Pattern-\<gtr\>Implementation>
  is said to <em|bind><index|binding> its pattern to its implementation. A
  sequence of rewrite declarations is called a <em|context><index|context>.
  For example, the block <verbatim|{x-\<gtr\>3;y-\<gtr\>4}> is a context that
  binds <verbatim|x> to <verbatim|3> and <verbatim|y> to <verbatim|4>.

  <\warning>
    The idea of formalizing the context and making it available to programs
    was only formalized after the standard and optimized mode were
    implemented. It is not currently working, but should be implemented in a
    future release. However, many notions described in this section apply
    internally to the existing implementations, i.e. the context order is
    substantially similar even if it is not made visible to programs in the
    way being described here.
  </warning>

  <subsubsection|Context Order>

  <index|context order>A context may contain multiple rewrites that hide one
  another.

  For example, in the context <verbatim|{x-\<gtr\>0;x-\<gtr\>1}>, the name
  <verbatim|x> is bound twice. The evaluation of <verbatim|x> in that context
  will return <verbatim|0> because rewrites are tested in order. In other
  words, the declaration <verbatim|x-\<gtr\>0> <em|shadows><index|shadowed
  binding> the declaration <verbatim|x-\<gtr\>1> in that context.

  For the purpose of finding the first match, a context is traversed depth
  first in left-to-right order, which is called <em|context order>.

  <subsubsection|Scoping>

  <index|scope>The left child of a context is called the <em|local
  scope><subindex|scope|local>. The right child of a context is the
  <em|enclosing context><subindex|context|enclosing>. All other left children
  in the sequence are the local scopes of expressions currently being
  evaluated. The first one being the <em|enclosing
  scope><subindex|scope|enclosing> (i.e. the local scope of the enclosing
  context) and the last one being the <em|global
  scope><subindex|scope|global>.

  This ensures that local declarations hide declarations from the surrounding
  context, since they are on the left of the right child, while allowing
  local declarations in the left child of the context to be kept in program
  order, so that the later ones are shadowed by the earlier ones.

  The child at the far right of a context is a catch-all<index|catch-all
  rewrite> rewrite intended to specify what happens when evaluating an
  undefined form<index|undefined form>.

  <subsubsection|Current context>

  <index|current context><subindex|context|current>Any evaluation in ELIOT is
  performed in a context called the <em|current context>. The current context
  is updated by the following operations:

  <\enumerate>
    <item>Evaluating the implementation of a rewrite creates a
    scope<subindex|scope|creation> binding all arguments to the corresponding
    parameters, then a new context with that scope as its left child and the
    old context as its right child. The implementation is then evaluated in
    the newly created context.

    <item>Evaluating a sequence initializes a local context with all
    declarations in that sequence, and creates a new current context with the
    newly created local context as its left child and the old context as its
    right child. Statements in the sequence are then evaluated in the newly
    created context.

    <item><index|assignment>Evaluating an assignment changes the
    implementation of an existing binding if there is one in the current
    context, or otherwise creates a new binding in the local scope.
  </enumerate>

  <subsubsection|References>

  <index|reference>An expression that can be placed on the left of an
  assignment to identify a particular binding is called a <em|reference>. A
  reference can be any pattern that would go on the left of a rewrite. In
  addition, it can be an index operator<index|index operator>:

  <\itemize>
    <item>If <verbatim|A> refers to a context, assigning to <verbatim|A.B>
    will affect the binding of <verbatim|B> in the context <verbatim|A>, and
    not the binding of <verbatim|A.B> in the current context.

    <item>If <verbatim|A> refers to a context, assigning to <verbatim|A[B]>
    (or, equivalently, to <verbatim|A{B}>, <verbatim|A(B)> or <verbatim|A B>)
    will affect the binding corresponding to <verbatim|B> in the context of
    <verbatim|A>, not the binding of <verbatim|A B> in the current context.
    The index <verbatim|B> will be evaluated in the current context if
    required to match patterns in <verbatim|A>, as explained in
    Section<nbsp><reference|evaluation>.

    <item>Special forms described in \ Section<nbsp><reference|tree-operations>,
    such as <verbatim|left X> refer to children of <verbatim|infix>,
    <verbatim|prefix>, <verbatim|postfix> or <verbatim|block> trees. They can
    be used as a reference in an assignment, and will modify the tree being
    referred to. This can be used to directly manipulate the program
    structure.\ 
  </itemize>

  <subsection|Evaluation><label|evaluation>

  <index|evaluation><em|Evaluation> is the process through which a given tree
  is rewritten.

  <subsubsection|Standard evaluation><label|standard-evaluation>

  <subindex|evaluation|standard case>Except for special forms described
  later, the evaluation of ELIOT trees is performed as follows:

  <\enumerate>
    <item>The tree to evaluate, <verbatim|T>, is matched against the
    available data and rewrite pattern. <verbatim|3*4+5> will match
    <verbatim|A*B+C> as well as <verbatim|A+B> (since the outermost tree is
    an infix <verbatim|+> as in <verbatim|A+B>).

    <item>Possible matches are tested in <em|context order><index|context
    order> (defined in Section<nbsp><reference|binding>) against the tree to
    evaluate. The first matching tree is selected. For example, in
    Figure<nbsp><reference|factorial>, <verbatim|(N-1)!> will be matched
    against the rules <verbatim|0!> and <verbatim|N!> in this order.

    <item><subindex|pattern|matching>Nodes in each candidate pattern
    <verbatim|P> are compared to the tree <verbatim|T> as follows:

    <\itemize>
      <item>Constant symbols or names in <verbatim|P> are compared to the
      corresponding element in <verbatim|T> and must match exactly. For
      example, the <verbatim|+> symbol in pattern <verbatim|A+B> will match
      the plus <verbatim|+> symbol in expression <verbatim|3*4+5> but not the
      minus <verbatim|-> symbol in <verbatim|3-5>.

      <item>Variables names in <verbatim|P> that are not bound to any value
      and are not part of a type declaration are bound to the corresponding
      fragment of the tree in <verbatim|T>. For example, for the expression
      <verbatim|3!>, the variable <verbatim|N> in
      Figure<nbsp><reference|factorial> will be bound to <verbatim|3>.

      <item>Variable names in <verbatim|P> that are bound to a value are
      compared to the corresponding tree fragment in <verbatim|T> after
      evaluation. For instance, if <verbatim|true> is bound at the point of
      the declaration in Figure<nbsp><reference|if-then-else>, the test
      <verbatim|if A\<less\>3 then X else Y> requires the evaluation of the
      expression <verbatim|A\<less\>3>, and the result will be compared
      against <verbatim|true>.

      <item>This rule applies even if the binding occured in the same
      pattern. For example, the pattern <verbatim|A+A> will match
      <verbatim|3+3> but not <verbatim|3+4>, because <verbatim|A> is first
      bound to <verbatim|3> and then cannot match <verbatim|4>. The pattern
      <verbatim|A+A> will also match <verbatim|(3-1)+(4-2)>: although
      <verbatim|A> may first be bound to the unevaluated value
      <verbatim|3-1>, verifying if the second <verbatim|A> matches requires
      evaluating both <verbatim|A> and the test value.

      <item>Type declarations in <verbatim|P> match if the result of
      evaluating the corresponding fragment in <verbatim|T> has the declared
      type, as defined in Section<nbsp><reference|types>. In that case, the
      variable being declared is bound to the evaluated value.

      <item>Constant values (integer, real and text) in <verbatim|P> are
      compared to the corresponding fragment of <verbatim|T> after
      evaluation. For example, in Figure<nbsp><reference|factorial>, when the
      expression <verbatim|(N-1)!> is compared against <verbatim|0!>, the
      expression <verbatim|(N-1)> is evaluated in order to be compared to
      <verbatim|0>.

      <item>Infix, prefix and postfix nodes in <verbatim|P> are compared to
      the matching node in <verbatim|T> by comparing their children in
      depth-first, left to right order.
    </itemize>

    The comparison process, called <em|pattern matching>, may cause fragments
    of the tree to be evaluated<subindex|evaluation|of arguments>. Each
    fragment is evaluated at most once for the process of evaluating the tree
    <verbatim|T>. Once the fragment has been evaluated, the evaluated value
    will be <em|memoized><subindex|memoization|of arguments> and used in any
    subsequent comparison or variable binding. For example, when computing
    <verbatim|F(3)!>, the evaluation of <verbatim|F(3)> is required in order
    to compare to <verbatim|0!>, guaranteeing that <verbatim|N> in
    <verbatim|N!> will be bound to the evaluated value if <verbatim|F(3)> is
    not equal to <verbatim|0>.

    <item><subindex|evaluation|mismatch>If there is no match found between
    any pattern <verbatim|P> and the tree to evaluate <verbatim|T>:

    <\itemize>
      <item>Integer, real and text terminals evaluates as themselves.

      <item>A block evaluates as the result of evaluating its child.

      <item>If the tree is a prefix with the left being a name containing
      <verbatim|error>, then the program immediatly aborts and shows the
      offending tree. This case corresponds to an unhandled error.

      <item>For a prefix node or postfix tree, the operator child (i.e. the
      left child for prefix and the right child for postfix) is evaluated,
      and if the result is different from the original operator child,
      evaluation is attempted again after replacing the original operator
      child with its evaluated version.

      <item>In any other case, the tree is prefixed with
      <verbatim|evaluation_error> and the result is evaluated. For example,
      <verbatim|<math|>$foo> will be transformed into
      <verbatim|evaluation_error $foo>. A prefix rewrite for
      <verbatim|evaluation_error> is supposed to handle such errors.
    </itemize>

    <item>If a match is found, variables in the first matching pattern
    (called <em|parameters>) are bound to the corresponding fragments of the
    tree to evaluate (called <em|arguments>)<subindex|binding|parameters>.

    <\itemize>
      <item>If an argument was evaluated (including as required for
      comparison with an earlier pattern), then the corresponding parameter
      is bound with the evaluated version.

      <item>If the argument was not evaluated, the corresponding parameter is
      bound with the tree fragment in context, as explained in
      Section<nbsp><reference|binding>. In line with the terminology used in
      functional languages, this context-including
      binding<subindex|context|passed with arguments> is called a
      <em|closure><index|closure>.
    </itemize>

    <item>Once all bindings have been performed, the implementation
    corresponding to the pattern in the previous step is itself evaluated.
    The result of the evaluation of the original form is the result of
    evaluating the implementation in the new
    context<subindex|context|parameter context> created by adding to the
    original context the bindings of parameters to their arguments. For a
    data form, the result of evaluation is the pattern after replacing
    parameters with the corresponding arguments.
  </enumerate>

  <subsubsection|Special forms>

  <index|special forms><subindex|evaluation|special forms>Some forms have a
  special meaning and are evaluated specially:

  <\enumerate>
    <item>A terminal node (integer, real, type, name) evaluates as itself,
    unless there is an explicit rewrite rule for it<\footnote>
      There are several use cases for allowing rewrite rules for integer,
      real or text constants, notably to implement data maps such as
      <verbatim|(1-\<gtr\>0; 0-\<gtr\>1)>, also known as associative arrays.
    </footnote>.

    <item>A block evaluate as the result of evaluating its child.

    <item>A rewrite rule evaluates as itself.

    <item>A data declaration evaluates as itself

    <item>An assignment binds the variable and evaluates as the named
    variable after assignment

    <item>Evaluating a sequence creates a new local context with all
    declarations in the sequence, then evaluates all its statements in order
    in that new local context. The result of evaluation is the result of the
    last statement, if there is one, or the newly created context if the
    sequence only contains declarations.

    <item>If <verbatim|C> is a context and <verbatim|E> is an expression,
    evaluating <verbatim|C E> is equivalent to evaluating <verbatim|E> in the
    current context, then evaluating the result in the context of
    <verbatim|C>. For example, <verbatim|(0-\<gtr\>3)(1-1)> will evaluate
    <verbatim|1-1>, resulting in <verbatim|0>, then evaluate the result in
    the context <verbatim|0-\<gtr\>3>, resulting in the value <verbatim|3>.

    <item>If <verbatim|C> is a context and <verbatim|E> is an expression,
    evaluating <verbatim|C.E> is equivalent to evaluating <verbatim|E> in the
    context of <verbatim|C>. For example,
    <verbatim|(foo-\<gtr\>1;bar-\<gtr\>2).bar> will return <verbatim|2>.
  </enumerate>

  <subsubsection|Lazy evaluation><label|lazy-evaluation>

  <index|lazy evaluation><subindex|evaluation|lazy>When an argument is bound
  to a parameter, it is associated to a context which allows correct
  evaluation at a later time, but the argument is in general not evaluated
  immediately. Instead, it is only evaluated when evaluation becomes
  necessary for the program to execute correctly. This technique is called
  <em|lazy evaluation>. It is intended to minimize unnecessary evaluations.

  Evaluation of an argument before binding it to its parameter occurs in the
  following cases, collectively called <em|demand-based
  evaluation><subindex|evaluation|demand-based>:

  <\enumerate>
    <item>The argument is compared to a constant value or bound name, see
    Section<nbsp><reference|standard-evaluation>, and the static value of the
    tree is not sufficient to perform the comparison. For example, in
    Figure<nbsp><reference|evaluation-for-comparison>, the expression
    <verbatim|4+X> requires evaluation of <verbatim|X> for comparison with
    <verbatim|4> to check if it matches <verbatim|A+A>; the expression
    <verbatim|B+B> can be statically bound to the form <verbatim|A+A> without
    requiring evaluation of <verbatim|B>; finally, in <verbatim|B+C>, both
    <verbatim|B> and <verbatim|C> need to be evaluated to compare if they are
    equal and if the form <verbatim|A+A> matches.

    <big-figure|<\verbatim>
      A+A -\<gtr\> 2*A

      4+X \ // X evaluated

      B+B \ // B not evaluated

      B+C \ // B and C evaluated
    </verbatim>|Evaluation for comparison><label|evaluation-for-comparison>

    <item>The argument is tested against a parameter with a type declaration,
    and the static type of the tree is not sufficient to guarantee a match.
    For example, in Figure<nbsp><reference|evaluation-for-type-comparison>,
    the expression <verbatim|Z+1> can statically be found to match the form
    <verbatim|X+Y>, so <verbatim|Z> needn't be evaluated. On the other hand,
    in <verbatim|1+Z>, it is necessary to evaluate <verbatim|Z> to type-check
    it against <verbatim|integer>.

    <big-figure|<\verbatim>
      X:tree + Y:integer -\<gtr\> ...

      Z + 1 // Z not evaluated

      1 + Z // Z evaluated
    </verbatim>|Evaluation for type comparison><label|evaluation-for-type-comparison>

    <item>A specific case of the above scenario is the left side of any index
    operator. In <verbatim|A.B> or <verbatim|A[B]>, the value <verbatim|A>
    needs to be evaluated to verify if it contains <verbatim|B>.
  </enumerate>

  When lazy evaluation happens, the expression being bound is a closure as
  explained in Section<nbsp><reference|standard-evaluation>, i.e. it will be
  an expression of the form <verbatim|C.E> where <verbatim|C> is the original
  evaluation context and <verbatim|E> is the original expression to evaluate.

  <\warning>
    Lazy evaluation was formalized after the compilers were implemented, and
    is not entirely consistent in the current implementations. This should be
    fixed in future versions.
  </warning>

  <subsubsection|Explicit evaluation><label|explicit-evaluation>

  <index|explicit evaluation><subindex|evaluation|explicit>Expressions are
  also evaluated in the following cases, collectively called <em|explicit
  evaluation>:

  <\enumerate>
    <item>An expression on the left or right of a sequence is evaluated. For
    example, in <verbatim|A;B>, the names <verbatim|A> and <verbatim|B> will
    be evaluated.

    <item>The prefix <verbatim|do> forces evaluation of its argument. For
    example, <verbatim|do X> will force the evaluation of <verbatim|X>.

    <item>The program itself is evaluated. Most useful programs are
    sequences.
  </enumerate>

  The explicit evaluation of a name does not change the value bound to that
  name in the current context. For example, if the current context contains
  <verbatim|A-\<gtr\>write "Hello">, each explicit evaluation of <verbatim|A>
  will cause the message <verbatim|Hello> to be written.

  <subsubsection|Memoization>

  <subindex|memoization|of parameters>Whenever a parameter is evaluated, the
  evaluated result may be used for all subsequent demand-based evaluations of
  the same tree, a process called <em|memoization>. What is memoized is
  associated with the original tree.

  \;

  \ Memoization does not happen for explicit evaluations. This is illustrated
  with the example in Figure<nbsp><reference|explicit-vs-lazy-evaluation>:

  <big-figure|<\verbatim>
    foo X -\<gtr\>

    \ \ \ \ X

    \ \ \ \ if X then writeln "X is true"

    \ \ \ \ if do X then writeln "X is true"

    \ \ \ \ X

    bar -\<gtr\>

    \ \ \ \ writeln "bar evaluated"

    \ \ \ \ true

    foo bar
  </verbatim>|Explicit vs. lazy evaluation><label|explicit-vs-lazy-evaluation>

  <subindex|evaluation|explicit vs. lazy>In
  Figure<nbsp><reference|explicit-vs-lazy-evaluation>, evaluation happens as
  follows:

  <\enumerate>
    <item>The expression <verbatim|foo bar> is evaluated explicitly, being
    part of a sequence. This matches the rewrite for <verbatim|foo X> on the
    first line.

    <item>The first reference to <verbatim|X> in the implementation is
    evaluated explicitly. This causes the message <samp|bar evaluated> to be
    written to the console.

    <item>The second reference to <verbatim|X> is demand-based, but since
    <verbatim|X> has already been evaluated, the result <verbatim|true> is
    used directly. The message <samp|X is true> is emitted on the console,
    but the message <samp|bar evaluated> is not.

    <item>The third reference to <verbatim|X> is an argument to
    <verbatim|do>, so it is evaluated again, which writes the message
    <samp|bar evaluated> on the console.

    <item>The last reference to <verbatim|X> is another explicit evaluation,
    so the message <samp|bar evaluated> is written on the console again.
  </enumerate>

  The purpose of these rules is to allow the programmer to pass code to be
  evaluated as an argument, while at the same time minimizing the number of
  repeated evaluations when a parameter is used for its value. In explicit
  evaluation, the value of the parameter is not used, making it clear that
  what matters is the effect of evaluation itself. In demand-based
  evaluation, it is on the contrary assumed that what matters is the result
  of the evaluation, not the process of evaluating it. It is always possible
  to force evaluation explicitly using <verbatim|do><subindex|evaluation|forcing
  explicit evaluation>.

  <\warning>
    Like lazy evaluation, memoization is not fully consistent in the current
    implementations.
  </warning>

  <subsection|Types><label|types>

  <em|Types><index|type> are expressions that appear on the right of the
  colon operator <verbatim|:> in type declarations<index|type
  declaration><subindex|type|declaration>. In ELIOT, a type identifies the
  <em|shape> of a tree. A value is said to <em|belong> to a type if it
  matches the shape defined by the type. A value may belong to multiple
  types<subindex|type|belonging to a type>.

  <\warning>
    Like contexts, the type system was largely redesigned based on experience
    with the first implementations of the language. As a result, the current
    implementations implement a very weak type system compared to what is
    being described in this section. At this point, user-defined types do not
    work as descried in either the standard or optimized implementation. This
    section defines the future implementation.
  </warning>

  <subsubsection|Predefined types>

  <subindex|type|predefined><index|predefined types>The following types are
  predefined:

  <\itemize>
    <item><verbatim|integer><index|integer> matches integer values

    <item><verbatim|real><index|real> matches real values

    <item><verbatim|text<index|text>> matches text values

    <item><verbatim|symbol><index|symbol> matches names and operator symbols

    <item><verbatim|name><index|name> matches names only

    <item><verbatim|operator><index|operator> matches operator symbols only

    <item><verbatim|infix><index|infix> matches infix nodes

    <item><verbatim|prefix><index|prefix> matches prefix nodes

    <item><verbatim|postfix><index|postfix> matches postfix nodes

    <item><verbatim|block><index|block> matches block nodes

    <item><verbatim|tree><index|tree> matches any abstract syntax tree, i.e.
    any representable XL value

    <item><verbatim|boolean><index|boolean> matches the names <verbatim|true>
    and <verbatim|false>.
  </itemize>

  <subsubsection|Type definition><label|type-definition>

  <index|type definition><subindex|type|definition><subindex|definition|of
  types>A <em|type definition> for type <verbatim|T> is a special form of
  tree rewrite declaration where the pattern has the form <verbatim|type X>.
  A type definition declares a type name, and the pattern that the type must
  match. For example, Figure<nbsp><reference|simple-type> declares a type
  named <verbatim|complex> requiring two real numbers called <verbatim|re>
  and <verbatim|im>, and another type named
  <verbatim|ifte><subindex|if-then-else|type> that contains three arbitrary
  trees called <verbatim|Cond>, <verbatim|TrueC> and <verbatim|FalseC>.

  <big-figure|<\verbatim>
    complex -\<gtr\> type (re:real, im:real)

    ifte -\<gtr\> type {if Cond then TrueC else FalseC}
  </verbatim>|Simple type declaration><label|simple-type>

  The outermost block of a type pattern, if it exists, is not part of the
  type pattern<index|type pattern><subindex|type|pattern><subindex|pattern|in
  type>. To create a type matching a specific block shape, two nested bocks
  are required, as illustrated with <verbatim|paren_block_type> in
  Figure<nbsp><reference|block-type-declaration>:

  <big-figure|<\verbatim>
    paren_block_type -\<gtr\> type((BlockChild))
  </verbatim>|Simple type declaration><label|block-type-declaration>

  Note that type definitions and type declarations should not be confused. A
  type <em|definition> defines a type and has the form <verbatim|Name
  -\<gtr\> type TypePattern>, whereas a type <em|declaration><subindex|type
  declaration|vs. type definition><subindex|type definition|vs. type
  declaration> declares the type of an entity and has the form
  <verbatim|Name:Type>. The type defined by a type definition can be used on
  the right of a type declaration. For example,
  Figure<nbsp><reference|using-complex> shows how to use the
  <verbatim|complex> type defined in Figure<nbsp><reference|simple-type> in
  parameters.

  <big-figure|<\verbatim>
    Z1:complex+Z2:complex -\<gtr\> (Z1.re+Z2.re, Z1.im+Z2.im)
  </verbatim>|Using the <verbatim|complex> type><label|using-complex>

  Parameters<subindex|parameters|of types> of types such as
  <verbatim|complex> are bound to contexts with declarations for the
  individual variables of the pattern of the type. For example, a
  <verbatim|complex> like <verbatim|Z1> in
  Figure<nbsp><reference|using-complex> contains a rewrite for <verbatim|re>
  and a rewrite for <verbatim|im>. Figure<nbsp><reference|binding-for-complex-parameter>
  possible bindings<subindex|bindings|in type definitions> when using the
  complex addition operator defined in Figure<nbsp><reference|using-complex>.
  The standard index notation<subindex|index|for user-defined types>
  described in Section<nbsp><reference|index-operators> applies, e.g. in
  <verbatim|Z1.re>, and these bindings can be assigned to.

  <big-figure|<\verbatim>
    // Expression being evaluated

    (3.4, 5.2)+(0.4, 2.22)

    \;

    // Possible resulting bindings

    // in the implementation of +

    Z1 -\<gtr\>

    \ \ \ \ re-\<gtr\>3.4

    \ \ \ \ im-\<gtr\>5.2

    \ \ \ \ re, im

    Z2 -\<gtr\>

    \ \ \ \ re-\<gtr\>0.4

    \ \ \ \ im-\<gtr\>2.22

    \ \ \ \ re, im
  </verbatim>|Binding for a <verbatim|complex>
  parameter><label|binding-for-complex-parameter>

  Figure<nbsp><reference|making-two-types-equivalent> shows two ways to make
  type <verbatim|A> equivalent to type <verbatim|B>:

  <big-figure|<\verbatim>
    A -\<gtr\> B

    A -\<gtr\> type X:B
  </verbatim>|Making type <verbatim|A> equivalent to type
  <verbatim|B>><label|making-two-types-equivalent>

  <subsubsection|Normal form for a type>

  <index|normal form><subindex|type|normal form>By default, the name of a
  type is not part of the pattern being recognized. It is often recommended
  to make data types easier to identify by making the pattern more
  specific<subindex|pattern|making type pattern specific>, for instance by
  including the type name in the pattern itself, as shown in
  Figure<nbsp><reference|more-specific-complex-types>:

  <big-figure|<\verbatim>
    complex -\<gtr\> type complex(re:real, im:real)
  </verbatim>|Named patterns for <verbatim|complex>><label|more-specific-complex-types>

  In general, multiple notations for a same type<subindex|type|multiple
  notations> can coexist. In that case, it is necessary to define a form for
  trees that the other possible forms will reduce to. This form is called the
  <em|normal form>. This is illustrated in
  Figure<nbsp><reference|complex-normal-form>, where the normal form is
  <verbatim|complex(re;im)> and the other notations are rewritten to this
  normal form for convenience.

  <big-figure|<\verbatim>
    // Normal form for the complex type

    complex -\<gtr\> type complex(re:real, im:real)

    \;

    // Other possible notations that reduce to the normal form

    i -\<gtr\> complex(0,1)

    A:real + i*B:real -\<gtr\> complex(A,B)

    A:real + B:real*i -\<gtr\> complex(A,B)
  </verbatim>|Creating a normal form for the complex
  type><label|complex-normal-form>

  <subsubsection|Properties>

  <index|properties><subindex|type|properties>Properties are types that match
  a number of trees, based not just on the shape of the tree, but on symbols
  bound in that tree. For instance, when you need a <verbatim|color> type
  representing red, green and blue components, you care about the value of
  the components, but not the order in which they appear.

  A <em|properties definition> is a rewrite declaration like the one shown in
  Figure<nbsp><reference|properties-declaration> where:

  <\enumerate>
    <item>The implementation is a prefix with the name <verbatim|properties>
    followed by a block.

    <item>The block contains a sequence of type declarations
    <verbatim|Name:Type> or assignments to type declarations
    <verbatim|Name:Type:=DefaultValue>, each such statement being called a
    <em|property><index|property>.

    <item>The block optionally contains one or more
    <verbatim|inherit><index|inherit> prefix (see
    Section<nbsp><reference|data-inheritance-section>)
  </enumerate>

  <big-figure|<\verbatim>
    color -\<gtr\> properties

    \ \ \ \ red \ \ : real

    \ \ \ \ green : real

    \ \ \ \ blue \ : real

    \ \ \ \ alpha : real := 1.0
  </verbatim>|Properties declaration><label|properties-declaration>

  Properties parameters<subindex|parameters|with properties types> match any
  block for which all the properties are defined. Properties are
  defined<index|property definition><subindex|definition|of properties>
  either if they exist in the argument's context, or if they are explicitly
  set in the block argument, or if a <em|default value><index|default
  value><subindex|property|default value> was assigned to the property in the
  properties declaration. An individual property can be
  set<subindex|property|setting> using an assignment or by using the property
  name as a prefix.

  For example, Figure<nbsp><reference|color-properties> shows how the
  <verbatim|color> type defined in Figure<nbsp><reference|properties-declaration>
  can be used in a parameter declaration<subindex|properties|as parameter
  types>, and how a <verbatim|color> argument<subindex|properties|arguments>
  can be passed.

  <big-figure|<\verbatim>
    write C:color -\<gtr\>

    \ \ \ \ write "R", C.red

    \ \ \ \ write "G", C.green

    \ \ \ \ write "B", C.blue

    \ \ \ \ write "A", C.alpha

    write_color { red 0.5; green 0.2; blue 0.6 }
  </verbatim>|Color properties><label|color-properties>

  Properties parameters are contexts containing local declarations called
  <em|getters><index|getter> and <em|setters><index|setter> for each
  individual property:

  <\itemize>
    <item>The setter is a prefix taking an argument of the property's type,
    and setting the property's value to its argument.

    <item>The getter returns the value of the property in the argument's
    context (which may be actually set in the argument's enclosing contexts),
    or the default value if the property is not bound in the argument's
    context.
  </itemize>

  This makes it possible to set default value in the caller's context, which
  will be injected in the argument, as illustrated in
  Figure<nbsp><reference|setting-default-arguments>, where the expression
  <verbatim|C.red> in <verbatim|write_color> will evaluate to <verbatim|0.5>,
  and the argument <verbatim|C.alpha> will evaluate to <verbatim|1.0> as
  specified by the default value:

  <big-figure|<\verbatim>
    red := 0.5

    write_color (blue 0.6; green 0.2)
  </verbatim>|Setting default arguments from the current
  context><label|setting-default-arguments>

  It is sufficient for the block argument to define all required
  properties<index|required property><subindex|property|required>. The block
  argument may also contain more code than just the references to the
  setters, as illustrated in Figure<nbsp><reference|extra-code-for-properties>:

  <big-figure|<\verbatim>
    write_color

    \ \ \ \ X:real := 0.444 * sin time

    \ \ \ \ if X \<less\> 0 then X := 1.0+X

    \ \ \ \ red X

    \ \ \ \ green X^2

    \ \ \ \ blue X^3
  </verbatim>|Additional code in properties><label|extra-code-for-properties>

  \;

  <subsubsection|Data inheritance><label|data-inheritance-section>

  <index|inherit><index|data inheritance>Properties declarations may
  <em|inherit> data from one or more other types by using one or more
  <verbatim|inherit> prefixes in the properties declaration, as illustrated
  in Figure<nbsp><reference|data-inheritance>, where the type <verbatim|rgb>
  contains three properties called <verbatim|red>, <verbatim|green> and
  <verbatim|blue>, and the type <verbatim|rgba> additionally contains an
  <verbatim|alpha> property:

  <big-figure|<\verbatim>
    rgb -\<gtr\> properties

    \ \ \ \ red \ \ : real

    \ \ \ \ green : real

    \ \ \ \ blue \ : real

    rgba -\<gtr\> properties

    \ \ \ \ inherit rgb

    \ \ \ \ alpha : real
  </verbatim>|Data inheritance><label|data-inheritance>

  Only declarations are inherited in this manner. The resulting types are not
  compatible, although they can be made compatible using automatic type
  conversions<index|automatic type conversion> (see
  Section<nbsp><reference|type-conversions>).

  <subsubsection|Explicit type check>

  <index|explicit type check><subindex|type|explicit type check><index|type
  check><subindex|type|check>Internally, a type is any context where a
  <verbatim|contains><index|contains> prefix can be evaluated. In such a
  context, the expression <verbatim|contains X> is called a <em|type check>
  for the type and for value <verbatim|X>. A type check must return a
  <verbatim|boolean> value to indicate if the value <verbatim|X> belongs to
  the given type.

  <subindex|type|identifying arbitrary tree shapes>Type checks can be
  declared explicitly to create types identifying arbitrary forms of trees
  that would be otherwise difficult to specify. This is illustrated in
  Figure<nbsp><reference|arbitrary-type> where we define an <verbatim|odd>
  type that contains only odd integers and the text <verbatim|"Odd">. We
  could similarly add a type check to the definition of <verbatim|rgb> in
  Figure<nbsp><reference|data-inheritance> to make sure that <verbatim|red>,
  <verbatim|green> and <verbatim|blue> are between <verbatim|0.0> and
  <verbatim|1.0>.

  <big-figure|<\verbatim>
    odd -\<gtr\>

    \ \ \ \ contains X:integer -\<gtr\> X mod 2 = 1

    \ \ \ \ contains "Odd" -\<gtr\> true

    \ \ \ \ contains X -\<gtr\> false
  </verbatim>|Defining a type identifying an arbitrary AST shape>

  <label|arbitrary-type>The type check for a type can be invoked explicitly
  using the infix <verbatim|contains> (with the type on the left) or
  <verbatim|is_a><index|is_a> (with the type on the right). \ This is shown
  in Figure<nbsp><reference|contains-tests>. The first type check
  <verbatim|odd contains 3> should return <verbatim|true>, since <verbatim|3>
  belongs to the <verbatim|odd> type. The second type check should return
  <verbatim|false> since <verbatim|rgb> expects the property <verbatim|blue>
  to be set.

  <big-figure|<\verbatim>
    if odd contains 3 then pass else fail

    if (red 1; green 1) is_a rgb then fail else pass
  </verbatim>|Explicit type check><label|contains-tests>

  <subsubsection|Explicit and automatic type
  conversions><label|type-conversions>

  <index|explicit type conversion><subindex|type|conversions>Prefix forms
  with the same name as a type can be provided to make it easy to convert
  values to type <verbatim|T>. Such forms are called <em|explicit type
  conversions>. This is illustrated in Figure<nbsp><reference|explicit-type-conversion>:

  <big-figure|<\verbatim>
    rgba C:rgb \ -\<gtr\> (red C.red; green C.green; blue C.blue; alpha 1.0)

    rgb \ C:rgba -\<gtr\> (red C.red; green C.green; blue C.blue)
  </verbatim>|Explicit type conversion><label|explicit-type-conversion>

  <index|automatic type conversion>An <em|automatic type conversion> is an
  infix <verbatim|as> form with a type on the right. If such a form exists,
  it can be invoked to automatically convert a value to the type on the right
  of <verbatim|as>. This is illustrated in
  Figure<nbsp><reference|automatic-type-conversion>.

  <big-figure|<\verbatim>
    X:integer as real -\<gtr\> real X

    1+1.5 \ \ // 1.0+1.5 using conversion above
  </verbatim>|Automatic type conversion><label|automatic-type-conversion>

  <subsubsection|Parameterized types>

  <index|parameterized types><subindex|type|parameterized type>Since type
  definitions are just regular rewrites, a type definition may contain a more
  complex pattern on the left of the rewrite. This is illustrated in
  Figure<nbsp><reference|parameterized-type>, where we define a
  <verbatim|one_modulo N> type that generalizes the <verbatim|odd> type.

  <big-figure|<\verbatim>
    one_modulo N:integer -\<gtr\>

    \ \ \ \ contains X:integer -\<gtr\> X mod N = 1

    \ \ \ \ contains X -\<gtr\> false

    show X:(one_modulo 1)
  </verbatim>|Parameterized type><label|parameterized-type>

  It is also possible to define tree forms that are neither name nor prefix.
  Figure<nbsp><reference|infix-type> shows how we can use an infix form with
  the <verbatim|..> operator to declare a range type.

  <big-figure|<\verbatim>
    Low:integer..High:integer -\<gtr\>

    \ \ \ \ contains X:integer -\<gtr\> X\<gtr\>=Low and X\<less\>=High

    \ \ \ \ contains X -\<gtr\> false

    foo X:1..5 -\<gtr\> write X
  </verbatim>|Declaring a range type using an infix form><label|infix-type>

  <subsubsection|Rewrite types>

  <index|rewrite type><subindex|type|rewrite type>The infix
  <verbatim|-\<gtr\>> operator can be used in a type definition to identify
  specific forms of rewrites that perform a particular kind of tree
  transformation. Figure<nbsp><reference|rewrite-type> illustrates this usage
  to declare an <verbatim|adder> type that will only match rewrites declaring
  an infix <verbatim|+> node:

  <big-figure|<\verbatim>
    adder -\<gtr\> type {X+Y -\<gtr\> Z}
  </verbatim>|Declaration of a rewrite type><label|rewrite-type>

  <section|Standard XL library>

  <index|library>The ELIOT language is intentionally very simple, with a
  strong focus on how to extend it rather than on built-in features. Most
  features that would be considered fundamental in other languages are
  implemented in the library in ELIOT. Implementing basic amenities that way
  is an important proof point to validate the initial design objective,
  extensibility of the language.

  <\warning>
    This describes the standard XL library for the core, text-only
    implementation of XL found in the open-source implementation. Since there
    is no real difference between built-in functions and library definitions,
    the XL language can be ``embedded'' in an application that will provide a
    much richer vocabulary. In particular, users of Tao Presentations should
    refer to the Tao Presentations on-line documentation for information
    about features specific to this product, such as 3D graphics, regular
    expressions, networking, etc.
  </warning>

  <subsection|Built-in operations><label|built-ins>

  <index|built-in operations>A number of operations are defined by the core
  run-time of the language, and appear in the context used to evaluate any
  ELIOT program.

  This section decsribes the minimum list of operations available in any
  ELIOT program. Operator priorities are defined by the
  <verbatim|xl.syntax><index|xl.syntax> file in
  Figure<nbsp><reference|syntax-file>. All operations listed in this section
  may be implemented specially in the compiler, or using regular rewrite
  rules defined in a particular file called
  <verbatim|builtins.xl><index|builtins.xl> that is loaded by ELIOT before
  evaluating any program, or a combination of both.

  <subsubsection|Arithmetic>

  <index|arithmetic>Arithmetic operators for <verbatim|integer> and
  <verbatim|real> values are listed in Table<nbsp><reference|arithmetic>,
  where <verbatim|x> and <verbatim|y> denote integer or real values.
  Arithmetic operators take arguments of the same type and return an argument
  of the same type. In addition, the power<index|power operator> operator
  <strong|^> can take a first <verbatim|real> argument and an
  <verbatim|integer> second argument.

  <big-table|<block*|<tformat|<table|<row|<cell|<strong|Form>>|<cell|<strong|Description>>>|<row|<cell|<verbatim|x+y>>|<cell|Addition>>|<row|<cell|<verbatim|x-y>>|<cell|Subtraction>>|<row|<cell|<verbatim|x*y>>|<cell|Multiplication>>|<row|<cell|<verbatim|x/y>>|<cell|Division>>|<row|<cell|<verbatim|x
  rem y>>|<cell|Remainder>>|<row|<cell|<verbatim|x mod
  y>>|<cell|Modulo>>|<row|<cell|<verbatim|x^y>>|<cell|Power>>|<row|<cell|<verbatim|-x>>|<cell|Negation>>|<row|<cell|<verbatim|x%>>|<cell|Percentage
  (<verbatim|x/100.0)>>>|<row|<cell|<verbatim|x!>>|<cell|Factorial>>>>>|Arithmetic
  operations><label|arithmetic>

  <subsubsection|Comparison>

  <index|comparisons>Comparison operators can take <verbatim|integer>,
  <verbatim|real> or <verbatim|text> argument, both arguments being of the
  same type, and return a <verbatim|boolean> argument, which can be either
  <verbatim|true> or <verbatim|false>. Text is compared using the
  lexicographic order<\footnote>
    There is currently no locale-dependent text comparison.
  </footnote>.

  <big-table|<block*|<tformat|<table|<row|<cell|<strong|Form>>|<cell|<strong|Description>>>|<row|<cell|<verbatim|x=y>>|<cell|Equal>>|<row|<cell|<verbatim|x\<less\>\<gtr\>y>>|<cell|Not
  equal>>|<row|<cell|<verbatim|x\<less\>y>>|<cell|Less-than>>|<row|<cell|<verbatim|x\<gtr\>y>>|<cell|Greater
  than>>|<row|<cell|<verbatim|x\<less\>=y>>|<cell|Less or
  equal>>|<row|<cell|<verbatim|x\<gtr\>=y>>|<cell|Greater or
  equal>>>>>|Comparisons><label|comparisons>

  <subsubsection|Bitwise arithmetic>

  <index|bitwise arithmetic><subindex|arithmetic|bitwise>Bitwise operators
  operate on the binary representation of <verbatim|integer> values, treating
  each bit indivudally.

  <big-table|<block*|<tformat|<table|<row|<cell|<strong|Form>>|<cell|<strong|Description>>>|<row|<cell|<verbatim|x
  shl y>>|<cell|Shift <verbatim|x> left by <verbatim|y>
  bits>>|<row|<cell|<verbatim|x shr y>>|<cell|Shift <verbatim|x> right by
  <verbatim|y> bits>>|<row|<cell|<verbatim|x and y>>|<cell|Bitwise
  and>>|<row|<cell|<verbatim|x or y>>|<cell|Bitwise
  or>>|<row|<cell|<verbatim|x xor y>>|<cell|Bitwise exclusive
  or>>|<row|<cell|<verbatim|not x>>|<cell|Bitwise complement>>>>>|Bitwise
  arithmetic operations><label|bitwise-arithmetic>

  <subsubsection|Boolean operations>

  <index|boolean>Boolean operators operate on the names <verbatim|true> and
  <verbatim|false>.

  <big-table|<block*|<tformat|<table|<row|<cell|<strong|Form>>|<cell|<strong|Description>>>|<row|<cell|<verbatim|x=y>>|<cell|Equal>>|<row|<cell|<verbatim|x\<less\>\<gtr\>y>>|<cell|Not
  equal>>|<row|<cell|<verbatim|x and y>>|<cell|Logical
  and>>|<row|<cell|<verbatim|x or y>>|<cell|Logical
  or>>|<row|<cell|<verbatim|x xor y>>|<cell|Logical exclusive
  or>>|<row|<cell|<verbatim|not x>>|<cell|Logical not>>>>>|Boolean
  operations><label|boolean-operations>

  <subsubsection|Mathematical functions>

  <index|mathematical functions>Mathematical functions operate on
  <verbatim|real> numbers. The <verbatim|random> function can also take two
  <verbatim|integer> arguments, in which case it returns an
  <verbatim|integer> value.

  <big-table|<block*|<tformat|<table|<row|<cell|<strong|Form>>|<cell|<strong|Description>>>|<row|<cell|<tformat|<table|<row|<cell|<verbatim|sqrt
  x>>>>>>|<cell|Square root>>|<row|<cell|<verbatim|sin
  x>>|<cell|Sine>>|<row|<cell|<verbatim|cos
  x>>|<cell|Cosine>>|<row|<cell|<verbatim|tan
  x>>|<cell|Tangent>>|<row|<cell|<verbatim|asin
  x>>|<cell|Arc-sine>>|<row|<cell|<verbatim|acos
  x>>|<cell|Arc-cosine>>|<row|<cell|<verbatim|atan
  x>>|<cell|Arc-tangent>>|<row|<cell|<verbatim|atan(y,x)>>|<cell|Coordinates
  arc-tangent (<verbatim|atan2> in C)>>|<row|<cell|<verbatim|exp
  x>>|<cell|Exponential>>|<row|<cell|<verbatim|expm1 x>>|<cell|Exponential
  minus one>>|<row|<cell|<verbatim|log x>>|<cell|Natural
  logarithm>>|<row|<cell|<verbatim|log2 x>>|<cell|Base 2
  logarithm>>|<row|<cell|<verbatim|log10 x>>|<cell|Base 10
  logarithm>>|<row|<cell|<verbatim|log1p x>>|<cell|Log of one plus
  x>>|<row|<cell|<verbatim|pi>>|<cell|Numerical constant
  <math|\<pi\>>>>|<row|<cell|<verbatim|random>>|<cell|A random value between
  0 and 1>>|<row|<cell|<verbatim|random x,y>>|<cell|A random value between
  <verbatim|x> and <verbatim|y>>>>>>|Mathematical
  operations><label|math-operations>

  <subsubsection|Text functions>

  <index|text functions>Text functions operate on <verbatim|text> values.

  <big-table|<block*|<tformat|<table|<row|<cell|<strong|Form>>|<cell|<strong|Description>>>|<row|<cell|<tformat|<table|<row|<cell|<verbatim|x&y>>>>>>|<cell|Concatenation>>|<row|<cell|<verbatim|text_length
  x>>|<cell|Length of the text>>|<row|<cell|<verbatim|text_range t, start,
  len>>|<cell|Range of characters in <verbatim|t>>>|<row|<cell|<verbatim|t[n]>>|<cell|Character
  at index <verbatim|n>>>|<row|<cell|<verbatim|t[n1..n2]>>|<cell|Characters
  in range <verbatim|n1..n2>>>>>>|Text operations><label|text-operations>

  The first character in a text is numbered <verbatim|0>.

  <subsubsection|Conversions>

  <index|conversions><subindex|type|conversions>Conversions operations
  transform data from one type to another.

  <big-table|<block*|<tformat|<table|<row|<cell|<strong|Form>>|<cell|<strong|Description>>>|<row|<cell|<tformat|<table|<row|<cell|<verbatim|real
  x:integer>>>>>>|<cell|Convert integer to real>>|<row|<cell|<verbatim|real
  x:text>>|<cell|Convert text to real>>|<row|<cell|<verbatim|integer
  x:real>>|<cell|Convert real to integer>>|<row|<cell|<verbatim|integer
  x:text>>|<cell|Convert text to real>>|<row|<cell|<verbatim|text
  x:integer>>|<cell|Convert integer to text>>|<row|<cell|<verbatim|text
  x:real>>|<cell|Convert real to text>>|<row|<cell|<verbatim|text
  n:name>>|<cell|Convert name to text>>|<row|<cell|<verbatim|name
  t:text>>|<cell|Convert text to name>>>>>|Conversions><label|conversions>

  <subindex|conversion|from text to number><subindex|conversion|from number
  to text>A conversion from text that fails returns the value <verbatim|0>.
  Conversions to text always use the format used for ELIOT source code, using
  dot as a decimal separator: <verbatim|text 0.0> is <verbatim|"0.0">.

  <subsubsection|Date and time>

  <index|date and time>Date and time functions manipulates time. Time is
  expressed with an integer representing a number of seconds since a time
  origin. Except for <verbatim|system_time> which never takes an argument,
  the functions can either take an explicit time represented as an
  <verbatim|integer> as returned by <verbatim|system_time>, or apply to the
  current time in the current time zone.

  <big-table|<block*|<tformat|<table|<row|<cell|<strong|Form>>|<cell|<strong|Description>>>|<row|<cell|<tformat|<table|<row|<cell|<verbatim|hours>>>>>>|<cell|Hours>>|<row|<cell|<verbatim|minutes>>|<cell|Minutes>>|<row|<cell|<verbatim|seconds>>|<cell|Seconds>>|<row|<cell|<verbatim|year>>|<cell|Year>>|<row|<cell|<verbatim|month>>|<cell|Month>>|<row|<cell|<verbatim|day>>|<cell|Day
  of the month>>|<row|<cell|<verbatim|week_day>>|<cell|Day of the
  week>>|<row|<cell|<verbatim|year_day>>|<cell|Day of the
  year>>|<row|<cell|<verbatim|system_time>>|<cell|Current time in
  seconds>>>>>|Date and time><label|time-operations>

  <subsubsection|Tree operations><label|tree-operations>

  <subindex|tree|operations>Tree operations allow direct manipulation of
  abstract syntax trees<subindex|AST|manipulations>.

  <big-table|<block*|<tformat|<table|<row|<cell|<strong|Form>>|<cell|<strong|Description>>>|<row|<cell|<tformat|<table|<row|<cell|<verbatim|identity
  x>>>>>>|<cell|Returns <verbatim|x>>>|<row|<cell|<verbatim|do
  x>>|<cell|Forces explicit evaluation of
  <verbatim|x>>>|<row|<cell|<verbatim|x.y>>|<cell|Evaluate <verbatim|y> in
  context of <verbatim|x>>>|<row|<cell|<verbatim|self>>|<cell|The input form
  in a rewrite implementation>>|<row|<cell|<verbatim|left X>, <verbatim|right
  X>>|<cell|Left and right child for infix, prefix,
  postfix>>|<row|<cell|<verbatim|child X>>|<cell|Child of a
  block>>|<row|<cell|<verbatim|symbol X>>|<cell|Symbol for an infix or name
  as text>>|<row|<cell|<verbatim|opening X>, <verbatim|closing
  X>>|<cell|Opening and closing of text or blocks>>>>>|Tree
  operations><label|tree-operations-table>

  The prefix <verbatim|left><index|left>, <verbatim|right<index|right>>,
  <verbatim|child><index|child>, <verbatim|symbol><index|symbol>,
  <verbatim|opening><index|opening> and <verbatim|closing><index|closing> can
  be assigned to, as described in Section<nbsp><reference|assignment>.

  <subsubsection|List operations, map, reduce and
  filter><label|list-operations>

  <index|list operations><subindex|list|operations on lists>By convention,
  ELIOT lists use comma-separated lists<subindex|list|comma-separated>, such
  as <verbatim|1,3,5,6>, although similar operations can be built with any
  other data structure. The map, reduce and filter operations act on such
  lists. They also can take a range <verbatim|Low..High> as input. An empty
  list is represented by the name <verbatim|nil>. Basic list operations are
  shown in Table<nbsp><reference|list-operations-table>:

  <big-table|<block*|<tformat|<table|<row|<cell|<strong|Form>>|<cell|<strong|Description>>>|<row|<cell|<tformat|<table|<row|<cell|<verbatim|nil>>>>>>|<cell|The
  empty list>>|<row|<cell|<verbatim|head,tail>>|<cell|A data form for
  lists>>|<row|<cell|<verbatim|length L>>|<cell|The length of list
  <verbatim|L>>>|<row|<cell|<verbatim|map F L>>|<cell|Map function
  <verbatim|F> to list <verbatim|L>>>|<row|<cell|<verbatim|reduce F
  L>>|<cell|Combine list elements in a single
  value>>|<row|<cell|<verbatim|filter F L>>|<cell|Filter elements of a
  list>>|<row|<cell|<verbatim|x with y>>|<cell|Convenience notation for Map,
  Reduce or Filter>>|<row|<cell|<verbatim|x..y>>|<cell|Create a range of
  elements between <verbatim|x> and <verbatim|y>>>|<row|<cell|<verbatim|head
  L> or <verbatim|L.head>>|<cell|Head of the list>>|<row|<cell|<verbatim|tail
  L> or <verbatim|L.tail>>|<cell|Tail of the list (all but first
  element)>>|<row|<cell|<verbatim|L1 & L2>>|<cell|Concatenation of
  lists>>>>>|List operations><label|list-operations-table>

  The <em|map><index|map operation> operation builds a list by applying the
  first argument as a prefix to each element of the list in turn. For
  example, <verbatim|map foo (1,3,5)> returns the list <verbatim|foo 1, foo
  3, foo 5>. Map can be used with anonymous functions: <verbatim|map
  (x-\<gtr\>x+1) (2,4,6)> returns <verbatim|(2+1,4+1,6+1)>.

  The <em|reduce><index|reduce operation> operation, sometimes called
  <em|fold> or <em|accumulate> in other functional languages, combines
  elements of the list two by two using a binary operation, and returns a
  single result. For example, <verbatim|reduce (x,y-\<gtr\>x+y) (1,3,5)>
  returns <verbatim|1+3+5>.

  The <em|filter><index|filter operation> operation takes a
  predicate<index|predicate> (i.e. a function taking a single argument and
  returning a <verbatim|boolean>) and a list, and returns elements of the
  list for which the predicate returns <verbatim|true>. For example,
  <verbatim|filter (x-\<gtr\>x\<less\>10) (1,12,17,2)> returns
  <verbatim|(1,2)>.

  The notation <verbatim|(X where Predicate X) with L> corresponds to a
  filter operation on list <verbatim|L> with predicate <verbatim|Predicate>.
  For example, <verbatim|(X where X\<less\>10) with (1,12,17,2)> returns
  <verbatim|(1,2)>.

  The notation <verbatim|(X,Y -\<gtr\> ...) with L> corresponds to a reduce
  operation on list <verbatim|L>. For example, <verbatim|(X,Y -\<gtr\> X+Y)
  with (2,4,6)> returns <verbatim|(2+1,4+1,6+1)>.

  For other forms of <verbatim|F>, the notation <verbatim|F with L>
  corresponds to a map operation on list <verbatim|L>. For example,
  <verbatim|sin with (1,3,5)> returns <verbatim|sin 1, sin 3, sin 5>.

  The notation <verbatim|x..y> is called a <em|range><index|range>. A range
  of integer, real numbers or text can be used as a type. A range of integers
  can also be used as a lazy enumeration of all elements as a comma-separated
  list. In other words, <verbatim|1..5> is a short-hand notation for
  <verbatim|1,2,3,4,5>.

  <subsection|Control structures>

  Control structures such as tests and loops are implemented in the ELIOT
  standard library.

  <\warning>
    The control structures described below are not necessarily all
    implemented at all optimization levels. Future implementations will add
    new control structures as soon as the compiler becomes smart enough to
    generate correct code for the definitions given in this section.
  </warning>

  <subsubsection|Tests>

  The defintion of the if-then-else statement in the library is as shown in
  Figure<nbsp><reference|if-then-else-definition><subindex|if-then-else|library
  definition>:

  <big-figure|<\verbatim>
    // Declaration of if-then-else

    if true then TrueClause else FalseClause \ \ -\<gtr\> TrueClause

    if false then TrueClause else FalseClause \ -\<gtr\> FalseClause
  </verbatim>|Library definition of if-then-else>

  <label|if-then-else-definition>This definition requires the value to be a
  <verbatim|boolean>, i.e. <verbatim|true> or <verbatim|false>. The
  <verbatim|good><index|good (function)> \ function shown in
  Figure<nbsp><reference|good-function> provides a behavior closer to what is
  seen in languages such as C, where the value <verbatim|0> is logically
  false and non-zero values are logically true.

  <\big-figure>
    <\verbatim>
      good false -\<gtr\> false

      good 0 \ \ \ \ -\<gtr\> false

      good 0.0 \ \ -\<gtr\> false

      good "" \ \ \ -\<gtr\> false

      good nil \ \ -\<gtr\> false

      good Other -\<gtr\> true
    </verbatim>

    \;
  </big-figure|The <verbatim|good> function>

  <label|good-function>It is possible to add declarations of <verbatim|good>
  for other data types. Such local declarations will precede the declarations
  for <verbatim|good> in scoping order, so that they override that
  ``default'' implementation of <verbatim|good> shown above.

  <subsubsection|Infinite Loops>\ 

  The ELIOT standard library provides a number of loop constructs.
  Figure<nbsp><reference|infinite-loop> shows an implementation for the
  simplest form of loop, the infinite loop. The repeated evaluation of
  <verbatim|Body> illustrates the importance of explicit evaluation (see
  Section<nbsp><reference|explicit-evaluation>). Note that such a recursive
  implementation is only efficient if tail recursion optimization works
  correctly (see Section<nbsp><reference|tail-recursion>).

  <big-figure|<\verbatim>
    loop Body -\<gtr\>

    \ \ \ \ Body

    \ \ \ \ loop Body
  </verbatim>|Infinite loop>

  <subsubsection|Conditional Loops (<verbatim|while> and <verbatim|until>
  loops)>

  <label|infinite-loop>Figure<nbsp><reference|while-loop> shows an
  implementation for the <verbatim|while> loop, which runs while a given
  condition is true. Explicit evaluation is not required for
  <verbatim|Condition> because it is evaluated only once in the
  implementation of the <verbatim|while> loop, preventing memoization of
  <verbatim|Condition> \ (see Section<nbsp><reference|lazy-evaluation>). The
  parameter <verbatim|Condition> is not given a <verbatim|boolean> type
  because we want the expression, not the result of evaluating that
  expression.

  <big-figure|<\verbatim>
    while Condition loop Body -\<gtr\>

    \ \ \ \ if Condition then

    \ \ \ \ \ \ \ \ Body

    \ \ \ \ \ \ \ \ while Condition loop Body
  </verbatim>|While loop>

  <label|while-loop>The <verbatim|until> loop shown in
  Figure<nbsp><reference|until-loop> is very similar to the <verbatim|while>
  loop except that it stops when the condition becomes <verbatim|true>
  instead of <verbatim|false>.

  <big-figure|<\verbatim>
    until Condition loop Body -\<gtr\>

    \ \ \ \ if not Condition then

    \ \ \ \ \ \ \ \ Body

    \ \ \ \ \ \ \ \ until Condition loop Body
  </verbatim>|Until loop>

  <subsubsection|Controlled Loops (<verbatim|for> loops)>

  <label|until-loop>The <verbatim|for> loop is the most complex kind of loop.
  It exists in multiple variants. The simplest one, shown in
  Figure<nbsp><reference|for-loop-integer-range>, iterates over a range of
  <verbatim|integer> values. Notice that it creates a local <verbatim|Index>
  variable to ensure it doesn't modify <verbatim|Variable> unless the loop is
  actually executed.

  <big-figure|<\verbatim>
    for Variable in Low:integer..High:integer loop Body -\<gtr\>

    \ \ \ \ Index : integer := Low

    \ \ \ \ while Index \<less\> High loop

    \ \ \ \ \ \ \ \ Variable := Index

    \ \ \ \ \ \ \ \ Body

    \ \ \ \ \ \ \ \ Index := Index + 1
  </verbatim>|For loop on an integer range>

  <label|for-loop-integer-range>The <verbatim|for> loop shown in
  Figure<nbsp><reference|for-loop-container> iterates on all elements of a
  container such as a list or a range. It updates its <verbatim|Variable> for
  each iteration with a new element in the container.

  <big-figure|<\verbatim>
    for Variable in Container loop Body -\<gtr\>

    \ \ \ \ C : tree := Container \ \ \ 

    \ \ \ \ while good C loop

    \ \ \ \ \ \ \ \ Variable := head C

    \ \ \ \ \ \ \ \ Body

    \ \ \ \ \ \ \ \ C := tail C
  </verbatim>|For loop on a container>

  <label|for-loop-container>There are several other kinds of <verbatim|for>
  loops, corresponding to the patterns shown in
  Figure<nbsp><reference|other-for-loops>:

  <big-figure|<\verbatim>
    for Variable in Low:integer..High:integer step Step:integer

    for Variable in Low:real..High:real

    for Variable in Low:real..High:real step Step:real
  </verbatim>|Other kinds of <verbatim|for> loop>

  <label|other-for-loops>It is not difficult to create custom for loops to
  explore other data structures.

  <\warning>
    The standard mode implements hard-coded <verbatim|for> loops. The
    optimized mode is not currently powerful enough to handle <verbatim|for>
    loop definitions properly.
  </warning>

  <subsubsection|Excursions<strong|>>

  \;

  <subsubsection|Error handling>

  \;

  <subsection|Library-defined types>

  A variety of types are defined in the library.

  <subsubsection|Range and range types>

  The notation <verbatim|low..high> defines a <em|range>. A range can be used
  as a list by list operations, as explained in
  Section<nbsp><reference|list-operations>, but also as a type. The range
  type <verbatim|low..high> accepts all values between <verbatim|low> and
  <verbatim|high> included. It is defined in a way substantially equivalent
  to Figure<nbsp><reference|range-type-definition>:

  <\big-figure|<\verbatim>
    low..high -\<gtr\>

    \ \ \ \ contains X -\<gtr\> X\<gtr\>=low and X\<less\>=high

    \ \ \ \ self
  </verbatim>>
    Range and range type definition
  </big-figure>

  <label|range-type-definition>Arithmetic operations are also defined on
  ranges of <verbatim|integer> and <verbatim|real> numbers, and operate
  simultaneously on the <verbatim|low> and <verbatim|high> part of the range.
  When <verbatim|low> and <verbatim|high> are <verbatim|real>, operations are
  performed with different rounding for <verbatim|low> and <verbatim|high>,
  so as to implement proper interval arithmetic<index|interval arithmetic>.

  Ranges of <verbatim|integer> can also be interpreted as lists, with
  <verbatim|head> and <verbatim|tail> operations implemented in a way
  substantially similar to Figure<nbsp><reference|ranges-as-lists>. Lazy
  evaluation ensures that very large ranges can be processed efficiently (see
  Section<nbsp><reference|infinite-data-structures>).

  <big-figure|<\verbatim>
    head low:integer..high:integer -\<gtr\> if low \<less\>= high then low
    else nil

    tail low:integer..high:integer -\<gtr\> if low \<less\> \ high then
    low+1..high else nil
  </verbatim>|Ranges as lists>

  <label|ranges-as-lists>A test is required to deal with the corner case of
  empty lists.

  <\warning>
    The <verbatim|range> type is not currently implemented, pending
    improvements in the type system.
  </warning>

  <subsubsection|Union types>

  The notation <verbatim|A\|B> in types is a <em|union type> for <verbatim|A>
  and <verbatim|B>, i.e. a type that can accept any element of types
  <verbatim|A> or <verbatim|B>. It is pre-defined in the standard library as
  in Figure<nbsp><reference|union-type-definition>:

  <big-figure|<\verbatim>
    A\|B -\<gtr\>

    \ \ \ \ contains X:A -\<gtr\> true

    \ \ \ \ contains X:B -\<gtr\> true

    \ \ \ \ contains X -\<gtr\> false
  </verbatim>|Union type definition><label|union-type-definition>

  Union types facilitate the definition of functions that work correctly on a
  multiplicity of data types, but not necessarily all of them, as shown in
  Figure<nbsp><reference|using-union-types>:

  <big-figure|<\verbatim>
    number \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ -\<gtr\> type X:(integer\|real)

    succ X:number \ \ \ \ \ \ \ \ -\<gtr\> X + 1

    pred X:(integer\|real) -\<gtr\> X-1
  </verbatim>|Using union types><label|using-union-types>

  <\warning>
    Union types are not implementede yet, pending improvements in the type
    system.
  </warning>

  <subsubsection|Enumeration types>

  An <em|enumeration type> accepts names in a predefined set. The notation
  <verbatim|enumeration(A, B, C)> corresponds to an enumeration accepting the
  names <verbatim|A>, <verbatim|B>, <verbatim|C>... This notation is
  pre-defined in the standard library as in
  Figure<nbsp><reference|enum-type-definition>:

  <big-figure|<\verbatim>
    enumeration(A:name,Rest) -\<gtr\>

    \ \ \ \ contains X:name -\<gtr\> text X = text A or enumeration(Rest)
    contains X

    \ \ \ \ contains X -\<gtr\> false
  </verbatim>|Enumeration type definition><label|enum-type-definition>

  Unlike in other languages, enumeration types are not distinct from one
  another and can overlap. For example, the name <verbatim|do> belongs to
  <verbatim|enumeration(do,undo,redo)> as well as to
  <verbatim|enumeration(do,re,mi,fa,sol,la,si)>. Also, as this enumeration
  example demonstrates, enumerations can use names such as <verbatim|do> that
  are also used by standard prefix functions.

  <\warning>
    Enumeration types are not implemented yet.
  </warning>

  <subsubsection|A type matching type declarations>

  Type declarations in a type definition are used to declare actual types, so
  a type that matches type declarations cannot be defined by a simple
  pattern. Figure<nbsp><reference|type-declaration-type> shows how the
  standard library defines a <verbatim|type_declaration> type using a type
  check.

  <big-figure|<\verbatim>
    type type_declaration -\<gtr\>

    \ \ \ contains X:infix -\<gtr\> X.name = ":"

    \ \ \ contains X -\<gtr\> false
  </verbatim>|Type matching a type declaration><label|type-declaration-type>

  <\warning>
    This definition of <verbatim|type_declaration> does not work yet, pending
    improvements in the type system implementation.
  </warning>

  <subsection|Modules>

  ELIOT modules make it possible to decompose a large ELIOT program in
  smaller units.

  <subsubsection|Import statement><label|import-statement>

  The <verbatim|import> prefix imports a source file or a
  module<index|module><index|import><subindex|module|import>, as shown in
  Figure<nbsp><reference|import-statement-example>:

  <big-figure|<\verbatim>
    import "file.xl"

    import MyModule

    import OtherModule 1.2

    import MOD = LongMessage 1.3
  </verbatim>|Import statements examples>

  <label|import-statement-example>An import statement can be followed by a
  file name or a module specification:

  <\itemize>
    <item>A file name provides a system-dependent file name for an ELIOT
    source file. By convention, ELIOT source file names end in
    <verbatim|.xl>. In order to improve compatibility between systems,
    backslash characters <verbatim|\\> in file names are converted to slash
    characters <verbatim|/> on Unix systems, and slash characters in file
    names are converted to backslash on Windows. Drive specifications such as
    <verbatim|C:> are not converted.

    <item>A module can also be identified by a name, optionally followed by a
    real number representing a minimum required version number. Modules
    source files are located in a set of directories defined by a <em|module
    path><index|module path>, and contain special module declarations
    specifying the module import name and the version number.

    <item>Finally, the module being imported can locally be given a short
    name<index|short module name><subindex|import|with a short name> with the
    syntax <verbatim|import M=ModSpec>. In that case, the contents of the
    module is only visible using the index notation with either the short
    name <verbatim|M> or the long module name.
  </itemize>

  Importing a module or file has the following effects:

  <\enumerate>
    <item>Any <verbatim|syntax> statement<subindex|syntax|in modules> in the
    imported module applies to the source code importing it.

    <item>A scope<subindex|scope|for modules> is created and populated with
    all declarations in the module.

    <item>Except if a short name is given, that scope is placed immediately
    to the right of the current context. In other words, it potentially
    shadows previously imported modules, but also is potentially
    shadowed<subindex|shadowing|in modules> by declarations in the current
    file.

    <item>If the module is identified by a name and not a file name, a
    binding<subindex|binding|of module names> of that module name to the
    newly created module scope, and another binding to the short name in case
    one was provided.
  </enumerate>

  <\warning>
    In the current implementation, the <verbatim|import> statement does not
    make the syntax visible yet.
  </warning>

  The rationale for these rules is to make different usage scenarios equally
  convenient:

  <\itemize>
    <item>If declarations in a module are going to be used extensively, using
    <verbatim|import Module> makes all declarations visible by default.

    <item>If a local declaration <verbatim|Foo> hides a declaration of
    <verbatim|Foo> in the module, it is still possible to refer to the
    module's declaration as <verbatim|Module.Foo>.

    <item>If it is undesirable to see declarations from the module, using
    <verbatim|import M=Module> will prevent the module from becoming visible,
    but will make it convenient to refer to entities declared in the module
    using the short name, as in <verbatim|M.Foo>.
  </itemize>

  <subsubsection|Declaring a module>

  A module is identified by a module description<index|module
  description><subindex|module|description> similar to
  Figure<nbsp><reference|module-definition>:

  <big-figure|<\verbatim>
    module_description

    \ \ \ \ id "B1E18CF6-0E3E-4992-98AD-0FD998C9C9CB"

    \ \ \ \ name "My Incredible Module"

    \ \ \ \ description "This is an example of module"

    \ \ \ \ import_name "MyModule"

    \ \ \ \ author "John Doe"

    \ \ \ \ website "http://www.taodyne.com"

    \ \ \ \ url "git://git.taodyne.com/MyModule"

    \ \ \ \ dependencies BaseLibrary 1.1, ELIOT 0.9

    \ \ \ \ version 1.0
  </verbatim>|Module definition>

  <label|module-definition>The module description contains information
  allowing the ELIOT compiler to identify the modules. Only the
  <verbatim|import_name> is required for that purpose. It is however
  considered good practice to provide the rest of the information, which can
  be used by various applications to provide meaningful information to the
  user, or useful utilities such as module dependency management.

  <section|Example code>

  <subsection|Minimum and maximum>

  The minumum and maximum can be defined as follows:

  <big-figure|<\verbatim>
    min x, y -\<gtr\> m:tree := min y; if m \<less\> x then m else x

    min x \ \ \ -\<gtr\> x

    max x, y -\<gtr\> m:tree := max y; if m \<gtr\> x then m else x

    max x \ \ \ -\<gtr\> x
  </verbatim>|Computing a minimum and a maximum>

  The functions as defined will work with any number of arguments, as well as
  with lists of items.

  <subsection|Complex numbers>

  \;

  <subsection|Vector and Matrix computations>

  <subsection|Linked lists with dynamic allocation>

  \;

  <subsection|Input / Output>

  \;

  <subsection|Object-Oriented Programming><label|object-oriented-programming>

  <subsubsection|Classes>

  <subsubsection|Methods>

  <subsubsection|Dynamic dispatch>

  <subsubsection|Polymorphism>

  <subsubsection|Inheritance>

  <subsubsection|Multi-methods>

  <subsubsection|Object prototypes>

  <subsection|Functional-Programming>

  <subsubsection|Map>

  <subsubsection|Reduce>

  <subsubsection|Filter>

  <subsubsection|Functions as first-class objects>

  <subsubsection|Anonymous functions (Lambda)>

  <\with|font-series|bold>
    <subsubsection|Y-Combinator>
  </with>

  <subsubsection|Infinite data structures><label|infinite-data-structures>

  Since arguments are evaluated lazily, the evaluation of one fragment of the
  form does not imply the evaluation of any other. This makes it possible to
  correctly evaluate infinite data structures, as illustrated in
  Figure<nbsp><reference|infinite-list>.

  <big-figure|<\verbatim>
    integers_above N:integer -\<gtr\> N, integers_above N+1

    head X,Y -\<gtr\> X

    tail X,Y -\<gtr\> Y

    \;

    // This computes 7 without evaluating integers_above 8

    head tail tail tail integers_above 4
  </verbatim>|Lazy evaluation of an infinite list><label|infinite-list>

  <section|Implementation notes><label|implementation-notes>

  This section describes the implementation as published at
  <verbatim|http://xlr.sourceforge.net>.

  <subsection|Lazy evaluation>

  <subsection|Type inference>

  <subsection|Built-in operations>

  <subsection|Controlled compilation>

  A special form, <verbatim|compile>, is used to tell the compiler how to
  compile its argument. This makes it possible to implement special
  optimization for often-used forms.

  <big-figure|<\verbatim>
    compile {if Condition then TrueForm else FalseForm} -\<gtr\>

    \ \ \ \ generate_if_then_else Condition, TrueForm, FalseForm \ \ \ \ 
  </verbatim>|Controlled compilation>

  Controlled compilation depends on low-level compilation primitives provided
  by the LLVM infrastructure<\footnote>
    For details, refer to <verbatim|http://llvm.org>.
  </footnote>, and assumes a good understanding of LLVM basic operations.
  Table<nbsp><reference|llvm-operations> shows the correspondance between
  LLVM primitives and primitives that can be used during controlled
  compilation.

  <big-table|<block*|<tformat|<table|<row|<cell|<strong|ELIOT
  Form>>|<cell|<strong|LLVM Entity>>|<cell|<strong|Description>>>|<row|<cell|<tformat|<table|<row|<cell|<verbatim|llvm_value
  x>>>>>>|<cell|<verbatim|Value *>>|<cell|The machine value associated to
  tree <verbatim|x>>>|<row|<cell|<verbatim|llvm_type x>>|<cell|<verbatim|Type
  *>>|<cell|The type associated to tree <verbatim|x>>>|<row|<cell|<verbatim|llvm_struct
  x>>|<cell|<verbatim|StructType *>>|<cell|The structure type for signature
  <verbatim|x>>>|<row|<cell|<verbatim|llvm_function_type
  x>>|<cell|<verbatim|FunctionType *>>|<cell|The function type for signature
  <verbatim|x>>>|<row|<cell|<verbatim|llvm_function
  x>>|<cell|<verbatim|Function *>>|<cell|The machine function associated to
  <verbatim|x>>>|<row|<cell|<verbatim|llvm_global
  x>>|<cell|<verbatim|GlobalValue *>>|<cell|The global value identifying tree
  <verbatim|x>>>|<row|<cell|<verbatim|llvm_bb n>>|<cell|<verbatim|BasicBlock
  *>>|<cell|A basic block with name <verbatim|n>>>|<row|<cell|<verbatim|llvm_type>>|<cell|>|<cell|>>>>>|LLVM
  operations><label|llvm-operations>

  \;

  <subsection|Tree representation>

  The tree representation is performed by the <verbatim|Tree> class, with one
  subclass per node type: <verbatim|Integer>, <verbatim|Real>,
  <verbatim|Text>, <verbatim|Name>, <verbatim|Infix>, <verbatim|Prefix>,
  <verbatim|Postfix> and <verbatim|Block>.

  The <verbatim|Tree> structure has template members <verbatim|GetInfo> and
  <verbatim|SetInfo> that make it possible to associate arbitrary data to a
  tree. Data is stored there using a class deriving from <verbatim|Info>.

  The rule of thumb is that <verbatim|Tree> only contains members for data
  that is used in the evaluation of any tree. Other data is stored using
  <verbatim|Info> entries.

  Currently, data that is directly associated to the <verbatim|Tree>
  includes:

  <\itemize>
    <item>The <verbatim|tag> field stores the kind of the tree as well as its
    position in the source code.

    <item>The <verbatim|info> field is a linked list of <verbatim|Info>
    entries.
  </itemize>

  <subsection|Evaluation of trees>

  Trees are evaluated in a given <em|context>, representing the evaluation
  environment. The context contains a lexical (static) and stack (dynamic)
  part.

  <\enumerate>
    <item>The <em|lexical context> represents the declarations that precede
    the tree being evaluated in the source code. It can be determined
    statically.

    <item>The <em|dynamic context> represents the declarations that were
    introduced as part of earlier evaluation, i.e. in the ``call stack''.
  </enumerate>

  A context is represented by a tree holding the declarations, along with
  associated code.

  <subsection|Tree position>

  The position held in the <verbatim|tag> field is character-precise. To save
  space, it counts the number of characters since the begining of compilation
  in a single integer value.

  The <verbatim|Positions> class defined in <verbatim|scanner.h> maps this
  count to the more practical file-line-column positioning. This process is
  relatively slow, but this is acceptable when emitting error messages.

  <subsection|Actions on trees>

  Recursive operations on tree are performed by the <verbatim|Action> class.
  This class implements virtual functions for each tree type called
  <verbatim|DoInteger>, <verbatim|DoReal>, <verbatim|DoText> and so on.

  <subsection|Symbols>

  The XL runtime environment maintains symbol tables which form a hierarchy.
  Each symbol table has a (possibly <verbatim|NULL>) parent, and contains two
  kinds of symbols: names and rewrites.

  <\itemize>
    <item>Names are associated directly with a tree value. For example,
    <verbatim|X-\<gtr\>0> will associate the value <verbatim|0> to name
    <verbatim|X>.

    <item>Rewrites are used for more complex tree rewrites, e.g.
    <verbatim|X+Y-\<gtr\>add X,Y>.
  </itemize>

  <subsection|Evaluating trees>

  A tree is evaluated as follows:

  <\enumerate>
    <item>Evaluation of a tree is performed by <verbatim|eliot_evaluate()> in
    <verbatim|runtime.cpp>.

    <item>This function checks the stack depth to report infinite recursion.

    <item>If <verbatim|code> is <verbatim|NULL>, then the tree is compiled
    first.

    <item>Then, evaluation is performed by calling <verbatim|code> with the
    tree as argument.
  </enumerate>

  The signature for <verbatim|code> is a function taking a <verbatim|Tree>
  pointer and returning a <verbatim|Tree> pointer.

  <subsection|Code generation for trees>

  Evaluation functions are functions with the signature shown in
  Figure<nbsp><reference|rewrite-code>:

  <big-figure|<verbatim|Tree * (*eval_fn) (eval_fn eval, Tree
  *self)>|Signature for rewrite code with two variables.><label|rewrite-code>

  Unfortunately, the signature in Figure<nbsp><reference|rewrite-code> is not
  valid in C or C++, so we need a lot of casting to achieve the desired
  effect.

  \;

  In general, the <verbatim|code> for a tree takes the tree as input, and
  returns the evaluated value.

  However, there are a few important exceptions to this rule:

  <subsubsection|Right side of a rewrite>

  If the tree is on the right of a rewrite (i.e. the right of an infix
  <verbatim|-\<gtr\>> operator), then <verbatim|code> will take additional
  input trees as arguments. Specifically, there will be one additional
  parameter in the code per variable in the rewrite rule pattern.

  For example, if a rewrite is <verbatim|X+Y-\<gtr\>foo X,Y>, then the
  <verbatim|code> field for <verbatim|foo X,Y> will have <verbatim|X> as its
  second argument and <verbatim|Y> as its third argument, as shown in
  Figure<nbsp><reference|rewrite-code>.

  \;

  \;

  In that case, the input tree for the actual expression being rewritten
  remains passed as the first argument, generally denoted as <verbatim|self>.

  <subsubsection|Closures>

  If a tree is passed as a <verbatim|tree> argument to a function, then it is
  encapsulated in a <em|closure>. The intent is to capture the environment
  that the passed tree depends on. Therefore, the associated <verbatim|code>
  will take additional arguments representing all the captured values. For
  instance, a closure for <verbatim|write X,Y> that captures variables
  <verbatim|X> and <verbatim|Y> will have the signature shown in
  Figure<nbsp><reference|closure-code>:

  <big-figure|<verbatim|Tree * (*code) (Tree *self, Tree *X, Tree
  *Y)>|Signature for rewrite code with two variables.><label|closure-code>

  At runtime, the closure is represented by a prefix tree with the original
  tree on the left, and the captured values cascading on the right. For
  consistency, the captured values are always on the left of a
  <verbatim|Prefix> tree. The rightmost child of the rightmost
  <verbatim|Prefix> is set to an arbitrary, unused value (specifically,
  <verbatim|false>).

  Closures are built by the function <verbatim|eliot_new_closure>, which is
  generally invoked from generated code. Their <verbatim|code> field is set
  to a function that reads all the arguments from the tree and invokes the
  code with the additional arguments.

  For example, <verbatim|do> takes a <verbatim|tree> argument. When
  evaluating <verbatim|do write X,Y>, the tree given as an argument to
  <verbatim|do> depends on variable <verbatim|X> and <verbatim|Y>, which may
  not be visible in the body of <verbatim|do>. These variables are therefore
  captured in the closure. If its values of <verbatim|X> and <verbatim|Y> are
  <verbatim|42> and <verbatim|Universe>, then <verbatim|do> receives a
  closure for <verbatim|write X,Y> with arguments <verbatim|42> and
  <verbatim|Universe>.

  \;

  <subsection|Tail recursion><label|tail-recursion>

  <subsection|Partial recompilation>

  <subsection|Machine Interface><label|machine-interface>

  <subsection|Machine Types and Normal Types><label|machine-types>

  <new-page*>

  <\table-of-contents|toc>
    <vspace*|1fn><with|font-series|bold|math-font-series|bold|1<space|2spc>Introduction>
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-1><vspace|0.5fn>

    <with|par-left|1.5fn|1.1<space|2spc>Design objectives
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-2>>

    <with|par-left|1.5fn|1.2<space|2spc>Keeping it simple
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-6>>

    <with|par-left|1.5fn|1.3<space|2spc>Examples
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-19>>

    <with|par-left|1.5fn|1.4<space|2spc>Concept programming
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-30>>

    <with|par-left|6fn|From concept to code: a lossy conversion
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-34><vspace|0.15fn>>

    <with|par-left|6fn|Pseudo-metrics <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-35><vspace|0.15fn>>

    <with|par-left|6fn|Influence on ELIOT
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-44><vspace|0.15fn>>

    <with|par-left|1.5fn|1.5<space|2spc>State of the implementation
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-45>>

    <vspace*|1fn><with|font-series|bold|math-font-series|bold|2<space|2spc>Syntax>
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-46><vspace|0.5fn>

    <with|par-left|1.5fn|2.1<space|2spc>Spaces and indentation
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-52>>

    <with|par-left|1.5fn|2.2<space|2spc>Comments and spaces
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-58>>

    <with|par-left|1.5fn|2.3<space|2spc>Literals
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-61>>

    <with|par-left|3fn|2.3.1<space|2spc>Integer constants
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-63>>

    <with|par-left|3fn|2.3.2<space|2spc>Real constants
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-69>>

    <with|par-left|3fn|2.3.3<space|2spc>Text literals
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-75>>

    <with|par-left|3fn|2.3.4<space|2spc>Name and operator
    symbols<flag|index|dark green|key><assign|auto-nr|90><label|auto-90><write|idx|<tuple|<tuple|symbols>|<pageref|auto-90>>>
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-90>>

    <with|par-left|1.5fn|2.4<space|2spc>Structured nodes
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-96>>

    <with|par-left|3fn|2.4.1<space|2spc>Infix nodes
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-103>>

    <with|par-left|3fn|2.4.2<space|2spc>Prefix and postfix nodes
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-105>>

    <with|par-left|3fn|2.4.3<space|2spc>Block nodes
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-112>>

    <with|par-left|1.5fn|2.5<space|2spc>Parsing rules
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-115>>

    <with|par-left|3fn|2.5.1<space|2spc>Precedence
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-121>>

    <with|par-left|3fn|2.5.2<space|2spc>Associativity
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-125>>

    <with|par-left|3fn|2.5.3<space|2spc>Infix versus Prefix versus Postfix
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-127>>

    <with|par-left|3fn|2.5.4<space|2spc>Expression versus statement
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-134>>

    <with|par-left|1.5fn|2.6<space|2spc>Syntax configuration
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-141>>

    <with|par-left|6fn|Format of syntax configuration
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-151><vspace|0.15fn>>

    <vspace*|1fn><with|font-series|bold|math-font-series|bold|3<space|2spc>Language
    semantics> <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-163><vspace|0.5fn>

    <with|par-left|1.5fn|3.1<space|2spc>Tree rewrite operators
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-167>>

    <with|par-left|3fn|3.1.1<space|2spc>Rewrite declarations
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-189>>

    <with|par-left|3fn|3.1.2<space|2spc>Data declaration
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-205>>

    <with|par-left|3fn|3.1.3<space|2spc>Type declaration
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-212>>

    <with|par-left|3fn|3.1.4<space|2spc>Assignment
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-220>>

    <with|par-left|6fn|Local variables <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-227><vspace|0.15fn>>

    <with|par-left|6fn|Assigning to references
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-236><vspace|0.15fn>>

    <with|par-left|6fn|Assigning to parameters
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-239><vspace|0.15fn>>

    <with|par-left|6fn|Assignments as expressions
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-242><vspace|0.15fn>>

    <with|par-left|3fn|3.1.5<space|2spc>Guards
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-245>>

    <with|par-left|3fn|3.1.6<space|2spc>Sequences
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-249>>

    <with|par-left|3fn|3.1.7<space|2spc>Index operators
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-256>>

    <with|par-left|6fn|Comparison with C <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-265><vspace|0.15fn>>

    <with|par-left|3fn|3.1.8<space|2spc>C interface
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-267>>

    <with|par-left|3fn|3.1.9<space|2spc>Machine Interface
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-275>>

    <with|par-left|1.5fn|3.2<space|2spc>Binding References to Values
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-279>>

    <with|par-left|3fn|3.2.1<space|2spc>Context Order
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-282>>

    <with|par-left|3fn|3.2.2<space|2spc>Scoping
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-285>>

    <with|par-left|3fn|3.2.3<space|2spc>Current context
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-293>>

    <with|par-left|3fn|3.2.4<space|2spc>References
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-298>>

    <with|par-left|1.5fn|3.3<space|2spc>Evaluation
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-301>>

    <with|par-left|3fn|3.3.1<space|2spc>Standard evaluation
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-303>>

    <with|par-left|3fn|3.3.2<space|2spc>Special forms
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-314>>

    <with|par-left|3fn|3.3.3<space|2spc>Lazy evaluation
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-317>>

    <with|par-left|3fn|3.3.4<space|2spc>Explicit evaluation
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-323>>

    <with|par-left|3fn|3.3.5<space|2spc>Memoization
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-326>>

    <with|par-left|1.5fn|3.4<space|2spc>Types
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-331>>

    <with|par-left|3fn|3.4.1<space|2spc>Predefined types
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-336>>

    <with|par-left|3fn|3.4.2<space|2spc>Type definition
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-351>>

    <with|par-left|3fn|3.4.3<space|2spc>Normal form for a type
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-369>>

    <with|par-left|3fn|3.4.4<space|2spc>Properties
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-376>>

    <with|par-left|3fn|3.4.5<space|2spc>Data inheritance
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-397>>

    <with|par-left|3fn|3.4.6<space|2spc>Explicit type check
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-402>>

    <with|par-left|3fn|3.4.7<space|2spc>Explicit and automatic type
    conversions <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-412>>

    <with|par-left|3fn|3.4.8<space|2spc>Parameterized types
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-418>>

    <with|par-left|3fn|3.4.9<space|2spc>Rewrite types
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-423>>

    <vspace*|1fn><with|font-series|bold|math-font-series|bold|4<space|2spc>Standard
    XL library> <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-427><vspace|0.5fn>

    <with|par-left|1.5fn|4.1<space|2spc>Built-in operations
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-429>>

    <with|par-left|3fn|4.1.1<space|2spc>Arithmetic
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-433>>

    <with|par-left|3fn|4.1.2<space|2spc>Comparison
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-437>>

    <with|par-left|3fn|4.1.3<space|2spc>Bitwise arithmetic
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-440>>

    <with|par-left|3fn|4.1.4<space|2spc>Boolean operations
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-444>>

    <with|par-left|3fn|4.1.5<space|2spc>Mathematical functions
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-447>>

    <with|par-left|3fn|4.1.6<space|2spc>Text functions
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-450>>

    <with|par-left|3fn|4.1.7<space|2spc>Conversions
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-453>>

    <with|par-left|3fn|4.1.8<space|2spc>Date and time
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-459>>

    <with|par-left|3fn|4.1.9<space|2spc>Tree operations
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-462>>

    <with|par-left|3fn|4.1.10<space|2spc>List operations, map, reduce and
    filter <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-472>>

    <with|par-left|1.5fn|4.2<space|2spc>Control structures
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-482>>

    <with|par-left|3fn|4.2.1<space|2spc>Tests
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-483>>

    <with|par-left|3fn|4.2.2<space|2spc>Infinite Loops
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-488>>

    <with|par-left|3fn|4.2.3<space|2spc>Conditional Loops
    (<with|font-family|tt|language|verbatim|while> and
    <with|font-family|tt|language|verbatim|until> loops)
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-490>>

    <with|par-left|3fn|4.2.4<space|2spc>Controlled Loops
    (<with|font-family|tt|language|verbatim|for> loops)
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-493>>

    <with|par-left|3fn|4.2.5<space|2spc>Excursions<with|font-series|bold|math-font-series|bold|>
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-497>>

    <with|par-left|3fn|4.2.6<space|2spc>Error handling
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-498>>

    <with|par-left|1.5fn|4.3<space|2spc>Library-defined types
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-499>>

    <with|par-left|3fn|4.3.1<space|2spc>Range and range types
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-500>>

    <with|par-left|3fn|4.3.2<space|2spc>Union types
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-504>>

    <with|par-left|3fn|4.3.3<space|2spc>Enumeration types
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-507>>

    <with|par-left|3fn|4.3.4<space|2spc>A type matching type declarations
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-509>>

    <with|par-left|1.5fn|4.4<space|2spc>Modules
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-511>>

    <with|par-left|3fn|4.4.1<space|2spc>Import statement
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-512>>

    <with|par-left|3fn|4.4.2<space|2spc>Declaring a module
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-524>>

    <vspace*|1fn><with|font-series|bold|math-font-series|bold|5<space|2spc>Example
    code> <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-528><vspace|0.5fn>

    <with|par-left|1.5fn|5.1<space|2spc>Minimum and maximum
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-529>>

    <with|par-left|1.5fn|5.2<space|2spc>Complex numbers
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-530>>

    <with|par-left|1.5fn|5.3<space|2spc>Vector and Matrix computations
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-531>>

    <with|par-left|1.5fn|5.4<space|2spc>Linked lists with dynamic allocation
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-532>>

    <with|par-left|1.5fn|5.5<space|2spc>Input / Output
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-533>>

    <with|par-left|1.5fn|5.6<space|2spc>Object-Oriented Programming
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-534>>

    <with|par-left|3fn|5.6.1<space|2spc>Classes
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-535>>

    <with|par-left|3fn|5.6.2<space|2spc>Methods
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-536>>

    <with|par-left|3fn|5.6.3<space|2spc>Dynamic dispatch
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-537>>

    <with|par-left|3fn|5.6.4<space|2spc>Polymorphism
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-538>>

    <with|par-left|3fn|5.6.5<space|2spc>Inheritance
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-539>>

    <with|par-left|3fn|5.6.6<space|2spc>Multi-methods
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-540>>

    <with|par-left|3fn|5.6.7<space|2spc>Object prototypes
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-541>>

    <with|par-left|1.5fn|5.7<space|2spc>Functional-Programming
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-542>>

    <with|par-left|3fn|5.7.1<space|2spc>Map
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-543>>

    <with|par-left|3fn|5.7.2<space|2spc>Reduce
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-544>>

    <with|par-left|3fn|5.7.3<space|2spc>Filter
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-545>>

    <with|par-left|3fn|5.7.4<space|2spc>Functions as first-class objects
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-546>>

    <with|par-left|3fn|5.7.5<space|2spc>Anonymous functions (Lambda)
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-547>>

    <with|par-left|3fn|5.7.6<space|2spc>Y-Combinator
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-548>>

    <with|par-left|3fn|5.7.7<space|2spc>Infinite data structures
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-549>>

    <vspace*|1fn><with|font-series|bold|math-font-series|bold|6<space|2spc>Implementation
    notes> <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-551><vspace|0.5fn>

    <with|par-left|1.5fn|6.1<space|2spc>Lazy evaluation
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-552>>

    <with|par-left|1.5fn|6.2<space|2spc>Type inference
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-553>>

    <with|par-left|1.5fn|6.3<space|2spc>Built-in operations
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-554>>

    <with|par-left|1.5fn|6.4<space|2spc>Controlled compilation
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-555>>

    <with|par-left|1.5fn|6.5<space|2spc>Tree representation
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-558>>

    <with|par-left|1.5fn|6.6<space|2spc>Evaluation of trees
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-559>>

    <with|par-left|1.5fn|6.7<space|2spc>Tree position
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-560>>

    <with|par-left|1.5fn|6.8<space|2spc>Actions on trees
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-561>>

    <with|par-left|1.5fn|6.9<space|2spc>Symbols
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-562>>

    <with|par-left|1.5fn|6.10<space|2spc>Evaluating trees
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-563>>

    <with|par-left|1.5fn|6.11<space|2spc>Code generation for trees
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-564>>

    <with|par-left|3fn|6.11.1<space|2spc>Right side of a rewrite
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-566>>

    <with|par-left|3fn|6.11.2<space|2spc>Closures
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-567>>

    <with|par-left|1.5fn|6.12<space|2spc>Tail recursion
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-569>>

    <with|par-left|1.5fn|6.13<space|2spc>Partial recompilation
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-570>>

    <with|par-left|1.5fn|6.14<space|2spc>Machine Interface
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-571>>

    <with|par-left|1.5fn|6.15<space|2spc>Machine Types and Normal Types
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-572>>

    <vspace*|1fn><with|font-series|bold|math-font-series|bold|Index>
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-90><vspace|0.5fn>

    <vspace*|1fn><with|font-series|bold|math-font-series|bold|List of
    figures> <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-91><vspace|0.5fn>

    <vspace*|1fn><with|font-series|bold|math-font-series|bold|List of tables>
    <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
    <no-break><pageref|auto-92><vspace|0.5fn>
  </table-of-contents>

  <new-page*>

  <\the-index|idx>
    <index-1|:=|<pageref|auto-222>>

    <index-1|abstract syntax tree|<pageref|auto-15>, <pageref|auto-47>>

    <index-1|anonymous function|<pageref|auto-201>>

    <index-1|argument|<pageref|auto-200>>

    <index-1|arithmetic|<pageref|auto-434>>

    <index-2|bitwise|<pageref|auto-442>>

    <index-1*|array>

    <index-2|as function|<pageref|auto-266>>

    <index-2|index|<pageref|auto-259>>

    <index-1|array index|<pageref|auto-260>>

    <index-1|art|<pageref|auto-41>>

    <index-1|assignment|<pageref|auto-180>, <pageref|auto-221>,
    <pageref|auto-297>>

    <index-2|in expression|<pageref|auto-243>>

    <index-2|to parameter|<pageref|auto-240>>

    <index-2|to type declaration|<pageref|auto-219>, <pageref|auto-228>>

    <index-1|associativity|<pageref|auto-118>, <pageref|auto-126>>

    <index-1*|AST>

    <index-2|manipulations|<pageref|auto-464>>

    <index-1|AST (abstract syntax tree)|<pageref|auto-16>>

    <index-1|automatic type conversion|<pageref|auto-401>,
    <pageref|auto-416>>

    <index-1|bandwidth|<pageref|auto-39>>

    <index-1|binding|<pageref|auto-181>, <pageref|auto-280>>

    <index-2|in assignment|<pageref|auto-224>>

    <index-2|local scope|<pageref|auto-230>>

    <index-2|of module names|<pageref|auto-523>>

    <index-2|parameters|<pageref|auto-310>>

    <index-2|with return type declaration|<pageref|auto-234>>

    <index-1*|bindings>

    <index-2|in type definitions|<pageref|auto-365>>

    <index-1|bitwise arithmetic|<pageref|auto-441>>

    <index-1|block|<pageref|auto-101>, <pageref|auto-113>,
    <pageref|auto-348>>

    <index-1|block delimiters|<pageref|auto-114>, <pageref|auto-156>>

    <index-1|block type|<pageref|auto-14>>

    <index-1|boolean|<pageref|auto-350>, <pageref|auto-445>>

    <index-1|built-in operations|<pageref|auto-430>>

    <index-1|builtins.xl|<pageref|auto-432>>

    <index-1|C interface|<pageref|auto-268>>

    <index-1|C symbols|<pageref|auto-161>>

    <index-1|catch-all rewrite|<pageref|auto-291>>

    <index-1|child|<pageref|auto-468>>

    <index-1|child node|<pageref|auto-102>>

    <index-1|closing|<pageref|auto-471>>

    <index-1|closure|<pageref|auto-312>>

    <index-1|code space|<pageref|auto-33>>

    <index-1|comments|<pageref|auto-59>>

    <index-1|comparisons|<pageref|auto-438>>

    <index-1|concept|<pageref|auto-31>>

    <index-1|concept space|<pageref|auto-32>>

    <index-1|constant|<pageref|auto-195>>

    <index-1|constant symbols|<pageref|auto-198>>

    <index-1|contains|<pageref|auto-407>>

    <index-1|context|<pageref|auto-203>, <pageref|auto-281>>

    <index-2|current|<pageref|auto-295>>

    <index-2|enclosing|<pageref|auto-288>>

    <index-2|parameter context|<pageref|auto-313>>

    <index-2|passed with arguments|<pageref|auto-311>>

    <index-1|context order|<pageref|auto-283>, <pageref|auto-305>>

    <index-1|control characters|<pageref|auto-79>>

    <index-1*|conversion>

    <index-2|from number to text|<pageref|auto-458>>

    <index-2|from text to number|<pageref|auto-457>>

    <index-1|conversions|<pageref|auto-454>>

    <index-1|C.syntax|<pageref|auto-271>>

    <index-2|connexion to xl.syntax|<pageref|auto-273>>

    <index-1|C.syntax file|<pageref|auto-160>>

    <index-1|current context|<pageref|auto-294>>

    <index-1|data declaration|<pageref|auto-207>>

    <index-1|data declarations|<pageref|auto-174>>

    <index-1|data inheritance|<pageref|auto-399>>

    <index-1|date and time|<pageref|auto-460>>

    <index-1|declaration|<pageref|auto-254>>

    <index-2|of data|<pageref|auto-206>>

    <index-2|of rewrites|<pageref|auto-190>>

    <index-2|of types|<pageref|auto-213>>

    <index-1|default precedence|<pageref|auto-154>>

    <index-1|default prefix (precedence)|<pageref|auto-131>>

    <index-1|default value|<pageref|auto-385>>

    <index-1*|definition>

    <index-2|of properties|<pageref|auto-384>>

    <index-2|of types|<pageref|auto-354>>

    <index-1|domain-specific language|<pageref|auto-17>>

    <index-1*|dot>

    <index-2|as decimal separator|<pageref|auto-70>>

    <index-2|as index operator|<pageref|auto-258>>

    <index-1|double quote|<pageref|auto-83>>

    <index-1|DSL (domain-specific language)|<pageref|auto-18>>

    <index-1|evaluation|<pageref|auto-166>, <pageref|auto-302>>

    <index-2|data declaration arguments|<pageref|auto-209>>

    <index-2|demand-based|<pageref|auto-320>>

    <index-2|explicit|<pageref|auto-325>>

    <index-2|explicit vs. lazy|<pageref|auto-329>>

    <index-2|forcing explicit evaluation|<pageref|auto-330>>

    <index-2|in assignment|<pageref|auto-223>>

    <index-2|lazy|<pageref|auto-319>>

    <index-2|mismatch|<pageref|auto-309>>

    <index-2|of arguments|<pageref|auto-307>>

    <index-2|order|<pageref|auto-252>>

    <index-2|special forms|<pageref|auto-316>>

    <index-2|standard case|<pageref|auto-304>>

    <index-1|evaluation order|<pageref|auto-185>>

    <index-1|execution (of programs)|<pageref|auto-165>>

    <index-1|explicit evaluation|<pageref|auto-324>>

    <index-1|explicit type check|<pageref|auto-403>>

    <index-1|explicit type conversion|<pageref|auto-413>>

    <index-1|exponent (for real constants)|<pageref|auto-73>>

    <index-1*|expression>

    <index-2|allowed on left of assignment|<pageref|auto-238>>

    <index-2|assignment as expression|<pageref|auto-244>>

    <index-1|expression (as opposed to statement)|<pageref|auto-138>>

    <index-1|expression vs. statement|<pageref|auto-94>, <pageref|auto-120>,
    <pageref|auto-135>>

    <index-1|Extensible language and runtime|<pageref|auto-4>>

    <index-1|extern syntax|<pageref|auto-269>>

    <index-1|external syntax file|<pageref|auto-159>>

    <index-1|field index|<pageref|auto-262>>

    <index-1|filter|<pageref|auto-25>>

    <index-1|filter operation|<pageref|auto-479>>

    <index-1|function|<pageref|auto-111>>

    <index-1|function precedence|<pageref|auto-110>, <pageref|auto-132>,
    <pageref|auto-155>>

    <index-1|functional programming|<pageref|auto-20>, <pageref|auto-26>>

    <index-1|getter|<pageref|auto-391>>

    <index-1|good (function)|<pageref|auto-486>>

    <index-1|guard|<pageref|auto-246>>

    <index-1|guard (in a rewrite declaration)|<pageref|auto-178>>

    <index-1|hash sign (as a radix delimiter)|<pageref|auto-66>>

    <index-1*|if-then-else>

    <index-2|library definition|<pageref|auto-484>>

    <index-2|statement|<pageref|auto-28>, <pageref|auto-192>>

    <index-2|type|<pageref|auto-355>>

    <index-1|implementation|<pageref|auto-172>>

    <index-1|import|<pageref|auto-514>>

    <index-2|with a short name|<pageref|auto-519>>

    <index-1|indentation|<pageref|auto-56>, <pageref|auto-152>,
    <pageref|auto-157>>

    <index-1|indentation (in long text)|<pageref|auto-86>>

    <index-1*|index>

    <index-2|array|<pageref|auto-261>>

    <index-2|field|<pageref|auto-263>>

    <index-2|for user-defined types|<pageref|auto-366>>

    <index-1|index operator|<pageref|auto-187>, <pageref|auto-257>,
    <pageref|auto-300>>

    <index-1|infix|<pageref|auto-98>, <pageref|auto-104>, <pageref|auto-345>>

    <index-1|infix type|<pageref|auto-11>>

    <index-1|infix vs. prefix vs. postfix|<pageref|auto-119>,
    <pageref|auto-128>>

    <index-1|inherit|<pageref|auto-380>, <pageref|auto-398>>

    <index-1|integer|<pageref|auto-339>>

    <index-1|integer constant|<pageref|auto-64>>

    <index-1|integer type|<pageref|auto-7>>

    <index-1|interval arithmetic|<pageref|auto-502>>

    <index-1|is_a|<pageref|auto-410>>

    <index-1|lambda function|<pageref|auto-202>>

    <index-1|lazy evaluation|<pageref|auto-318>>

    <index-1|left|<pageref|auto-466>>

    <index-1|library|<pageref|auto-428>>

    <index-1|line-terminating characters|<pageref|auto-78>>

    <index-1*|list>

    <index-2|comma-separated|<pageref|auto-475>>

    <index-2|operations on lists|<pageref|auto-474>>

    <index-1|list operations|<pageref|auto-473>>

    <index-1|literal node types|<pageref|auto-62>>

    <index-1|local scope|<pageref|auto-231>>

    <index-1|long text|<pageref|auto-84>>

    <index-1|machine interface|<pageref|auto-276>>

    <index-1|map|<pageref|auto-23>>

    <index-1|map operation|<pageref|auto-477>>

    <index-1|mathematical functions|<pageref|auto-448>>

    <index-1*|memoization>

    <index-2|of arguments|<pageref|auto-308>>

    <index-2|of parameters|<pageref|auto-327>>

    <index-1|meta-programming|<pageref|auto-21>>

    <index-1|module|<pageref|auto-513>>

    <index-2|description|<pageref|auto-526>>

    <index-2|import|<pageref|auto-515>>

    <index-1|module description|<pageref|auto-525>>

    <index-1|module path|<pageref|auto-517>>

    <index-1|music|<pageref|auto-43>>

    <index-1|name|<pageref|auto-92>, <pageref|auto-343>>

    <index-1|name type|<pageref|auto-10>>

    <index-1|noise|<pageref|auto-42>>

    <index-1|normal form|<pageref|auto-370>>

    <index-1|normal ELIOT|<pageref|auto-51>>

    <index-1|off-side rule|<pageref|auto-53>>

    <index-1|opcode|<pageref|auto-277>>

    <index-1|opening|<pageref|auto-470>>

    <index-1|operand (in prefix and postfix)|<pageref|auto-108>>

    <index-1|operator|<pageref|auto-344>>

    <index-1|operator symbols|<pageref|auto-93>>

    <index-1|operators|<pageref|auto-144>>

    <index-1|overloading|<pageref|auto-217>>

    <index-1|parameter|<pageref|auto-199>>

    <index-1|parameterized types|<pageref|auto-419>>

    <index-1*|parameters>

    <index-2|of types|<pageref|auto-364>>

    <index-2|with properties types|<pageref|auto-382>>

    <index-1|parsing|<pageref|auto-116>, <pageref|auto-129>>

    <index-1|parsing ambiguities|<pageref|auto-130>, <pageref|auto-136>>

    <index-1|pattern|<pageref|auto-171>, <pageref|auto-194>>

    <index-2|in type|<pageref|auto-359>>

    <index-2|making type pattern specific|<pageref|auto-372>>

    <index-2|matching|<pageref|auto-306>>

    <index-1|postfix|<pageref|auto-100>, <pageref|auto-107>,
    <pageref|auto-347>>

    <index-1|postfix type|<pageref|auto-13>>

    <index-1|power operator|<pageref|auto-435>>

    <index-1|precedence|<pageref|auto-117>, <pageref|auto-122>,
    <pageref|auto-146>>

    <index-1|predefined types|<pageref|auto-338>>

    <index-1|predicate|<pageref|auto-480>>

    <index-1|prefix|<pageref|auto-99>, <pageref|auto-106>,
    <pageref|auto-346>>

    <index-1|prefix type|<pageref|auto-12>>

    <index-1|programming paradigm|<pageref|auto-3>>

    <index-1|properties|<pageref|auto-377>>

    <index-2|arguments|<pageref|auto-389>>

    <index-2|as parameter types|<pageref|auto-388>>

    <index-1|property|<pageref|auto-379>>

    <index-2|default value|<pageref|auto-386>>

    <index-2|required|<pageref|auto-395>>

    <index-2|setting|<pageref|auto-387>>

    <index-1|property definition|<pageref|auto-383>>

    <index-1|pseudo-metric|<pageref|auto-36>>

    <index-1|quote|<pageref|auto-81>>

    <index-1*|radix>

    <index-2|in integer numbers|<pageref|auto-65>>

    <index-2|in real numbers|<pageref|auto-71>>

    <index-1|range|<pageref|auto-481>>

    <index-1|real|<pageref|auto-340>>

    <index-1|real type|<pageref|auto-8>>

    <index-1|reduce|<pageref|auto-24>>

    <index-1|reduce operation|<pageref|auto-478>>

    <index-1|reference|<pageref|auto-299>>

    <index-1|required property|<pageref|auto-394>>

    <index-1|return type declaration|<pageref|auto-215>>

    <index-2|in assignment|<pageref|auto-233>>

    <index-1|rewrite declaration|<pageref|auto-191>>

    <index-1|rewrite declarations|<pageref|auto-170>>

    <index-1|rewrite type|<pageref|auto-424>>

    <index-1|right|<pageref|auto-467>>

    <index-1|scope|<pageref|auto-286>>

    <index-2|creation|<pageref|auto-296>>

    <index-2|enclosing|<pageref|auto-289>>

    <index-2|for modules|<pageref|auto-521>>

    <index-2|global|<pageref|auto-290>>

    <index-2|local|<pageref|auto-232>, <pageref|auto-287>>

    <index-1|self|<pageref|auto-211>>

    <index-1|semantic noise|<pageref|auto-38>>

    <index-1|semantics|<pageref|auto-164>>

    <index-1|sequence|<pageref|auto-183>, <pageref|auto-250>>

    <index-2|evaluation order|<pageref|auto-251>>

    <index-1|sequence operator|<pageref|auto-184>>

    <index-1|setter|<pageref|auto-392>>

    <index-1|shadowed binding|<pageref|auto-284>>

    <index-1*|shadowing>

    <index-2|in modules|<pageref|auto-522>>

    <index-1|short module name|<pageref|auto-518>>

    <index-1|signal-noise ratio|<pageref|auto-40>>

    <index-1|single quote|<pageref|auto-82>>

    <index-1|spaces (for indentation)|<pageref|auto-54>>

    <index-1|special forms|<pageref|auto-315>>

    <index-1|standard operators|<pageref|auto-145>>

    <index-1|statement|<pageref|auto-137>, <pageref|auto-255>>

    <index-1|statement precedence|<pageref|auto-140>, <pageref|auto-153>>

    <index-1|structured node types|<pageref|auto-97>>

    <index-1|subject and complement|<pageref|auto-139>>

    <index-1|symbol|<pageref|auto-342>, <pageref|auto-469>>

    <index-1|symbols|<pageref|auto-91>, <pageref|auto-89>>

    <index-1|syntactic noise|<pageref|auto-37>>

    <index-1*|syntax>

    <index-2|in modules|<pageref|auto-520>>

    <index-1|syntax configuration|<pageref|auto-50>, <pageref|auto-133>,
    <pageref|auto-142>>

    <index-1|syntax statement|<pageref|auto-124>, <pageref|auto-148>,
    <pageref|auto-150>>

    <index-1|tabs (for indentation)|<pageref|auto-55>>

    <index-1|text|<pageref|auto-341>>

    <index-1|text delimiters|<pageref|auto-80>, <pageref|auto-158>>

    <index-1|text functions|<pageref|auto-451>>

    <index-1|text literals|<pageref|auto-76>>

    <index-1|text type|<pageref|auto-9>>

    <index-1|tree|<pageref|auto-349>>

    <index-2|operations|<pageref|auto-463>>

    <index-1|tree rewrite|<pageref|auto-168>>

    <index-1|tree rewrite operators|<pageref|auto-169>>

    <index-1|type|<pageref|auto-332>>

    <index-2|belonging to a type|<pageref|auto-335>>

    <index-2|check|<pageref|auto-406>>

    <index-2|conversions|<pageref|auto-414>, <pageref|auto-455>>

    <index-2|declaration|<pageref|auto-334>>

    <index-2|definition|<pageref|auto-353>>

    <index-2|explicit type check|<pageref|auto-404>>

    <index-2|identifying arbitrary tree shapes|<pageref|auto-408>>

    <index-2|multiple notations|<pageref|auto-374>>

    <index-2|normal form|<pageref|auto-371>>

    <index-2|parameterized type|<pageref|auto-420>>

    <index-2|pattern|<pageref|auto-358>>

    <index-2|predefined|<pageref|auto-337>>

    <index-2|properties|<pageref|auto-378>>

    <index-2|rewrite type|<pageref|auto-425>>

    <index-1|type check|<pageref|auto-405>>

    <index-1|type declaration|<pageref|auto-214>, <pageref|auto-333>>

    <index-2|in assignment|<pageref|auto-218>, <pageref|auto-229>>

    <index-2|vs. type definition|<pageref|auto-361>>

    <index-1|type declarations|<pageref|auto-176>>

    <index-1|type definition|<pageref|auto-352>>

    <index-2|vs. type declaration|<pageref|auto-362>>

    <index-1|type pattern|<pageref|auto-357>>

    <index-1|undefined form|<pageref|auto-292>>

    <index-1*|underscore>

    <index-2|as digit separator|<pageref|auto-67>, <pageref|auto-72>>

    <index-1|UTF-8|<pageref|auto-77>>

    <index-1|value (of text literals)|<pageref|auto-88>>

    <index-1|variable|<pageref|auto-196>>

    <index-1|when infix operator|<pageref|auto-247>>

    <index-1|XL0 (abstract syntax tree for ELIOT)|<pageref|auto-48>>

    <index-1|ELIOT (eXtensible Language and Runtime|<pageref|auto-5>>

    <index-1|xl.syntax|<pageref|auto-49>, <pageref|auto-109>,
    <pageref|auto-123>, <pageref|auto-143>, <pageref|auto-431>>

    <index-2|connexion to C.syntax|<pageref|auto-272>>
  </the-index>

  <new-page*>

  <\list-of-figures|figure>
    <glossary-1|Declaration of the factorial function|<pageref|auto-22>>

    <glossary-1|Map, reduce and filter|<pageref|auto-27>>

    <glossary-1|Declaration of if-then-else|<pageref|auto-29>>

    <glossary-1|Off-side rule: Using indentation to mark program
    structure.|<pageref|auto-57>>

    <glossary-1|Single-line and multi-line comments|<pageref|auto-60>>

    <glossary-1|Valid integer constants|<pageref|auto-68>>

    <glossary-1|Valid real constants|<pageref|auto-74>>

    <glossary-1|Valid text constants|<pageref|auto-85>>

    <glossary-1|Long text and indentation|<pageref|auto-87>>

    <glossary-1|Examples of valid operator and name
    symbols|<pageref|auto-95>>

    <glossary-1|Default syntax configuration file|<pageref|auto-147>>

    <glossary-1|Use of the <with|font-family|tt|language|verbatim|syntax>
    specification in a source file|<pageref|auto-149>>

    <glossary-1|C syntax configuration file|<pageref|auto-162>>

    <glossary-1|Example of rewrite declaration|<pageref|auto-173>>

    <glossary-1|Example of data declaration|<pageref|auto-175>>

    <glossary-1|Example of data declarations containing type
    declarations|<pageref|auto-177>>

    <glossary-1|Example of guard to build the Syracuse
    suite|<pageref|auto-179>>

    <glossary-1|Example of assignment|<pageref|auto-182>>

    <glossary-1|Example of sequence|<pageref|auto-186>>

    <glossary-1|Examples of index operators|<pageref|auto-188>>

    <glossary-1|Examples of tree rewrites|<pageref|auto-193>>

    <glossary-1|Constants vs. Variable symbols|<pageref|auto-197>>

    <glossary-1|Declarations are visible to the entire sequence containing
    them|<pageref|auto-204>>

    <glossary-1|Declaring a comma-separated list|<pageref|auto-208>>

    <glossary-1|Declaring a <with|font-family|tt|language|verbatim|complex>
    data type|<pageref|auto-210>>

    <glossary-1|Simple type declarations|<pageref|auto-216>>

    <glossary-1|Creating a new binding<label|creating-a-new-binding>|<pageref|auto-225>>

    <glossary-1|Assignment to existing binding<label|nonlocal-assignment>|<pageref|auto-226>>

    <glossary-1|Assigning to new local variable
    <label|assign-to-new-local>|<pageref|auto-235>>

    <glossary-1|Assignment to references|<pageref|auto-237>>

    <glossary-1|Assigning to parameter|<pageref|auto-241>>

    <glossary-1|Guard limit the validity of operations|<pageref|auto-248>>

    <glossary-1|Code writing <with|font-family|tt|language|verbatim|A>, then
    <with|font-family|tt|language|verbatim|B>, then
    <with|font-family|tt|language|verbatim|f(100)+f(200)>|<pageref|auto-253>>

    <glossary-1|Structured data|<pageref|auto-264>>

    <glossary-1|Creating an interface for a C function|<pageref|auto-270>>

    <glossary-1|Generating machine code using opcode
    declarations|<pageref|auto-278>>

    <glossary-1|Evaluation for comparison|<pageref|auto-321>>

    <glossary-1|Evaluation for type comparison|<pageref|auto-322>>

    <glossary-1|Explicit vs. lazy evaluation|<pageref|auto-328>>

    <glossary-1|Simple type declaration|<pageref|auto-356>>

    <glossary-1|Simple type declaration|<pageref|auto-360>>

    <glossary-1|Using the <with|font-family|tt|language|verbatim|complex>
    type|<pageref|auto-363>>

    <glossary-1|Binding for a <with|font-family|tt|language|verbatim|complex>
    parameter|<pageref|auto-367>>

    <glossary-1|Making type <with|font-family|tt|language|verbatim|A>
    equivalent to type <with|font-family|tt|language|verbatim|B>|<pageref|auto-368>>

    <glossary-1|Named patterns for <with|font-family|tt|language|verbatim|complex>|<pageref|auto-373>>

    <glossary-1|Creating a normal form for the complex
    type|<pageref|auto-375>>

    <glossary-1|Properties declaration|<pageref|auto-381>>

    <glossary-1|Color properties|<pageref|auto-390>>

    <glossary-1|Setting default arguments from the current
    context|<pageref|auto-393>>

    <glossary-1|Additional code in properties|<pageref|auto-396>>

    <glossary-1|Data inheritance|<pageref|auto-400>>

    <glossary-1|Defining a type identifying an arbitrary AST
    shape|<pageref|auto-409>>

    <glossary-1|Explicit type check|<pageref|auto-411>>

    <glossary-1|Explicit type conversion|<pageref|auto-415>>

    <glossary-1|Automatic type conversion|<pageref|auto-417>>

    <glossary-1|Parameterized type|<pageref|auto-421>>

    <glossary-1|Declaring a range type using an infix
    form|<pageref|auto-422>>

    <glossary-1|Declaration of a rewrite type|<pageref|auto-426>>

    <glossary-1|Library definition of if-then-else|<pageref|auto-485>>

    <glossary-1|The <with|font-family|tt|language|verbatim|good>
    function|<pageref|auto-487>>

    <glossary-1|Infinite loop|<pageref|auto-489>>

    <glossary-1|While loop|<pageref|auto-491>>

    <glossary-1|Until loop|<pageref|auto-492>>

    <glossary-1|For loop on an integer range|<pageref|auto-494>>

    <glossary-1|For loop on a container|<pageref|auto-495>>

    <glossary-1|Other kinds of <with|font-family|tt|language|verbatim|for>
    loop|<pageref|auto-496>>

    <\glossary-1>
      Range and range type definition
    </glossary-1|<pageref|auto-501>>

    <glossary-1|Ranges as lists|<pageref|auto-503>>

    <glossary-1|Union type definition|<pageref|auto-505>>

    <glossary-1|Using union types|<pageref|auto-506>>

    <glossary-1|Enumeration type definition|<pageref|auto-508>>

    <glossary-1|Type matching a type declaration|<pageref|auto-510>>

    <glossary-1|Import statements examples|<pageref|auto-516>>

    <glossary-1|Module definition|<pageref|auto-527>>

    <glossary-1|Lazy evaluation of an infinite list|<pageref|auto-550>>

    <glossary-1|Controlled compilation|<pageref|auto-556>>

    <glossary-1|Signature for rewrite code with two
    variables.|<pageref|auto-565>>

    <glossary-1|Signature for rewrite code with two
    variables.|<pageref|auto-568>>
  </list-of-figures>

  <new-page*>

  <\list-of-tables|table>
    <glossary-1|Type correspondances in a C interface|<pageref|auto-274>>

    <glossary-1|Arithmetic operations|<pageref|auto-436>>

    <glossary-1|Comparisons|<pageref|auto-439>>

    <glossary-1|Bitwise arithmetic operations|<pageref|auto-443>>

    <glossary-1|Boolean operations|<pageref|auto-446>>

    <glossary-1|Mathematical operations|<pageref|auto-449>>

    <glossary-1|Text operations|<pageref|auto-452>>

    <glossary-1|Conversions|<pageref|auto-456>>

    <glossary-1|Date and time|<pageref|auto-461>>

    <glossary-1|Tree operations|<pageref|auto-465>>

    <glossary-1|List operations|<pageref|auto-476>>

    <glossary-1|LLVM operations|<pageref|auto-557>>
  </list-of-tables>
</body>

<\initial>
  <\collection>
    <associate|par-hyphen|normal>
  </collection>
</initial>

<\references>
  <\collection>
    <associate|Binding|<tuple|3.5|?>>
    <associate|C-interface|<tuple|35|18>>
    <associate|C-library|<tuple|4.4|27>>
    <associate|C-syntax-file|<tuple|13|12>>
    <associate|C-types-conversion|<tuple|1|18>>
    <associate|arbitrary-type|<tuple|52|26>>
    <associate|arithmetic|<tuple|2|28>>
    <associate|array-assign|<tuple|22|?>>
    <associate|assign-to-new-local|<tuple|6.15|46>>
    <associate|assign-to-parameter|<tuple|31|16>>
    <associate|assign-to-reference|<tuple|6.15|?>>
    <associate|assign-to-reference-example|<tuple|30|16>>
    <associate|assignment|<tuple|3.1.4|15>>
    <associate|assignments-cant-override-patterns|<tuple|24|14>>
    <associate|auto-1|<tuple|1|1>>
    <associate|auto-10|<tuple|1.2|2>>
    <associate|auto-100|<tuple|3|7>>
    <associate|auto-101|<tuple|4|7>>
    <associate|auto-102|<tuple|4|7>>
    <associate|auto-103|<tuple|2.4.1|7>>
    <associate|auto-104|<tuple|2.4.1|7>>
    <associate|auto-105|<tuple|2.4.2|7>>
    <associate|auto-106|<tuple|2.4.2|7>>
    <associate|auto-107|<tuple|2.4.2|7>>
    <associate|auto-108|<tuple|2.4.2|7>>
    <associate|auto-109|<tuple|2.4.2|7>>
    <associate|auto-11|<tuple|1.2|2>>
    <associate|auto-110|<tuple|2.4.2|7>>
    <associate|auto-111|<tuple|2.4.2|7>>
    <associate|auto-112|<tuple|2.4.3|8>>
    <associate|auto-113|<tuple|2.4.3|8>>
    <associate|auto-114|<tuple|2.4.3|8>>
    <associate|auto-115|<tuple|2.5|8>>
    <associate|auto-116|<tuple|2.5|8>>
    <associate|auto-117|<tuple|1|8>>
    <associate|auto-118|<tuple|2|8>>
    <associate|auto-119|<tuple|3|8>>
    <associate|auto-12|<tuple|1.2|2>>
    <associate|auto-120|<tuple|4|8>>
    <associate|auto-121|<tuple|2.5.1|8>>
    <associate|auto-122|<tuple|2.5.1|8>>
    <associate|auto-123|<tuple|2.5.1|8>>
    <associate|auto-124|<tuple|2.5.1|8>>
    <associate|auto-125|<tuple|2.5.2|8>>
    <associate|auto-126|<tuple|2.5.2|8>>
    <associate|auto-127|<tuple|2.5.3|8>>
    <associate|auto-128|<tuple|2.5.3|8>>
    <associate|auto-129|<tuple|2.5.3|8>>
    <associate|auto-13|<tuple|1.2|2>>
    <associate|auto-130|<tuple|2.5.3|8>>
    <associate|auto-131|<tuple|<with|mode|<quote|math>|\<bullet\>>|9>>
    <associate|auto-132|<tuple|<with|mode|<quote|math>|\<bullet\>>|9>>
    <associate|auto-133|<tuple|<with|mode|<quote|math>|\<bullet\>>|9>>
    <associate|auto-134|<tuple|2.5.4|9>>
    <associate|auto-135|<tuple|2.5.4|9>>
    <associate|auto-136|<tuple|2.5.4|9>>
    <associate|auto-137|<tuple|2.5.4|9>>
    <associate|auto-138|<tuple|2.5.4|9>>
    <associate|auto-139|<tuple|2.5.4|9>>
    <associate|auto-14|<tuple|1.2|2>>
    <associate|auto-140|<tuple|2.5.4|9>>
    <associate|auto-141|<tuple|2.6|9>>
    <associate|auto-142|<tuple|2.6|9>>
    <associate|auto-143|<tuple|2.6|9>>
    <associate|auto-144|<tuple|2.6|9>>
    <associate|auto-145|<tuple|2.6|9>>
    <associate|auto-146|<tuple|2.6|9>>
    <associate|auto-147|<tuple|11|10>>
    <associate|auto-148|<tuple|11|11>>
    <associate|auto-149|<tuple|12|11>>
    <associate|auto-15|<tuple|1.2|2>>
    <associate|auto-150|<tuple|12|11>>
    <associate|auto-151|<tuple|2.6.0.1|11>>
    <associate|auto-152|<tuple|2.6.0.1|11>>
    <associate|auto-153|<tuple|<with|mode|<quote|math>|<rigid|\<circ\>>>|11>>
    <associate|auto-154|<tuple|<with|mode|<quote|math>|<rigid|\<circ\>>>|11>>
    <associate|auto-155|<tuple|<with|mode|<quote|math>|<rigid|\<circ\>>>|11>>
    <associate|auto-156|<tuple|<with|mode|<quote|math>|\<bullet\>>|11>>
    <associate|auto-157|<tuple|<with|mode|<quote|math>|<rigid|\<circ\>>>|11>>
    <associate|auto-158|<tuple|<with|mode|<quote|math>|\<bullet\>>|11>>
    <associate|auto-159|<tuple|<with|mode|<quote|math>|\<bullet\>>|11>>
    <associate|auto-16|<tuple|1.2|2>>
    <associate|auto-160|<tuple|<with|mode|<quote|math>|\<bullet\>>|11>>
    <associate|auto-161|<tuple|<with|mode|<quote|math>|\<bullet\>>|11>>
    <associate|auto-162|<tuple|13|12>>
    <associate|auto-163|<tuple|3|12>>
    <associate|auto-164|<tuple|3|12>>
    <associate|auto-165|<tuple|3|12>>
    <associate|auto-166|<tuple|3|12>>
    <associate|auto-167|<tuple|3.1|12>>
    <associate|auto-168|<tuple|3.1|12>>
    <associate|auto-169|<tuple|3.1|12>>
    <associate|auto-17|<tuple|1.2|2>>
    <associate|auto-170|<tuple|<with|mode|<quote|math>|\<bullet\>>|12>>
    <associate|auto-171|<tuple|<with|mode|<quote|math>|\<bullet\>>|12>>
    <associate|auto-172|<tuple|<with|mode|<quote|math>|\<bullet\>>|12>>
    <associate|auto-173|<tuple|14|12>>
    <associate|auto-174|<tuple|<with|mode|<quote|math>|\<bullet\>>|12>>
    <associate|auto-175|<tuple|15|12>>
    <associate|auto-176|<tuple|<with|mode|<quote|math>|\<bullet\>>|12>>
    <associate|auto-177|<tuple|16|13>>
    <associate|auto-178|<tuple|<with|mode|<quote|math>|\<bullet\>>|13>>
    <associate|auto-179|<tuple|17|13>>
    <associate|auto-18|<tuple|1.2|2>>
    <associate|auto-180|<tuple|<with|mode|<quote|math>|\<bullet\>>|13>>
    <associate|auto-181|<tuple|<with|mode|<quote|math>|\<bullet\>>|13>>
    <associate|auto-182|<tuple|18|13>>
    <associate|auto-183|<tuple|<with|mode|<quote|math>|\<bullet\>>|13>>
    <associate|auto-184|<tuple|<with|mode|<quote|math>|\<bullet\>>|13>>
    <associate|auto-185|<tuple|<with|mode|<quote|math>|\<bullet\>>|13>>
    <associate|auto-186|<tuple|19|13>>
    <associate|auto-187|<tuple|<with|mode|<quote|math>|\<bullet\>>|13>>
    <associate|auto-188|<tuple|20|13>>
    <associate|auto-189|<tuple|3.1.1|13>>
    <associate|auto-19|<tuple|1.3|2>>
    <associate|auto-190|<tuple|3.1.1|13>>
    <associate|auto-191|<tuple|3.1.1|13>>
    <associate|auto-192|<tuple|3.1.1|13>>
    <associate|auto-193|<tuple|21|13>>
    <associate|auto-194|<tuple|21|13>>
    <associate|auto-195|<tuple|21|13>>
    <associate|auto-196|<tuple|21|13>>
    <associate|auto-197|<tuple|22|14>>
    <associate|auto-198|<tuple|22|14>>
    <associate|auto-199|<tuple|22|14>>
    <associate|auto-2|<tuple|1.1|1>>
    <associate|auto-20|<tuple|1.3|2>>
    <associate|auto-200|<tuple|22|14>>
    <associate|auto-201|<tuple|22|14>>
    <associate|auto-202|<tuple|5|14>>
    <associate|auto-203|<tuple|5|14>>
    <associate|auto-204|<tuple|23|14>>
    <associate|auto-205|<tuple|3.1.2|14>>
    <associate|auto-206|<tuple|3.1.2|14>>
    <associate|auto-207|<tuple|3.1.2|14>>
    <associate|auto-208|<tuple|24|14>>
    <associate|auto-209|<tuple|24|14>>
    <associate|auto-21|<tuple|1.3|2>>
    <associate|auto-210|<tuple|25|14>>
    <associate|auto-211|<tuple|25|14>>
    <associate|auto-212|<tuple|3.1.3|15>>
    <associate|auto-213|<tuple|3.1.3|15>>
    <associate|auto-214|<tuple|3.1.3|15>>
    <associate|auto-215|<tuple|3.1.3|15>>
    <associate|auto-216|<tuple|26|15>>
    <associate|auto-217|<tuple|26|15>>
    <associate|auto-218|<tuple|7|15>>
    <associate|auto-219|<tuple|7|15>>
    <associate|auto-22|<tuple|1|2>>
    <associate|auto-220|<tuple|3.1.4|15>>
    <associate|auto-221|<tuple|3.1.4|15>>
    <associate|auto-222|<tuple|3.1.4|15>>
    <associate|auto-223|<tuple|3.1.4|15>>
    <associate|auto-224|<tuple|3.1.4|15>>
    <associate|auto-225|<tuple|27|15>>
    <associate|auto-226|<tuple|28|15>>
    <associate|auto-227|<tuple|3.1.4.1|15>>
    <associate|auto-228|<tuple|3.1.4.1|15>>
    <associate|auto-229|<tuple|3.1.4.1|15>>
    <associate|auto-23|<tuple|1|2>>
    <associate|auto-230|<tuple|3.1.4.1|15>>
    <associate|auto-231|<tuple|3.1.4.1|15>>
    <associate|auto-232|<tuple|3.1.4.1|15>>
    <associate|auto-233|<tuple|3.1.4.1|15>>
    <associate|auto-234|<tuple|3.1.4.1|15>>
    <associate|auto-235|<tuple|29|15>>
    <associate|auto-236|<tuple|3.1.4.2|16>>
    <associate|auto-237|<tuple|30|16>>
    <associate|auto-238|<tuple|30|16>>
    <associate|auto-239|<tuple|3.1.4.3|16>>
    <associate|auto-24|<tuple|1|2>>
    <associate|auto-240|<tuple|3.1.4.3|16>>
    <associate|auto-241|<tuple|31|16>>
    <associate|auto-242|<tuple|3.1.4.4|16>>
    <associate|auto-243|<tuple|3.1.4.4|16>>
    <associate|auto-244|<tuple|3.1.4.4|16>>
    <associate|auto-245|<tuple|3.1.5|16>>
    <associate|auto-246|<tuple|3.1.5|16>>
    <associate|auto-247|<tuple|3.1.5|16>>
    <associate|auto-248|<tuple|32|16>>
    <associate|auto-249|<tuple|3.1.6|17>>
    <associate|auto-25|<tuple|1|2>>
    <associate|auto-250|<tuple|3.1.6|17>>
    <associate|auto-251|<tuple|3.1.6|17>>
    <associate|auto-252|<tuple|3.1.6|17>>
    <associate|auto-253|<tuple|33|17>>
    <associate|auto-254|<tuple|33|17>>
    <associate|auto-255|<tuple|33|17>>
    <associate|auto-256|<tuple|3.1.7|17>>
    <associate|auto-257|<tuple|3.1.7|17>>
    <associate|auto-258|<tuple|3.1.7|17>>
    <associate|auto-259|<tuple|3.1.7|17>>
    <associate|auto-26|<tuple|1|2>>
    <associate|auto-260|<tuple|3.1.7|17>>
    <associate|auto-261|<tuple|3.1.7|17>>
    <associate|auto-262|<tuple|3.1.7|17>>
    <associate|auto-263|<tuple|3.1.7|17>>
    <associate|auto-264|<tuple|34|17>>
    <associate|auto-265|<tuple|3.1.7.1|17>>
    <associate|auto-266|<tuple|<with|mode|<quote|math>|\<bullet\>>|17>>
    <associate|auto-267|<tuple|3.1.8|18>>
    <associate|auto-268|<tuple|3.1.8|18>>
    <associate|auto-269|<tuple|3.1.8|18>>
    <associate|auto-27|<tuple|2|2>>
    <associate|auto-270|<tuple|35|18>>
    <associate|auto-271|<tuple|35|18>>
    <associate|auto-272|<tuple|35|18>>
    <associate|auto-273|<tuple|35|18>>
    <associate|auto-274|<tuple|1|18>>
    <associate|auto-275|<tuple|3.1.9|18>>
    <associate|auto-276|<tuple|3.1.9|18>>
    <associate|auto-277|<tuple|3.1.9|18>>
    <associate|auto-278|<tuple|36|18>>
    <associate|auto-279|<tuple|3.2|18>>
    <associate|auto-28|<tuple|2|2>>
    <associate|auto-280|<tuple|3.2|18>>
    <associate|auto-281|<tuple|3.2|18>>
    <associate|auto-282|<tuple|3.2.1|18>>
    <associate|auto-283|<tuple|3.2.1|18>>
    <associate|auto-284|<tuple|3.2.1|19>>
    <associate|auto-285|<tuple|3.2.2|19>>
    <associate|auto-286|<tuple|3.2.2|19>>
    <associate|auto-287|<tuple|3.2.2|19>>
    <associate|auto-288|<tuple|3.2.2|19>>
    <associate|auto-289|<tuple|3.2.2|19>>
    <associate|auto-29|<tuple|3|3>>
    <associate|auto-290|<tuple|3.2.2|19>>
    <associate|auto-291|<tuple|3.2.2|19>>
    <associate|auto-292|<tuple|3.2.2|19>>
    <associate|auto-293|<tuple|3.2.3|19>>
    <associate|auto-294|<tuple|3.2.3|19>>
    <associate|auto-295|<tuple|3.2.3|19>>
    <associate|auto-296|<tuple|1|19>>
    <associate|auto-297|<tuple|3|19>>
    <associate|auto-298|<tuple|3.2.4|19>>
    <associate|auto-299|<tuple|3.2.4|19>>
    <associate|auto-3|<tuple|1.1|1>>
    <associate|auto-30|<tuple|1.4|3>>
    <associate|auto-300|<tuple|3.2.4|19>>
    <associate|auto-301|<tuple|3.3|19>>
    <associate|auto-302|<tuple|3.3|19>>
    <associate|auto-303|<tuple|3.3.1|19>>
    <associate|auto-304|<tuple|3.3.1|19>>
    <associate|auto-305|<tuple|2|20>>
    <associate|auto-306|<tuple|3|20>>
    <associate|auto-307|<tuple|<with|mode|<quote|math>|\<bullet\>>|20>>
    <associate|auto-308|<tuple|<with|mode|<quote|math>|\<bullet\>>|20>>
    <associate|auto-309|<tuple|4|20>>
    <associate|auto-31|<tuple|1.4|3>>
    <associate|auto-310|<tuple|5|20>>
    <associate|auto-311|<tuple|<with|mode|<quote|math>|\<bullet\>>|21>>
    <associate|auto-312|<tuple|<with|mode|<quote|math>|\<bullet\>>|21>>
    <associate|auto-313|<tuple|6|21>>
    <associate|auto-314|<tuple|3.3.2|21>>
    <associate|auto-315|<tuple|3.3.2|21>>
    <associate|auto-316|<tuple|3.3.2|21>>
    <associate|auto-317|<tuple|3.3.3|21>>
    <associate|auto-318|<tuple|3.3.3|21>>
    <associate|auto-319|<tuple|3.3.3|21>>
    <associate|auto-32|<tuple|1.4|3>>
    <associate|auto-320|<tuple|3.3.3|21>>
    <associate|auto-321|<tuple|37|21>>
    <associate|auto-322|<tuple|38|22>>
    <associate|auto-323|<tuple|3.3.4|22>>
    <associate|auto-324|<tuple|3.3.4|22>>
    <associate|auto-325|<tuple|3.3.4|22>>
    <associate|auto-326|<tuple|3.3.5|22>>
    <associate|auto-327|<tuple|3.3.5|22>>
    <associate|auto-328|<tuple|39|22>>
    <associate|auto-329|<tuple|39|22>>
    <associate|auto-33|<tuple|1.4|3>>
    <associate|auto-330|<tuple|5|23>>
    <associate|auto-331|<tuple|3.4|23>>
    <associate|auto-332|<tuple|3.4|23>>
    <associate|auto-333|<tuple|3.4|23>>
    <associate|auto-334|<tuple|3.4|23>>
    <associate|auto-335|<tuple|3.4|23>>
    <associate|auto-336|<tuple|3.4.1|23>>
    <associate|auto-337|<tuple|3.4.1|23>>
    <associate|auto-338|<tuple|3.4.1|23>>
    <associate|auto-339|<tuple|<with|mode|<quote|math>|\<bullet\>>|23>>
    <associate|auto-34|<tuple|1|3>>
    <associate|auto-340|<tuple|<with|mode|<quote|math>|\<bullet\>>|23>>
    <associate|auto-341|<tuple|<with|mode|<quote|math>|\<bullet\>>|23>>
    <associate|auto-342|<tuple|<with|mode|<quote|math>|\<bullet\>>|23>>
    <associate|auto-343|<tuple|<with|mode|<quote|math>|\<bullet\>>|23>>
    <associate|auto-344|<tuple|<with|mode|<quote|math>|\<bullet\>>|23>>
    <associate|auto-345|<tuple|<with|mode|<quote|math>|\<bullet\>>|23>>
    <associate|auto-346|<tuple|<with|mode|<quote|math>|\<bullet\>>|23>>
    <associate|auto-347|<tuple|<with|mode|<quote|math>|\<bullet\>>|23>>
    <associate|auto-348|<tuple|<with|mode|<quote|math>|\<bullet\>>|23>>
    <associate|auto-349|<tuple|<with|mode|<quote|math>|\<bullet\>>|23>>
    <associate|auto-35|<tuple|2|3>>
    <associate|auto-350|<tuple|<with|mode|<quote|math>|\<bullet\>>|23>>
    <associate|auto-351|<tuple|3.4.2|23>>
    <associate|auto-352|<tuple|3.4.2|23>>
    <associate|auto-353|<tuple|3.4.2|23>>
    <associate|auto-354|<tuple|3.4.2|23>>
    <associate|auto-355|<tuple|3.4.2|23>>
    <associate|auto-356|<tuple|40|23>>
    <associate|auto-357|<tuple|40|23>>
    <associate|auto-358|<tuple|40|23>>
    <associate|auto-359|<tuple|40|23>>
    <associate|auto-36|<tuple|2|3>>
    <associate|auto-360|<tuple|41|24>>
    <associate|auto-361|<tuple|41|24>>
    <associate|auto-362|<tuple|41|24>>
    <associate|auto-363|<tuple|42|24>>
    <associate|auto-364|<tuple|42|24>>
    <associate|auto-365|<tuple|42|24>>
    <associate|auto-366|<tuple|42|24>>
    <associate|auto-367|<tuple|43|24>>
    <associate|auto-368|<tuple|44|24>>
    <associate|auto-369|<tuple|3.4.3|24>>
    <associate|auto-37|<tuple|1|3>>
    <associate|auto-370|<tuple|3.4.3|24>>
    <associate|auto-371|<tuple|3.4.3|24>>
    <associate|auto-372|<tuple|3.4.3|24>>
    <associate|auto-373|<tuple|45|24>>
    <associate|auto-374|<tuple|45|24>>
    <associate|auto-375|<tuple|46|24>>
    <associate|auto-376|<tuple|3.4.4|25>>
    <associate|auto-377|<tuple|3.4.4|25>>
    <associate|auto-378|<tuple|3.4.4|25>>
    <associate|auto-379|<tuple|2|25>>
    <associate|auto-38|<tuple|2|3>>
    <associate|auto-380|<tuple|3|25>>
    <associate|auto-381|<tuple|47|25>>
    <associate|auto-382|<tuple|47|25>>
    <associate|auto-383|<tuple|47|25>>
    <associate|auto-384|<tuple|47|25>>
    <associate|auto-385|<tuple|47|25>>
    <associate|auto-386|<tuple|47|25>>
    <associate|auto-387|<tuple|47|25>>
    <associate|auto-388|<tuple|47|25>>
    <associate|auto-389|<tuple|47|25>>
    <associate|auto-39|<tuple|3|3>>
    <associate|auto-390|<tuple|48|25>>
    <associate|auto-391|<tuple|48|25>>
    <associate|auto-392|<tuple|48|25>>
    <associate|auto-393|<tuple|49|25>>
    <associate|auto-394|<tuple|49|25>>
    <associate|auto-395|<tuple|49|25>>
    <associate|auto-396|<tuple|50|25>>
    <associate|auto-397|<tuple|3.4.5|26>>
    <associate|auto-398|<tuple|3.4.5|26>>
    <associate|auto-399|<tuple|3.4.5|26>>
    <associate|auto-4|<tuple|1.1|1>>
    <associate|auto-40|<tuple|4|3>>
    <associate|auto-400|<tuple|51|26>>
    <associate|auto-401|<tuple|51|26>>
    <associate|auto-402|<tuple|3.4.6|26>>
    <associate|auto-403|<tuple|3.4.6|26>>
    <associate|auto-404|<tuple|3.4.6|26>>
    <associate|auto-405|<tuple|3.4.6|26>>
    <associate|auto-406|<tuple|3.4.6|26>>
    <associate|auto-407|<tuple|3.4.6|26>>
    <associate|auto-408|<tuple|3.4.6|26>>
    <associate|auto-409|<tuple|52|26>>
    <associate|auto-41|<tuple|4|3>>
    <associate|auto-410|<tuple|52|26>>
    <associate|auto-411|<tuple|53|26>>
    <associate|auto-412|<tuple|3.4.7|26>>
    <associate|auto-413|<tuple|3.4.7|26>>
    <associate|auto-414|<tuple|3.4.7|26>>
    <associate|auto-415|<tuple|54|26>>
    <associate|auto-416|<tuple|54|26>>
    <associate|auto-417|<tuple|55|26>>
    <associate|auto-418|<tuple|3.4.8|27>>
    <associate|auto-419|<tuple|3.4.8|27>>
    <associate|auto-42|<tuple|4|3>>
    <associate|auto-420|<tuple|3.4.8|27>>
    <associate|auto-421|<tuple|56|27>>
    <associate|auto-422|<tuple|57|27>>
    <associate|auto-423|<tuple|3.4.9|27>>
    <associate|auto-424|<tuple|3.4.9|27>>
    <associate|auto-425|<tuple|3.4.9|27>>
    <associate|auto-426|<tuple|58|27>>
    <associate|auto-427|<tuple|4|27>>
    <associate|auto-428|<tuple|4|27>>
    <associate|auto-429|<tuple|4.1|27>>
    <associate|auto-43|<tuple|4|3>>
    <associate|auto-430|<tuple|4.1|27>>
    <associate|auto-431|<tuple|4.1|27>>
    <associate|auto-432|<tuple|4.1|27>>
    <associate|auto-433|<tuple|4.1.1|27>>
    <associate|auto-434|<tuple|4.1.1|27>>
    <associate|auto-435|<tuple|4.1.1|27>>
    <associate|auto-436|<tuple|2|28>>
    <associate|auto-437|<tuple|4.1.2|28>>
    <associate|auto-438|<tuple|4.1.2|28>>
    <associate|auto-439|<tuple|3|28>>
    <associate|auto-44|<tuple|3|4>>
    <associate|auto-440|<tuple|4.1.3|28>>
    <associate|auto-441|<tuple|4.1.3|28>>
    <associate|auto-442|<tuple|4.1.3|28>>
    <associate|auto-443|<tuple|4|28>>
    <associate|auto-444|<tuple|4.1.4|28>>
    <associate|auto-445|<tuple|4.1.4|28>>
    <associate|auto-446|<tuple|5|28>>
    <associate|auto-447|<tuple|4.1.5|28>>
    <associate|auto-448|<tuple|4.1.5|28>>
    <associate|auto-449|<tuple|6|29>>
    <associate|auto-45|<tuple|1.5|4>>
    <associate|auto-450|<tuple|4.1.6|29>>
    <associate|auto-451|<tuple|4.1.6|29>>
    <associate|auto-452|<tuple|7|29>>
    <associate|auto-453|<tuple|4.1.7|29>>
    <associate|auto-454|<tuple|4.1.7|29>>
    <associate|auto-455|<tuple|4.1.7|29>>
    <associate|auto-456|<tuple|8|29>>
    <associate|auto-457|<tuple|8|29>>
    <associate|auto-458|<tuple|8|29>>
    <associate|auto-459|<tuple|4.1.8|29>>
    <associate|auto-46|<tuple|2|4>>
    <associate|auto-460|<tuple|4.1.8|29>>
    <associate|auto-461|<tuple|9|30>>
    <associate|auto-462|<tuple|4.1.9|30>>
    <associate|auto-463|<tuple|4.1.9|30>>
    <associate|auto-464|<tuple|4.1.9|30>>
    <associate|auto-465|<tuple|10|30>>
    <associate|auto-466|<tuple|10|30>>
    <associate|auto-467|<tuple|10|30>>
    <associate|auto-468|<tuple|10|30>>
    <associate|auto-469|<tuple|10|30>>
    <associate|auto-47|<tuple|2|4>>
    <associate|auto-470|<tuple|10|30>>
    <associate|auto-471|<tuple|10|30>>
    <associate|auto-472|<tuple|4.1.10|30>>
    <associate|auto-473|<tuple|4.1.10|30>>
    <associate|auto-474|<tuple|4.1.10|30>>
    <associate|auto-475|<tuple|4.1.10|30>>
    <associate|auto-476|<tuple|11|30>>
    <associate|auto-477|<tuple|11|30>>
    <associate|auto-478|<tuple|11|30>>
    <associate|auto-479|<tuple|11|31>>
    <associate|auto-48|<tuple|2|4>>
    <associate|auto-480|<tuple|11|31>>
    <associate|auto-481|<tuple|11|31>>
    <associate|auto-482|<tuple|4.2|31>>
    <associate|auto-483|<tuple|4.2.1|31>>
    <associate|auto-484|<tuple|4.2.1|31>>
    <associate|auto-485|<tuple|59|31>>
    <associate|auto-486|<tuple|59|31>>
    <associate|auto-487|<tuple|60|31>>
    <associate|auto-488|<tuple|4.2.2|31>>
    <associate|auto-489|<tuple|61|31>>
    <associate|auto-49|<tuple|2|4>>
    <associate|auto-490|<tuple|4.2.3|32>>
    <associate|auto-491|<tuple|62|32>>
    <associate|auto-492|<tuple|63|32>>
    <associate|auto-493|<tuple|4.2.4|32>>
    <associate|auto-494|<tuple|64|32>>
    <associate|auto-495|<tuple|65|32>>
    <associate|auto-496|<tuple|66|32>>
    <associate|auto-497|<tuple|4.2.5|33>>
    <associate|auto-498|<tuple|4.2.6|33>>
    <associate|auto-499|<tuple|4.3|33>>
    <associate|auto-5|<tuple|1.1|1>>
    <associate|auto-50|<tuple|2|4>>
    <associate|auto-500|<tuple|4.3.1|33>>
    <associate|auto-501|<tuple|67|33>>
    <associate|auto-502|<tuple|67|33>>
    <associate|auto-503|<tuple|68|33>>
    <associate|auto-504|<tuple|4.3.2|33>>
    <associate|auto-505|<tuple|69|33>>
    <associate|auto-506|<tuple|70|33>>
    <associate|auto-507|<tuple|4.3.3|34>>
    <associate|auto-508|<tuple|71|34>>
    <associate|auto-509|<tuple|4.3.4|34>>
    <associate|auto-51|<tuple|2|4>>
    <associate|auto-510|<tuple|72|34>>
    <associate|auto-511|<tuple|4.4|34>>
    <associate|auto-512|<tuple|4.4.1|34>>
    <associate|auto-513|<tuple|4.4.1|34>>
    <associate|auto-514|<tuple|4.4.1|34>>
    <associate|auto-515|<tuple|4.4.1|34>>
    <associate|auto-516|<tuple|73|34>>
    <associate|auto-517|<tuple|<with|mode|<quote|math>|\<bullet\>>|34>>
    <associate|auto-518|<tuple|<with|mode|<quote|math>|\<bullet\>>|35>>
    <associate|auto-519|<tuple|<with|mode|<quote|math>|\<bullet\>>|35>>
    <associate|auto-52|<tuple|2.1|4>>
    <associate|auto-520|<tuple|1|35>>
    <associate|auto-521|<tuple|2|35>>
    <associate|auto-522|<tuple|3|35>>
    <associate|auto-523|<tuple|4|35>>
    <associate|auto-524|<tuple|4.4.2|35>>
    <associate|auto-525|<tuple|4.4.2|35>>
    <associate|auto-526|<tuple|4.4.2|35>>
    <associate|auto-527|<tuple|74|35>>
    <associate|auto-528|<tuple|5|35>>
    <associate|auto-529|<tuple|5.1|35>>
    <associate|auto-53|<tuple|2.1|4>>
    <associate|auto-530|<tuple|75|36>>
    <associate|auto-531|<tuple|5.2|36>>
    <associate|auto-532|<tuple|5.3|36>>
    <associate|auto-533|<tuple|5.4|36>>
    <associate|auto-534|<tuple|5.5|36>>
    <associate|auto-535|<tuple|5.6|36>>
    <associate|auto-536|<tuple|5.6.1|36>>
    <associate|auto-537|<tuple|5.6.2|36>>
    <associate|auto-538|<tuple|5.6.3|36>>
    <associate|auto-539|<tuple|5.6.4|36>>
    <associate|auto-54|<tuple|2.1|4>>
    <associate|auto-540|<tuple|5.6.5|36>>
    <associate|auto-541|<tuple|5.6.6|36>>
    <associate|auto-542|<tuple|5.6.7|36>>
    <associate|auto-543|<tuple|5.7|36>>
    <associate|auto-544|<tuple|5.7.1|36>>
    <associate|auto-545|<tuple|5.7.2|36>>
    <associate|auto-546|<tuple|5.7.3|36>>
    <associate|auto-547|<tuple|5.7.4|36>>
    <associate|auto-548|<tuple|5.7.5|36>>
    <associate|auto-549|<tuple|5.7.6|36>>
    <associate|auto-55|<tuple|2.1|4>>
    <associate|auto-550|<tuple|5.7.7|36>>
    <associate|auto-551|<tuple|76|37>>
    <associate|auto-552|<tuple|6|37>>
    <associate|auto-553|<tuple|6.1|37>>
    <associate|auto-554|<tuple|6.2|37>>
    <associate|auto-555|<tuple|6.3|37>>
    <associate|auto-556|<tuple|6.4|37>>
    <associate|auto-557|<tuple|77|37>>
    <associate|auto-558|<tuple|12|37>>
    <associate|auto-559|<tuple|6.5|37>>
    <associate|auto-56|<tuple|2.1|4>>
    <associate|auto-560|<tuple|6.6|38>>
    <associate|auto-561|<tuple|6.7|38>>
    <associate|auto-562|<tuple|6.8|38>>
    <associate|auto-563|<tuple|6.9|38>>
    <associate|auto-564|<tuple|6.10|38>>
    <associate|auto-565|<tuple|6.11|38>>
    <associate|auto-566|<tuple|78|38>>
    <associate|auto-567|<tuple|6.11.1|39>>
    <associate|auto-568|<tuple|6.11.2|39>>
    <associate|auto-569|<tuple|79|39>>
    <associate|auto-57|<tuple|4|4>>
    <associate|auto-570|<tuple|6.12|39>>
    <associate|auto-571|<tuple|6.13|39>>
    <associate|auto-572|<tuple|6.14|39>>
    <associate|auto-573|<tuple|6.15|39>>
    <associate|auto-58|<tuple|2.2|5>>
    <associate|auto-59|<tuple|2.2|5>>
    <associate|auto-6|<tuple|1.2|1>>
    <associate|auto-60|<tuple|5|5>>
    <associate|auto-61|<tuple|2.3|5>>
    <associate|auto-62|<tuple|2.3|5>>
    <associate|auto-63|<tuple|2.3.1|5>>
    <associate|auto-64|<tuple|2.3.1|5>>
    <associate|auto-65|<tuple|2|5>>
    <associate|auto-66|<tuple|2|5>>
    <associate|auto-67|<tuple|2|5>>
    <associate|auto-68|<tuple|6|5>>
    <associate|auto-69|<tuple|2.3.2|5>>
    <associate|auto-7|<tuple|1.2|2>>
    <associate|auto-70|<tuple|2.3.2|5>>
    <associate|auto-71|<tuple|2.3.2|6>>
    <associate|auto-72|<tuple|2.3.2|6>>
    <associate|auto-73|<tuple|2.3.2|6>>
    <associate|auto-74|<tuple|7|6>>
    <associate|auto-75|<tuple|2.3.3|6>>
    <associate|auto-76|<tuple|2.3.3|6>>
    <associate|auto-77|<tuple|2.3.3|6>>
    <associate|auto-78|<tuple|2.3.3|6>>
    <associate|auto-79|<tuple|2.3.3|6>>
    <associate|auto-8|<tuple|1.2|2>>
    <associate|auto-80|<tuple|2.3.3|6>>
    <associate|auto-81|<tuple|2.3.3|6>>
    <associate|auto-82|<tuple|2.3.3|6>>
    <associate|auto-83|<tuple|2.3.3|6>>
    <associate|auto-84|<tuple|2.3.3|6>>
    <associate|auto-85|<tuple|8|6>>
    <associate|auto-86|<tuple|8|6>>
    <associate|auto-87|<tuple|9|6>>
    <associate|auto-88|<tuple|9|6>>
    <associate|auto-89|<tuple|2.3.4|7>>
    <associate|auto-9|<tuple|1.2|2>>
    <associate|auto-90|<tuple|6.15|40>>
    <associate|auto-91|<tuple|6.15|43>>
    <associate|auto-92|<tuple|6.15|46>>
    <associate|auto-93|<tuple|6.15|48>>
    <associate|auto-94|<tuple|3|7>>
    <associate|auto-95|<tuple|10|7>>
    <associate|auto-96|<tuple|2.4|7>>
    <associate|auto-97|<tuple|2.4|7>>
    <associate|auto-98|<tuple|1|7>>
    <associate|auto-99|<tuple|2|7>>
    <associate|automatic-type-conversion|<tuple|55|26>>
    <associate|binding|<tuple|3.2|18>>
    <associate|binding-for-complex-parameter|<tuple|43|24>>
    <associate|bitwise-arithmetic|<tuple|4|28>>
    <associate|block-type-declaration|<tuple|41|24>>
    <associate|boolean-operations|<tuple|5|28>>
    <associate|built-ins|<tuple|4.1|27>>
    <associate|class-like-data|<tuple|22|?>>
    <associate|closure-code|<tuple|79|39>>
    <associate|color-properties|<tuple|48|25>>
    <associate|comma-separated-list|<tuple|6.13|11>>
    <associate|comma-separated-list-data-declaration|<tuple|24|14>>
    <associate|comments|<tuple|5|5>>
    <associate|comparisons|<tuple|3|28>>
    <associate|complex-normal-form|<tuple|46|24>>
    <associate|complex-type|<tuple|25|14>>
    <associate|concept-programming|<tuple|1.4|3>>
    <associate|contains-tests|<tuple|53|26>>
    <associate|conversions|<tuple|8|29>>
    <associate|creating-a-new-binding|<tuple|6.15|46>>
    <associate|data-inheritance|<tuple|51|26>>
    <associate|data-inheritance-section|<tuple|3.4.5|26>>
    <associate|data-loading-operations|<tuple|11|26>>
    <associate|enum-type-definition|<tuple|71|34>>
    <associate|evaluation|<tuple|3.3|19>>
    <associate|evaluation-for-comparison|<tuple|37|21>>
    <associate|evaluation-for-type-comparison|<tuple|38|22>>
    <associate|explicit-and-automatic-type-conversions|<tuple|3.4.7|?>>
    <associate|explicit-evaluation|<tuple|3.3.4|22>>
    <associate|explicit-type-conversion|<tuple|54|26>>
    <associate|explicit-vs-lazy-evaluation|<tuple|39|22>>
    <associate|expression-vs-statement-section|<tuple|2.5.4|9>>
    <associate|extra-code-for-properties|<tuple|50|25>>
    <associate|factorial|<tuple|1|2>>
    <associate|footnote-1|<tuple|1|1>>
    <associate|footnote-10|<tuple|10|37>>
    <associate|footnote-2|<tuple|2|5>>
    <associate|footnote-3|<tuple|3|7>>
    <associate|footnote-4|<tuple|4|9>>
    <associate|footnote-5|<tuple|5|14>>
    <associate|footnote-6|<tuple|6|14>>
    <associate|footnote-7|<tuple|7|15>>
    <associate|footnote-8|<tuple|8|21>>
    <associate|footnote-9|<tuple|9|28>>
    <associate|footnr-1|<tuple|1|1>>
    <associate|footnr-10|<tuple|10|37>>
    <associate|footnr-2|<tuple|2|5>>
    <associate|footnr-3|<tuple|3|7>>
    <associate|footnr-4|<tuple|4|9>>
    <associate|footnr-5|<tuple|5|14>>
    <associate|footnr-6|<tuple|6|14>>
    <associate|footnr-7|<tuple|7|15>>
    <associate|footnr-8|<tuple|8|21>>
    <associate|footnr-9|<tuple|9|28>>
    <associate|for-loop-container|<tuple|65|32>>
    <associate|for-loop-integer-range|<tuple|64|32>>
    <associate|good-function|<tuple|60|31>>
    <associate|guard|<tuple|32|16>>
    <associate|if-then|<tuple|3|3>>
    <associate|if-then-else|<tuple|21|13>>
    <associate|if-then-else-colorized|<tuple|22|14>>
    <associate|if-then-else-definition|<tuple|59|31>>
    <associate|implementation-notes|<tuple|6|37>>
    <associate|import-statement|<tuple|4.4.1|34>>
    <associate|import-statement-example|<tuple|73|34>>
    <associate|index-operators|<tuple|3.1.7|17>>
    <associate|infinite-data-structures|<tuple|5.7.7|36>>
    <associate|infinite-list|<tuple|76|37>>
    <associate|infinite-loop|<tuple|4.2.3|32>>
    <associate|infix-type|<tuple|57|27>>
    <associate|iterations|<tuple|3|?>>
    <associate|lazy-evaluation|<tuple|3.3.3|21>>
    <associate|list-operations|<tuple|4.1.10|30>>
    <associate|list-operations-table|<tuple|11|30>>
    <associate|literals|<tuple|2.3|5>>
    <associate|llvm-operations|<tuple|12|37>>
    <associate|local-and-nonlocal-assignment|<tuple|21|13>>
    <associate|long-text-indent|<tuple|9|6>>
    <associate|machine-interface|<tuple|6.14|39>>
    <associate|machine-types|<tuple|6.15|39>>
    <associate|making-two-types-equivalent|<tuple|44|24>>
    <associate|map-reduce-filter|<tuple|2|2>>
    <associate|math-operations|<tuple|6|29>>
    <associate|mathematical-functions|<tuple|5|?>>
    <associate|module-definition|<tuple|74|35>>
    <associate|more-specific-complex-types|<tuple|45|24>>
    <associate|nonlocal-assignment|<tuple|6.15|46>>
    <associate|object-oriented-programming|<tuple|5.6|36>>
    <associate|odd-type|<tuple|34|16>>
    <associate|off-side-rule|<tuple|4|4>>
    <associate|opcode-declaration|<tuple|36|18>>
    <associate|other-for-loops|<tuple|66|32>>
    <associate|out-of-order-declarations|<tuple|23|14>>
    <associate|parameterized-type|<tuple|56|27>>
    <associate|person-properties|<tuple|26|?>>
    <associate|precedence|<tuple|2.6|9>>
    <associate|properties-declaration|<tuple|47|25>>
    <associate|range-type-definitino|<tuple|48|?>>
    <associate|range-type-definition|<tuple|67|33>>
    <associate|ranges-as-lists|<tuple|68|33>>
    <associate|references|<tuple|3.4|?>>
    <associate|return-type-declaration|<tuple|20|?>>
    <associate|rewrite-code|<tuple|78|38>>
    <associate|rewrite-type|<tuple|58|27>>
    <associate|sequence|<tuple|33|17>>
    <associate|setting-default-arguments|<tuple|49|25>>
    <associate|simple-type|<tuple|40|23>>
    <associate|simpleprog|<tuple|8|3>>
    <associate|simultaneously-type-and-data|<tuple|47|?>>
    <associate|source-syntax|<tuple|12|11>>
    <associate|standard-evaluation|<tuple|3.3.1|19>>
    <associate|state-of-implementation|<tuple|1.5|4>>
    <associate|structured-data|<tuple|34|17>>
    <associate|syntax-file|<tuple|11|10>>
    <associate|tail-recursion|<tuple|6.12|39>>
    <associate|text-operations|<tuple|7|29>>
    <associate|time-operations|<tuple|9|30>>
    <associate|tree-operations|<tuple|4.1.9|30>>
    <associate|tree-operations-table|<tuple|10|30>>
    <associate|tree-rewrite-operators|<tuple|3.1|12>>
    <associate|type-conversion|<tuple|34|17>>
    <associate|type-conversions|<tuple|3.4.7|26>>
    <associate|type-declaration|<tuple|26|15>>
    <associate|type-declaration-type|<tuple|72|34>>
    <associate|type-definition|<tuple|3.4.2|23>>
    <associate|type-name|<tuple|34|17>>
    <associate|types|<tuple|3.4|23>>
    <associate|union-type-definition|<tuple|69|33>>
    <associate|until-loop|<tuple|4.2.4|32>>
    <associate|using-automatic-type-conversion|<tuple|35|?>>
    <associate|using-complex|<tuple|42|24>>
    <associate|using-union-types|<tuple|70|33>>
    <associate|while-loop|<tuple|62|32>>
    <associate|xlsyntax|<tuple|2.1|?>>
  </collection>
</references>

<\auxiliary>
  <\collection>
    <\associate|figure>
      <tuple|normal|Declaration of the factorial function|<pageref|auto-22>>

      <tuple|normal|Map, reduce and filter|<pageref|auto-27>>

      <tuple|normal|Declaration of if-then-else|<pageref|auto-29>>

      <tuple|normal|Off-side rule: Using indentation to mark program
      structure.|<pageref|auto-57>>

      <tuple|normal|Single-line and multi-line comments|<pageref|auto-60>>

      <tuple|normal|Valid integer constants|<pageref|auto-68>>

      <tuple|normal|Valid real constants|<pageref|auto-74>>

      <tuple|normal|Valid text constants|<pageref|auto-85>>

      <tuple|normal|Long text and indentation|<pageref|auto-87>>

      <tuple|normal|Examples of valid operator and name
      symbols|<pageref|auto-95>>

      <tuple|normal|Default syntax configuration file|<pageref|auto-147>>

      <tuple|normal|Use of the <with|font-family|<quote|tt>|language|<quote|verbatim>|syntax>
      specification in a source file|<pageref|auto-149>>

      <tuple|normal|C syntax configuration file|<pageref|auto-162>>

      <tuple|normal|Example of rewrite declaration|<pageref|auto-173>>

      <tuple|normal|Example of data declaration|<pageref|auto-175>>

      <tuple|normal|Example of data declarations containing type
      declarations|<pageref|auto-177>>

      <tuple|normal|Example of guard to build the Syracuse
      suite|<pageref|auto-179>>

      <tuple|normal|Example of assignment|<pageref|auto-182>>

      <tuple|normal|Example of sequence|<pageref|auto-186>>

      <tuple|normal|Examples of index operators|<pageref|auto-188>>

      <tuple|normal|Examples of tree rewrites|<pageref|auto-193>>

      <tuple|normal|Constants vs. Variable symbols|<pageref|auto-197>>

      <tuple|normal|Declarations are visible to the entire sequence
      containing them|<pageref|auto-204>>

      <tuple|normal|Declaring a comma-separated list|<pageref|auto-208>>

      <tuple|normal|Declaring a <with|font-family|<quote|tt>|language|<quote|verbatim>|complex>
      data type|<pageref|auto-210>>

      <tuple|normal|Simple type declarations|<pageref|auto-216>>

      <tuple|normal|Creating a new binding<label|creating-a-new-binding>|<pageref|auto-225>>

      <tuple|normal|Assignment to existing
      binding<label|nonlocal-assignment>|<pageref|auto-226>>

      <tuple|normal|Assigning to new local variable
      <label|assign-to-new-local>|<pageref|auto-235>>

      <tuple|normal|Assignment to references|<pageref|auto-237>>

      <tuple|normal|Assigning to parameter|<pageref|auto-241>>

      <tuple|normal|Guard limit the validity of
      operations|<pageref|auto-248>>

      <tuple|normal|Code writing <with|font-family|<quote|tt>|language|<quote|verbatim>|A>,
      then <with|font-family|<quote|tt>|language|<quote|verbatim>|B>, then
      <with|font-family|<quote|tt>|language|<quote|verbatim>|f(100)+f(200)>|<pageref|auto-253>>

      <tuple|normal|Structured data|<pageref|auto-264>>

      <tuple|normal|Creating an interface for a C
      function|<pageref|auto-270>>

      <tuple|normal|Generating machine code using opcode
      declarations|<pageref|auto-278>>

      <tuple|normal|Evaluation for comparison|<pageref|auto-321>>

      <tuple|normal|Evaluation for type comparison|<pageref|auto-322>>

      <tuple|normal|Explicit vs. lazy evaluation|<pageref|auto-328>>

      <tuple|normal|Simple type declaration|<pageref|auto-356>>

      <tuple|normal|Simple type declaration|<pageref|auto-360>>

      <tuple|normal|Using the <with|font-family|<quote|tt>|language|<quote|verbatim>|complex>
      type|<pageref|auto-363>>

      <tuple|normal|Binding for a <with|font-family|<quote|tt>|language|<quote|verbatim>|complex>
      parameter|<pageref|auto-367>>

      <tuple|normal|Making type <with|font-family|<quote|tt>|language|<quote|verbatim>|A>
      equivalent to type <with|font-family|<quote|tt>|language|<quote|verbatim>|B>|<pageref|auto-368>>

      <tuple|normal|Named patterns for <with|font-family|<quote|tt>|language|<quote|verbatim>|complex>|<pageref|auto-373>>

      <tuple|normal|Creating a normal form for the complex
      type|<pageref|auto-375>>

      <tuple|normal|Properties declaration|<pageref|auto-381>>

      <tuple|normal|Color properties|<pageref|auto-390>>

      <tuple|normal|Setting default arguments from the current
      context|<pageref|auto-393>>

      <tuple|normal|Additional code in properties|<pageref|auto-396>>

      <tuple|normal|Data inheritance|<pageref|auto-400>>

      <tuple|normal|Defining a type identifying an arbitrary AST
      shape|<pageref|auto-409>>

      <tuple|normal|Explicit type check|<pageref|auto-411>>

      <tuple|normal|Explicit type conversion|<pageref|auto-415>>

      <tuple|normal|Automatic type conversion|<pageref|auto-417>>

      <tuple|normal|Parameterized type|<pageref|auto-421>>

      <tuple|normal|Declaring a range type using an infix
      form|<pageref|auto-422>>

      <tuple|normal|Declaration of a rewrite type|<pageref|auto-426>>

      <tuple|normal|Library definition of if-then-else|<pageref|auto-485>>

      <tuple|normal|The <with|font-family|<quote|tt>|language|<quote|verbatim>|good>
      function|<pageref|auto-487>>

      <tuple|normal|Infinite loop|<pageref|auto-489>>

      <tuple|normal|While loop|<pageref|auto-491>>

      <tuple|normal|Until loop|<pageref|auto-492>>

      <tuple|normal|For loop on an integer range|<pageref|auto-494>>

      <tuple|normal|For loop on a container|<pageref|auto-495>>

      <tuple|normal|Other kinds of <with|font-family|<quote|tt>|language|<quote|verbatim>|for>
      loop|<pageref|auto-496>>

      <\tuple|normal>
        Range and range type definition
      </tuple|<pageref|auto-501>>

      <tuple|normal|Ranges as lists|<pageref|auto-503>>

      <tuple|normal|Union type definition|<pageref|auto-505>>

      <tuple|normal|Using union types|<pageref|auto-506>>

      <tuple|normal|Enumeration type definition|<pageref|auto-508>>

      <tuple|normal|Type matching a type declaration|<pageref|auto-510>>

      <tuple|normal|Import statements examples|<pageref|auto-516>>

      <tuple|normal|Module definition|<pageref|auto-527>>

      <tuple|normal|Computing a minimum and a maximum|<pageref|auto-530>>

      <tuple|normal|Lazy evaluation of an infinite list|<pageref|auto-551>>

      <tuple|normal|Controlled compilation|<pageref|auto-557>>

      <tuple|normal|Signature for rewrite code with two
      variables.|<pageref|auto-566>>

      <tuple|normal|Signature for rewrite code with two
      variables.|<pageref|auto-569>>
    </associate>
    <\associate|idx>
      <tuple|<tuple|programming paradigm>|<pageref|auto-3>>

      <tuple|<tuple|Extensible language and runtime>|<pageref|auto-4>>

      <tuple|<tuple|ELIOT (eXtensible Language and Runtime>|<pageref|auto-5>>

      <tuple|<tuple|integer type>|<pageref|auto-7>>

      <tuple|<tuple|real type>|<pageref|auto-8>>

      <tuple|<tuple|text type>|<pageref|auto-9>>

      <tuple|<tuple|name type>|<pageref|auto-10>>

      <tuple|<tuple|infix type>|<pageref|auto-11>>

      <tuple|<tuple|prefix type>|<pageref|auto-12>>

      <tuple|<tuple|postfix type>|<pageref|auto-13>>

      <tuple|<tuple|block type>|<pageref|auto-14>>

      <tuple|<tuple|abstract syntax tree>|<pageref|auto-15>>

      <tuple|<tuple|AST (abstract syntax tree)>|<pageref|auto-16>>

      <tuple|<tuple|domain-specific language>|<pageref|auto-17>>

      <tuple|<tuple|DSL (domain-specific language)>|<pageref|auto-18>>

      <tuple|<tuple|functional programming>|<pageref|auto-20>>

      <tuple|<tuple|meta-programming>|<pageref|auto-21>>

      <tuple|<tuple|map>|<pageref|auto-23>>

      <tuple|<tuple|reduce>|<pageref|auto-24>>

      <tuple|<tuple|filter>|<pageref|auto-25>>

      <tuple|<tuple|functional programming>|<pageref|auto-26>>

      <tuple|<tuple|if-then-else|statement>|<pageref|auto-28>>

      <tuple|<tuple|concept>|<pageref|auto-31>>

      <tuple|<tuple|concept space>|<pageref|auto-32>>

      <tuple|<tuple|code space>|<pageref|auto-33>>

      <tuple|<tuple|pseudo-metric>|<pageref|auto-36>>

      <tuple|<tuple|syntactic noise>|<pageref|auto-37>>

      <tuple|<tuple|semantic noise>|<pageref|auto-38>>

      <tuple|<tuple|bandwidth>|<pageref|auto-39>>

      <tuple|<tuple|signal-noise ratio>|<pageref|auto-40>>

      <tuple|<tuple|art>|<pageref|auto-41>>

      <tuple|<tuple|noise>|<pageref|auto-42>>

      <tuple|<tuple|music>|<pageref|auto-43>>

      <tuple|<tuple|abstract syntax tree>|<pageref|auto-47>>

      <tuple|<tuple|XL0 (abstract syntax tree for ELIOT)>|<pageref|auto-48>>

      <tuple|<tuple|xl.syntax>|<pageref|auto-49>>

      <tuple|<tuple|syntax configuration>|<pageref|auto-50>>

      <tuple|<tuple|normal ELIOT>|<pageref|auto-51>>

      <tuple|<tuple|off-side rule>|<pageref|auto-53>>

      <tuple|<tuple|spaces (for indentation)>|<pageref|auto-54>>

      <tuple|<tuple|tabs (for indentation)>|<pageref|auto-55>>

      <tuple|<tuple|indentation>|<pageref|auto-56>>

      <tuple|<tuple|comments>|<pageref|auto-59>>

      <tuple|<tuple|literal node types>|<pageref|auto-62>>

      <tuple|<tuple|integer constant>|<pageref|auto-64>>

      <tuple|<tuple|radix|in integer numbers>|<pageref|auto-65>>

      <tuple|<tuple|hash sign (as a radix delimiter)>|<pageref|auto-66>>

      <tuple|<tuple|underscore|as digit separator>|<pageref|auto-67>>

      <tuple|<tuple|dot|as decimal separator>|<pageref|auto-70>>

      <tuple|<tuple|radix|in real numbers>|<pageref|auto-71>>

      <tuple|<tuple|underscore|as digit separator>|<pageref|auto-72>>

      <tuple|<tuple|exponent (for real constants)>|<pageref|auto-73>>

      <tuple|<tuple|text literals>|<pageref|auto-76>>

      <tuple|<tuple|UTF-8>|<pageref|auto-77>>

      <tuple|<tuple|line-terminating characters>|<pageref|auto-78>>

      <tuple|<tuple|control characters>|<pageref|auto-79>>

      <tuple|<tuple|text delimiters>|<pageref|auto-80>>

      <tuple|<tuple|quote>|<pageref|auto-81>>

      <tuple|<tuple|single quote>|<pageref|auto-82>>

      <tuple|<tuple|double quote>|<pageref|auto-83>>

      <tuple|<tuple|long text>|<pageref|auto-84>>

      <tuple|<tuple|indentation (in long text)>|<pageref|auto-86>>

      <tuple|<tuple|value (of text literals)>|<pageref|auto-88>>

      <tuple|<tuple|symbols>|<pageref|auto-91>>

      <tuple|<tuple|name>|<pageref|auto-92>>

      <tuple|<tuple|operator symbols>|<pageref|auto-93>>

      <tuple|<tuple|expression vs. statement>|<pageref|auto-94>>

      <tuple|<tuple|structured node types>|<pageref|auto-97>>

      <tuple|<tuple|infix>|<pageref|auto-98>>

      <tuple|<tuple|prefix>|<pageref|auto-99>>

      <tuple|<tuple|postfix>|<pageref|auto-100>>

      <tuple|<tuple|block>|<pageref|auto-101>>

      <tuple|<tuple|child node>|<pageref|auto-102>>

      <tuple|<tuple|infix>|<pageref|auto-104>>

      <tuple|<tuple|prefix>|<pageref|auto-106>>

      <tuple|<tuple|postfix>|<pageref|auto-107>>

      <tuple|<tuple|operand (in prefix and postfix)>|<pageref|auto-108>>

      <tuple|<tuple|xl.syntax>|<pageref|auto-109>>

      <tuple|<tuple|function precedence>|<pageref|auto-110>>

      <tuple|<tuple|function>|<pageref|auto-111>>

      <tuple|<tuple|block>|<pageref|auto-113>>

      <tuple|<tuple|block delimiters>|<pageref|auto-114>>

      <tuple|<tuple|parsing>|<pageref|auto-116>>

      <tuple|<tuple|precedence>|<pageref|auto-117>>

      <tuple|<tuple|associativity>|<pageref|auto-118>>

      <tuple|<tuple|infix vs. prefix vs. postfix>|<pageref|auto-119>>

      <tuple|<tuple|expression vs. statement>|<pageref|auto-120>>

      <tuple|<tuple|precedence>|<pageref|auto-122>>

      <tuple|<tuple|xl.syntax>|<pageref|auto-123>>

      <tuple|<tuple|syntax statement>|<pageref|auto-124>>

      <tuple|<tuple|associativity>|<pageref|auto-126>>

      <tuple|<tuple|infix vs. prefix vs. postfix>|<pageref|auto-128>>

      <tuple|<tuple|parsing>|<pageref|auto-129>>

      <tuple|<tuple|parsing ambiguities>|<pageref|auto-130>>

      <tuple|<tuple|default prefix (precedence)>|<pageref|auto-131>>

      <tuple|<tuple|function precedence>|<pageref|auto-132>>

      <tuple|<tuple|syntax configuration>|<pageref|auto-133>>

      <tuple|<tuple|expression vs. statement>|<pageref|auto-135>>

      <tuple|<tuple|parsing ambiguities>|<pageref|auto-136>>

      <tuple|<tuple|statement>|<pageref|auto-137>>

      <tuple|<tuple|expression (as opposed to statement)>|<pageref|auto-138>>

      <tuple|<tuple|subject and complement>|<pageref|auto-139>>

      <tuple|<tuple|statement precedence>|<pageref|auto-140>>

      <tuple|<tuple|syntax configuration>|<pageref|auto-142>>

      <tuple|<tuple|xl.syntax>|<pageref|auto-143>>

      <tuple|<tuple|operators>|<pageref|auto-144>>

      <tuple|<tuple|standard operators>|<pageref|auto-145>>

      <tuple|<tuple|precedence>|<pageref|auto-146>>

      <tuple|<tuple|syntax statement>|<pageref|auto-148>>

      <tuple|<tuple|syntax statement>|<pageref|auto-150>>

      <tuple|<tuple|indentation>|<pageref|auto-152>>

      <tuple|<tuple|statement precedence>|<pageref|auto-153>>

      <tuple|<tuple|default precedence>|<pageref|auto-154>>

      <tuple|<tuple|function precedence>|<pageref|auto-155>>

      <tuple|<tuple|block delimiters>|<pageref|auto-156>>

      <tuple|<tuple|indentation>|<pageref|auto-157>>

      <tuple|<tuple|text delimiters>|<pageref|auto-158>>

      <tuple|<tuple|external syntax file>|<pageref|auto-159>>

      <tuple|<tuple|C.syntax file>|<pageref|auto-160>>

      <tuple|<tuple|C symbols>|<pageref|auto-161>>

      <tuple|<tuple|semantics>|<pageref|auto-164>>

      <tuple|<tuple|execution (of programs)>|<pageref|auto-165>>

      <tuple|<tuple|evaluation>|<pageref|auto-166>>

      <tuple|<tuple|tree rewrite>|<pageref|auto-168>>

      <tuple|<tuple|tree rewrite operators>|<pageref|auto-169>>

      <tuple|<tuple|rewrite declarations>|<pageref|auto-170>>

      <tuple|<tuple|pattern>|<pageref|auto-171>>

      <tuple|<tuple|implementation>|<pageref|auto-172>>

      <tuple|<tuple|data declarations>|<pageref|auto-174>>

      <tuple|<tuple|type declarations>|<pageref|auto-176>>

      <tuple|<tuple|guard (in a rewrite declaration)>|<pageref|auto-178>>

      <tuple|<tuple|assignment>|<pageref|auto-180>>

      <tuple|<tuple|binding>|<pageref|auto-181>>

      <tuple|<tuple|sequence>|<pageref|auto-183>>

      <tuple|<tuple|sequence operator>|<pageref|auto-184>>

      <tuple|<tuple|evaluation order>|<pageref|auto-185>>

      <tuple|<tuple|index operator>|<pageref|auto-187>>

      <tuple|<tuple|declaration|of rewrites>|<pageref|auto-190>>

      <tuple|<tuple|rewrite declaration>|<pageref|auto-191>>

      <tuple|<tuple|if-then-else|statement>|<pageref|auto-192>>

      <tuple|<tuple|pattern>|<pageref|auto-194>>

      <tuple|<tuple|constant>|<pageref|auto-195>>

      <tuple|<tuple|variable>|<pageref|auto-196>>

      <tuple|<tuple|constant symbols>|<pageref|auto-198>>

      <tuple|<tuple|parameter>|<pageref|auto-199>>

      <tuple|<tuple|argument>|<pageref|auto-200>>

      <tuple|<tuple|anonymous function>|<pageref|auto-201>>

      <tuple|<tuple|lambda function>|<pageref|auto-202>>

      <tuple|<tuple|context>|<pageref|auto-203>>

      <tuple|<tuple|declaration|of data>|<pageref|auto-206>>

      <tuple|<tuple|data declaration>|<pageref|auto-207>>

      <tuple|<tuple|evaluation|data declaration
      arguments>|<pageref|auto-209>>

      <tuple|<tuple|self>|<pageref|auto-211>>

      <tuple|<tuple|declaration|of types>|<pageref|auto-213>>

      <tuple|<tuple|type declaration>|<pageref|auto-214>>

      <tuple|<tuple|return type declaration>|<pageref|auto-215>>

      <tuple|<tuple|overloading>|<pageref|auto-217>>

      <tuple|<tuple|type declaration|in assignment>|<pageref|auto-218>>

      <tuple|<tuple|assignment|to type declaration>|<pageref|auto-219>>

      <tuple|<tuple|assignment>|<pageref|auto-221>>

      <tuple|<tuple|:=>|<pageref|auto-222>>

      <tuple|<tuple|evaluation|in assignment>|<pageref|auto-223>>

      <tuple|<tuple|binding|in assignment>|<pageref|auto-224>>

      <tuple|<tuple|assignment|to type declaration>|<pageref|auto-228>>

      <tuple|<tuple|type declaration|in assignment>|<pageref|auto-229>>

      <tuple|<tuple|binding|local scope>|<pageref|auto-230>>

      <tuple|<tuple|local scope>|<pageref|auto-231>>

      <tuple|<tuple|scope|local>|<pageref|auto-232>>

      <tuple|<tuple|return type declaration|in
      assignment>|<pageref|auto-233>>

      <tuple|<tuple|binding|with return type declaration>|<pageref|auto-234>>

      <tuple|<tuple|expression|allowed on left of
      assignment>|<pageref|auto-238>>

      <tuple|<tuple|assignment|to parameter>|<pageref|auto-240>>

      <tuple|<tuple|assignment|in expression>|<pageref|auto-243>>

      <tuple|<tuple|expression|assignment as expression>|<pageref|auto-244>>

      <tuple|<tuple|guard>|<pageref|auto-246>>

      <tuple|<tuple|when infix operator>|<pageref|auto-247>>

      <tuple|<tuple|sequence>|<pageref|auto-250>>

      <tuple|<tuple|sequence|evaluation order>|<pageref|auto-251>>

      <tuple|<tuple|evaluation|order>|<pageref|auto-252>>

      <tuple|<tuple|declaration>|<pageref|auto-254>>

      <tuple|<tuple|statement>|<pageref|auto-255>>

      <tuple|<tuple|index operator>|<pageref|auto-257>>

      <tuple|<tuple|dot|as index operator>|<pageref|auto-258>>

      <tuple|<tuple|array|index>|<pageref|auto-259>>

      <tuple|<tuple|array index>|<pageref|auto-260>>

      <tuple|<tuple|index|array>|<pageref|auto-261>>

      <tuple|<tuple|field index>|<pageref|auto-262>>

      <tuple|<tuple|index|field>|<pageref|auto-263>>

      <tuple|<tuple|array|as function>|<pageref|auto-266>>

      <tuple|<tuple|C interface>|<pageref|auto-268>>

      <tuple|<tuple|extern syntax>|<pageref|auto-269>>

      <tuple|<tuple|C.syntax>|<pageref|auto-271>>

      <tuple|<tuple|xl.syntax|connexion to C.syntax>|<pageref|auto-272>>

      <tuple|<tuple|C.syntax|connexion to xl.syntax>|<pageref|auto-273>>

      <tuple|<tuple|machine interface>|<pageref|auto-276>>

      <tuple|<tuple|opcode>|<pageref|auto-277>>

      <tuple|<tuple|binding>|<pageref|auto-280>>

      <tuple|<tuple|context>|<pageref|auto-281>>

      <tuple|<tuple|context order>|<pageref|auto-283>>

      <tuple|<tuple|shadowed binding>|<pageref|auto-284>>

      <tuple|<tuple|scope>|<pageref|auto-286>>

      <tuple|<tuple|scope|local>|<pageref|auto-287>>

      <tuple|<tuple|context|enclosing>|<pageref|auto-288>>

      <tuple|<tuple|scope|enclosing>|<pageref|auto-289>>

      <tuple|<tuple|scope|global>|<pageref|auto-290>>

      <tuple|<tuple|catch-all rewrite>|<pageref|auto-291>>

      <tuple|<tuple|undefined form>|<pageref|auto-292>>

      <tuple|<tuple|current context>|<pageref|auto-294>>

      <tuple|<tuple|context|current>|<pageref|auto-295>>

      <tuple|<tuple|scope|creation>|<pageref|auto-296>>

      <tuple|<tuple|assignment>|<pageref|auto-297>>

      <tuple|<tuple|reference>|<pageref|auto-299>>

      <tuple|<tuple|index operator>|<pageref|auto-300>>

      <tuple|<tuple|evaluation>|<pageref|auto-302>>

      <tuple|<tuple|evaluation|standard case>|<pageref|auto-304>>

      <tuple|<tuple|context order>|<pageref|auto-305>>

      <tuple|<tuple|pattern|matching>|<pageref|auto-306>>

      <tuple|<tuple|evaluation|of arguments>|<pageref|auto-307>>

      <tuple|<tuple|memoization|of arguments>|<pageref|auto-308>>

      <tuple|<tuple|evaluation|mismatch>|<pageref|auto-309>>

      <tuple|<tuple|binding|parameters>|<pageref|auto-310>>

      <tuple|<tuple|context|passed with arguments>|<pageref|auto-311>>

      <tuple|<tuple|closure>|<pageref|auto-312>>

      <tuple|<tuple|context|parameter context>|<pageref|auto-313>>

      <tuple|<tuple|special forms>|<pageref|auto-315>>

      <tuple|<tuple|evaluation|special forms>|<pageref|auto-316>>

      <tuple|<tuple|lazy evaluation>|<pageref|auto-318>>

      <tuple|<tuple|evaluation|lazy>|<pageref|auto-319>>

      <tuple|<tuple|evaluation|demand-based>|<pageref|auto-320>>

      <tuple|<tuple|explicit evaluation>|<pageref|auto-324>>

      <tuple|<tuple|evaluation|explicit>|<pageref|auto-325>>

      <tuple|<tuple|memoization|of parameters>|<pageref|auto-327>>

      <tuple|<tuple|evaluation|explicit vs. lazy>|<pageref|auto-329>>

      <tuple|<tuple|evaluation|forcing explicit
      evaluation>|<pageref|auto-330>>

      <tuple|<tuple|type>|<pageref|auto-332>>

      <tuple|<tuple|type declaration>|<pageref|auto-333>>

      <tuple|<tuple|type|declaration>|<pageref|auto-334>>

      <tuple|<tuple|type|belonging to a type>|<pageref|auto-335>>

      <tuple|<tuple|type|predefined>|<pageref|auto-337>>

      <tuple|<tuple|predefined types>|<pageref|auto-338>>

      <tuple|<tuple|integer>|<pageref|auto-339>>

      <tuple|<tuple|real>|<pageref|auto-340>>

      <tuple|<tuple|text>|<pageref|auto-341>>

      <tuple|<tuple|symbol>|<pageref|auto-342>>

      <tuple|<tuple|name>|<pageref|auto-343>>

      <tuple|<tuple|operator>|<pageref|auto-344>>

      <tuple|<tuple|infix>|<pageref|auto-345>>

      <tuple|<tuple|prefix>|<pageref|auto-346>>

      <tuple|<tuple|postfix>|<pageref|auto-347>>

      <tuple|<tuple|block>|<pageref|auto-348>>

      <tuple|<tuple|tree>|<pageref|auto-349>>

      <tuple|<tuple|boolean>|<pageref|auto-350>>

      <tuple|<tuple|type definition>|<pageref|auto-352>>

      <tuple|<tuple|type|definition>|<pageref|auto-353>>

      <tuple|<tuple|definition|of types>|<pageref|auto-354>>

      <tuple|<tuple|if-then-else|type>|<pageref|auto-355>>

      <tuple|<tuple|type pattern>|<pageref|auto-357>>

      <tuple|<tuple|type|pattern>|<pageref|auto-358>>

      <tuple|<tuple|pattern|in type>|<pageref|auto-359>>

      <tuple|<tuple|type declaration|vs. type definition>|<pageref|auto-361>>

      <tuple|<tuple|type definition|vs. type declaration>|<pageref|auto-362>>

      <tuple|<tuple|parameters|of types>|<pageref|auto-364>>

      <tuple|<tuple|bindings|in type definitions>|<pageref|auto-365>>

      <tuple|<tuple|index|for user-defined types>|<pageref|auto-366>>

      <tuple|<tuple|normal form>|<pageref|auto-370>>

      <tuple|<tuple|type|normal form>|<pageref|auto-371>>

      <tuple|<tuple|pattern|making type pattern specific>|<pageref|auto-372>>

      <tuple|<tuple|type|multiple notations>|<pageref|auto-374>>

      <tuple|<tuple|properties>|<pageref|auto-377>>

      <tuple|<tuple|type|properties>|<pageref|auto-378>>

      <tuple|<tuple|property>|<pageref|auto-379>>

      <tuple|<tuple|inherit>|<pageref|auto-380>>

      <tuple|<tuple|parameters|with properties types>|<pageref|auto-382>>

      <tuple|<tuple|property definition>|<pageref|auto-383>>

      <tuple|<tuple|definition|of properties>|<pageref|auto-384>>

      <tuple|<tuple|default value>|<pageref|auto-385>>

      <tuple|<tuple|property|default value>|<pageref|auto-386>>

      <tuple|<tuple|property|setting>|<pageref|auto-387>>

      <tuple|<tuple|properties|as parameter types>|<pageref|auto-388>>

      <tuple|<tuple|properties|arguments>|<pageref|auto-389>>

      <tuple|<tuple|getter>|<pageref|auto-391>>

      <tuple|<tuple|setter>|<pageref|auto-392>>

      <tuple|<tuple|required property>|<pageref|auto-394>>

      <tuple|<tuple|property|required>|<pageref|auto-395>>

      <tuple|<tuple|inherit>|<pageref|auto-398>>

      <tuple|<tuple|data inheritance>|<pageref|auto-399>>

      <tuple|<tuple|automatic type conversion>|<pageref|auto-401>>

      <tuple|<tuple|explicit type check>|<pageref|auto-403>>

      <tuple|<tuple|type|explicit type check>|<pageref|auto-404>>

      <tuple|<tuple|type check>|<pageref|auto-405>>

      <tuple|<tuple|type|check>|<pageref|auto-406>>

      <tuple|<tuple|contains>|<pageref|auto-407>>

      <tuple|<tuple|type|identifying arbitrary tree
      shapes>|<pageref|auto-408>>

      <tuple|<tuple|is_a>|<pageref|auto-410>>

      <tuple|<tuple|explicit type conversion>|<pageref|auto-413>>

      <tuple|<tuple|type|conversions>|<pageref|auto-414>>

      <tuple|<tuple|automatic type conversion>|<pageref|auto-416>>

      <tuple|<tuple|parameterized types>|<pageref|auto-419>>

      <tuple|<tuple|type|parameterized type>|<pageref|auto-420>>

      <tuple|<tuple|rewrite type>|<pageref|auto-424>>

      <tuple|<tuple|type|rewrite type>|<pageref|auto-425>>

      <tuple|<tuple|library>|<pageref|auto-428>>

      <tuple|<tuple|built-in operations>|<pageref|auto-430>>

      <tuple|<tuple|xl.syntax>|<pageref|auto-431>>

      <tuple|<tuple|builtins.xl>|<pageref|auto-432>>

      <tuple|<tuple|arithmetic>|<pageref|auto-434>>

      <tuple|<tuple|power operator>|<pageref|auto-435>>

      <tuple|<tuple|comparisons>|<pageref|auto-438>>

      <tuple|<tuple|bitwise arithmetic>|<pageref|auto-441>>

      <tuple|<tuple|arithmetic|bitwise>|<pageref|auto-442>>

      <tuple|<tuple|boolean>|<pageref|auto-445>>

      <tuple|<tuple|mathematical functions>|<pageref|auto-448>>

      <tuple|<tuple|text functions>|<pageref|auto-451>>

      <tuple|<tuple|conversions>|<pageref|auto-454>>

      <tuple|<tuple|type|conversions>|<pageref|auto-455>>

      <tuple|<tuple|conversion|from text to number>|<pageref|auto-457>>

      <tuple|<tuple|conversion|from number to text>|<pageref|auto-458>>

      <tuple|<tuple|date and time>|<pageref|auto-460>>

      <tuple|<tuple|tree|operations>|<pageref|auto-463>>

      <tuple|<tuple|AST|manipulations>|<pageref|auto-464>>

      <tuple|<tuple|left>|<pageref|auto-466>>

      <tuple|<tuple|right>|<pageref|auto-467>>

      <tuple|<tuple|child>|<pageref|auto-468>>

      <tuple|<tuple|symbol>|<pageref|auto-469>>

      <tuple|<tuple|opening>|<pageref|auto-470>>

      <tuple|<tuple|closing>|<pageref|auto-471>>

      <tuple|<tuple|list operations>|<pageref|auto-473>>

      <tuple|<tuple|list|operations on lists>|<pageref|auto-474>>

      <tuple|<tuple|list|comma-separated>|<pageref|auto-475>>

      <tuple|<tuple|map operation>|<pageref|auto-477>>

      <tuple|<tuple|reduce operation>|<pageref|auto-478>>

      <tuple|<tuple|filter operation>|<pageref|auto-479>>

      <tuple|<tuple|predicate>|<pageref|auto-480>>

      <tuple|<tuple|range>|<pageref|auto-481>>

      <tuple|<tuple|if-then-else|library definition>|<pageref|auto-484>>

      <tuple|<tuple|good (function)>|<pageref|auto-486>>

      <tuple|<tuple|interval arithmetic>|<pageref|auto-502>>

      <tuple|<tuple|module>|<pageref|auto-513>>

      <tuple|<tuple|import>|<pageref|auto-514>>

      <tuple|<tuple|module|import>|<pageref|auto-515>>

      <tuple|<tuple|module path>|<pageref|auto-517>>

      <tuple|<tuple|short module name>|<pageref|auto-518>>

      <tuple|<tuple|import|with a short name>|<pageref|auto-519>>

      <tuple|<tuple|syntax|in modules>|<pageref|auto-520>>

      <tuple|<tuple|scope|for modules>|<pageref|auto-521>>

      <tuple|<tuple|shadowing|in modules>|<pageref|auto-522>>

      <tuple|<tuple|binding|of module names>|<pageref|auto-523>>

      <tuple|<tuple|module description>|<pageref|auto-525>>

      <tuple|<tuple|module|description>|<pageref|auto-526>>

      <tuple|<tuple|symbols>|<pageref|auto-90>>
    </associate>
    <\associate|table>
      <tuple|normal|Type correspondances in a C interface|<pageref|auto-274>>

      <tuple|normal|Arithmetic operations|<pageref|auto-436>>

      <tuple|normal|Comparisons|<pageref|auto-439>>

      <tuple|normal|Bitwise arithmetic operations|<pageref|auto-443>>

      <tuple|normal|Boolean operations|<pageref|auto-446>>

      <tuple|normal|Mathematical operations|<pageref|auto-449>>

      <tuple|normal|Text operations|<pageref|auto-452>>

      <tuple|normal|Conversions|<pageref|auto-456>>

      <tuple|normal|Date and time|<pageref|auto-461>>

      <tuple|normal|Tree operations|<pageref|auto-465>>

      <tuple|normal|List operations|<pageref|auto-476>>

      <tuple|normal|LLVM operations|<pageref|auto-558>>
    </associate>
    <\associate|toc>
      <vspace*|1fn><with|font-series|<quote|bold>|math-font-series|<quote|bold>|1<space|2spc>Introduction>
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-1><vspace|0.5fn>

      <with|par-left|<quote|1.5fn>|1.1<space|2spc>Design objectives
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-2>>

      <with|par-left|<quote|1.5fn>|1.2<space|2spc>Keeping it simple
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-6>>

      <with|par-left|<quote|1.5fn>|1.3<space|2spc>Examples
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-19>>

      <with|par-left|<quote|1.5fn>|1.4<space|2spc>Concept programming
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-30>>

      <with|par-left|<quote|6fn>|From concept to code: a lossy conversion
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-34><vspace|0.15fn>>

      <with|par-left|<quote|6fn>|Pseudo-metrics
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-35><vspace|0.15fn>>

      <with|par-left|<quote|6fn>|Influence on ELIOT
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-44><vspace|0.15fn>>

      <with|par-left|<quote|1.5fn>|1.5<space|2spc>State of the implementation
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-45>>

      <vspace*|1fn><with|font-series|<quote|bold>|math-font-series|<quote|bold>|2<space|2spc>Syntax>
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-46><vspace|0.5fn>

      <with|par-left|<quote|1.5fn>|2.1<space|2spc>Spaces and indentation
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-52>>

      <with|par-left|<quote|1.5fn>|2.2<space|2spc>Comments and spaces
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-58>>

      <with|par-left|<quote|1.5fn>|2.3<space|2spc>Literals
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-61>>

      <with|par-left|<quote|3fn>|2.3.1<space|2spc>Integer constants
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-63>>

      <with|par-left|<quote|3fn>|2.3.2<space|2spc>Real constants
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-69>>

      <with|par-left|<quote|3fn>|2.3.3<space|2spc>Text literals
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-75>>

      <with|par-left|<quote|3fn>|2.3.4<space|2spc>Name and operator
      symbols<flag|index|dark green|key><assign|auto-nr|<quote|90>><label|auto-90><write|idx|<tuple|<tuple|symbols>|<pageref|auto-90>>>
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-90>>

      <with|par-left|<quote|1.5fn>|2.4<space|2spc>Structured nodes
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-96>>

      <with|par-left|<quote|3fn>|2.4.1<space|2spc>Infix nodes
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-103>>

      <with|par-left|<quote|3fn>|2.4.2<space|2spc>Prefix and postfix nodes
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-105>>

      <with|par-left|<quote|3fn>|2.4.3<space|2spc>Block nodes
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-112>>

      <with|par-left|<quote|1.5fn>|2.5<space|2spc>Parsing rules
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-115>>

      <with|par-left|<quote|3fn>|2.5.1<space|2spc>Precedence
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-121>>

      <with|par-left|<quote|3fn>|2.5.2<space|2spc>Associativity
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-125>>

      <with|par-left|<quote|3fn>|2.5.3<space|2spc>Infix versus Prefix versus
      Postfix <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-127>>

      <with|par-left|<quote|3fn>|2.5.4<space|2spc>Expression versus statement
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-134>>

      <with|par-left|<quote|1.5fn>|2.6<space|2spc>Syntax configuration
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-141>>

      <with|par-left|<quote|6fn>|Format of syntax configuration
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-151><vspace|0.15fn>>

      <vspace*|1fn><with|font-series|<quote|bold>|math-font-series|<quote|bold>|3<space|2spc>Language
      semantics> <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-163><vspace|0.5fn>

      <with|par-left|<quote|1.5fn>|3.1<space|2spc>Tree rewrite operators
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-167>>

      <with|par-left|<quote|3fn>|3.1.1<space|2spc>Rewrite declarations
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-189>>

      <with|par-left|<quote|3fn>|3.1.2<space|2spc>Data declaration
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-205>>

      <with|par-left|<quote|3fn>|3.1.3<space|2spc>Type declaration
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-212>>

      <with|par-left|<quote|3fn>|3.1.4<space|2spc>Assignment
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-220>>

      <with|par-left|<quote|6fn>|Local variables
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-227><vspace|0.15fn>>

      <with|par-left|<quote|6fn>|Assigning to references
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-236><vspace|0.15fn>>

      <with|par-left|<quote|6fn>|Assigning to parameters
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-239><vspace|0.15fn>>

      <with|par-left|<quote|6fn>|Assignments as expressions
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-242><vspace|0.15fn>>

      <with|par-left|<quote|3fn>|3.1.5<space|2spc>Guards
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-245>>

      <with|par-left|<quote|3fn>|3.1.6<space|2spc>Sequences
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-249>>

      <with|par-left|<quote|3fn>|3.1.7<space|2spc>Index operators
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-256>>

      <with|par-left|<quote|6fn>|Comparison with C
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-265><vspace|0.15fn>>

      <with|par-left|<quote|3fn>|3.1.8<space|2spc>C interface
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-267>>

      <with|par-left|<quote|3fn>|3.1.9<space|2spc>Machine Interface
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-275>>

      <with|par-left|<quote|1.5fn>|3.2<space|2spc>Binding References to
      Values <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-279>>

      <with|par-left|<quote|3fn>|3.2.1<space|2spc>Context Order
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-282>>

      <with|par-left|<quote|3fn>|3.2.2<space|2spc>Scoping
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-285>>

      <with|par-left|<quote|3fn>|3.2.3<space|2spc>Current context
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-293>>

      <with|par-left|<quote|3fn>|3.2.4<space|2spc>References
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-298>>

      <with|par-left|<quote|1.5fn>|3.3<space|2spc>Evaluation
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-301>>

      <with|par-left|<quote|3fn>|3.3.1<space|2spc>Standard evaluation
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-303>>

      <with|par-left|<quote|3fn>|3.3.2<space|2spc>Special forms
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-314>>

      <with|par-left|<quote|3fn>|3.3.3<space|2spc>Lazy evaluation
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-317>>

      <with|par-left|<quote|3fn>|3.3.4<space|2spc>Explicit evaluation
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-323>>

      <with|par-left|<quote|3fn>|3.3.5<space|2spc>Memoization
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-326>>

      <with|par-left|<quote|1.5fn>|3.4<space|2spc>Types
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-331>>

      <with|par-left|<quote|3fn>|3.4.1<space|2spc>Predefined types
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-336>>

      <with|par-left|<quote|3fn>|3.4.2<space|2spc>Type definition
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-351>>

      <with|par-left|<quote|3fn>|3.4.3<space|2spc>Normal form for a type
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-369>>

      <with|par-left|<quote|3fn>|3.4.4<space|2spc>Properties
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-376>>

      <with|par-left|<quote|3fn>|3.4.5<space|2spc>Data inheritance
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-397>>

      <with|par-left|<quote|3fn>|3.4.6<space|2spc>Explicit type check
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-402>>

      <with|par-left|<quote|3fn>|3.4.7<space|2spc>Explicit and automatic type
      conversions <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-412>>

      <with|par-left|<quote|3fn>|3.4.8<space|2spc>Parameterized types
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-418>>

      <with|par-left|<quote|3fn>|3.4.9<space|2spc>Rewrite types
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-423>>

      <vspace*|1fn><with|font-series|<quote|bold>|math-font-series|<quote|bold>|4<space|2spc>Standard
      XL library> <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-427><vspace|0.5fn>

      <with|par-left|<quote|1.5fn>|4.1<space|2spc>Built-in operations
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-429>>

      <with|par-left|<quote|3fn>|4.1.1<space|2spc>Arithmetic
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-433>>

      <with|par-left|<quote|3fn>|4.1.2<space|2spc>Comparison
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-437>>

      <with|par-left|<quote|3fn>|4.1.3<space|2spc>Bitwise arithmetic
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-440>>

      <with|par-left|<quote|3fn>|4.1.4<space|2spc>Boolean operations
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-444>>

      <with|par-left|<quote|3fn>|4.1.5<space|2spc>Mathematical functions
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-447>>

      <with|par-left|<quote|3fn>|4.1.6<space|2spc>Text functions
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-450>>

      <with|par-left|<quote|3fn>|4.1.7<space|2spc>Conversions
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-453>>

      <with|par-left|<quote|3fn>|4.1.8<space|2spc>Date and time
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-459>>

      <with|par-left|<quote|3fn>|4.1.9<space|2spc>Tree operations
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-462>>

      <with|par-left|<quote|3fn>|4.1.10<space|2spc>List operations, map,
      reduce and filter <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-472>>

      <with|par-left|<quote|1.5fn>|4.2<space|2spc>Control structures
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-482>>

      <with|par-left|<quote|3fn>|4.2.1<space|2spc>Tests
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-483>>

      <with|par-left|<quote|3fn>|4.2.2<space|2spc>Infinite Loops
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-488>>

      <with|par-left|<quote|3fn>|4.2.3<space|2spc>Conditional Loops
      (<with|font-family|<quote|tt>|language|<quote|verbatim>|while> and
      <with|font-family|<quote|tt>|language|<quote|verbatim>|until> loops)
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-490>>

      <with|par-left|<quote|3fn>|4.2.4<space|2spc>Controlled Loops
      (<with|font-family|<quote|tt>|language|<quote|verbatim>|for> loops)
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-493>>

      <with|par-left|<quote|3fn>|4.2.5<space|2spc>Excursions<with|font-series|<quote|bold>|math-font-series|<quote|bold>|>
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-497>>

      <with|par-left|<quote|3fn>|4.2.6<space|2spc>Error handling
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-498>>

      <with|par-left|<quote|1.5fn>|4.3<space|2spc>Library-defined types
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-499>>

      <with|par-left|<quote|3fn>|4.3.1<space|2spc>Range and range types
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-500>>

      <with|par-left|<quote|3fn>|4.3.2<space|2spc>Union types
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-504>>

      <with|par-left|<quote|3fn>|4.3.3<space|2spc>Enumeration types
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-507>>

      <with|par-left|<quote|3fn>|4.3.4<space|2spc>A type matching type
      declarations <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-509>>

      <with|par-left|<quote|1.5fn>|4.4<space|2spc>Modules
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-511>>

      <with|par-left|<quote|3fn>|4.4.1<space|2spc>Import statement
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-512>>

      <with|par-left|<quote|3fn>|4.4.2<space|2spc>Declaring a module
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-524>>

      <vspace*|1fn><with|font-series|<quote|bold>|math-font-series|<quote|bold>|5<space|2spc>Example
      code> <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-528><vspace|0.5fn>

      <with|par-left|<quote|1.5fn>|5.1<space|2spc>Minimum and maximum
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-529>>

      <with|par-left|<quote|1.5fn>|5.2<space|2spc>Complex numbers
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-531>>

      <with|par-left|<quote|1.5fn>|5.3<space|2spc>Vector and Matrix
      computations <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-532>>

      <with|par-left|<quote|1.5fn>|5.4<space|2spc>Linked lists with dynamic
      allocation <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-533>>

      <with|par-left|<quote|1.5fn>|5.5<space|2spc>Input / Output
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-534>>

      <with|par-left|<quote|1.5fn>|5.6<space|2spc>Object-Oriented Programming
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-535>>

      <with|par-left|<quote|3fn>|5.6.1<space|2spc>Classes
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-536>>

      <with|par-left|<quote|3fn>|5.6.2<space|2spc>Methods
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-537>>

      <with|par-left|<quote|3fn>|5.6.3<space|2spc>Dynamic dispatch
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-538>>

      <with|par-left|<quote|3fn>|5.6.4<space|2spc>Polymorphism
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-539>>

      <with|par-left|<quote|3fn>|5.6.5<space|2spc>Inheritance
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-540>>

      <with|par-left|<quote|3fn>|5.6.6<space|2spc>Multi-methods
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-541>>

      <with|par-left|<quote|3fn>|5.6.7<space|2spc>Object prototypes
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-542>>

      <with|par-left|<quote|1.5fn>|5.7<space|2spc>Functional-Programming
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-543>>

      <with|par-left|<quote|3fn>|5.7.1<space|2spc>Map
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-544>>

      <with|par-left|<quote|3fn>|5.7.2<space|2spc>Reduce
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-545>>

      <with|par-left|<quote|3fn>|5.7.3<space|2spc>Filter
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-546>>

      <with|par-left|<quote|3fn>|5.7.4<space|2spc>Functions as first-class
      objects <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-547>>

      <with|par-left|<quote|3fn>|5.7.5<space|2spc>Anonymous functions
      (Lambda) <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-548>>

      <with|par-left|<quote|3fn>|5.7.6<space|2spc>Y-Combinator
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-549>>

      <with|par-left|<quote|3fn>|5.7.7<space|2spc>Infinite data structures
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-550>>

      <vspace*|1fn><with|font-series|<quote|bold>|math-font-series|<quote|bold>|6<space|2spc>Implementation
      notes> <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-552><vspace|0.5fn>

      <with|par-left|<quote|1.5fn>|6.1<space|2spc>Lazy evaluation
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-553>>

      <with|par-left|<quote|1.5fn>|6.2<space|2spc>Type inference
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-554>>

      <with|par-left|<quote|1.5fn>|6.3<space|2spc>Built-in operations
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-555>>

      <with|par-left|<quote|1.5fn>|6.4<space|2spc>Controlled compilation
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-556>>

      <with|par-left|<quote|1.5fn>|6.5<space|2spc>Tree representation
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-559>>

      <with|par-left|<quote|1.5fn>|6.6<space|2spc>Evaluation of trees
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-560>>

      <with|par-left|<quote|1.5fn>|6.7<space|2spc>Tree position
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-561>>

      <with|par-left|<quote|1.5fn>|6.8<space|2spc>Actions on trees
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-562>>

      <with|par-left|<quote|1.5fn>|6.9<space|2spc>Symbols
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-563>>

      <with|par-left|<quote|1.5fn>|6.10<space|2spc>Evaluating trees
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-564>>

      <with|par-left|<quote|1.5fn>|6.11<space|2spc>Code generation for trees
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-565>>

      <with|par-left|<quote|3fn>|6.11.1<space|2spc>Right side of a rewrite
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-567>>

      <with|par-left|<quote|3fn>|6.11.2<space|2spc>Closures
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-568>>

      <with|par-left|<quote|1.5fn>|6.12<space|2spc>Tail recursion
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-570>>

      <with|par-left|<quote|1.5fn>|6.13<space|2spc>Partial recompilation
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-571>>

      <with|par-left|<quote|1.5fn>|6.14<space|2spc>Machine Interface
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-572>>

      <with|par-left|<quote|1.5fn>|6.15<space|2spc>Machine Types and Normal
      Types <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-573>>

      <vspace*|1fn><with|font-series|<quote|bold>|math-font-series|<quote|bold>|Index>
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-91><vspace|0.5fn>

      <vspace*|1fn><with|font-series|<quote|bold>|math-font-series|<quote|bold>|List
      of figures> <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-92><vspace|0.5fn>

      <vspace*|1fn><with|font-series|<quote|bold>|math-font-series|<quote|bold>|List
      of tables> <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-93><vspace|0.5fn>
    </associate>
  </collection>
</auxiliary>