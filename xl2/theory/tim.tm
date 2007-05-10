<TeXmacs|1.0.6>

<style|<tuple|article|maxima>>

<\body>
  <assign|by-text|> <assign|name|<macro|x|<arg|x>>> <doc-data|<doc-title|A
  Theory of Incomplete Measurements<with|color|grey|<with|color|red| - Draft
  24>>>||<\doc-author-data|<author-name|Christophe de
  Dinechin>|<\author-address>
    Sophia Antipolis, France
  </author-address>>
    \;
  </doc-author-data|<\author-address>
    <hlink|christophe@dinechin.org|mailto:christophe@dinechin.org>
  </author-address>>>

  <\abstract>
    When physicists write an equation like
    <with|mode|math|<wide|F|\<vect\>>=m*<wide|a|\<vect\>>>, how does it
    depend on the precise physical process used to measure
    <with|mode|math|<wide|F|\<vect\>>>, <with|mode|math|m> or
    <with|mode|math|<wide|a|\<vect\>>>? To answer this question, we must
    define a notation allowing formal reasoning on physical experiments. The
    proposed notation leaves us free to use any mathematical tool of our
    choice to represent physics laws. This formalism can be used to discuss
    the properties of the physical processes we are willing to call
    ``measurements'', and enables a precise definition.

    Under a reasonable set of conditions commonly observed in nature,
    properties such as linearity or continuity emerge from choices we are
    inclined to make about physical measurements, instead of from arbitrary
    axioms. This reasoning applies in particular to space and time
    coordinates, giving rise to a notion of ``continuum'' despite the
    discrete nature of every physical measurements. The properties we
    traditionally attribute to ``space-time'' can then be reinterpreted as
    properties of specific physical measurements. A large fraction of the
    framework of general relativity can be reconstructed using this approach.

    Predictions on future measurements are, in the most general case,
    probabilistic. In that case, we can demonstrate that the same definition
    of measurements given earlier implies many of the standard axioms of
    quantum mechanics. We can formally justify the traditional form of the
    wave-function, its evolution (or ``collapse''), as well as several widely
    used ad-hoc techniques. The resulting interpretation of quantum mechanics
    is devoid of what is usually called the ``measurement problem'' and
    suggests a new approach to renormalization.
  </abstract>

  <section|Introduction>

  For a very long time, we have used mathematics to represent the laws of
  physics. For instance, we write Newton's second law as
  <with|mode|math|<wide|F|\<vect\>>=m*<wide|a|\<vect\>>>, where
  <with|mode|math|<wide|F|\<vect\>>> and <with|mode|math|<wide|a|\<vect\>>>
  are ``vectors'', abstract mathematical entities made up of 3 real numbers,
  which are themselves abstract mathematical entities. The mathematics being
  used has only grown more sophisticated with time, to the point where, in
  recent days, progress in mathematics and progress in physics are often
  simultaneous. This use of mathematics, and the success we had with it, is
  so essential to any kind of modern physics that we practically never
  question it.

  The present article studies how and why mathematics can actually be used to
  represent the physical world. The fundamental premise is so simple that it
  is generally overlooked entirely: the mathematical entities we manipulate
  are symbolic representations for the result of measurements. For example,
  <with|mode|math|<wide|F|\<vect\>>> represents a physical force, something
  that we can measure for instance with a dynamometer; <with|mode|math|m>
  represents a mass, that we can measure with weights;
  <with|mode|math|<wide|a|\<vect\>>> is an acceleration, that we can derive
  from speed or position measurements; and so on.

  However, a question follows from the fundamental premise: does it matter,
  how we measure <with|mode|math|<wide|F|\<vect\>>>, <with|mode|math|m> or
  <with|mode|math|<wide|a|\<vect\>>>? The laws of physics are never written
  with a specification of how you must measure a given physical entity.
  Therefore, the classical answer appears to be that it does not matter.

  The main thesis presented here is, on the contrary, that the physical
  measurement being chosen does matter. Furthermore, we will attempt to
  demonstrate that many of the properties that we take for granted, or that
  we use as axioms for physics laws, are indeed a consequence of the choices
  we make with respect to the physical processes we accept to call
  ``measurements''.

  In the process, we will establish a number of essential properties of both
  general relativity and quantum mechanics, which become two approximations
  of the theory presented here. It is important to note that this result is
  not obtained by suggesting some new and improved Lagrangian or action that
  models both, an approach that has been tried many times without clear
  success. Instead, the result is obtained by revisiting a much deeper
  foundation of physics theories, namely the very meaning given to variables
  like <with|mode|math|x> in the equations.

  <paragraph|Overview of the article>In the first section of this article, we
  will propose a definition of measurements (section <reference|postulates>),
  introduce a notation allowing us to reason about this and other physics
  problem without constraining the mathematics (section
  <reference|formalism>), and illustrate how the formalism can be used to
  discuss simple problems (section <reference|reasoning>).

  The next section will focus on general relativity, beginning with a
  definition of space and time based on measurements (section
  <reference|spacetime>). We will explain special relativity as a
  transcription of observed relations between multiple measurements of
  distance (section <reference|specialrelativity>). We will argue that the
  use of curved space-time representations in general relativity is justified
  because we do not know how to build an Euclidean geometry out of physical
  measurements (section <reference|noneuclidean>). We will distinguish among
  the equations of general relativity those that are of geometric nature, and
  those that directly relate to measurements (section
  <reference|generalrelativity>). Finally, we will show that the relatively
  recent idea of scale relativity may be justified when we change
  measurements to ``zoom in'' or ``zoom out'' (section
  <reference|scalerelativity>).

  The last section will focus on quantum mechanics. We will first demonstrate
  that quantum mechanics axioms can be derived from the existence of
  measurements that can only be predicted with probabilities (section
  <reference|predictions>). We will illustrate the precise mathematical
  parallel with the traditional formalism, justify the emergence of a
  complex-valued wave-function describing the probability of presence, and
  suggest a discrete (as opposed to continuous) normalization condition for
  the wave-function (section <reference|analogy>). This will lead us to a new
  interpretation of quantum mechanics devoid of the so-called ``quantum
  measurement problem'' (section <reference|interpretation>).

  <section|Defining Measurements>

  While we live in the universe, we have no direct perception of it. Instead,
  we rely on measurements to gather information about the universe, whether
  performed by biological physical processes in our body or by physical
  devices we invented. All physical laws hinge on measurements, which
  suggests that a precise definition can give us new insights on the meaning
  and validity of these physical laws.

  \;

  <subsection|<label|postulates>Measurement Postulates>

  This article postulates that a measurement is:

  <\enumerate-numeric>
    <item>a valid physical process,

    <item>impacting two fragments of the universe chosen in advance,

    <item>in a repeatable way,

    <item>gathering information about an ``unknown'' or ``input'' fragment

    <item>represented by a change in a ``display'' or ``output'' fragment

    <item>which can be given a numerical or symbolic interpretation.
  </enumerate-numeric>

  It may be useful to justify each of these propositions.

  <\enumerate-numeric>
    <item>A measurement is done from within the universe, and it obeys the
    laws of the universe. It must therefore be a valid physical process. This
    proposition states that a voltmeter does not operate through magic, but
    through the normal laws of physics.

    <item>In order for a measurement to be useful, it needs to correlate
    fragments of the universe that are known to the measurement observer. A
    voltmeter must measure its inputs, not a voltage picked up at random in
    the universe, and it must show the result in a known way on a previously
    identified display.

    <item>A measurement needs to be repeatable, giving consistent results for
    consistent inputs. A voltmeter that, given the same input, displays
    random values is considered unreliable, and it should not be used for
    actual measurements.

    <item>A measurement needs to gather information about the unknown
    fragment. When you roll a dice, this is a valid physical process, and it
    presumably correlates the final value shown by the dice with the initial
    position and speed of your hand. However, it does not present useful
    information about the state of your hand, and therefore is not considered
    a measurement.

    <item>Since we live in the universe, the result of the measurement
    manifests as a change in the ``display'' universe fragment. If your
    voltmeter display does not move in response to voltage, or moves
    noticeably in response to some other influence, the voltmeter is broken
    and cannot be used to perform actual measurements.

    <item>In order for the measurement to give quantitative and not just
    qualitative results, the ``display'' fragment of the universe is
    associated with a graduation, which allows us to map the changes to a
    symbolic or numeric value. Only this graduation and associated
    calibration allow us to know that a particular voltmeter state
    corresponds to a particular voltage on the inputs.
  </enumerate-numeric>

  These statements are a possible definition of what we want to accept as
  measurements, and not a statement about how the universe works. We can
  however make a statement about the universe: experimentally, there are
  physical processes with these properties. In particular, repeatability
  relies on the observed existence of symmetries, such as invariance by
  rotation or translation through space and time, allowing multiple identical
  physical processes in the universe.

  <subsection|<label|formalism>Formal transcription>

  It is possible to transpose the statements made earlier into a formal
  notation that facilitates a precise description of complex statements about
  the universe<nocite|malgebras>.<assign|eq|<macro|refid|eq.<nbsp>(<arg|refid>)>><assign|eqref|<macro|rfid|<eq|<reference|<arg|rfid>>>>>
  This formalism is called the ``theory of incomplete measurements'' because
  it describes measurements as a very partial and fragmentary perception of
  the universe, but also because individual measurements may take some time
  to converge, and this theory provides a unified view of the laws of physics
  before, after and during the measurement.

  <paragraph|Possible>We can denote <with|mode|math|\<frak-F\>> the fragment
  of the universe <with|mode|math|\<frak-U\>> on which a measurement is being
  performed, and <with|mode|math|\<frak-D\>> the display fragment where the
  result shows up. We can call <with|mode|math|P> an arbitrary physical
  process that is not necessarily a measurement, and <with|mode|math|M> a
  physical process corresponding to a measurement.

  Let us define the notation <with|mode|math|P*\<frak-F\>> as the application
  of process <with|mode|math|P> to universe fragment
  <with|mode|math|\<frak-F\>>. In reality, <with|mode|math|P> presumably
  applies in the universe as a whole, but the notation
  <with|mode|math|P*\<frak-F\>> tells us that:

  <\itemize>
    <item>the physical process <with|mode|math|P> is compatible with the
    state of the universe fragment <with|mode|math|\<frak-F\>>.

    <item>we are interested in the effects of the process <with|mode|math|P>
    with respect to the fragment <with|mode|math|\<frak-F\>>.
  </itemize>

  In many cases, we can presume that <with|mode|math|P> has an effect that is
  local to fragment <with|mode|math|\<frak-F\>>, and that we can ignore any
  effect on the rest of the universe, but this is not a general rule.

  <paragraph|Correlating>We can define the notation
  <with|mode|math|\<frak-F\><rsub|1>=\<frak-F\><rsub|2>> to indicate that
  what we know about the universe fragment
  <with|mode|math|\<frak-F\><rsub|1>> is compatible with what we know about
  the universe fragment <with|mode|math|\<frak-F\><rsub|2>>. A notation using
  an equal sign is reasonable, because the relation is reflexive, transitive
  and symmetric like the traditional mathematical equality.

  However, this relation does not necessarily map a single state of the
  universe fragment <with|mode|math|\<frak-F\><rsub|1>> to a single state of
  the universe fragment <with|mode|math|\<frak-F\><rsub|2>>. Instead, it
  relates all the possible compatible states. We can then write
  <eqref|eq:display> to indicate that a measurement <with|mode|math|M> is a
  valid physical process collecting information from a universe fragment
  <with|mode|math|\<frak-F\>> and showing that information in the ``display''
  fragment <with|mode|math|\<frak-D\>>:

  <\equation>
    <label|eq:display>M*\<frak-F\>=M*\<frak-D\>
  </equation>

  It may be helpful to suggest that <eqref|eq:display> should read as:
  <dfn|The information we have about the display fragment
  <with|mode|math|\<frak-D\>> and the unknown fragment
  <with|mode|math|\<frak-F\>> is compatible with the fact that they are
  connected through physical process M.>

  In particular, <eqref|eq:display> does not specify anything about the rest
  of the universe, and specifically \ nothing about
  <with|mode|math|\<frak-F\>> and <with|mode|math|\<frak-D\>> ``before'' the
  measurement. Note that both sides of <eqref|eq:display> contain
  <with|mode|math|M> because we are interested in the effect on the fragments
  <with|mode|math|\<frak-F\>> and <with|mode|math|\<frak-D\>><samp|> of the
  same physical process <samp|<with|mode|math|M>>. An equation like
  <with|mode|math|\<frak-D\>=M*\<frak-F\>> would indicate that applying the
  process <with|mode|math|M> to a fragment <with|mode|math|\<frak-F\>>
  transforms it into <with|mode|math|\<frak-D\>>, which is not what happens
  in a measurement. By contrast, what <eqref|eq:display> tells us is only
  that we can choose <with|mode|math|\<frak-F\>> and
  <with|mode|math|\<frak-D\>> ``before'' performing the measurement.

  <paragraph|Repeatable>The measurement process needs to be repeatable. After
  applying the measurement once, applying it a second time should result in
  the same observation. Without invoking any notion of ``time'', we can
  therefore write <eqref|eq:repeat>, which holds for measurements but not for
  arbitrary physical processes:

  <\equation>
    <label|eq:repeat>M*M*\<frak-F\>=M*\<frak-F\>
  </equation>

  <paragraph|Focused>The measurement process correlates two selected
  fragments of the universe <with|mode|math|\<frak-F\>> and
  <with|mode|math|\<frak-D\>>, but it really collects information only about
  <with|mode|math|\<frak-F\>>, something which is not apparent in
  <eqref|eq:display>: it does not depend on the rest of the universe. This is
  described by <eqref|eq:focused>:

  <\equation>
    <label|eq:focused>M*\<frak-F\>=M*\<frak-U\>
  </equation>

  At first glance, <eqref|eq:focused> might appear ``dimensionally
  challenged'' if one does not remember that, as above, it relates what we
  know about <with|mode|math|\<frak-F\>> and what we know about
  <with|mode|math|\<frak-U\>>. A possible reading of that equation would be:
  ``<em|all that we learned about the universe <with|mode|math|\<frak-U\>> by
  applying measurement <with|mode|math|M> is what we learned about the
  unknown fragment <with|mode|math|\<frak-F\>>><em| by applying measurement
  <with|mode|math|M>>''.

  Other physical processes may have this property, and will be said to be
  <dfn|focused on <with|mode|math|\<frak-F\>>>.

  <paragraph|Quantifiable>The relation between fragment
  <with|mode|math|\<frak-F\>> and the display fragment
  <with|mode|math|\<frak-D\>> is already visible in <eqref|eq:display>, but
  for <with|mode|math|M> to be a measurement, it must also be possible to
  give a numerical or symbolic interpretation <with|mode|math|m> to the
  changes that are caused by the physical process <with|mode|math|M> to the
  display <with|mode|math|\<frak-D\>>. We can use a ``<dfn|graduation
  equation>'' like <eqref|eq:graduation> to show how a particular state of
  the display fragment <with|mode|math|\<frak-D\>> is interpreted
  numerically.

  <\equation>
    <label|eq:grad>\|M*\<frak-D\><mid|\|>=m
  </equation>

  Graduations are somewhat arbitrary. They integrate the frequently used
  notion of measurement <dfn|unit>. A graduation like <eqref|eq:graduation>
  relates the possible states of <with|mode|math|\<frak-D\>> to the possible
  values of <with|mode|math|m>. We do not specify the mathematical
  formulation of <with|mode|math|m>, and specifically, <with|mode|math|m>
  does not need to be a real number, or even a number at all. For example,
  <eqref|eq:grad> could relate an unknown or partially known state of
  <with|mode|math|\<frak-D\>> to a probability distribution
  <with|mode|math|m>.

  Conversely, the display can show a range of numerical values, and a
  <dfn|state prediction equation> like the one shown in <eqref|eq:predict>
  describes the possible states corresponding to a particular numerical
  value. These predictions do not in general define a unique state of the
  universe, but rather set constraints on what states are compatible with a
  given measurement result.

  <\equation>
    <label|eq:predict>M*\<frak-F\>=<wide|m*|\<bar\>>*
  </equation>

  <subsection|<label|reasoning>Formal reasoning>

  If we stay within physically reasonable constraints, we can use the
  equations above to do some reasoning about the results of a measurement.

  <paragraph|Repeatable Process>In many cases, a same process
  <with|mode|math|P> can be repeated a large number of times. If we apply the
  process <with|mode|math|P> a total of <with|mode|math|k> times to a
  universe fragment <with|mode|math|\<frak-F\>>, we can write
  <with|mode|math|P*P*P*\<ldots\>*P*\<frak-F\>> as
  <with|mode|math|P<rsup|k>*\<frak-F\>>. In that case,
  <with|mode|math|P<rsup|k>*\<frak-F\>> it is to be understood as being valid
  only for physically acceptable values of <with|mode|math|P>,
  <with|mode|math|k> and <with|mode|math|\<frak-F\>>. Such a process
  <with|mode|math|P> is said to be <dfn|repeatable>.

  A particular application of this notation is when the physical process can
  somehow be reversed as far as the system being studied is concerned. For
  example, when observing the layout of a room, the physical process
  reversing ``adding a chair to the room'' is the process ``removing the
  chair that was added''. Such a process is said to be <dfn|reversible>.

  If such a process exists for process <with|mode|math|A>, we can denote
  <with|mode|math|A<rsup|-1>> the inverse process, which obeys
  <eqref|eq:inverse> for all physically acceptable value of
  <with|mode|math|A>, <with|mode|math|k> and <with|mode|math|\<frak-F\>>. In
  particular, depending on the physical process being considered, this may
  include the cases <with|mode|math|k=0> or <with|mode|math|k\<less\>0>.

  <\equation>
    <label|eq:inverse>A<rsup|-1>*A<rsup|k>*\<frak-F\>=A<rsup|k-1>*\<frak-F\>
  </equation>

  At a macroscopic scale, most measurement processes appear to have, at least
  approximately, an inverse with respect to the system being measured and the
  measurement process. For example, the inverse of ``connecting a voltmeter''
  is ``disconnecting a voltmeter''; the inverse of ``placing a rod alongside
  an object'' is ``putting the rod back in the original place''. This is not
  a general rule: destructive measurements do not have an obvious inverse,
  since the object being measured cannot easily be reconstructed.

  <paragraph|Identical Processes>In many cases, we will have to write
  equations that are valid for all physically acceptable systems, such as
  <eqref|eq:inverse>. A shortcut notation in that case is to consider this an
  identity of the physical processes themselves, as in <eqref|eq:identity>:

  <\equation>
    <label|eq:identity>A<rsup|-1>*A<rsup|k>*=A<rsup|k-1>*
  </equation>

  This equation reads as follows: ``<dfn|for all acceptable physical systems,
  what we know about the system when applying the physical process
  <with|mode|math|A<rsup|-1>A<rsup|k>> is compatible with what we know about
  the system when applying the physical process>
  <with|mode|math|A<rsup|k-1>>''.

  When an identity <with|mode|math|P=Q> holds, the processes
  <with|mode|math|P> and <with|mode|math|Q> are said to be <dfn|identical>.
  In practice, two processes may be identical only for a large range of
  fragments <with|mode|math|\<frak-F\>>, in which case the identity
  <with|mode|math|P=Q> is an approximation valid on that range.

  <paragraph|Quasi-identical Processes>In general, two physical processes
  <with|mode|math|P> and <with|mode|math|Q> are not truly identical, but we
  can find a reversible physical process <with|mode|math|T> so that
  <with|mode|math|P=T*Q>. In that case, we will say that <with|mode|math|P>
  and <with|mode|math|Q> are identical by transformation <with|mode|math|T>.
  In particular, we will often consider physical translation or rotation
  transformations, and state that <with|mode|math|P> and <with|mode|math|Q>
  are identical by rotation or translation. While it is more precise to say
  that two physical processes are <dfn|quasi-identical> if they are identical
  by a combination of translations and rotations through space and time, in
  the rest of this article, we will simply call them identical unless the
  context requires the clarification.

  <paragraph|Commutativity>Consider now two independent measurements
  <with|mode|math|X> and <with|mode|math|Y>, ``independent'' meaning that the
  result of <with|mode|math|X> does not depend on the fact that we performed
  a measurement of <with|mode|math|Y> previously, and conversely. The first
  condition can be written as <eqref|eq:commutx> and the second condition can
  be written as <eqref|eq:commuty>:

  <\equation>
    <label|eq:commutx>X*=X*Y*
  </equation>

  <\equation>
    <label|eq:commuty>Y*=Y*X*
  </equation>

  \;

  If we further know an inverse process <with|mode|math|X<rsup|-1>>, then
  <eqref|eq:commuty> yields <with|mode|math|X*Y=X*Y*X>, <eqref|eq:identity>
  leads to <with|mode|math|X*Y=X<rsup|-1>*X*X*Y*X>, <eqref|eq:repeat> leads
  to <with|mode|math|X*Y=X<rsup|-1>*X*Y*X>. Finally, another application of
  <eqref|eq:identity> results in <eqref|eq:commut>:

  <\equation>
    <label|eq:commut>X*Y=Y*X
  </equation>

  In physical terms, this shows that under the conditions stated above, two
  physical measurements that are independent from one another correspond to
  commuting physical processes. Note that <eqref|eq:commutx> and
  <eqref|eq:commuty> also hold when <with|mode|math|X> and <with|mode|math|Y>
  are the same measurement. Per <eqref|eq:repeat>, one could argue that a
  measurement process is also independent from itself, since its result does
  not change if it was performed earlier.

  <paragraph|Linearity>If <with|mode|math|P> is a repeatable process,
  <with|mode|math|P*\<frak-F\>>, <with|mode|math|P<rsup|2>*\<frak-F\>>, and
  more generally <with|mode|math|P<rsup|k>\<frak-F\>> all describe valid
  physical processes applied to <with|mode|math|\<frak-F\>.> Experience tells
  us that sometimes, a measurement <with|mode|math|M> is unable to
  distinguish between one instance of <with|mode|math|P> and another. For
  instance, if <with|mode|math|P> is a process adding an identical photon
  each time, measurements will not be able to tell one of the added photons
  from another.

  When these conditions are met, we can choose for <with|mode|math|M> a
  <dfn|linear graduation> obeying <eqref|eq:linear>: if the measurement
  cannot distinguish between two individual instances of the process, the
  best it can do is count how many times it occurred.

  <\equation>
    <label|eq:linear><mid|\|>M*A<rsup|n>*\<frak-F\><mid|\|>=n**\|M*A*\<frak-F\><mid|\|>
  </equation>

  Sometimes, a measurement may distinguish between two physical processes
  <with|mode|math|A> and <with|mode|math|B> without being able to distinguish
  two instances of <with|mode|math|A> or two instances of <with|mode|math|B>.
  If arbitrary sequences like <with|mode|math|A*B*B*B*A*B*A*A*\<frak-F\>> can
  represent a physical process, and if physical processes <with|mode|math|A>
  and <with|mode|math|B> commute, we can extend the reasoning leading to
  <eqref|eq:linear>: if we cannot tell two instances of A apart, nor tell two
  instances of B apart, the best we can do is to count how many times each
  was applied.

  More generally, if <with|mode|math|\<frak-F\>> can be constructed from a
  ``ground state'' <with|mode|math|\<frak-F\><rsub|0>> by repeated
  application of commuting physical processes
  <with|mode|math|P<rsub|1>,\<ldots\>,P<rsub|n>>, that is if
  <with|mode|math|\<frak-F\>=P<rsub|1<rsup|>><rsup|k<rsub|1>*>*P<rsub|2><rsup|k<rsub|2>>*\<ldots\>P<rsub|n><rsup|k<rsub|n>>\<frak-F\><rsub|0>>,
  then the graduation for any measurement <with|mode|math|M> can be chosen to
  verify <eqref|eq:linearity>:

  <\equation>
    <label|eq:linearity>m=\|M*\<frak-F\>\|=<big|sum><rsub|i>k<rsub|i>\|M*P<rsub|i>*\<frak-F\><rsub|0>\|=<big|sum><rsub|i>k<rsub|i>*m<rsub|i>
  </equation>

  We can call <with|mode|math|\<frak-E\>> the set of physical states that are
  constructed from a common ground state <with|mode|math|\<frak-F\><rsub|0>>
  using a set of independent physical processes <with|mode|math|P<rsub|i>>.
  As demonstrated above, the condition that the <with|mode|math|P<rsub|i>>
  are independent is sufficient for them to commute. Consequently,
  \ <eqref|eq:linearity> applies. By this definition, any physical state
  <with|mode|math|\<frak-F\>\<in\>\<frak-E\>> can be written as
  <with|mode|math|\<frak-F\>=<big|prod>P<rsub|i><rsup|k<rsub|i>>*\<frak-F\><rsub|0>>
  and is therefore defined by the vector <with|mode|math|(k<rsub|i>)\<in\>\<bbb-N\><rsup|n>>.
  Measurements are linear operators on these vectors. This condition will be
  called the <dfn|linear approximation>, and it is frequently applicable in
  practice. For example, the measurements of mass for particles that move
  slowly with respect to one another follow a linear approximation.

  <paragraph|Eigenvalues>The display prediction operator introduced in
  <eqref|eq:predict>, combined with repeatability as expressed by
  <eqref|eq:repeat> easily yields <eqref|eq:eigen>:

  <\equation>
    <label|eq:eigen>M**\<frak-F\>=M*<wide|m|\<bar\>>
  </equation>

  This equation simply states that whatever changes are caused by the
  measurement process must be compatible with the value actually being
  measured.

  <section|General Relativity>

  The formalism presented above can be used to reconstruct most of special
  and general relativity, beginning with the mathematical structure of
  space-time coordinates.

  <subsection|<label|spacetime>Space-time>

  The space-time coordinates are traditionally noted as <with|mode|math|x>,
  <with|mode|math|y>, <with|mode|math|z> and <with|mode|math|t>. In the
  formalism presented here, we need to specify how these coordinates are
  being measured.

  <paragraph|Time>Time is usually measured by counting the repetitions of a
  given physical phenomenon<cite|timemsmt>, whether it is a cosmological
  physical process like the rotation of earth around its axis, the
  oscillation of a pendulum, the oscillation of a quartz crystal, or the
  decay of the excited states of some atom in atomic clocks.

  These measurements of time are therefore based on the repetition of a same
  physical process, the ``beat'' of the clock, that we can denote as
  <with|mode|math|\<delta\>T>, and on a distinct time measurement process
  <with|mode|math|T> that counts these processes to measure a time. The
  general definition of a time measurement based on periodic processes \ is
  given in <eqref|eq:timedef>, where linearity is a consequence from the fact
  that two <with|mode|math|\<delta\>T> are identical, or at least
  indistinguishable through measurement <with|mode|math|T>.

  <\equation>
    <label|eq:timedef>t=\|T*\<delta\>T<rsup|k>*\<frak-F\><rsub|0>\|=k*\|T*\<delta\>T*\<frak-F\><rsub|0>\|=k*\<delta\>t
  </equation>

  This choice places the origin of time based on a universe fragment
  <with|mode|math|\<frak-F\><rsub|0>> that we can consider as the universe
  before we start counting time. More importantly, according to
  <eqref|eq:focused>, the measurement of time is local to
  <with|mode|math|\<frak-F\><rsub|0>>, as required by special relativity: two
  different observers may measure a different time even if they apply
  identical processes <with|mode|math|T> and <with|mode|math|\<delta\>T> to
  different systems <with|mode|math|\<frak-F\>> and
  <with|mode|math|\<frak-F\><rprime|'>>. We will call <with|mode|math|T> the
  <dfn|measure of time>, <with|mode|math|\<delta\>T> a <dfn|displacement
  through time>, the corresponding numerical values <with|mode|math|t> is a
  <dfn|time coordinate>, and the numerical value <with|mode|math|\<delta\>t>
  is the <dfn|time resolution> of our measurement.

  The linearity condition in <eqref|eq:timedef> only holds to the extent that
  <with|mode|math|T> cannot distinguish one <with|mode|math|\<delta\>T> from
  another. Often, physical processes are indistinguishable only for a limited
  number of repetitions: oscillations of a pendulum decay over time if no
  energy is injected into the system; carbon-14 dating relies on atom decays
  that are individually indistinguishable, but end up changing the overall
  atom population measurably over a large period of time. Linearity allows us
  to consider two measurements of time based on different physical processes
  as identical (up to a scaling factor), and thus measuring the same time. On
  the other hand, such a simple scaling-based identity may hold only for
  short-scale time measurements.

  All physical systems evolve through time. Choosing any particular local
  displacement through time <with|mode|math|\<delta\>T> is sufficient to
  define the time evolution of any universe fragment
  <with|mode|math|\<frak-F\><rsub|0>> using <eqref|eq:timevol>.

  <\equation>
    <label|eq:timevol>\<frak-F\><rsub|t>=\<delta\>T<rsup|k>\<frak-F\><rsub|0>
  </equation>

  An interpretation of this equation is that the system evolves along time
  through the same physical process that causes the passage of time we
  measure with <with|mode|math|T>. The observed evolution of
  <with|mode|math|\<frak-F\>> depends on which process
  <with|mode|math|\<delta\>T> was picked up. For instance, consider that
  <with|mode|math|\<delta\>T<rsub|1>> is the beat of a clock you hold in your
  hand, and <with|mode|math|\<delta\>T<rsub|2>> the beat of an identical
  clock that stays in the lab. If you start moving, special relativity tells
  us that the observed evolution of the universe according to
  <with|mode|math|\<delta\>T<rsub|1>> diverges from the evolution according
  to <with|mode|math|\<delta\>T<rsub|2>>.

  <paragraph|Distance>Measuring distance is also performed by counting
  identical physical events in space, for instance how many times a hero's
  foot would fit between two objects, or how many wavelength of an
  electromagnetic wave define one meter<cite|spacemsmt>. Like for time, we
  can identify a process of displacement through space
  <with|mode|math|\<delta\>X> and the associated measure of space
  <with|mode|math|X>, with a linear definition of a space coordinate
  <with|mode|math|x> shown in <eqref|eq:spacedef>.

  <\equation>
    <label|eq:spacedef>x=\|X*\<delta\>X<rsup|k>*\<frak-F\><rsub|0>\|=k*\|X*\<delta\>X*\<frak-F\><rsub|0>\|=k*\<delta\>x
  </equation>

  Like for space, the choice of origin for a distance measurement is based on
  a particular system <with|mode|math|\<frak-F\><rsub|0>> that we consider as
  the ``center of the universe'' for the measurement being chosen. The
  measurement of space is local to <with|mode|math|\<frak-F\><rsub|0>> as
  required by special relativity. Like for time, <with|mode|math|X> is the
  measure of distance, <with|mode|math|\<delta\>X> a displacement through
  space, <with|mode|math|x> is a spatial coordinate, and
  <with|mode|math|\<delta\>x> is the resolution of our distance measurement.

  There is an added twist for space because one cannot find a single choice
  for the displacement of the system through space. Instead, experience shows
  that we need three locally independent displacements through space to
  represent the displacement between two arbitrary systems. A generalized
  local displacement through space is given by an equation like
  <eqref|eq:spacevol>.

  <\equation>
    <label|eq:spacevol>\<frak-F\><rsub|x*y*z>=\<delta\>X<rsup|x>*\<delta\>Y<rsup|y>*\<delta\>Z<rsup|z>*\<frak-F\><rsub|0>
  </equation>

  The processes <with|mode|math|\<delta\>X>, <with|mode|math|\<delta\>Y> and
  <with|mode|math|\<delta\>Z>, as well as the associated measurements
  <with|mode|math|X>, <with|mode|math|Y> and <with|mode|math|Z>, can be
  chosen to be identical to one another by rotation. In the rest of this
  article, we will ignore the complication of choosing processes that are not
  identical through rotation, even if this is often done in practice: one may
  very well be measuring altitude using air pressure while latitude and
  longitude are measured using star positions.

  <paragraph|Space-time displacement>Displacement through space and time can
  be chosen to be locally independent from one another: space-time is locally
  Minkowskian. A more general form for the displacement of a system through
  both space and time is given by <eqref|eq:spacetimevol>:

  <\equation>
    <label|eq:spacetimevol>\<frak-F\><rsub|x*y*z*t>=\<delta\>X<rsup|x>*\<delta\>Y<rsup|y>*\<delta\>Z<rsup|z>*\<delta\>T<rsup|t>*\<frak-F\><rsub|0>
  </equation>

  Adopting standard terminology, a choice of four independent measurements
  <with|mode|math|X>, <with|mode|math|Y>, <with|mode|math|Z> and
  <with|mode|math|T> is a choice of space-time coordinates, since it dictates
  the corresponding coordinates <with|mode|math|x>, <with|mode|math|y>,
  <with|mode|math|z>, and <with|mode|math|t>. Following a common general
  relativity convention, we can index the coordinates and call them
  <with|mode|math|X<rsub|i>>, where <with|mode|math|X<rsub|0>> is time and
  <with|mode|math|X<rsub|1>>, <with|mode|math|X<rsub|2>>,
  <with|mode|math|X<rsub|3>> are spatial coordinates.

  As long as the processes <with|mode|math|\<delta\>X<rsub|i>> and
  <with|mode|math|X<rsub|i>> exists physically, we can generate any
  particular coordinate value <with|mode|math|x<rsub|i>> (up to a resolution
  <with|mode|math|\<delta\>x<rsub|i>>) by applying the corresponding
  displacement \ enough times, according to <eqref|eq:timedef> and
  <eqref|eq:spacedef>. For that reason, the corresponding individual
  displacements <with|mode|math|\<delta\>X<rsub|i>> can be called the
  <dfn|generators> of the coordinates <with|mode|math|X<rsub|i>>. On the
  other hand, that does not necessarily imply that we can reach all points of
  the universe using a particular choice of coordinates.

  <paragraph|Change of coordinates>The choice of coordinates
  <with|mode|math|X<rsub|i>> is not unique. We must now consider two
  different kinds of coordinate changes:

  <\itemize>
    <item>Coordinate changes where quasi-identical physical processes are
    used, and

    <item>Coordinate changes due to a change in the physical processes being
    used.
  </itemize>

  The theory of general relativity focuses on the first kind of coordinate
  changes, called a ``change of reference frame''. Implicitly, general
  relativity assumes that space and time coordinates as measured in a given
  reference frame are unique and independent of the physical process by which
  they are being measured.

  <subsection|<label|specialrelativity>Special Relativity>

  <paragraph|Continuum vs. Discrete>Equations used in special and general
  relativity are problematic when analyzed from a measurements perspective.
  For instance, it was remarked in the earliest days of the theories that the
  choice of a Gaussian metric is only possible if small enough domains of the
  continuum can be considered Euclidean<cite|gausscoordinates>. An implicit
  requirement of the formulation of general relativity is that physical
  entities, including space-time, energy and momentum, are continuously
  differentiable. Our space and time measurements, however, are not even
  continuous, since they are reported using a discrete graduation, and as we
  discussed above, generally based on a discrete count of identical physical
  processes.

  However, any measurement for which the linear approximation
  <eqref|eq:linearity> is valid can be identified with a linear function
  <with|mode|math|m> of the measurement coordinates
  <with|mode|math|k<rsub|i>>, and that linear approximation
  <with|mode|math|m(k<rsub|1>,\<ldots\>,k<rsub|n>)=<big|sum>k<rsub|i>*m<rsub|i>>
  is continuously differentiable on <with|mode|math|\<bbb-R\><rsup|n>> with
  respect to each <with|mode|math|k<rsub|i>>. This makes it possible to
  replace the actual measurements with a continuously differentiable function
  that matches any possible measured value. This is true in particular of
  space and time coordinates.

  <paragraph|Preserving distance>If we measure <with|mode|math|X>,
  <with|mode|math|Y> and <with|mode|math|Z> using physical processes that are
  quasi-identical to a distance measurement <with|mode|math|D>, the distance
  <with|mode|math|d=\|D\|> is preserved through a change of spatial reference
  frame. Experimentally, the mathematical distance takes the shape of an
  Euclidean metric at least locally. Therefore, the coordinate change for the
  measurement values <with|mode|math|x>, <with|mode|math|y> and
  <with|mode|math|z> can be modeled as a combination of a mathematical
  rotation and a translation.

  For systems that can be assimilated to a point moving in space along a
  trajectory, we often perform the measurements of space-time coordinates
  relative to the individual systems. Therefore, we need to consider the
  translation and rotation required to bring one tangent referential system
  to another, as shown in figure <reference|fig:rotation>.

  <\with|par-mode|center>
    <small-figure|<with|gr-mode|<tuple|edit|text-at>|gr-frame|<tuple|scale|1cm|<tuple|0.199999gw|0.199999gh>>|gr-geometry|<tuple|geometry|7cm|5cm|center>|gr-grid|<tuple|cartesian|<point|0|0>|1>|gr-grid-old|<tuple|cartesian|<point|0|0>|1>|gr-edit-grid-aspect|<tuple|<tuple|axes|none>|<tuple|1|none>|<tuple|10|none>>|gr-edit-grid|<tuple|cartesian|<point|0|0>|1>|gr-edit-grid-old|<tuple|cartesian|<point|0|0>|1>|gr-line-arrows|none|gr-line-width|1ln|gr-fill-color|default|gr-color|orange|gr-dash-style|<tuple|1|0>|<graphics|<spline|<point|-1.3|0.9>|<point|0.1|2.4>|<point|2.1|2.1>|<point|4|1.3>|<point|5.4|2.2>>|<with|color|red|line-width|2ln|line-arrows|<tuple|<with|dash-style|none|<line|<tuple|-10ln|6ln>|<tuple|0ln|0ln>|<tuple|-10ln|-6ln>>>>|fill-color|red|<line|<point|0|0>|<point|1|0>>>|<with|color|red|line-width|2ln|line-arrows|<tuple|<with|dash-style|none|<line|<tuple|-10ln|6ln>|<tuple|0ln|0ln>|<tuple|-10ln|-6ln>>>>|fill-color|red|<line|<point|0|0>|<point|0|1>>>|<text-at|<with|mode|math|<with|color|red|x>>|<point|-0.3|0.4>>|<text-at|<with|mode|math|<with|color|red|y>>|<point|0.4|-0.3>>|<text-at|<with|mode|math|<with|color|blue|y<rprime|'>>><with|mode|math|<rprime|>>|<point|2.1|1.6>>|<text-at|<with|color|blue|<with|mode|math|x<rprime|'>>>|<point|1.9|2.7>>|<with|line-width|2ln|fill-color|red|<line|<point|1.5|2.2>|<point|3.1|2.2>>>|<with|line-width|2ln|fill-color|red|<arc|<point|2.5|1.9>|<point|2.6|2.2>|<point|2.6|2.3>>>|<text-at|<with|mode|math|\<theta\>>|<point|2.7|1.9>>|<text-at|<with|mode|math|\<frak-F\><rprime|'>>|<point|4.8|2.2>>|<text-at|<with|mode|math|\<frak-F\>>|<point|5.1|-0.5>>|<with|line-width|2ln|fill-color|red|<line|<point|-1.3|0>|<point|5.3|0>>>|<with|color|red|line-width|2ln|fill-color|red|<line|<point|0|0>|<point|1|0>>>|<with|color|orange|dash-style|<tuple|1|0>|<line|<point|2|2.4>|<point|2|-0.4>>>|<with|color|orange|dash-style|<tuple|1|0>|<line|<point|2.2|2.2>|<point|-0.8|2.2>>>|<text-at|<with|mode|math|y<rsub|0>>|<point|1.9|-0.4>>|<text-at|<with|mode|math|x<rsub|0>>|<point|-0.9|2.2>>|<with|color|blue|line-width|2ln|line-arrows|<tuple|<with|dash-style|none|<line|<tuple|-10ln|6ln>|<tuple|0ln|0ln>|<tuple|-10ln|-6ln>>>>|fill-color|red|<line|<point|2|2.2>|<point|2.8|1.6>>>|<with|color|blue|line-width|2ln|line-arrows|<tuple|<with|dash-style|none|<line|<tuple|-10ln|6ln>|<tuple|0ln|0ln>|<tuple|-10ln|-6ln>>>>|fill-color|red|<line|<point|2|2.2>|<point|2.6|3>>>>>|<label|fig:rotation>Change
    of coordinates>
  </with>

  The coordinate changes between <with|mode|math|(x,y)> and
  <with|mode|math|(x<rprime|'>,y<rprime|'>)> for the two-dimensional example
  shown in figure <reference|fig:rotation> can be written as a combination
  of:

  <\itemize>
    <item>a translation by a vector <with|mode|math|(x<rsub|0>,y<rsub|0>)>
    where <with|mode|math|x<rsub|0>> and <with|mode|math|y<rsub|0>>
    correspond to the measurement relative to <with|mode|math|\<frak-F\>> of
    the point that measures as <with|mode|math|(0,0)> relative to the system
    <with|mode|math|\<frak-F\><rprime|'>>.

    <item>a rotation by angle <with|mode|math|\<theta\>>.
  </itemize>

  The general formulation of the coordinates change is well known and given
  by <eqref|eq:coordrot>:

  <\equation>
    <label|eq:coordrot><choice|<tformat|<table|<row|<cell|x<rprime|'>=(x-x<rsub|0>)*cos
    \<theta\>-(y-y<rsub|0>)*sin \<theta\>>>|<row|<cell|y<rprime|'>=(x-x<rsub|0>)*sin
    \<theta\>+(y-y<rsub|0>)*cos \<theta\>>>>>>
  </equation>

  If the coordinates of the trajectory of system
  <with|mode|math|\<frak-F\><rprime|'>> relative to system
  <with|mode|math|\<frak-F\>> are parameterized as continuously
  differentiable functions <with|mode|math|x(s)> and <with|mode|math|y(s)>,
  we can also express <with|mode|math|cos \<theta\>> and <with|mode|math|sin
  \<theta\>> as <eqref|eq:cossin>:

  <\equation>
    <label|eq:cossin><choice|<tformat|<table|<row|<cell|cos
    \<theta\>=<normal-size|<large|<very-large|<very-large|<frac|<frac|\<mathd\>y|\<mathd\>s>|<sqrt|<rsup|<rsup|(<frac|\<mathd\>x|\<mathd\>s>)<rsup|2><rsub|>+(<frac|\<mathd\>y|\<mathd\>s>)<rsup|2>>>>><huge|>>>>>>>|<row|<cell|sin
    \<theta\>=<very-large|<frac|<frac|\<mathd\>x|\<mathd\>s>|<sqrt|<rsup|<rsup|(<frac|\<mathd\>x|\<mathd\>s>)<rsup|2><rsub|>+(<frac|\<mathd\>y|\<mathd\>s>)<rsup|2>>>>>>>>>>>
  </equation>

  Assuming <with|mode|math|<frac|\<mathd\>y|\<mathd\>s>\<neq\>0>, another
  formulation that will soon prove useful is <eqref|eq:cossin2>:

  <\equation>
    <label|eq:cossin2><choice|<tformat|<table|<row|<cell|cos
    \<theta\>=<normal-size|<large|<very-large|<frac|1|<sqrt|<rsup|<rsup|1+(<frac|\<mathd\>x|\<mathd\>y>)<rsup|2><rsub|>>>>>>>>>>|<row|<cell|sin
    \<theta\>=<very-large|<frac|<frac|\<mathd\>x|\<mathd\>y>|<sqrt|<rsup|<rsup|1+(<frac|\<mathd\>x|\<mathd\>y>)<rsup|2><rsub|>>>>>>>>>>>
  </equation>

  <paragraph|Time coordinate>One of the essential findings of special
  relativity was that the reasoning above also holds when \ considering a
  coordinate change involving a space and a time coordinate, with a
  significant difference: the numerical distance <with|mode|math|s> that is
  preserved verifies <with|mode|math|\<mathd\>s<rsup|2>=c<rsup|2>\<mathd\>t<rsup|2>-\<mathd\>x<rsup|2>>
  instead of the definite positive Gaussian metric
  <with|mode|math|\<mathd\>s<rsup|2>=\<mathd\>x<rsup|2>+\<mathd\>y<rsup|2>>
  that is preserved for space. We can speculate that this particular choice
  of distance is the result of our use of a single physical phenomenon to
  measure both space and time. We will see below that it is reasonable to
  presume that this single phenomenon is the electromagnetic interaction.

  There are interesting consequences to a distance written as
  <with|mode|math|\<mathd\>s<rsup|2>=c<rsup|2>\<mathd\>t<rsup|2>-\<mathd\>x<rsup|2>>.
  Compared to the Euclidean metric, it can be seen as a transformation
  <with|mode|math|y\<rightarrow\>i*c*t> where <with|mode|math|i<rsup|2>=-1>.
  In particular, the reasoning that distance is preserved by rotations and
  translations remains if we extend the definition of ``rotation'' slightly.
  Whereas for classical rotation between two space coordinates,
  <with|mode|math|cos \<theta\>\<leqslant\>1>, for rotations between a space
  and a time coordinate, <with|mode|math|cos \<theta\>\<geqslant\>1>, since
  it takes the mathematical form of an hyperbolic cosine. This
  <with|mode|math|cos \<theta\>> factor is traditionally denoted as
  <with|mode|math|\<gamma\>> in special relativity textbooks, and
  <with|mode|math|sin \<theta\>> is traditionally written as
  <with|mode|math|i*\<beta\>*\<gamma\>>. In that case, \ <eqref|eq:coordrot>
  is nothing else than the Lorenz equation which lies at the core of special
  relativity.

  <paragraph|Special relativity effects>The now well known and well verified
  effects of special relativity are easy to interpret as what we usually call
  ``perspective'' for space-space rotations, after the transformation
  <with|mode|math|y\<rightarrow\>i*c*t> is performed. Perspective induces
  contractions for space-space rotations that are related to
  <with|mode|math|cos \<theta\>\<leqslant\>1>, and so it induces dilatation
  for space-time rotations where <with|mode|math|cos \<theta\>\<geqslant\>1>.
  Conversely, dilatation effects for space-space rotations that are related
  to <with|mode|math|cos \<theta\>> turn into contractions for a space-time
  rotation.

  For instance, the so-called ``twins paradox''<cite|langevin> is illustrated
  on figure <reference|fig:twins>. The blue straight arrow along the
  horizontal axis represents the trajectory of a twin who remains on earth.
  The red curve represents the trajectory of a twin who travels in space at
  high speed. For space-space rotations, as shown on the figure, the red
  curve is always longer than the blue line. It is easy to verify that the
  elongation factor takes the form of <with|mode|math|<big|int><frac|1|cos
  \<theta\>>> along the curve, which is always greater than 1 when
  <with|mode|math|cos \<theta\>\<leqslant\>1>. On the contrary, if
  <with|mode|math|cos \<theta\>\<geqslant\>1>, then the red curve is always
  shorter than the blue curve: less time elapsed for the moving twin than for
  the twin who remained on earth.

  <\with|par-mode|center>
    <small-figure|<with|gr-mode|<tuple|group-edit|props>|gr-frame|<tuple|scale|1cm|<tuple|0.100003gw|0.39999gh>>|gr-geometry|<tuple|geometry|7cm|3cm|center>|gr-grid|<tuple|cartesian|<point|0|0>|1>|gr-grid-old|<tuple|cartesian|<point|0|0>|1>|gr-edit-grid-aspect|<tuple|<tuple|axes|none>|<tuple|1|none>|<tuple|10|none>>|gr-edit-grid|<tuple|cartesian|<point|0|0>|1>|gr-edit-grid-old|<tuple|cartesian|<point|0|0>|1>|gr-line-width|3|gr-line-arrows|<tuple|<with|dash-style|none|<line|<tuple|-10ln|6ln>|<tuple|0ln|0ln>|<tuple|-10ln|-6ln>>>>|gr-color|red|gr-dash-style|<tuple|1|0>|<graphics|<with|color|red|line-width|2ln|line-arrows|<tuple|<with|dash-style|<quote|none>|<line|<tuple|-10ln|6ln>|<tuple|0ln|0ln>|<tuple|-10ln|-6ln>>>>|<spline|<point|0|0>|<point|1|0.1>|<point|2|0.5>|<point|2.6|1>|<point|3.3|1.1>|<point|3.9|0.5>|<point|4.4|-0.3>|<point|4.9|-0.5>|<point|5.2|-0.3>|<point|5.5|-0.1>|<point|5.7|0>|<point|6|0>>>|<with|color|blue|line-width|2ln|line-arrows|<tuple|<with|dash-style|<quote|none>|<line|<tuple|-10ln|6ln>|<tuple|0ln|0ln>|<tuple|-10ln|-6ln>>>>|<line|<point|0|0>|<point|6.2|0>>>>>|<label|fig:twins>Twins
    paradox>
  </with>

  While the above reasoning is applied to special relativity alone, we did
  not need for this explanation any notion of ``inertial frame'' or
  ``non-accelerated observer''. As a matter of fact, the reasoning above
  explicitly includes accelerated movement.

  <paragraph|Is light creating space-time?>The reasoning we just made shows
  that the properties usually attributed to space-time by special relativity
  can be derived solely from the hypothesis that we preserve a notion of
  distance related to detecting the wavefronts of electromagnetic waves.

  We could postulate, as has often been done since the beginning of physics,
  that there exists an absolute space-time with an existence that is
  independent from its contents. However, the remark above strongly suggests
  that this hypothesis is unnecessary. Instead, the properties of space-time
  could simply be a consequence from the fact that all our measurements of
  space and time are related to the properties of electromagnetic waves.

  At a macroscopic level, this certainly seems plausible. Our definition of
  the meter is now based on light waves. Measurements involving rods use the
  properties of solids which are held together primarily by electromagnetic
  forces. Our definition of time is based on the de-excitation of atoms, and
  this is the fundamental process generating photons. The vast majority of
  our observations of the position of planets, stars or galaxies are based on
  electromagnetic waves.

  It remains possible to perform distance measurements using other physical
  processes. Historically, the measure of time was often based on
  gravitational interaction, for example as it defines the movement of
  celestial bodies. However, as we have seen with time, two distinct physical
  processes that can be linearized locally to match one another do not
  necessarily measure the same space and time coordinates at any scale. It
  seems therefore unreasonable to assume that definitions of ``space'' or
  ``time'' based on distinct physical processes that happen to be in
  agreement locally will remain in agreement at any scale.

  <subsection|<label|noneuclidean>Non-Euclidean Geometry>

  One of the fundamental innovations of general relativity was the use of
  non-Euclidean geometry. Non-Euclidean geometry is also required in the
  present theory.

  <paragraph|Non-commutativity>The reasoning in the previous section requires
  that displacement through space and time coordinates commute locally. For
  example, <eqref|eq:spacetimevol> is valid only to the extent that we cannot
  distinguish through any measurement between the state of the universe
  defined by <with|mode|math|\<frak-F\><rsub|x*y>=\<delta\>X*\<delta\>Y*\<frak-F\><rsub|0>>
  and <with|mode|math|\<frak-F\><rsub|y*x>=\<delta\>Y*\<delta\>X*\<frak-F\><rsub|0>>.
  Otherwise, <eqref|eq:spacetimevol> \ would predict two different evolutions
  of the universe depending on which path is chosen, and this contradicts our
  original definition of the notation <with|mode|math|\<frak-F\><rsub|1>=\<frak-F\><rsub|2>>.

  The fact that displacements commute locally does not imply that they still
  commute after being repeated a large number of times. As a matter of fact,
  at a large enough scale, we observe that displacements through space-time
  do not generally commute. To illustrate this, consider mobiles subjected to
  gravitation forces keeping them at the surface of our planet. On earth,
  moving one meter forward followed by one meter sideways brings you
  practically at the same point as moving one meter sideways followed by one
  meter forward, as illustrated on figure <reference|fig:commuting>.

  <\with|par-mode|center>
    <small-figure|<with|gr-mode|<tuple|group-edit|move>|gr-frame|<tuple|scale|1cm|<tuple|0.299998gw|0.299998gh>>|gr-geometry|<tuple|geometry|5cm|5cm|center>|gr-grid|<tuple|cartesian|<tuple|0|0>|1>|gr-grid-old|<tuple|cartesian|<tuple|0|0>|1>|gr-grid-aspect|<tuple|<tuple|axes|#808080>|<tuple|1|#c0c0c0>|<tuple|6|#e0e0ff>>|gr-grid-aspect-props|<tuple|<tuple|axes|#808080>|<tuple|1|#c0c0c0>|<tuple|6|#e0e0ff>>|gr-line-width|2ln|gr-line-arrows|<tuple|<with|dash-style|none|<line|<tuple|-10ln|6ln>|<tuple|0ln|0ln>|<tuple|-10ln|-6ln>>>>|gr-color|green|gr-edit-grid-aspect|<tuple|<tuple|axes|none>|<tuple|1|none>|<tuple|6|none>>|gr-edit-grid|<tuple|cartesian|<tuple|0|0>|1>|gr-edit-grid-old|<tuple|cartesian|<tuple|0|0>|1>|<graphics|<text-at|<with|mode|math|\<delta\>X>|<point|0.763014|-0.376207>>|<with|color|magenta|line-width|2ln|line-arrows|<tuple|<with|dash-style|none|<line|<tuple|-10ln|6ln>|<tuple|0ln|0ln>|<tuple|-10ln|-6ln>>>>>|<with|color|green|line-width|2ln|line-arrows|<tuple|<with|dash-style|none|<line|<tuple|-10ln|6ln>|<tuple|0ln|0ln>|<tuple|-10ln|-6ln>>>>|<line|<point|2|0>|<point|2|2>>>|<with|color|red|line-width|2ln|line-arrows|<tuple|<with|dash-style|none|<line|<tuple|-10ln|6ln>|<tuple|0ln|0ln>|<tuple|-10ln|-6ln>>>>|<line|<point|2|2>|<point|0|2>>>|<with|color|blue|line-width|2ln|line-arrows|<tuple|<with|dash-style|none|<line|<tuple|-10ln|6ln>|<tuple|0ln|0ln>|<tuple|-10ln|-6ln>>>>|<line|<point|0|0>|<point|2.01186|0.00479561>>>|<with|color|magenta|line-width|2ln|line-arrows|<tuple|<with|dash-style|none|<line|<tuple|-10ln|6ln>|<tuple|0ln|0ln>|<tuple|-10ln|-6ln>>>>|<line|<point|0|2>|<point|0|0>>>|<with|text-at-halign|right|<text-at|<with|mode|math|\<delta\>Y>|<point|2.46418|0.822473>>>|<text-at|<with|mode|math|\<delta\>X<rsup|-1>>|<point|0.762838|2.123963>>|<text-at|<with|mode|math|\<delta\>Y<rsup|-1>>|<point|-0.903653|0.845957>>>>|<label|fig:commuting>Commuting
    displacements>\ 
  </with>

  The displacement in figure <reference|fig:commuting> is is practically
  indistinguishable from ``returning to the same point''. Considering for
  simplicity that Earth has a radius of roughly 6000<space|0.2spc>km, the
  angle <with|mode|math|\<alpha\>> seen from the center of Earth for a
  1<space|0.2spc>m displacement is at first order
  <with|mode|math|\<alpha\>=<frac|1<space|0.2spc>m|6000*10<rsup|3><space|0.2spc>m>>.
  The curvature error is in the order of <with|mode|math|6000<space|0.2spc>km*(1-cos
  \<alpha\>)>, and for such a small value of <with|mode|math|\<alpha\>> we
  can rewrite that as <with|mode|math|6000<space|0.2spc>km*<frac|\<alpha\><rsup|2>|2>>,
  so the displacement error made by assuming that earth is flat is less than
  <with|mode|math|10<rsup|-7>><space|0.2spc>m. Except under the most
  stringent laboratory conditions, this error is simply negligible.\ 

  However, if the distance in each direction is 10000<nbsp>km, that is
  approximately a quarter of the circumference of earth, then the two points
  you will reach are about 10000<nbsp>km apart, as illustrated on figure
  <reference|fig:coordinates> as seen from a pole. It is necessary to point
  out that, from the local point of view of the mobile, there is no
  difference in the local movements between figure <reference|fig:commuting>
  and figure <reference|fig:coordinates>. In both cases, we are moving for
  example ``N steps forwards, N steps left, N steps backwards, N step
  right''.

  <\with|par-mode|center>
    <small-figure|<with|gr-mode|<tuple|group-edit|move>|gr-frame|<tuple|scale|1cm|<tuple|0.299998gw|0.399997gh>>|gr-geometry|<tuple|geometry|5cm|5cm|center>|gr-grid|<tuple|polar|<tuple|0|0>|1|12>|gr-grid-old|<tuple|polar|<tuple|0|0>|1|12>|gr-grid-aspect|<tuple|<tuple|axes|#808080>|<tuple|1|#c0c0c0>|<tuple|6|#e0e0ff>>|gr-grid-aspect-props|<tuple|<tuple|axes|#808080>|<tuple|1|#c0c0c0>|<tuple|6|#e0e0ff>>|gr-line-width|2ln|gr-line-arrows|<tuple|<with|dash-style|none|<line|<tuple|-10ln|6ln>|<tuple|0ln|0ln>|<tuple|-10ln|-6ln>>>>|gr-color|magenta|<graphics|<with|color|blue|line-width|2ln|line-arrows|<tuple|<with|dash-style|none|<line|<tuple|-10ln|6ln>|<tuple|0ln|0ln>|<tuple|-10ln|-6ln>>>>|<line|<point|-0.0201581|0.00479561>|<point|2.01186|0.00479561>>>|<with|color|green|line-width|2ln|line-arrows|<tuple|<with|dash-style|none|<line|<tuple|-10ln|6ln>|<tuple|0ln|0ln>|<tuple|-10ln|-6ln>>>>|<arc|<point|2.01186|-0.122205>|<point|1.31335|1.52881>|<point|-0.0201581|1.99448>>>|<with|color|red|line-width|2ln|line-arrows|<tuple|<with|dash-style|none|<line|<tuple|-10ln|6ln>|<tuple|0ln|0ln>|<tuple|-10ln|-6ln>>>>|<line|<point|0.00100873|1.97331>|<point|0.00100873|-0.037538>>>|<with|color|magenta|line-width|2ln|line-arrows|<tuple|<with|dash-style|none|<line|<tuple|-10ln|6ln>|<tuple|0ln|0ln>|<tuple|-10ln|-6ln>>>>|<line|<point|0.00100873|-0.101038>|<point|2.01186|-0.0798717>>>|<with|text-at-halign|right|<text-at|<with|mode|math|\<delta\>Y>|<point|1.63085|1.65581>>>|<text-at|<with|mode|math|\<delta\>X<rsup|-1>>|<point|-0.930332|0.893804>>|<text-at|<with|mode|math|\<delta\>X>|<point|0.763014|0.131796>>|<with|text-at-halign|center|<text-at|<with|mode|math|><with|mode|math|\<delta\>Y<rsup|-1>>|<point|1.08052|-0.397373>>>>>|<label|fig:coordinates>Non-commuting
    displacements>
  </with>

  In this example, the non-commutativity of space-time displacements appears
  related to curvature. This is not surprising, as the Riemann curvature
  tensor measures the non-commutativity of the covariant derivative.
  Intuitively, by following displacements along two different displacements
  <with|mode|math|\<delta\>X> and <with|mode|math|\<delta\>Y>, we are
  reconstructing in physical terms the parallel transport used to evaluate
  the Riemann curvature tensor.

  <paragraph|Geodesics>One might be tempted to think that non-commutativity
  is due to a bad choice of coordinates, because on earth we are not moving
  ``straight'' but along the ``curved'' surface of earth. By this reasoning,
  the problem would disappear if we followed a ``straight line'' along each
  axis. Our space and time measurements would then correspond to an Euclidean
  geometry.

  Unfortunately, in physical terms, it is difficult to define what a
  ``straight line'' is. Arguably, one of the best physical definitions of a
  straight line is the path followed by a ray of light, yet a number of
  physical experiments show two or more light rays starting at the same time
  from the same point and reaching the same point at the same time. A simple
  optical lens shows this property, but even in the vacuum, the property
  appears at large scale (gravitational lensing) or in the presence of
  obstacles (Young's slits experiments).

  A similar problem exists for other definitions of straight lines (or
  geodesics). For instance, one could define geodesics as the paths followed
  by ``free-falling observers''. One space measurement would be points
  separated by a constant local time interval along the path. But in that
  case too, one can find two geodesics with the same start and end points in
  space-time, for instance two opposite trajectories along the same orbit.

  <paragraph|Non-Euclidean geometry>Mathematics tell us that if there are
  more than one straight lines between two individual points, then the
  geometry is not Euclidean. Since the best definitions of a straight line we
  have in the physical world exhibit this non-Euclidean property, we need to
  give up the possibility to associate an Euclidean geometry to any system of
  coordinates based on physical space and time measurements.

  The justification given here is different from the reason given by Albert
  Einstein to postulate a non-Euclidean geometry for space-time, namely the
  existence of non-Euclidean space-time hyper-surfaces, such as the surface
  of a rotating disk along its proper time<cite|justifyrelativity>. That
  observation alone is not sufficient to prove that space-time itself is
  curved, just like the existence of spheres in space does not prove that
  space is curved. Indeed, the hyper-surface selected for his example, the
  path in space-time of the points along the radius of a rotating galaxy, is
  analogous in space-time to an helicoid in space, and an helicoid is not an
  Euclidean surface. However, this observation led to using non-Euclidean
  geometry to describe such accelerated frames of reference. Using this
  non-Euclidean geometry, all accelerations, including the acceleration
  caused by gravitation, could be modeled as a curvature of space-time.

  The general relativity justification for a non-Euclidean geometry is the
  possibility to unify gravitation and acceleration in a single mathematical
  framework. The justification presented here for non-Euclidean geometry is
  the impossibility to define an Euclidean geometry applying to space-time
  measurements.

  <subsection|<label|generalrelativity>General Relativity equations>

  A reminder of the traditional formulation of general relativity is useful
  to see how it relates to the theory of measurements presented here. In the
  following equations, we will use Einstein's convention of summing terms
  with identical lower and upper indices.

  <paragraph|Mathematical tools>Many of the formulas in general relativity
  are mathematical tools that do not need any matching physical reality to be
  used legitimately. There is no issue using them to manipulate the extension
  to <with|mode|math|\<bbb-R\><rsup|n>> of any linear approximation function
  <with|mode|math|m(x<rsub|1>,\<ldots\>,x<rsub|n>)>, including, but not
  limited to, the space-time coordinates themselves. These mathematical tools
  can, in particular, be used freely on mathematical spaces that have a
  different number of dimensions than the 4 dimensions we observe in the
  physical space-time.

  Let's consider a set of coordinates <with|mode|math|x<rsub|i>> in
  <with|mode|math|\<bbb-R\><rsup|n>>. A Gaussian metric <with|mode|math|g> is
  a rank (0,2) symmetric tensor defining a measure of distance from
  individual coordinates, as shown in <eqref|eq:gauss>. The inverse of
  <with|mode|math|g<rsub|\<mu\>\<nu\>>> is written
  <with|mode|math|g<rsup|\<mu\>\<nu\>>>.

  <\equation>
    <label|eq:gauss>\<mathd\>s<rsup|2>=g<rsub|\<mu\>\<nu\>>\<mathd\>x<rsup|\<mu\>>\<mathd\>x<rsup|\<nu\>>
  </equation>

  The Levi-Civita connection <with|mode|math|\<nabla\>> defines a derivative
  <with|mode|math|D> for a vector field <with|mode|math|V> along a curve
  <with|mode|math|\<gamma\>> as <with|mode|math|D<rsub|t>*V=\<nabla\><rsub|\<gamma\>(t)>V>.
  The covariant derivative generalizes partial derivative, adding a term that
  is the change caused by curvature itself, so it writes as
  <with|mode|math|D<rsub|\<mu\>>A<rsup|\<nu\>>=\<partial\>A<rsup|\<nu\>>+\<Gamma\><rsup|\<nu\>><rsub|\<rho\>\<mu\>>A<rsup|\<rho\>>>.
  The added term uses the Christoffel symbol
  <with|mode|math|\<Gamma\><rsup|\<rho\>><rsub|\<mu\>\<nu\>>=<frac|1|2>g<rsup|\<rho\>\<lambda\>>(\<partial\><rsub|\<nu\>>g<rsub|\<lambda\>\<mu\>>+\<partial\><rsub|\<mu\>>g<rsub|\<lambda\>\<nu\>>-\<partial\><rsub|\<lambda\>>g<rsub|\<mu\>\<nu\>>)>.

  The covariant derivatives do not commute, and the Riemann curvature tensor
  <with|mode|math|R<rsup|\<lambda\>><rsub|\<rho\>\<mu\>\<nu\>>> is a rank
  (1,3) tensor defined as <with|mode|math|R(u,v,w)=\<nabla\><rsub|u>\<nabla\><rsub|v>w-\<nabla\><rsub|v>\<nabla\><rsub|u>w-\<nabla\><rsub|[u,v]>w>.
  For the coordinate vector fields <with|mode|math|u=<frac|\<partial\>|\<partial\>x<rsub|i>>>
  and <with|mode|math|v=<frac|\<partial\>|\<partial\>x<rsub|j>>>,
  <with|mode|math|[u,v]=0> and the Riemann tensor measures the commutator of
  the covariant derivative, i.e. <with|mode|math|R<rsup|\<lambda\>><rsub|\<rho\>\<nu\>\<mu\>>A<rsub|\<lambda\>>=(D<rsub|\<mu\>>D<rsub|\<nu\>>-D<rsub|\<nu\>>D<rsub|\<mu\>>)A<rsub|\<rho\>>>.

  The Ricci tensor is defined from the Riemann tensor in coordinate terms as
  <with|mode|math|R<rsub|\<mu\>\<nu\>>=R<rsup|\<rho\>><rsub|\<mu\>\<rho\>\<nu\>>>.
  We can then define the Ricci scalar by raising an index to get
  <with|mode|math|R<rsup|\<mu\>><rsub|\<nu\>>=g<rsup|\<mu\>\<nu\>>R<rsub|\<mu\>\<nu\>>>
  and then contracting to obtain the Ricci scalar
  <with|mode|math|R=R<rsup|\<mu\>><rsub|\<mu\>>>.

  None of these equations makes any hypothesis about the physical universe,
  and they can therefore be accepted as purely mathematical relationships in
  the present theory.

  <paragraph|Physical values>If we only care about gravitation and ignore
  other forces, there are three entities in general relativity that are
  directly related to physical measurements: the space-time metric, related
  to space and time measurements, the stress-energy tensor, related to mass,
  energy or momentum measurements, and a ``cosmological constant''
  <with|mode|math|\<Lambda\>> that relates to the shape of the universe
  observed at large scale. These are the quantities for which a
  measurement-based analysis is required.

  <paragraph|Metric>The metric <with|mode|math|g<rsub|\<mu\>\<nu\>>> as
  defined above relates a choice of distance
  <with|mode|math|\<mathd\>s<rsup|2>> to small variations
  <with|mode|math|\<mathd\>x<rsub|\<mu\>>> of the individual space or time
  measurements <with|mode|math|x<rsub|\<mu\>>>, which we can assume to be
  defined as a result of a particular choice of coordinate measurements
  <with|mode|math|X<rsub|i>>. Provided the measurements give definite, non
  probabilistic results, there is no particular problem computing the
  Gaussian metric associated with a chosen set of measurements.

  There is, however, a problem in choosing which physical processes are
  equivalent to a space-time metric in general relativity. Clearly, mass
  measurements do not apply. But what about choosing between measurements
  made with a metal rod and measurements made by measuring the trip time of a
  laser beam? What if the metal rod becomes really hot? What if the laser
  beam has to travel through water?

  For the present theory, following remarks made about special relativity,
  and based on the kind of verifications already performed for general
  relativity, we will postulate that the only measurements for which the
  field equation of general relativity is valid use metric measurements that
  are practically equivalent to the propagation of electromagnetic waves in a
  vacuum. Other measurements, like solid rods, can be considered equivalent
  to the extent that they are primarily defined by such propagation.

  <paragraph|Existence of a Minkowskian metric>In general relativity, the
  equivalence principle of gravitation and inertial acceleration is expressed
  as the existence of coordinate systems where the metric is locally
  Minkowskian, and where the equation of motion of a free particle moving
  along 4-vector <with|mode|math|u<rsub|\<mu\>>> is
  <with|mode|math|D*u<rsub|\<mu\>>= 0>.

  For the present theory, we postulated instead that:

  <\enumerate>
    <item>we can find for any system <with|mode|math|\<frak-F\><rsub|0>> four
    repeatable time and space displacements processes
    <with|mode|math|\<delta\>X<rsub|i>> that are mutually independent.

    <item>the evolution of the system <with|mode|math|\<frak-F\><rsub|0>>
    over space and time follows the local time displacement
    <with|mode|math|\<delta\>T=\<delta\>X<rsub|0>> corresponding to the
    ``passage of time'', i.e. <with|mode|math|\<frak-F\>=\<delta\>T<rsup|k>*\<frak-F\><rsub|0>>.
  </enumerate>

  This implies the existence of a local Minkowski metric. Since the
  <with|mode|math|\<delta\>X<rsub|i>> are repeatable, the corresponding
  measurements <with|mode|math|x<rsub|i>> obey the linear approximation. It
  is possible to associate a metric <with|mode|math|g<rsub|\<mu\>\<nu\>>> to
  the <with|mode|math|x<rsub|i>>. The hypothesis that the
  <with|mode|math|\<delta\>X<rsub|i>> are independent implies that the metric
  is diagonal. By scaling of the various <with|mode|math|x<rsub|i>>, we can
  select a metric which has only <with|mode|math|+1>, <with|mode|math|-1> on
  the diagonal. No scaling is even necessary if we used identical processes
  for the various <with|mode|math|X<rsub|i>> and
  <with|mode|math|\<delta\>X<rsub|i>>.

  We can't have zeroes on the diagonal, as this would correspond to a
  non-observed physical situation where a displacement along one axis does
  not change the distance measurement. This is particularly true for any
  measurement based on counting electromagnetic wavefronts: there is no way
  to select a space or time coordinate along which light is standing still.

  The second postulate then requires that one of the four coordinates
  (corresponding to time) be different from the others. We end up with a
  local metric verifying <with|mode|math|\<delta\>s<rsup|2>=\<delta\>x<rsub|0><rsup|2>-\<delta\>x<rsub|1><rsup|2>-\<delta\>x<rsub|2><rsup|2>-\<delta\>x<rsub|3><rsup|2>>
  or <with|mode|math|<with|mode|text|<with|mode|math|\<delta\>s<rsup|2>=-\<delta\>x<rsub|0><rsup|2>+\<delta\>x<rsub|1><rsup|2>+\<delta\>x<rsub|2><rsup|2>+\<delta\>x<rsub|3><rsup|2>>>>,
  i.e. a Minkowski metric.

  <paragraph|Geodesics>The postulate that the passage of time is a physical
  process <with|mode|math|\<delta\>T> indicates that, locally, the
  measurement <with|mode|math|T> will result in linearly increasing time
  values <with|mode|math|t>. The condition that the
  <with|mode|math|\<delta\>X<rsub|i>> are mutually independent implies that
  at short scale, there is no linear dependency on time of the chosen local
  Minkowskian space coordinates <with|mode|math|x<rsub|i>> for
  <with|mode|math|i=1,2,3>. This yields the local geodesic equation
  <with|mode|math|D*u<rsub|\<mu\>>=0>. As demonstrated in ``classical''
  general relativity, the geodesic equation in arbitrary coordinates
  transforms into <with|mode|math|<frac|\<mathd\><rsup|2>x<rsup|\<mu\>>|\<mathd\>s<rsup|2>>+\<Gamma\><rsup|\<mu\>><rsub|\<nu\>\<rho\>><frac|\<mathd\>x<rsup|\<nu\>>|\<mathd\>s><frac|\<mathd\>x<rsup|\<rho\>>|\<mathd\>s>=0>,
  since the reasoning leading to that other formulation is purely
  mathematical.

  <paragraph|Cosmological constant>The Einstein tensor is defined by
  <eqref|eq:einsteintensor>, where <with|mode|math|\<Lambda\>> is a constant,
  called the ``cosmological constant''.

  <\equation>
    <label|eq:einsteintensor>G<rsub|\<mu\>\<nu\>>=R<rsub|\<mu\>\<nu\>>-<frac|1|2>*R**g<rsub|\<mu\>\<nu\>>-\<Lambda\>g<rsub|\<mu\>\<nu\>>
  </equation>

  The value of the cosmological constant can be deduced from the observations
  at large scale of the universe. There is no strong reason at this point to
  believe that all choices of coordinates <with|mode|math|X<rsub|i>> would
  result in the same observed cosmological constant. We can speculate that
  <with|mode|math|\<Lambda\>> is to some extent a free parameter that relates
  to the choice of physical measurement defining the coordinates and thus the
  metric. This suggestion might help resolve the logical problems that can be
  identified with a more conservative approach<cite|cosmocst>.

  <paragraph|Stress-energy tensor>The stress-energy tensor <with|mode|math|T>
  is a rank (0,2) tensor that indicates how energy and momentum flow.
  Specifically, <with|mode|math|T(u,v)> is the flow along direction
  <with|mode|math|u> of momentum along direction <with|mode|math|v>, where
  energy is represented by the momentum along the time dimension, and the
  flow along the direction of time represents a density.

  Like for the metric, we have a problem defining precisely what measurement
  is valid to measure <with|mode|math|T>. At first sight, a measurement of
  mass at rest with respect to the measurement apparatus seems relatively
  easy to define: we put the mass on a scale. But this simple definition
  relies on the reaction of the mass to the pull of gravitation.

  It seems that a better choice is to define it as a linearization of the
  property of individual particles using <eqref|eq:linearity>, where the
  <with|mode|math|m<rsub|i>> are chosen to balance out the reaction to a
  force of a body made only of particles <with|mode|math|i> and of a body
  made only of particles <with|mode|math|j>. In other words, we define system
  <with|mode|math|\<alpha\>> as being twice as massive as system
  <with|mode|math|\<beta\>> because system <with|mode|math|\<alpha\>> reacts
  to an external force like two systems <with|mode|math|\<beta\>> put
  together.

  But this definition remains problematic, because it does itself rely on
  acceleration. In other words, it also relies on a definition of time and
  space. It would be more satisfying if we could avoid such an implicitly
  recursive definition of the mass.

  Charles Francis has implicitly proposed such a definition<cite|relaq> by
  showing how, for a single particle, the Schwarzschild metric can be derived
  from an intrinsic mass-dependent proper time delay
  <with|mode|math|\<chi\>=4*G*m/c<rsup|3>> between the absorption and the
  re-emission of a photon by the particle. By this construction, a mass is
  simply identified to a proper time.

  <paragraph|Field equation>The law of gravitation in general relativity
  relates the stress-energy tensor <with|mode|math|T> to the Einstein tensor
  <with|mode|math|G> as in <eqref|eq:grfield>, where we choose
  <with|mode|math|\<kappa\>=<frac|8*\<pi\>*k<rsub|G>|c<rsup|2>>> to match
  observations, and <with|mode|math|k<rsub|G>> is Newton's gravitational
  constant.

  <\equation>
    <label|eq:grfield>G=\<kappa\>T
  </equation>

  \ This relation is the fundamental law in general relativity. The
  discussion on fundamental laws is outside the scope of this article. For
  the moment, it is sufficient to note that we can apply this law to the
  results of space-time and stress-energy measurements.

  But this raises an issue. As noted earlier, we have some choice for the
  measurements we use for space/time coordinates, and even if they agree to a
  very good extent locally, they may not agree at a large scale. If we have
  two possible choices for <with|mode|math|X> that lead to different values
  for <with|mode|math|G> at very large scales, the value
  <with|mode|math|\<kappa\>*T> cannot possibly be equal to both.

  We may have already observed this effect. The problem of the ``dark
  matter''<cite|darkmatter> or the anomalous slowdown of the Pioneer
  spacecraft<cite|pioneer> may be the first indications of a divergence at
  large scale between the measurements we chose for space and time compared
  to the measurements we chose for energy or mass.

  <paragraph|Electromagnetism>In general relativity, the electromagnetic
  field tensor <with|mode|math|F<rsup|\<mu\>\<nu\>>> and the 4-vector
  electromagnetic current <with|mode|math|J<rsup|\<nu\>>> are related by the
  equation <with|mode|math|D<rsub|\<mu\>>F<rsup|\<mu\>\<nu\>>=<frac|4*\<pi\>|c>J<rsup|\<nu\>>>.
  The current <with|mode|math|J<rsup|\<nu\>>> is a conserved quantity, so it
  verifies <with|mode|math|\<partial\><rsup|\<lambda\>>F<rsup|\<mu\>\<nu\>>+\<partial\><rsup|\<mu\>>F<rsup|\<nu\>\<lambda\>>+\<partial\><rsup|\<nu\>>F<rsup|\<lambda\>\<mu\>>=0>.
  The effect on a charged object is then given by the impact on the
  momentum-energy 4-vector <with|mode|math|P<rsup|\<mu\>>> of the
  electromagnetic field <with|mode|math|F<rsup|\<mu\>\<nu\>>>, given by the
  field equation <eqref|eq:grelmmove>:

  <\equation>
    <label|eq:grelmmove>D<rsub|t>P<rsup|\<mu\>>=<frac|q|m>P<rsub|\<nu\>>F<rsup|\<mu\>\<nu\>>
  </equation>

  The laws of electromagnetism in general relativity are not obviously
  related to space-time curvature. In particular, <eqref|eq:grelmmove> is
  nothing less than the introduction of a ``force'' in general relativity.
  General relativity does not do away with forces, it gives an interpretation
  of gravitation that is not based on geometric
  considerations<cite|freefalling><nocite|liftsandtopologies>. The movement
  predicted by <eqref|eq:grelmmove> cannot be reduced to any curvature of
  space-time, because it depends on the charge of the particle. A single
  space-time curvature cannot explain why one particle would turn left while
  another with exactly the same properties but the charge would turn right.

  General relativity did not dispense with the notion of ``force'' entirely.
  While it showed that specific geometric properties are equivalent to a
  force, all forces are not necessarily of geometric nature or origin.

  <subsection|<label|scalerelativity>Scale Relativity>

  <paragraph|Zoom>As we change scale, or <dfn|zoom>, we may need to change
  the choice of space and time measurement for practical reasons. We
  therefore replace one choice of coordinates
  <with|mode|math|X<rsub|i/\<lambda\>>> valid at scale
  <with|mode|math|\<lambda\>> with another choice of coordinates
  <with|mode|math|X<rsub|i/\<mu\>>> valid at scale <with|mode|math|\<mu\>>.
  In order for the two choices of coordinate
  <with|mode|math|X<rsub|i/\<lambda\>>> and <with|mode|math|X<rsub|i/\<mu\>>>
  to measure the same physical locations, there must be a large range of
  systems <with|mode|math|\<frak-F\>> where the two measurements give the
  same result, i.e. they verify <eqref|eq:samespace>:

  <\equation>
    <label|eq:samespace>\|X<rsub|i/\<lambda\>>*\<frak-F\>\|=\|X<rsub|i/\<mu\>>*\<frak-F\>\|
  </equation>

  The set of systems where <eqref|eq:samespace> is verified is called the
  <dfn|overlapping range> between <with|mode|math|X<rsub|i/\<lambda\>>> and
  <with|mode|math|X<rsub|i/\<mu\>>>. Using <eqref|eq:spacedef>, we obtain
  <eqref|eq:spaceidentity> which indicates that on the overlapping range, the
  two measurements are related by a linear relation.

  <\equation>
    <label|eq:spaceidentity>x<rsub|i>=k<rsub|i/\<lambda\>>\<delta\>x<rsub|i/\<lambda\>>=k<rsub|i/\<mu\>>\<delta\>x<rsub|i/\<mu\>>
  </equation>

  In general, <eqref|eq:spaceidentity> does not necessarily hold at all
  scale, in particular for all possible values of
  <with|mode|math|k<rsub|i/\<lambda\>>> and
  <with|mode|math|k<rsub|i/\<mu\>>>. In other words, just because two
  measurements are equal on a given range of scales does not imply that they
  are equal at any scale. One may remember the example of the movement of a
  mobile on the surface of a planet, where the tangent coordinates as
  measured along a laser beam match the planet coordinates locally, but not
  at large enough scale.

  When zooming on the system and changing the coordinates measurements from
  <with|mode|math|X<rsub|i/\<lambda\>>> to <with|mode|math|X<rsub|i/\<mu\>>>,
  we can define the zooming factor <with|mode|math|\<zeta\><rsub|\<lambda\>\<mu\>>>
  relating actual measurements done with the choice of coordinates
  <with|mode|math|X<rsub|i/\<lambda\>>> to the actual measurements done with
  the choice of coordinates <with|mode|math|X<rsub|i/\<mu\>>>. Since the
  measurements themselves are performed by counting physical processes, we
  define <with|mode|math|\<zeta\><rsub|\<lambda\>\<mu\>>=<frac|k<rsub|i/\<mu\>>|k<rsub|i/\<lambda\>>>>.
  Whenever <eqref|eq:spaceidentity> holds, we also have
  <with|mode|math|\<zeta\><rsub|\<lambda\>\<mu\>>=<frac|\<delta\>x<rsub|i/\<lambda\>>|\<delta\>x<rsub|i/\<mu\>>>>,
  which justifies the name ``zooming factor'': the zooming factor increases
  as resolution becomes finer.

  <paragraph|Renormalization>The same laws of physics apply on the
  overlapping range where <eqref|eq:samespace> holds, whether they are
  defined using the measurement <with|mode|math|X<rsub|i/\<lambda\>>> or
  <with|mode|math|X<rsub|i/\<mu\>>>. Combined with the linearity of
  <eqref|eq:spaceidentity>, the equality on a large enough range of systems
  <with|mode|math|\<frak-F\>> often implies that the laws themselves must
  remain identical under a change of measurements. This is certainly true for
  ``simple enough'' laws, such as laws relating polynomial functions of the
  coordinates.

  On the other hand, distant laws of physics, for example laws based on
  values measured at ``infinity'', are not guaranteed to remain identical
  under a change of space-time measurements. The measurements may match only
  locally, but not at a large enough scale.

  These remarks can be seen as a physical justification for
  <dfn|renormalization>, a common technique when searching for quantized
  versions of field theories<cite|renormalization>. Renormalization assumes
  that the laws of physics remain identical when zooming out, after one has
  eliminated degrees of freedom that are no longer relevant at the scale
  being considered. When renormalizing, integrals to infinity often diverge
  and must be replaced with actual measured values, whereas the local laws of
  physics remain identical.

  An important practical consequence is that we can consider local laws as
  valid at any scale, but that distant laws may depend at large scale on
  which physical measurement was used.

  <paragraph|Scale invariance>Change of coordinate measurements corresponding
  to a zoom introduces a new coordinate, the resolution
  <with|mode|math|\<lambda\>> at which the measurement is being done.

  The consequences on the structure of space-time of an explicit resolution
  coordinates were studied in details by Laurent Nottale. In particular, he
  postulated the existence of a universal, absolute and impassable scale of
  nature that is invariant under change of scale<cite|scalerelat>. This scale
  plays for zooming a role similar to <with|mode|math|c> for velocity, and
  similarly requires a non-Galilean reformulation of the law describing the
  combination of two changes of scale.

  This leads to an interpretation of space-time as having fractal
  properties<cite|fractalspacetime>. A number of interesting results and
  predictions were made based on this theory, including predictions on the
  location of planets in solar systems, or finer computations of the coupling
  constants.

  Local laws of physics must be invariant through renormalization after zoom.
  Since Planck's scale can be extracted from such local laws, most notably
  Schroedinger's equation, Nottale's hypothesis appears necessary, much like
  the appearance of an arbitrary constant <with|mode|math|c> in Maxwell's
  equations made coordinate changes following the Lorentz transform
  necessary.

  <section|Quantum Mechanics>

  The formalism of the theory of incomplete measurements is also sufficient
  to reconstruct most of quantum mechanics.

  <subsection|<label|predictions>Predictions and Probabilities>

  In general, measurements performed earlier on a system
  <with|mode|math|\<frak-F\>> are not sufficient to make a detailed
  prediction of the result of a new measurement. Instead, what we know about
  <with|mode|math|\<frak-F\>> allows us to make a probabilistic prediction on
  the outcome of the measurement.

  <paragraph|N-valued measurement>The simplest most-general form of
  measurement symbolic result is a set with <with|mode|math|n> different
  values <with|mode|math|m<rsub|1>,\<ldots\>,m<rsub|n>>. We do not need to
  consider continuous measurements, for they are physically unrealistic, in
  the sense that the graduation itself cannot be continuous. For such a
  measurement <with|mode|math|M>, a prediction on the measurement result
  <with|mode|math|m> is represented by a vector of probabilities
  <with|mode|math|(u<rsub|i><rsup|>)> where <with|mode|math|u<rsub|i><rsup|>>
  is the probability to measure <with|mode|math|m<rsub|i>>. Such a vector can
  therefore represent the state of any system <with|mode|math|\<frak-F\>> as
  far as the measurement <with|mode|math|M> is concerned.

  <paragraph|Ket representation>Since <with|mode|math|u<rsub|i>> represents
  probabilities, two conditions must be met:
  <with|mode|math|0\<leqslant\>u<rsub|i>\<leqslant\>1> and
  <with|mode|math|<big|sum>u<rsub|i>=1>. These conditions are more easily
  expressed if we write each probability as a square, i.e. we write
  <with|mode|math|u<rsub|i>=\<psi\><rsub|i><rsup|2>>. The condition
  <with|mode|math|<big|sum>\<psi\><rsub|i><rsup|2>=1> is necessary and
  sufficient to guarantee both conditions on the <with|mode|math|u<rsub|i>>,
  so the <with|mode|math|\<psi\><rsub|i>> can be seen as forming a unit
  vector in <with|mode|math|\<bbb-R\><rsup|n>>, i.e. they belong the the unit
  sphere <with|mode|math|\<bbb-S\><rsup|n-1>>. Using Dirac's bra-ket
  notation, we can write the vector as <with|mode|math|\|\<psi\>\<rangle\>>
  and the probability condition as <with|mode|math|\<langle\>\<psi\>\|\<psi\>\<rangle\>=1>.
  One particular vector state written as <with|mode|math|\|k\<rangle\>>
  correspond to a certainty that the measurement will return the symbolic
  value <with|mode|math|m<rsub|k>>. Its component representation verifies
  <with|mode|math|\<psi\><rsub|i><rsup|2>=\<delta\><rsub|i k>>. Based on the
  definition of the <with|mode|math|\<psi\><rsub|i><rsup|2>>, the probability
  to find the result <with|mode|math|\|k\<rangle\>> when the system is in a
  state <with|mode|math|\|\<psi\>\<rangle\>> is
  <with|mode|math|\<langle\>k\|\<psi\>\<rangle\><rsup|2>>. The various
  <with|mode|math|\|k\<rangle\>> form a basis of
  <with|mode|math|\<bbb-R\><rsup|n>>.

  <paragraph|Physical processes>If <with|mode|math|\<frak-F\><rsub|0>> is an
  initial system state represented by a probability vector
  <with|mode|math|\|\<psi\><rsub|0>\<rangle\>>, <with|mode|math|P> an
  arbitrary physical process, <with|mode|math|\<frak-F\><rsub|>=P*\<frak-F\><rsub|0>>
  the final system state after applying the physical process
  <with|mode|math|P>, and <with|mode|math|\|\<psi\>\<rangle\>> the
  probability vector in the final system state, then the effect of
  <with|mode|math|P> as far as we can tell based on measurement
  <with|mode|math|M> is to transform the probability distribution as
  <with|mode|math|\|\<psi\>\<rangle\>=<wide|P|^>\|\<psi\><rsub|0>\<rangle\>>,
  where <with|mode|math|<wide|P|^>> is some arbitrary operator in
  <with|mode|math|\<bbb-S\><rsup|n-1>>. There is no reason for the operator
  associated to an arbitrary physical process to be linear. For instance, one
  might consider a physical process ensuring that the next measurement result
  for <with|mode|math|M> will be <with|mode|math|m<rsub|k>>. Such a process
  would transform any probability state <with|mode|math|\|\<psi\>\<rangle\>>
  into a state <with|mode|math|\|k\<rangle\>>.

  In general, <with|mode|math|<wide|P|^>> is not even an injective function,
  since for a measurement <with|mode|math|><with|mode|math|M>, the operator
  <with|mode|math|<wide|M|^>> maps a probability state
  <with|mode|math|\|\<psi\>\<rangle\>> to a random state
  <with|mode|math|\|k\<rangle\>> with probability
  <with|mode|math|><with|mode|math|\<langle\>k\|\<psi\>\<rangle\><rsup|2>>.
  We can define the non-injective ``random'' operator
  <with|mode|math|<wide|R<rsub|n><rsub|>|^>> that performs this same
  operation. All perfectly definite measurements, i.e. all measurements for
  which there is no uncertainty at the end of the measurement, verify
  <with|mode|math|<wide|M|^>=<wide|R<rsub|n>|^>>.

  <paragraph|Indistinguishable processes>If <with|mode|math|P<rsub|1>> and
  <with|mode|math|P<rsub|2>> are indistinguishable processes as far as
  <with|mode|math|M> is concerned, then the probability outcome should not
  depend on whether the process <with|mode|math|P<rsub|1>> or
  <with|mode|math|P<rsub|2>> is applied to the system
  <with|mode|math|\<frak-F\>>. Assuming that the system
  <with|mode|math|\<frak-F\>> can be constructed with an arbitrary
  probability state <with|mode|math|\|\<psi\>\<rangle\>>, we get
  <with|mode|math|\<forall\>\<psi\>, <wide|P<rsub|1>|^>\|\<psi\>\<rangle\>=<wide|P<rsub|2>|^>\|\<psi\>\<rangle\>>,
  which means that <with|mode|math|<wide|P<rsub|1>|^>=<wide|P<rsub|2>|^>>.
  Consequently, processes that are indistinguishable with respect to
  <with|mode|math|M> are associated with the same operator on
  <with|mode|math|\<bbb-S\><rsup|n-1>>. Identical processes are
  indistinguishable for all measurements.

  <paragraph|Measurements>If the physical process <with|mode|math|P> is the
  measurement <with|mode|math|M> itself, the probability vector
  <with|mode|math|<wide|M|^>\|\<psi\>\<rangle\>> indicates a certainty for
  the measured value <with|mode|math|m<rsub|k>> and an impossibility for the
  others, so <with|mode|math|<wide|M|^>\|\<psi\>\<rangle\>=\|k\<rangle\>>.
  The repeatability condition <with|mode|math|\|M*M*\<frak-F\>\|=\|M*\<frak-F\>\|>
  implies that once the measurement result <with|mode|math|m<rsub|k>> is
  obtained, the measurement will continue to yield the result
  <with|mode|math|m<rsub|k>>. This implies
  <with|mode|math|<wide|M*|^>*<wide|M|^>\|\<psi\>\<rangle\>=<wide|M|^>\|\<psi\>\<rangle\>>,
  or <with|mode|math|\|k\<rangle\>=<wide|M|^>\|k\<rangle\>>. Therefore, the
  probability state immediately after a measurement is an eigenvector of the
  operator <with|mode|math|<wide|M|^>> associated to the measurement process
  <with|mode|math|M>.

  <paragraph|Linearized operator>The operator <with|mode|math|<wide|M|^>> is
  a possibly non-linear operator over <with|mode|math|\<bbb-S\><rsup|n-1>>
  verifying <with|mode|math|<wide|M|^>\|k\<rangle\>=\|k\<rangle\>>. If the
  <with|mode|math|m<rsub|i>> are real numbers, one can define a linear
  operator <with|mode|math|<wide|M|\<check\>>> defined as taking the
  <with|mode|math|\|k\<rangle\>> as eigenvectors with value
  <with|mode|math|m<rsub|k>>. Since the <with|mode|math|\|k\<rangle\>> form a
  basis, <with|mode|math|<wide|M|\<check\>>> is defined entirely by the
  equations <with|mode|math|<wide|M|\<check\>>\|k\<rangle\>=m<rsub|k>\|k\<rangle\>>.
  This operator has two interesting properties: it is linear, and it can be
  used to compute the expected value (in a probability sense)
  <with|mode|math|E(m)> of the measurement <with|mode|math|M> for a system
  described by the probability vector <with|mode|math|\|\<psi\>\<rangle\>>.

  The reasoning is exactly the same as in ``traditional'' quantum mechanics.
  Projecting <with|mode|math|\|\<psi\>\<rangle\>> over the individual basis
  vector <with|mode|math|\|i\<rangle\>>, <with|mode|math|\|\<psi\>\<rangle\>=<big|sum>\<langle\>\<psi\>\|i\<rangle\>*\|i\<rangle\>>.
  Applying <with|mode|math|<wide|M|\<check\>>>, we have
  <with|mode|math|<wide|M|\<check\>>\|\<psi\>\<rangle\>=<big|sum>\<langle\>\<psi\>\|i\<rangle\>*<wide|M|\<check\>>\|i\<rangle\>>.
  Applying the eigenvector equation, we get
  <with|mode|math|<wide|M|\<check\>>\|\<psi\>\<rangle\>=<big|sum>\<langle\>\<psi\>\|i\<rangle\>*m<rsub|i>\|i\<rangle\>>.
  Taking the inner product by <with|mode|math|\|\<psi\>\<rangle\>>, we obtain
  <with|mode|math|\<langle\>\<psi\>\|<wide|M|\<check\>>\|\<psi\>\<rangle\>=<big|sum>m<rsub|i>\<langle\>\<psi\>\|i\<rangle\><rsup|2>>.
  Since <with|mode|math|\<langle\>\<psi\>\|i\<rangle\><rsup|2>=\<psi\><rsub|i><rsup|2>>
  is the probability to obtain measurement result <with|mode|math|m<rsub|i>>,
  we finally obtain <eqref|eq:expected>:

  <\equation>
    <label|eq:expected>E(m)=\<langle\>\<psi\>\|<wide|M|\<check\>>\|\<psi\>\<rangle\>
  </equation>

  Note however that <with|mode|math|<wide|M|\<check\>>\|\<psi\>\<rangle\>> is
  no longer on the unit sphere <with|mode|math|\<bbb-S\><rsup|n-1>> in
  general, so it is no longer a probability vector. This is a practical
  drawback of <with|mode|math|<wide|M|\<check\>>> compared to
  <with|mode|math|<wide|M|^>>, in particular if 0 is an acceptable
  measurement value, because we need to renormalize probabilities after
  applying it.

  <paragraph|Linear approximation>A measurement <with|mode|math|M> verifying
  the linear approximation in <eqref|eq:linearity> defines a countable set of
  possible measurement results <with|mode|math|m<rsub|k<rsub|1>,\<ldots\>,k<rsub|n>>>,
  since all the <with|mode|math|k<rsub|i>> belong to a countable set. We can
  write the probability vector indicating a certainty that the measurement
  result is <with|mode|math|m<rsub|k<rsub|1>,\<ldots\>,k<rsub|n>>> as
  <with|mode|math|\|k<rsub|1>,\<ldots\>,k<rsub|n>\<rangle\>>.

  By definition of the <with|mode|math|k<rsub|i>> in relationship to the
  physical processes <with|mode|math|P<rsub|i>>, the operator
  <with|mode|math|<wide|P<rsub|i>|^>> transforms the vector
  <with|mode|math|\|k<rsub|1>,\<ldots\>,k<rsub|i>,\<ldots\>,k<rsub|n>\<rangle\>>
  into the vector <with|mode|math|\|k<rsub|1>,\<ldots\>,k<rsub|i>+1,\<ldots\>,k<rsub|n>\<rangle\>>.
  Since the probability vectors form a basis of the probability state, we can
  define a linear operator <with|mode|math|<wide|P<rsub|i>|^>> defined by
  this relationship, and that operator will generate the same predictions
  with respect to measurement <with|mode|math|M>. Using the reasoning about
  identical processes made above, the operator
  <with|mode|math|<wide|P<rsub|i>|^>> in that case can reasonably be
  assimilated to the linear operator defined by <eqref|eq:linprob>:

  <\equation>
    <label|eq:linprob><wide|P<rsub|i>|^>\|k<rsub|1>,\<ldots\>,k<rsub|i>,\<ldots\>,k<rsub|n>\<rangle\>=\|k<rsub|1>,\<ldots\>,k<rsub|i>+1,\<ldots\>,k<rsub|n>\<rangle\>
  </equation>

  <paragraph|Combined system>If we want to evaluate two distinct measurement
  processes <with|mode|math|M<rsub|1>> giving <with|mode|math|n<rsub|1>>
  different results and <with|mode|math|M<rsub|2>> giving
  <with|mode|math|n<rsub|2>> different results, the system will be described
  by two probability vectors <with|mode|math|\|\<psi\><rsub|1>\<rangle\>> and
  <with|mode|math|\|\<psi\><rsub|2>\<rangle\>>, and so the state of the
  system is described in the tensor product
  <with|mode|math|\<bbb-R\><rsup|n<rsub|1>>\<otimes\>\<bbb-R\><rsup|n<rsub|2>>>
  on the unit sphere <with|mode|math|\<bbb-S\><rsup|n<rsub|1>+n<rsub|2>-1>>.

  If the two measurements are independent, then they have no effect on one
  another's probability state, and therefore the corresponding operators
  <with|mode|math|<wide|M|\<check\>>> and <with|mode|math|<wide|M|^>> on the
  tensor product space are built as tensor products of the original operator
  by an identity matrix. Note that <eqref|eq:focused> implies that the effect
  of the rest of the universe can be ignored.

  Not all physical processes are independent from <with|mode|math|M<rsub|1>>
  and <with|mode|math|M<rsub|2>>, however. Such physical processes may create
  ``entangled'' states in the tensor space.

  <subsection|<label|analogy>Formal Analogy>

  The structure of the observations made in the previous section is extremely
  similar to the way traditional quantum mechanics is usually written,
  provided we replace ``<em|wave-function>'' with ``<em|probability
  vector>'', and ``<em|observable>'' with ``<em|measurement operator>''.

  Many formulation of quantum mechanics exist. We will use the following
  principles<cite|basdevant><cite|cohen> to illustrate the parallel with the
  formalism presented here:

  <\enumerate>
    <item>The state of a physical system is defined by a ket
    <with|mode|math|<mid|\|>\<psi\><mid|rangle>> in a Hilbert space
    <with|mode|math|\<cal-E\>>

    <item>Every measurable physical value <with|mode|math|\<cal-A\>> is
    described by an linear operator <with|mode|math|A> on
    <with|mode|math|\<cal-E\>>; this operator is an observable, i.e. a
    self-adjoint operator on <with|mode|math|\<cal-E\>>.

    <item>A measurement of <with|mode|math|\<cal-A\>> can only give as a
    result one of the eigenvalues of <with|mode|math|A>. In particular, the
    result is a real number. If <with|mode|math|A> has a discrete spectrum,
    then the results of the measurement are quantized.

    <item>The expected value of the observable <with|mode|math|A> for a
    system in a state represented by <with|mode|math|<mid|\|>\<psi\><mid|rangle>>
    is <with|mode|math|<mid|langle>\<psi\><mid|\|>A<mid|\|>\<psi\><mid|rangle>>.

    <item>If a measurement on a system in state
    <with|mode|math|<mid|\|>\<psi\><mid|rangle>> for observable
    <with|mode|math|A> gives result <with|mode|math|a>, then the state of the
    system immediately after the measurement is the normed projection of
    <with|mode|math|<mid|\|>\<psi\><mid|rangle>> on the eigenspace associated
    to <with|mode|math|a>.

    <item> The Hilbert space for a composite system is the tensor product of
    the state spaces for the component systems.
  </enumerate>

  Compare this to some conclusions of the formalism presented here:

  <\enumerate>
    <item>What can be predicted about a measurement <with|mode|math|M> is
    represented by a probability vector <with|mode|math|\|\<psi\>\<rangle\>>
    on <with|mode|math|\<bbb-S\><rsup|n-1>> (i.e. a vector of
    <with|mode|math|\<bbb-R\><rsup|n>> verifying
    <with|mode|math|\<langle\>\<psi\>\|\<psi\>\<rangle\>=1>), where
    <with|mode|math|n> is the number of distinct symbolic or numerical
    measurement results <with|mode|math|m<rsub|k>>.

    <item>Every physical process <with|mode|math|P> (including measurements)
    is associated to a possibly non-linear operator
    <with|mode|math|<wide|P|^>> on <with|mode|math|\<bbb-S\><rsup|n-1>>
    transforming the probability vector <with|mode|math|\|\<psi\>\<rangle\>>.
    A measurement <with|mode|math|M> where the <with|mode|math|m<rsub|k>> are
    real is associated to a linear operator
    <with|mode|math|<wide|M|\<check\>>> that has the
    <with|mode|math|m<rsub|k>> as eigenvalues.

    <item>When the system is in state <with|mode|math|\|\<psi\>\<rangle\>>,
    the probability that the measurement will give the symbolic result
    <with|mode|math|m<rsub|k>> is <with|mode|math|\<langle\>k\|\<psi\>\<rangle\><rsup|2>>,
    where <with|mode|math|\|k\<rangle\>> is the vector of
    <with|mode|math|\<bbb-S\><rsup|n-1>> defined by the components
    <with|mode|math|\<delta\><rsub|i*k>>. <with|mode|math|\|k\<rangle\>>
    verifies the equations <with|mode|math|<wide|M|^>\|k\<rangle\>=\|k\<rangle\>>
    and, if the <with|mode|math|m<rsub|k>> are real,
    <with|mode|math|<wide|M|\<check\>>\|k\<rangle\>=m<rsub|k>\|k\<rangle\>>.

    <item>If the <with|mode|math|m<rsub|k>> are real, the expected value of
    the measurement results for <with|mode|math|M> when the system is
    described by the probability vector <with|mode|math|\|\<psi\>\<rangle\>>
    is <with|mode|math|\<langle\>\<psi\>\|<wide|M|\<check\>>\|\<psi\>\<rangle\>>.

    <item>If a measurement for a system gives result
    <with|mode|math|m<rsub|k>> with certainty, then the state of the system
    immediately after the measurement is <with|mode|math|\|k\<rangle\>>.

    <item>The probability state associated to a composite system is the
    tensor product of the state spaces for the component systems.
  </enumerate>

  This parallel suggests that quantum mechanics can be interpreted as a
  theory of measurements. In that interpretation, some of the axioms of
  quantum mechanics, like linearity, appear related to specific experimental
  conditions or choices of graduation, while others are more general and
  derived from what physical processes we chose to accept as measurements.

  <paragraph|Trajectory measurement>The trajectory of a particle can be
  defined as a set of identical measurements <with|mode|math|M<rsub|exist>>
  applied to a family of systems <with|mode|math|\<frak-F\><rsub|x*y*z*t>>,
  where each measurement determines whether the particle was found or not at
  coordinates <with|mode|math|(x,y,z,t)=(x<rsub|i>)>. Therefore, the
  measurement can return two symbolic results <verbatim|found> when the
  particle was found to be present in <with|mode|math|\<frak-F\><rsub|x*y*z*t>>,
  and <verbatim|not-found> when the particle was not found to be present.

  Since the measurements <with|mode|math|M<rsub|exist>> are two-valued, what
  can be predicted about the measurement results can be represented by a
  vector on the unit circle <with|mode|math|\<bbb-S\><rsup|1>>, or, by
  isomorphism, by a complex number of the form
  <with|mode|math|e<rsup|i*\<theta\>>>, where
  <with|mode|math|Re(e<rsup|i*\<theta\>>)<rsup|2>=cos<rsup|2> \<theta\>> is,
  for instance, the probability that the particle is found. Therefore, it
  makes sense to define <with|mode|math|\<psi\>(x<rsub|i>)=e<rsup|i*\<theta\>(x<rsub|i>)>>
  as the representation of the probability state that the particle is found
  for that family of universe fragments.

  <paragraph|Normalization of the wave-function>A second condition is
  necessary, however. The various <with|mode|math|\<psi\>(x<rsub|i>)> are not
  independent from one another. If we perform <with|mode|math|M<rsub|exist>>
  at multiple points of space simultaneously in a given frame of reference,
  only one of them may give a result <verbatim|found>, all the others must
  give the result <verbatim|not-found>. This means that the sum of the
  probability vectors corresponding to <verbatim|found> over all the space
  where we test existence must be at most 1, and exactly 1 if the measured
  entity cannot escape the measurement apparatus. This probability condition
  then writes as <with|mode|math|<big|sum><rsub|x<rsub|i>>Re(\<psi\>(x<rsub|i>))<rsup|2>=1>.

  We can define a wave-function <with|mode|math|\<Psi\>> with amplitude
  <with|mode|math|\<\|\|\>\<Psi\>(x<rsub|i>)\<\|\|\><rsup|2>=Re(\<psi\>(x<rsub|i>))<rsup|2>>
  and with the same phases as <with|mode|math|\<psi\>(x<rsub|i>)>. The first
  condition corresponds to the normalization condition for the wave-function
  <with|mode|math|\<Psi\>> in traditional quantum mechanics, i.e.
  <with|mode|math|\<langle\>\<Psi\>\|\<Psi\>\<rangle\>=1>. The second
  condition, preserving the same phase as
  <with|mode|math|\<psi\>(x<rsub|i>)>, keeps the vectors
  <with|mode|math|\<Psi\>(x<rsub|i>)> and <with|mode|math|\<psi\>(x<rsub|i>)>
  aligned in the complex plane. Therefore, it ensures that the linear
  operators for which <with|mode|math|\<psi\>(x<rsub|i>)> is an eigenvector
  also have <with|mode|math|\<Psi\>(x<rsub|i>)> as an eigenvector.

  However, in the reasoning above, <with|mode|math|\<Psi\>> would not be
  normalized by summing over all of space-time, but only over the points of
  space-time where the existence measurement is being carried out. This
  remark eliminate the largest portion of the problems that arise when
  renormalizing the wave-function in quantum mechanics, notably in
  relationship with the kind of non-Euclidean changes of coordinates required
  by general relativity. Here, we compute the norm of the wave-function
  <with|mode|math|\<Psi\>> only on the points of space time where a test is
  being carried out, and at the time where it could be carried out.

  For example, if we place a very large photographic plate, so that it takes
  one second for light to go from the center to one of the border, the
  normalization condition for a photon has to be understood as covering
  points in space-time that are not simultaneous, since the detection, if it
  happens at the border of the plate, will happen one second later than if it
  happens in the center. The wave-function <with|mode|math|\<Psi\>> does not
  collapse ``instantaneously'' as in quantum mechanics, it instead collapses
  on a light cone centered on the ``last known position'' of the photon, i.e.
  the last universe fragment <with|mode|math|\<frak-F\>> where
  <with|mode|math|M<rsub|exist>> returned <verbatim|found>. Seen from a point
  where the measurement actually finds <verbatim|found>, the collapse
  therefore may contain points that are both in the future and in the past.
  The points that are farther away from the last known position will collapse
  in the future, while the points that are closer from the last known
  position have already collapsed.

  <paragraph|Planar wave-function in ``empty space''>If we consider an
  evolution along individual local-time displacements
  <with|mode|math|\<delta\>T> of a particle, if the
  <with|mode|math|\<delta\>T> are indistinguishable with respect to
  <with|mode|math|M<rsub|exist>>, then any two
  <with|mode|math|<wide|\<delta\>T|^>> \ should be identical. As we noted
  earlier, in the general case, <with|mode|math|<wide|P|^>> is not
  necessarily an injective function, let alone linear. However, in classical
  theories of physics, measurements are approximated by locally
  differentiable functions of time, which implies that they are locally
  linear to the first order (with the form
  <with|mode|math|<frac|\<mathd\>m|\<mathd\>t>*\<delta\>t>). Applying the
  reasoning leading to <eqref|eq:linprob>,
  <with|mode|math|<wide|\<delta\>T|^>> itself can therefore be assimilated to
  a linear operator on the probability space. In turn, if
  <with|mode|math|<wide|\<delta\>T|^>> is linear with respect to these
  various measurements, it is not unreasonable to assume that it is linear
  with respect to <with|mode|math|M<rsub|exist>> as well.

  We will therefore <em|postulate> that <with|mode|math|<wide|\<delta\>T|^>>
  is a linear operator with respect to <with|mode|math|M<rsub|exist>>. In
  that case, the most general linear transform on
  <with|mode|math|\<bbb-S\><rsup|1>> is a rotation by angle
  <with|mode|math|\<delta\>\<theta\>>. Since the local laws of physics,
  including evolution through local time, should not change if we change the
  choice of <with|mode|math|\<delta\>T>, an application of
  <eqref|eq:spaceidentity> below when changing the choice of
  <with|mode|math|\<delta\>T> implies that
  <with|mode|math|\<delta\>\<theta\>> is a linear function of
  <with|mode|math|\<delta\>t>, i.e. <with|mode|math|\<delta\>\<theta\>=\<omega\>*\<delta\>t>.
  Therefore, after applying the time definition in
  <with|mode|math|<eqref|eq:timedef>>, we expect <with|mode|math|\<Psi\>> to
  take a general form <with|mode|math|\<Psi\>(x,y,z,t)=e<rsup|i*\<omega\>*t>>,
  and the probability of presence takes the form of a planar wave from a
  perspective of local time.

  This is exactly what is being observed experimentally for example in
  Young's slits experiment. If we perform a relativistic change of
  coordinates, we obtain the more general form
  <with|mode|math|\<Psi\>(<wide|x|\<vect\>>)=e<rsup|i*\<langle\><wide|k|\<vect\>>\|<wide|x|\<vect\>>\<rangle\>>>,
  where <with|mode|math|<wide|x|\<vect\>>> is the position 4-vector and
  <with|mode|math|<wide|k|\<vect\>>> is the wave 4-vector. This ``planar
  wave'' condition remains true for all universe fragments
  <with|mode|math|\<frak-F\>> connected through one another by
  <with|mode|math|\<delta\>T> that are indistinguishable from one another for
  <with|mode|math|M<rsub|exist>> and where
  <with|mode|math|<wide|\<delta\>T|^>> is linear.

  <subsection|<label|interpretation>Interpretation>

  Implications of this parallel between quantum mechanics and measurements
  are discussed below.

  <paragraph|Disentanglement>In quantum mechanics, combining two systems is
  done by considering the tensor product of the spaces describing each
  individual system. At the limit, the universe would be represented by a
  wave-function in an infinite tensor product combining the Hilbert space of
  all possible systems (a Fock space), and the universe would contain any
  possible entangled state. An ad-hoc simplification in quantum mechanics is
  to ignore all but the Hilbert space describing the system, as if the rest
  of the universe was not there.

  We justify this simplification with <eqref|eq:focused>. Measurements are
  designed to provide focus in the physical realm, and to ignore all but a
  selected fragment of the universe.

  <paragraph|Real eigenvalues>In quantum mechanics, the possible results of a
  measurement are postulated to be the eigenvalue of an observable, i.e. a
  self-adjoint operator on a Hilbert space. Quantum measurement results are
  real numbers because observables only have real eigenvalues.

  In the theory of incomplete measurements, measurement results for any
  physical measurement are always discrete, because we do not know how to
  make measurements that are truly continuous. However, if the symbolic
  values for each measurement value are real numbers, one can build a linear
  operator <with|mode|math|<wide|M|\<check\>>> that has the possible
  measurement values as real eigenvalues.

  <paragraph|Wave-function Collapse>Since quantum mechanics is linear and
  reversible, it is difficult to explain why the wave-function should
  collapse. The need for this rule is widely considered as a problem, and
  many suggestions have been made to try and explain
  it<cite|epr><cite|pondicherry><cite|relatquantmech>.

  In the present formalism, the wave-function collapses because the
  measurement system is designed to produce a repeatable numerical result.
  The wave-function collapse is not instantaneous but happens as part of the
  measurement process itself, and is a consequence of the non-linearity of
  <with|mode|math|<wide|M|^>>. It is possible to consider <dfn|incomplete>
  measurements where the resulting measurement probability
  <with|mode|math|\|\<psi\>\<rangle\>> is only partially collapsed. These
  states would correspond to measurements containing a dose of uncertainty,
  like <with|mode|math|2.56V\<pm\>0.2%>.

  While the present theory does not need an additional postulate than ``there
  are measurements'' to predict the collapse of the wave-function, it
  ultimately does not answer the question ``but then, why are there
  measurements?''. In a sense, the question has only been rewritten. In the
  context of the present theory, quantum decoherence<cite|quantdecoherence>
  for example is a useful analysis of how measurements might appear as a
  consequence of more fundamental processes.

  <paragraph|Superposition>In quantum mechanics, any linear combination of
  valid states is a valid state. In particular, linear combinations of
  eigenvectors that are not themselves eigenvectors are acceptable kets.

  The present formalism observes that in general, a sequence in time or
  juxtaposition in space of physically acceptable processes is a physically
  acceptable process. There are physical processes that are not measurements
  and do not obey <eqref|eq:repeat> <with|mode|math|M*M*\<frak-F\>=M*\<frak-F\>
  >. They do not need to turn any universe fragment into an ``eigenstates''
  of the measurement. Consequently, it is safer to assume that any
  probability state <with|mode|math|\|\<psi\>\<rangle\>> in
  <with|mode|math|\<bbb-S\><rsup|n-1>> may represent a physically acceptable
  prediction for <with|mode|math|M> applied to some hypothetical state
  <with|mode|math|\<frak-F\>>.

  The method used to justify the use of a wave-function as a representation
  of a particle probability also explains why, in Young's slits experiment,
  <em|removing> a part of the system (the second slit) would actually cause
  the wave-functions to <em|add up> instead of being subtracted. The reason
  is that by removing part of the obstacle, we actually added a new
  possibility for the particle to cross over, so we need to add the
  probabilities of measuring a <verbatim|found> or <verbatim|not-found>
  state.\ 

  <paragraph|Incomplete measurements>In quantum mechanics, probabilities only
  exist before a measurement, and are defined by how an ``entangled'' ket, a
  linear combination of eigenbasis kets, would project on the eigenbasis.
  After the measurement, there are no more probabilities, both the
  measurement result and corresponding ket state are known with absolute
  precision.

  In practice, measurements are not always exact nor
  instantaneous<cite|jacobssteck>. The result itself may be probabilistic,
  something that is expressed in a measurement result like
  <with|mode|math|2.45V\<pm\>0.1V>. The formalism presented here allows the
  state after a measurement result to be itself a probability vector. There
  is no instantaneous collapse either. As the measurement progresses, the
  distribution becomes narrower, and the set of compatible universe states
  predicted by <eqref|eq:predict> becomes smaller. At least for some
  measurements, the fully collapsed wave-function and the infinitely precise
  measurement may only be idealized limits.

  <paragraph|Measurement limits>Many quantum physics textbooks state
  confidently that the probability of finding the particle being observed on
  the other side of the moon<cite|darksideofthemoon> is infinitesimal, but
  not null, because the wave-function vanishes quickly with distance but
  never reaches zero.

  In the formalism presented here, only results that can actually be
  generated by the measurement are acceptable. If your measurement apparatus
  is a photographic plate measuring 10x10 centimeters, the chances that it
  will report that it found the photon somewhere on the moon (or anywhere
  outside the plate, for that matter) is <em|exactly> zero. Note that the
  point is not that the probability for the photon to miss the plate is zero,
  only that we cannot talk about the probability to detect the photon at a
  specific location where there is no detection instrument.

  <paragraph|Linearity>The mathematics of quantum mechanics uses linear
  algebra on the system states, as represented by wave-functions or kets.

  The present formalism represents probability distributions as a vector in
  <with|mode|math|\<bbb-S\><rsup|n-1>>. In general, operators on the
  probability states are not linear, but for a measurement <with|mode|math|M>
  giving real measurement results, the linear operator
  <with|mode|math|<wide|M|\<check\>>> has the properties associated to
  observables in quantum mechanics.

  <paragraph|Commutation of Observables>For mathematical reasons, quantum
  mechanics observables that do not commute correspond to measurements that
  cannot be done independently. Heisenberg's inequality is the archetypal
  application of that observation for position and momentum observables.

  In the present formalism, the reasoning relating <eqref|eq:commutx>,
  \ <eqref|eq:commuty> and \ <eqref|eq:commut> suggest that, as long as a
  physical process is reversible, independent measurements correspond to
  physical processes that commute, and conversely, measurements that do not
  commute impact one another and do not give independent results. Once
  non-commutativity is established, the quantitative mathematical treatment
  given to the probability state <with|mode|math|\|\<psi\>\<rangle\>> on the
  tensor product space is identical to the mathematical treatment given in
  quantum mechanics to the ket <with|mode|math|\|\<psi\>\<rangle\>>.

  <paragraph|Quantum space-time>In the traditional approach of quantum
  mechanics, the space and time operators commute with one another, because
  they are based on multiplying a ket with a coordinate, and the
  multiplication of real numbers commutes. Based on the above analysis, this
  defines Euclidean space-time coordinates, and this is clearly a source of
  conflict both with physical observation and with general relativity.

  Research in the field of non-commutative geometry<cite|connes> is one of
  the promising paths towards a quantum theory of gravity. However, most
  applications to physics remain firmly grounded in
  mathematics<cite|noncomm>. The present work shows a relatively simple
  physical interpretation for this work, and suggests that surprising aspects
  of it should not be too quickly discarded as mathematically absurd.

  For example, one particular issue with respect to a traditional or
  perturbative treatment of space-time non-commutativity is the appearance of
  non-unitarity in the S-matrix<cite|nonunit>. Non-unitarity, if interpreted
  as the sum of all probabilities no longer being 1, raises a serious problem
  from a quantum mechanics point of view.

  But if we accept that non-commutativity must be possible, then this
  indicates that the S-matrix lost its physical significance. This is not
  surprising considering that an S-matrix relates the state of the system to
  states ``at infinity'', and those depend on the large-scale space-time
  structure. By analogy, if one is computing the probability to find a mobile
  running on the surface of earth, summing probabilities on a tangent plane
  will yield incorrect results.

  <paragraph|Hidden Variables>The well known theorem of Bell gave a
  verifiable method for determining if quantum mechanics can be explained by
  hidden variables. While the various kinds of hidden variable theories
  (local, non-local, stochastic) make a final conclusion harder to reach, to
  this day, experiments conducted by Alain Aspect <cite|aspect> and others
  would tend to disprove hidden variable theories.

  The interpretation of quantum theory given here does not rely on hidden
  variables. The reasoning is based entirely on the existence of
  probabilities based on what we know, and this is the meaning we gave to
  <em|incomplete>. But it does not imply that we can know more than what
  measurements give us. The existence of randomness in the most precise
  description we can make of the universe remains possible. Thus, even if the
  measurement is said to be incomplete, it may still be the most complete
  description of a system we can give.

  <section|Conclusion and Further work>

  We have presented a formalism to discuss physical experiments without
  implicitly assuming a specific measurement process. This allowed us to
  propose a definition of measurements, and based on formal reasoning, to
  reconstruct many important aspects of both general relativity and quantum
  mechanics, while suggesting a number of limitations in these theories which
  were not obvious from a purely mathematical standpoint. We should now try
  to understand the impact of these limitations on experiments we have made
  or will make.

  <paragraph|Fundamental equations>One important closing remark is that we
  did not present any ``fundamental equation'', i.e. something equivalent to
  the field equation in general relativity or to Schroedinger's equation in
  quantum mechanics. An equation like <eqref|eq:timevol> is clearly not
  specific enough to make predictions.

  For the moment, we can simply ``borrow'' the existing fundamental equations
  from the respective theories, with an understanding that these may be
  approximations corresponding to a particular choice of physical
  measurement. This way, we can leverage the numerous advances made during
  the past century, including the many forms of Lagrange equations and the
  gauge invariance arguments that have proven so successful in predicting
  particle interactions. In that sense, the present theory is ``backwards''
  from most recent research: whereas recent work is often about suggesting a
  new fundamental equation, the theory presented here replaced practically
  everything but the fundamental equations.

  <paragraph|Characteristics of the theory>With these borrowed fundamental
  equations, the theory presented here has the following characteristics:

  <\itemize>
    <item>All measurements are treated identically, including time and space
    measurements.

    <item>The theory addresses incomplete measurements, i.e. measurements
    from which only probabilistic measurements can be made. This is true
    whether the uncertainty is due to a partial collection of data we believe
    to be complete, or whether the uncertainty itself is believed to be
    fundamental.

    <item>At small scale, where measurements results can as far as we know
    only be predicted statistically, the theory is formally almost identical
    to quantum mechanics, and therefore incorporates its predictions by
    reference. In particular, the theory predicts the mathematical shape of
    the quantum mechanics wave-function under a reasonable set of conditions.
    Quantum mechanics is seen as an approximation that considers only linear
    operators and ignores the effects of non-commutativity of displacements
    along space and time. The theory presented here can be understood as
    providing yet another interpretation of quantum mechanics.

    <item>At large scale, the effects of non-commutativity become dominant,
    but measurements return quasi-deterministic results, so that measurement
    values can be approximated by continuously differentiable functions. In
    that case, the present formalism becomes formally identical to general
    relativity, and therefore incorporates its predictions by reference.
    General relativity is seen as an approximation where measurements are
    considered continuously differentiable.

    <item>The transition between different time and space measurements
    introduces an explicit factor <with|mode|math|\<lambda\>>, which is
    assimilated to the scale factor formally introduced in the theory of
    scale relativity. It is therefore expected that the present theory
    incorporates scale relativity results by reference, and explains them
    from a physical perspective.
  </itemize>

  <paragraph|Future work>Obviously, simply borrowing fundamental equations
  and incorporating them in the present theory is not very satisfying. For
  this reason, finding a fundamental equation should now be the most active
  topic of research.

  In that respect, we can already make a number of preliminary observations.
  First, we can note that all theories of physics accepted so far have at
  least one local and one distant fundamental law (and generally several
  distant laws corresponding to different interactions):

  <\itemize>
    <item>The local law describes how the system being studied behaves. In
    general relativity, the equation <with|mode|math|D*u<rsub|\<mu\>>=0>
    plays that role, whereas in quantum mechanics, various formulations
    coexist, starting with Schroedinger's equation.

    <item>The distant law describes how the rest of the universe can be
    ``summarized'' as far as the local system is concerned. In general
    relativity, the field equation plays that role. In quantum mechanics,
    theories are specified by their Lagrangian or, equivalently, by a
    Hamiltonian or action.
  </itemize>

  Consequently, it is fair to say that none of our theories at this point can
  be considered as fully unified, in the sense that none can derive
  large-scale laws like the shape of the Hamiltonian from local laws like the
  local evolution of the wave-function.

  Attempting to resolve that issue brings the second observation, which we
  can make using the gravitational interaction. Mass and energy play two
  complementary roles in general relativity. Locally, mass is a resistance to
  change, as shown in <eqref|eq:grelmmove>: the higher the mass, the lesser
  the impact of external interaction. Globally, mass causes change, as shown
  in <eqref|eq:grfield>: the higher the mass, the bigger the impact in
  external interaction. The relation <with|mode|math|E=h*\<nu\>> relates
  these two aspects, and therefore there seems to be a relation between the
  wave-particle duality and the local-distant duality.

  <section*|Acknowledgments>

  This paper is dedicated to my parents who taught me practically everything
  I know about the universe. Special thanks go to my wife Christine and our
  children, who patiently accommodated the long hours thinking, writing, and
  then starting all over again.

  <\bibliography|bib|plain|tim.bib>
    <\bib-list|10>
      <bibitem*|1><label|bib-epr>Boris<nbsp>Podolsky Albert<nbsp>Einstein and
      Nathan Rosen. <newblock>Can quantum-mechanical description of physical
      reality be considered complete? <newblock><with|font-shape|italic|Phys.
      Rev.>, 47:777, 1935.

      <bibitem*|2><label|bib-noncomm>C.<nbsp>Molina P. Teotonio-Sobrinho
      A.P.<nbsp>Balachandran, T.R.<nbsp>Govindarajan. <newblock>Unitary
      quantum physics with time-space noncommutativity.
      <newblock><with|font-shape|italic|JHEP>, 0410:072, 2004.

      <bibitem*|3><label|bib-aspect>Alain Aspect. <newblock>Three
      experimental tests of bell inequalities by the measurement of
      polarization correlations between photons.
      <newblock><with|font-shape|italic|PhD Thesis>, 1983.

      <bibitem*|4><label|bib-basdevant>Jean-Louis Basdevant.
      <newblock><with|font-shape|italic|Mecanique Quantique, cours de l'Ecole
      Polytechnique>. <newblock>Ellipse, Paris.

      <bibitem*|5><label|bib-timemsmt>Bernard<nbsp>Guinot Claude<nbsp>Audoin.
      <newblock><with|font-shape|italic|Les fondements de la mesure du
      temps>. <newblock>Masson, Paris.

      <bibitem*|6><label|bib-cohen>Frank<nbsp>Laloe Claude Cohen-Tannoudji,
      Bernard<nbsp>Diu. <newblock><with|font-shape|italic|Mecanique Quantique
      I>. <newblock>Hermann, Paris.

      <bibitem*|7><label|bib-connes>Alain Connes.
      <newblock><with|font-shape|italic|Noncommutative Geometry>.
      <newblock>Academic Press, San Diego, 1994.

      <bibitem*|8><label|bib-malgebras>Dov M.<nbsp>Gabbay Daniel<nbsp>Lehman,
      Kurt<nbsp>Engesser. <newblock>Algebras of measurements: the logical
      structure of quantum mechanics. <newblock><with|font-shape|italic|International
      Journal of Theoretical Physics>, 45(4).

      <bibitem*|9><label|bib-renormalization>Bertrand Delamotte. <newblock>A
      hint of renormalization. <newblock><with|font-shape|italic|Am.J.Phys.>,
      72:170--184, 2004.

      <bibitem*|10><label|bib-gausscoordinates>Albert Einstein.
      <newblock><with|font-shape|italic|Ueber die spezielle und allgemeine
      Relativitaetstheorie>, chapter<nbsp>25. <newblock>Springer-Verlag,
      Berlin.

      <bibitem*|11><label|bib-justifyrelativity>Albert Einstein.
      <newblock><with|font-shape|italic|Ueber die spezielle und allgemeine
      Relativitaetstheorie>, chapter<nbsp>23. <newblock>Springer-Verlag,
      Berlin.

      <bibitem*|12><label|bib-spacemsmt>E.<nbsp>Ogita et<nbsp>al.
      <newblock>Process in absolute distance measurement using a dye laser.
      <newblock><with|font-shape|italic|Nanotechnology>, 1995.

      <bibitem*|13><label|bib-pioneer>John<nbsp>Anderson et<nbsp>al.
      <newblock>Study of the anomalous acceleration of pioneer 10 and 11.
      <newblock><with|font-shape|italic|Phys. Rev.>, D 65.

      <bibitem*|14><label|bib-freefalling>Luca Fabbri. <newblock>Free falling
      electric charge in a static homogeneous gravitational field.
      <newblock><with|font-shape|italic|Annales de la Fondation Louis de
      Broglie>, vol 30 no 1:87--95, 2005.

      <bibitem*|15><label|bib-relaq>Charles Francis. <newblock>A relational
      quantum theory incorporating gravity.
      <newblock><with|font-shape|italic|gr-qc/0508077>, 2005.

      <bibitem*|16><label|bib-darksideofthemoon>Brian Greene.
      <newblock><with|font-shape|italic|The fabric of the cosmos>,
      chapter<nbsp>4, page<nbsp>93. <newblock>Vintage, New York, 2005.

      <bibitem*|17><label|bib-jacobssteck>Kurt Jacobs and Daniel<nbsp>A.
      Steck. <newblock>A straightforward introduction to continuous quantum
      measurement. <newblock><with|font-shape|italic|Contemporary Physics>,
      quant-ph/0611067, 2006.

      <bibitem*|18><label|bib-nonunit>Thomas<nbsp>Mehen Jaume<nbsp>Gomis.
      <newblock>Space-time noncommutative field theories and unitarity.
      <newblock><with|font-shape|italic|Nucl. Phys.>, B591:265--276, 2000.

      <bibitem*|19><label|bib-langevin>Paul Langevin. <newblock>L'evolution
      de l'espace et du temps. <newblock><with|font-shape|italic|Scientia>,
      X, 1911.

      <bibitem*|20><label|bib-cosmocst>Leonard<nbsp>Susskind Lisa<nbsp>Dyson,
      Matthew<nbsp>Kleban. <newblock>Disturbing implications of a
      cosmological constant. <newblock><with|font-shape|italic|JHEP>, 0210,
      2002.

      <bibitem*|21><label|bib-pondicherry>Ulrich Mohrhoff. <newblock>The
      pondicherry interpretation of quantum mechanics.
      <newblock><with|font-shape|italic|Am.J.Phys>, 68:728--745, 2000.

      <bibitem*|22><label|bib-scalerelat>Laurent Nottale. <newblock>The
      theory of scale relativity. <newblock><with|font-shape|italic|Int. J.
      Mod. Phys.>, A7:4899--4936, 1992.

      <bibitem*|23><label|bib-fractalspacetime>Laurent Nottale.
      <newblock><with|font-shape|italic|Fractal Space Time and Microphysics>.
      <newblock>World Scientific, Singapore, 1993.

      <bibitem*|24><label|bib-relatquantmech>Carlo Rovelli.
      <newblock>Relational quantum mechanics.
      <newblock><with|font-shape|italic|Int. J. of Theor. Phys.>, 35, 1996.

      <bibitem*|25><label|bib-quantdecoherence>Maximilian Schlosshauer.
      <newblock>Decoherence, the measurement problem, and interpretations of
      quantum mechanics. <newblock><with|font-shape|italic|Rev. Mod. Phys.>,
      76:1267--1305, 2004.

      <bibitem*|26><label|bib-liftsandtopologies>Gavriel Segre.
      <newblock>Einstein's lifts and topologies: topological investigations
      on the principle of equivalence.

      <bibitem*|27><label|bib-darkmatter>Fritz Zwicky. <newblock>Die
      rotverschiebung von extragalaktischen nebeln.
      <newblock><with|font-shape|italic|Helvetica Physica Acta>, 6:110--127,
      1933.
    </bib-list>
  </bibliography>
</body>

<\initial>
  <\collection>
    <associate|language|english>
    <associate|sfactor|5>
  </collection>
</initial>

<\references>
  <\collection>
    <associate|analogy|<tuple|4.2|17>>
    <associate|auto-1|<tuple|1|1>>
    <associate|auto-10|<tuple|6|4>>
    <associate|auto-11|<tuple|2.3|4>>
    <associate|auto-12|<tuple|7|4>>
    <associate|auto-13|<tuple|8|5>>
    <associate|auto-14|<tuple|9|5>>
    <associate|auto-15|<tuple|10|5>>
    <associate|auto-16|<tuple|11|5>>
    <associate|auto-17|<tuple|12|6>>
    <associate|auto-18|<tuple|3|6>>
    <associate|auto-19|<tuple|3.1|6>>
    <associate|auto-2|<tuple|1|2>>
    <associate|auto-20|<tuple|13|6>>
    <associate|auto-21|<tuple|14|7>>
    <associate|auto-22|<tuple|15|7>>
    <associate|auto-23|<tuple|16|8>>
    <associate|auto-24|<tuple|3.2|8>>
    <associate|auto-25|<tuple|17|8>>
    <associate|auto-26|<tuple|18|8>>
    <associate|auto-27|<tuple|1|8>>
    <associate|auto-28|<tuple|19|9>>
    <associate|auto-29|<tuple|20|9>>
    <associate|auto-3|<tuple|2|2>>
    <associate|auto-30|<tuple|2|10>>
    <associate|auto-31|<tuple|21|10>>
    <associate|auto-32|<tuple|3.3|10>>
    <associate|auto-33|<tuple|22|10>>
    <associate|auto-34|<tuple|3|11>>
    <associate|auto-35|<tuple|4|11>>
    <associate|auto-36|<tuple|23|11>>
    <associate|auto-37|<tuple|24|12>>
    <associate|auto-38|<tuple|3.4|12>>
    <associate|auto-39|<tuple|25|12>>
    <associate|auto-4|<tuple|2.1|2>>
    <associate|auto-40|<tuple|26|13>>
    <associate|auto-41|<tuple|27|13>>
    <associate|auto-42|<tuple|28|13>>
    <associate|auto-43|<tuple|29|13>>
    <associate|auto-44|<tuple|30|13>>
    <associate|auto-45|<tuple|31|14>>
    <associate|auto-46|<tuple|32|14>>
    <associate|auto-47|<tuple|33|14>>
    <associate|auto-48|<tuple|3.5|15>>
    <associate|auto-49|<tuple|34|15>>
    <associate|auto-5|<tuple|2.2|3>>
    <associate|auto-50|<tuple|35|15>>
    <associate|auto-51|<tuple|36|15>>
    <associate|auto-52|<tuple|4|16>>
    <associate|auto-53|<tuple|4.1|16>>
    <associate|auto-54|<tuple|37|16>>
    <associate|auto-55|<tuple|38|16>>
    <associate|auto-56|<tuple|39|16>>
    <associate|auto-57|<tuple|40|16>>
    <associate|auto-58|<tuple|41|16>>
    <associate|auto-59|<tuple|42|17>>
    <associate|auto-6|<tuple|2|3>>
    <associate|auto-60|<tuple|43|17>>
    <associate|auto-61|<tuple|44|17>>
    <associate|auto-62|<tuple|4.2|17>>
    <associate|auto-63|<tuple|45|18>>
    <associate|auto-64|<tuple|46|18>>
    <associate|auto-65|<tuple|47|19>>
    <associate|auto-66|<tuple|4.3|19>>
    <associate|auto-67|<tuple|48|19>>
    <associate|auto-68|<tuple|49|19>>
    <associate|auto-69|<tuple|50|20>>
    <associate|auto-7|<tuple|3|3>>
    <associate|auto-70|<tuple|51|20>>
    <associate|auto-71|<tuple|52|20>>
    <associate|auto-72|<tuple|53|20>>
    <associate|auto-73|<tuple|54|20>>
    <associate|auto-74|<tuple|55|21>>
    <associate|auto-75|<tuple|56|21>>
    <associate|auto-76|<tuple|57|21>>
    <associate|auto-77|<tuple|5|21>>
    <associate|auto-78|<tuple|58|22>>
    <associate|auto-79|<tuple|59|22>>
    <associate|auto-8|<tuple|4|4>>
    <associate|auto-80|<tuple|60|22>>
    <associate|auto-81|<tuple|<with|mode|<quote|math>|\<bullet\>>|23>>
    <associate|auto-82|<tuple|<with|mode|<quote|math>|\<bullet\>>|23>>
    <associate|auto-83|<tuple|60|22>>
    <associate|auto-84|<tuple|61|22>>
    <associate|auto-85|<tuple|62|22>>
    <associate|auto-86|<tuple|5.4|22>>
    <associate|auto-87|<tuple|63|23>>
    <associate|auto-88|<tuple|6|?>>
    <associate|auto-89|<tuple|<with|mode|<quote|math>|\<bullet\>>|?>>
    <associate|auto-9|<tuple|5|4>>
    <associate|bib-aspect|<tuple|3|23>>
    <associate|bib-basdevant|<tuple|4|23>>
    <associate|bib-cohen|<tuple|6|23>>
    <associate|bib-connes|<tuple|7|23>>
    <associate|bib-cosmocst|<tuple|20|23>>
    <associate|bib-darkmatter|<tuple|27|23>>
    <associate|bib-darksideofthemoon|<tuple|16|23>>
    <associate|bib-epr|<tuple|1|23>>
    <associate|bib-fractalspacetime|<tuple|23|23>>
    <associate|bib-freefalling|<tuple|14|23>>
    <associate|bib-gausscoordinates|<tuple|10|23>>
    <associate|bib-jacobssteck|<tuple|17|23>>
    <associate|bib-justifyrelativity|<tuple|11|23>>
    <associate|bib-langevin|<tuple|19|23>>
    <associate|bib-liftsandtopologies|<tuple|26|23>>
    <associate|bib-malgebras|<tuple|8|23>>
    <associate|bib-noncomm|<tuple|2|23>>
    <associate|bib-nonunit|<tuple|18|23>>
    <associate|bib-pioneer|<tuple|13|23>>
    <associate|bib-pondicherry|<tuple|21|23>>
    <associate|bib-qmforce|<tuple|8|21>>
    <associate|bib-quantdecoherence|<tuple|25|23>>
    <associate|bib-relaq|<tuple|15|?>>
    <associate|bib-relativity|<tuple|8|9>>
    <associate|bib-relatquantmech|<tuple|24|23>>
    <associate|bib-renormalization|<tuple|9|23>>
    <associate|bib-scalerelat|<tuple|22|23>>
    <associate|bib-spacemsmt|<tuple|12|23>>
    <associate|bib-timemsmt|<tuple|5|23>>
    <associate|eq1|<tuple|2|?>>
    <associate|eq:christoffel|<tuple|23|15>>
    <associate|eq:commut|<tuple|10|5>>
    <associate|eq:commutx|<tuple|8|5>>
    <associate|eq:commuty|<tuple|9|5>>
    <associate|eq:coordrot|<tuple|19|9>>
    <associate|eq:cossin|<tuple|20|9>>
    <associate|eq:cossin2|<tuple|21|9>>
    <associate|eq:display|<tuple|1|3>>
    <associate|eq:eigen|<tuple|13|6>>
    <associate|eq:einsteintensor|<tuple|23|13>>
    <associate|eq:elmmove|<tuple|41|?>>
    <associate|eq:expected|<tuple|28|17>>
    <associate|eq:field|<tuple|30|21>>
    <associate|eq:focused|<tuple|3|4>>
    <associate|eq:gad|<tuple|4|?>>
    <associate|eq:gauss|<tuple|22|12>>
    <associate|eq:grad|<tuple|4|4>>
    <associate|eq:graduation|<tuple|4|?>>
    <associate|eq:grcurrent|<tuple|25|16>>
    <associate|eq:grelm|<tuple|25|16>>
    <associate|eq:grelmmove|<tuple|25|14>>
    <associate|eq:grfield|<tuple|24|14>>
    <associate|eq:identity|<tuple|7|5>>
    <associate|eq:inverse|<tuple|6|5>>
    <associate|eq:linear|<tuple|11|6>>
    <associate|eq:linear2|<tuple|12|3>>
    <associate|eq:linearity|<tuple|12|6>>
    <associate|eq:linprob|<tuple|29|17>>
    <associate|eq:lorentz|<tuple|22|?>>
    <associate|eq:measurement|<tuple|13|?>>
    <associate|eq:possible|<tuple|1|?>>
    <associate|eq:predict|<tuple|5|4>>
    <associate|eq:repeat|<tuple|2|4>>
    <associate|eq:riemanntensor|<tuple|23|15>>
    <associate|eq:samespace|<tuple|26|15>>
    <associate|eq:sametime|<tuple|15|9>>
    <associate|eq:spacedef|<tuple|16|7>>
    <associate|eq:spaceidentity|<tuple|27|15>>
    <associate|eq:spacetimevol|<tuple|18|7>>
    <associate|eq:spacevol|<tuple|17|7>>
    <associate|eq:timaccel|<tuple|31|22>>
    <associate|eq:time|<tuple|14|?>>
    <associate|eq:timedef|<tuple|14|6>>
    <associate|eq:timevol|<tuple|15|7>>
    <associate|fig:commuting|<tuple|3|11>>
    <associate|fig:coordinates|<tuple|4|11>>
    <associate|fig:rotation|<tuple|1|8>>
    <associate|fig:twins|<tuple|2|10>>
    <associate|formalism|<tuple|2.2|3>>
    <associate|fundamental|<tuple|5|19>>
    <associate|generalrelativity|<tuple|3.4|12>>
    <associate|interpretation|<tuple|4.3|19>>
    <associate|noneuclidean|<tuple|3.3|10>>
    <associate|postulates|<tuple|2.1|2>>
    <associate|predictions|<tuple|4.1|16>>
    <associate|reasoning|<tuple|2.3|4>>
    <associate|scalerelativity|<tuple|3.5|15>>
    <associate|spacetime|<tuple|3.1|6>>
    <associate|specialrelativity|<tuple|3.2|8>>
  </collection>
</references>

<\auxiliary>
  <\collection>
    <\associate|bib>
      malgebras

      timemsmt

      spacemsmt

      gausscoordinates

      langevin

      justifyrelativity

      cosmocst

      relaq

      darkmatter

      pioneer

      freefalling

      liftsandtopologies

      renormalization

      scalerelat

      fractalspacetime

      basdevant

      cohen

      epr

      pondicherry

      relatquantmech

      quantdecoherence

      jacobssteck

      darksideofthemoon

      connes

      noncomm

      nonunit

      aspect
    </associate>
    <\associate|figure>
      <tuple|normal|<label|fig:rotation>Change of
      coordinates|<pageref|auto-27>>

      <tuple|normal|<label|fig:twins>Twins paradox|<pageref|auto-30>>

      <tuple|normal|<label|fig:commuting>Commuting
      displacements|<pageref|auto-34>>

      <tuple|normal|<label|fig:coordinates>Non-commuting
      displacements|<pageref|auto-35>>
    </associate>
    <\associate|toc>
      <vspace*|1fn><with|font-series|<quote|bold>|math-font-series|<quote|bold>|1<space|2spc>Introduction>
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-1><vspace|0.5fn>

      <with|par-left|<quote|6fn>|Overview of the article
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-2><vspace|0.15fn>>

      <vspace*|1fn><with|font-series|<quote|bold>|math-font-series|<quote|bold>|2<space|2spc>Defining
      Measurements> <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-3><vspace|0.5fn>

      <with|par-left|<quote|1.5fn>|2.1<space|2spc><label|postulates>Measurement
      Postulates <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-4>>

      <with|par-left|<quote|1.5fn>|2.2<space|2spc><label|formalism>Formal
      transcription <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-5>>

      <with|par-left|<quote|6fn>|Possible
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-6><vspace|0.15fn>>

      <with|par-left|<quote|6fn>|Correlating
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-7><vspace|0.15fn>>

      <with|par-left|<quote|6fn>|Repeatable
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-8><vspace|0.15fn>>

      <with|par-left|<quote|6fn>|Focused <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-9><vspace|0.15fn>>

      <with|par-left|<quote|6fn>|Quantifiable
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-10><vspace|0.15fn>>

      <with|par-left|<quote|1.5fn>|2.3<space|2spc><label|reasoning>Formal
      reasoning <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-11>>

      <with|par-left|<quote|6fn>|Repeatable Process
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-12><vspace|0.15fn>>

      <with|par-left|<quote|6fn>|Identical Processes
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-13><vspace|0.15fn>>

      <with|par-left|<quote|6fn>|Quasi-identical Processes
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-14><vspace|0.15fn>>

      <with|par-left|<quote|6fn>|Commutativity
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-15><vspace|0.15fn>>

      <with|par-left|<quote|6fn>|Linearity
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-16><vspace|0.15fn>>

      <with|par-left|<quote|6fn>|Eigenvalues
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-17><vspace|0.15fn>>

      <vspace*|1fn><with|font-series|<quote|bold>|math-font-series|<quote|bold>|3<space|2spc>General
      Relativity> <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-18><vspace|0.5fn>

      <with|par-left|<quote|1.5fn>|3.1<space|2spc><label|spacetime>Space-time
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-19>>

      <with|par-left|<quote|6fn>|Time <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-20><vspace|0.15fn>>

      <with|par-left|<quote|6fn>|Distance
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-21><vspace|0.15fn>>

      <with|par-left|<quote|6fn>|Space-time displacement
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-22><vspace|0.15fn>>

      <with|par-left|<quote|6fn>|Change of coordinates
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-23><vspace|0.15fn>>

      <with|par-left|<quote|1.5fn>|3.2<space|2spc><label|specialrelativity>Special
      Relativity <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-24>>

      <with|par-left|<quote|6fn>|Continuum vs. Discrete
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-25><vspace|0.15fn>>

      <with|par-left|<quote|6fn>|Preserving distance
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-26><vspace|0.15fn>>

      <with|par-left|<quote|6fn>|Time coordinate
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-28><vspace|0.15fn>>

      <with|par-left|<quote|6fn>|Special relativity effects
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-29><vspace|0.15fn>>

      <with|par-left|<quote|6fn>|Is light creating space-time?
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-31><vspace|0.15fn>>

      <with|par-left|<quote|1.5fn>|3.3<space|2spc><label|noneuclidean>Non-Euclidean
      Geometry <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-32>>

      <with|par-left|<quote|6fn>|Non-commutativity
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-33><vspace|0.15fn>>

      <with|par-left|<quote|6fn>|Geodesics
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-36><vspace|0.15fn>>

      <with|par-left|<quote|6fn>|Non-Euclidean geometry
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-37><vspace|0.15fn>>

      <with|par-left|<quote|1.5fn>|3.4<space|2spc><label|generalrelativity>General
      Relativity equations <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-38>>

      <with|par-left|<quote|6fn>|Mathematical tools
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-39><vspace|0.15fn>>

      <with|par-left|<quote|6fn>|Physical values
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-40><vspace|0.15fn>>

      <with|par-left|<quote|6fn>|Metric <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-41><vspace|0.15fn>>

      <with|par-left|<quote|6fn>|Existence of a Minkowskian metric
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-42><vspace|0.15fn>>

      <with|par-left|<quote|6fn>|Geodesics
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-43><vspace|0.15fn>>

      <with|par-left|<quote|6fn>|Cosmological constant
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-44><vspace|0.15fn>>

      <with|par-left|<quote|6fn>|Stress-energy tensor
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-45><vspace|0.15fn>>

      <with|par-left|<quote|6fn>|Field equation
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-46><vspace|0.15fn>>

      <with|par-left|<quote|6fn>|Electromagnetism
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-47><vspace|0.15fn>>

      <with|par-left|<quote|1.5fn>|3.5<space|2spc><label|scalerelativity>Scale
      Relativity <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-48>>

      <with|par-left|<quote|6fn>|Zoom <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-49><vspace|0.15fn>>

      <with|par-left|<quote|6fn>|Renormalization
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-50><vspace|0.15fn>>

      <with|par-left|<quote|6fn>|Scale invariance
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-51><vspace|0.15fn>>

      <vspace*|1fn><with|font-series|<quote|bold>|math-font-series|<quote|bold>|4<space|2spc>Quantum
      Mechanics> <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-52><vspace|0.5fn>

      <with|par-left|<quote|1.5fn>|4.1<space|2spc><label|predictions>Predictions
      and Probabilities <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-53>>

      <with|par-left|<quote|6fn>|N-valued measurement
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-54><vspace|0.15fn>>

      <with|par-left|<quote|6fn>|Ket representation
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-55><vspace|0.15fn>>

      <with|par-left|<quote|6fn>|Physical processes
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-56><vspace|0.15fn>>

      <with|par-left|<quote|6fn>|Indistinguishable processes
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-57><vspace|0.15fn>>

      <with|par-left|<quote|6fn>|Measurements
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-58><vspace|0.15fn>>

      <with|par-left|<quote|6fn>|Linearized operator
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-59><vspace|0.15fn>>

      <with|par-left|<quote|6fn>|Linear approximation
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-60><vspace|0.15fn>>

      <with|par-left|<quote|6fn>|Combined system
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-61><vspace|0.15fn>>

      <with|par-left|<quote|1.5fn>|4.2<space|2spc><label|analogy>Formal
      Analogy <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-62>>

      <with|par-left|<quote|6fn>|Trajectory measurement
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-63><vspace|0.15fn>>

      <with|par-left|<quote|6fn>|Normalization of the wave-function
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-64><vspace|0.15fn>>

      <with|par-left|<quote|6fn>|Planar wave-function in ``empty space''
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-65><vspace|0.15fn>>

      <with|par-left|<quote|1.5fn>|4.3<space|2spc><label|interpretation>Interpretation
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-66>>

      <with|par-left|<quote|6fn>|Disentanglement
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-67><vspace|0.15fn>>

      <with|par-left|<quote|6fn>|Real eigenvalues
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-68><vspace|0.15fn>>

      <with|par-left|<quote|6fn>|Wave-function Collapse
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-69><vspace|0.15fn>>

      <with|par-left|<quote|6fn>|Superposition
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-70><vspace|0.15fn>>

      <with|par-left|<quote|6fn>|Incomplete measurements
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-71><vspace|0.15fn>>

      <with|par-left|<quote|6fn>|Measurement limits
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-72><vspace|0.15fn>>

      <with|par-left|<quote|6fn>|Linearity
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-73><vspace|0.15fn>>

      <with|par-left|<quote|6fn>|Commutation of Observables
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-74><vspace|0.15fn>>

      <with|par-left|<quote|6fn>|Quantum space-time
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-75><vspace|0.15fn>>

      <with|par-left|<quote|6fn>|Hidden Variables
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-76><vspace|0.15fn>>

      <vspace*|1fn><with|font-series|<quote|bold>|math-font-series|<quote|bold>|5<space|2spc>Conclusion
      and Further work> <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-77><vspace|0.5fn>

      <with|par-left|<quote|6fn>|Fundamental equations
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-78><vspace|0.15fn>>

      <with|par-left|<quote|6fn>|Characteristics of the theory
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-79><vspace|0.15fn>>

      <with|par-left|<quote|6fn>|Future work
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-80><vspace|0.15fn>>

      <vspace*|1fn><with|font-series|<quote|bold>|math-font-series|<quote|bold>|Acknowledgments>
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-81><vspace|0.5fn>

      <vspace*|1fn><with|font-series|<quote|bold>|math-font-series|<quote|bold>|Bibliography>
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-82><vspace|0.5fn>
    </associate>
  </collection>
</auxiliary>