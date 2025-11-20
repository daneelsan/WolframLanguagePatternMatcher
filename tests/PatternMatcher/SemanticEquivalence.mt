If[FindFile["tests/CustomLoad.m"] =!= $Failed,
	Get["tests/CustomLoad.m"]
]


Needs["DanielS`PatternMatcher`Utilities`TestSuiteUtilities`"]

BeginTestSection[$CurrentTestSource]


Needs["DanielS`PatternMatcher`"]


Global`contextState = TestStatePush[]


(*==============================================================================
	Semantic Equivalence Tests

	These tests verify that PatternMatcherMatchQ[patt, expr] produces the same
	results as the built-in MatchQ[expr, patt] for various pattern types.

	This is a focused test suite covering representative cases, not exhaustive
	coverage (which is in PatternMatcherExecute.mt).
==============================================================================*)


(*==============================================================================
	Literals
==============================================================================*)
Test[
	PatternMatcherMatchQ[42][42]
	,
	MatchQ[42][42]
	,
	TestID->"SemanticEquivalence-20251120-L1"
]

Test[
	PatternMatcherMatchQ[42][43]
	,
	MatchQ[42][43]
	,
	TestID->"SemanticEquivalence-20251120-L2"
]

Test[
	PatternMatcherMatchQ["hello"]["hello"]
	,
	MatchQ["hello"]["hello"]
	,
	TestID->"SemanticEquivalence-20251120-L3"
]


(*==============================================================================
	Blank (_)
==============================================================================*)
Test[
	PatternMatcherMatchQ[_][42]
	,
	MatchQ[_][42]
	,
	TestID->"SemanticEquivalence-20251120-B1"
]

Test[
	PatternMatcherMatchQ[_Integer][42]
	,
	MatchQ[_Integer][42]
	,
	TestID->"SemanticEquivalence-20251120-B2"
]

Test[
	PatternMatcherMatchQ[_Integer][42.0]
	,
	MatchQ[_Integer][42.0]
	,
	TestID->"SemanticEquivalence-20251120-B3"
]

Test[
	PatternMatcherMatchQ[_String]["hello"]
	,
	MatchQ[_String]["hello"]
	,
	TestID->"SemanticEquivalence-20251120-B4"
]


(*==============================================================================
	Named Patterns (x_)
==============================================================================*)
Test[
	PatternMatcherMatchQ[x_][42]
	,
	MatchQ[x_][42]
	,
	TestID->"SemanticEquivalence-20251120-N1"
]

Test[
	PatternMatcherMatchQ[x_Integer][42]
	,
	MatchQ[x_Integer][42]
	,
	TestID->"SemanticEquivalence-20251120-N2"
]


(*==============================================================================
	Compound Patterns
==============================================================================*)
Test[
	PatternMatcherMatchQ[f[x_, y_]][f[1, 2]]
	,
	MatchQ[f[x_, y_]][f[1, 2]]
	,
	TestID->"SemanticEquivalence-20251120-C1"
]

Test[
	PatternMatcherMatchQ[f[x_, y_]][f[1, 2, 3]]
	,
	MatchQ[f[x_, y_]][f[1, 2, 3]]
	,
	TestID->"SemanticEquivalence-20251120-C2"
]

Test[
	PatternMatcherMatchQ[{x_, y_}][{1, 2}]
	,
	MatchQ[{x_, y_}][{1, 2}]
	,
	TestID->"SemanticEquivalence-20251120-C3"
]

Test[
	PatternMatcherMatchQ[{_Integer, _String}][{42, "hello"}]
	,
	MatchQ[{_Integer, _String}][{42, "hello"}]
	,
	TestID->"SemanticEquivalence-20251120-C4"
]


(*==============================================================================
	BlankSequence (__)
==============================================================================*)
Test[
	PatternMatcherMatchQ[f[__]][f[1]]
	,
	MatchQ[f[__]][f[1]]
	,
	TestID->"SemanticEquivalence-20251120-BS1"
]

Test[
	PatternMatcherMatchQ[f[__]][f[1, 2, 3]]
	,
	MatchQ[f[__]][f[1, 2, 3]]
	,
	TestID->"SemanticEquivalence-20251120-BS2"
]

Test[
	PatternMatcherMatchQ[f[__]][f[]]
	,
	MatchQ[f[__]][f[]]
	,
	TestID->"SemanticEquivalence-20251120-BS3"
]

Test[
	PatternMatcherMatchQ[f[__Integer]][f[1, 2, 3]]
	,
	MatchQ[f[__Integer]][f[1, 2, 3]]
	,
	TestID->"SemanticEquivalence-20251120-BS4"
]

Test[
	PatternMatcherMatchQ[f[__Integer]][f[1, 2.0, 3]]
	,
	MatchQ[f[__Integer]][f[1, 2.0, 3]]
	,
	TestID->"SemanticEquivalence-20251120-BS5"
]


(*==============================================================================
	BlankNullSequence (___)
==============================================================================*)
Test[
	PatternMatcherMatchQ[f[___]][f[]]
	,
	MatchQ[f[___]][f[]]
	,
	TestID->"SemanticEquivalence-20251120-BNS1"
]

Test[
	PatternMatcherMatchQ[f[___]][f[1, 2, 3]]
	,
	MatchQ[f[___]][f[1, 2, 3]]
	,
	TestID->"SemanticEquivalence-20251120-BNS2"
]

Test[
	PatternMatcherMatchQ[f[___Integer]][f[]]
	,
	MatchQ[f[___Integer]][f[]]
	,
	TestID->"SemanticEquivalence-20251120-BNS3"
]

Test[
	PatternMatcherMatchQ[f[___Integer]][f[1, 2, 3]]
	,
	MatchQ[f[___Integer]][f[1, 2, 3]]
	,
	TestID->"SemanticEquivalence-20251120-BNS4"
]


(*==============================================================================
	Mixed Patterns with Sequences
==============================================================================*)
Test[
	PatternMatcherMatchQ[f[x_, __]][f[1, 2, 3]]
	,
	MatchQ[f[x_, __]][f[1, 2, 3]]
	,
	TestID->"SemanticEquivalence-20251120-MIX1"
]

Test[
	PatternMatcherMatchQ[f[__, y_]][f[1, 2, 3]]
	,
	MatchQ[f[__, y_]][f[1, 2, 3]]
	,
	TestID->"SemanticEquivalence-20251120-MIX2"
]

Test[
	PatternMatcherMatchQ[f[x_, ___, y_]][f[1, 2]]
	,
	MatchQ[f[x_, ___, y_]][f[1, 2]]
	,
	TestID->"SemanticEquivalence-20251120-MIX3"
]

Test[
	PatternMatcherMatchQ[f[x_, ___, y_]][f[1, 2, 3, 4]]
	,
	MatchQ[f[x_, ___, y_]][f[1, 2, 3, 4]]
	,
	TestID->"SemanticEquivalence-20251120-MIX4"
]


(*==============================================================================
	Alternatives (|)
==============================================================================*)
Test[
	PatternMatcherMatchQ[_Integer | _String][42]
	,
	MatchQ[_Integer | _String][42]
	,
	TestID->"SemanticEquivalence-20251120-ALT1"
]

Test[
	PatternMatcherMatchQ[_Integer | _String]["hello"]
	,
	MatchQ[_Integer | _String]["hello"]
	,
	TestID->"SemanticEquivalence-20251120-ALT2"
]

Test[
	PatternMatcherMatchQ[_Integer | _String][3.14]
	,
	MatchQ[_Integer | _String][3.14]
	,
	TestID->"SemanticEquivalence-20251120-ALT3"
]

Test[
	PatternMatcherMatchQ[f[x_Integer | x_Real]][f[5]]
	,
	MatchQ[f[x_Integer | x_Real]][f[5]]
	,
	TestID->"SemanticEquivalence-20251120-ALT4"
]

Test[
	PatternMatcherMatchQ[f[x_Integer | x_Real]][f[5.5]]
	,
	MatchQ[f[x_Integer | x_Real]][f[5.5]]
	,
	TestID->"SemanticEquivalence-20251120-ALT5"
]


(*==============================================================================
	Condition (/;)
==============================================================================*)
Test[
	PatternMatcherMatchQ[x_ /; x > 0][5]
	,
	MatchQ[x_ /; x > 0][5]
	,
	TestID->"SemanticEquivalence-20251120-COND1"
]

Test[
	PatternMatcherMatchQ[x_ /; x > 0][-5]
	,
	MatchQ[x_ /; x > 0][-5]
	,
	TestID->"SemanticEquivalence-20251120-COND2"
]

Test[
	PatternMatcherMatchQ[x_Integer /; EvenQ[x]][4]
	,
	MatchQ[x_Integer /; EvenQ[x]][4]
	,
	TestID->"SemanticEquivalence-20251120-COND3"
]

Test[
	PatternMatcherMatchQ[x_Integer /; EvenQ[x]][5]
	,
	MatchQ[x_Integer /; EvenQ[x]][5]
	,
	TestID->"SemanticEquivalence-20251120-COND4"
]

Test[
	PatternMatcherMatchQ[{x_, y_} /; x < y][{1, 2}]
	,
	MatchQ[{x_, y_} /; x < y][{1, 2}]
	,
	TestID->"SemanticEquivalence-20251120-COND5"
]

Test[
	PatternMatcherMatchQ[{x_, y_} /; x < y][{2, 1}]
	,
	MatchQ[{x_, y_} /; x < y][{2, 1}]
	,
	TestID->"SemanticEquivalence-20251120-COND6"
]


(*==============================================================================
	Nested Patterns
==============================================================================*)
Test[
	PatternMatcherMatchQ[f[g[x_]]][f[g[42]]]
	,
	MatchQ[f[g[x_]]][f[g[42]]]
	,
	TestID->"SemanticEquivalence-20251120-NEST1"
]

Test[
	PatternMatcherMatchQ[{f[x_], g[y_]}][{f[1], g[2]}]
	,
	MatchQ[{f[x_], g[y_]}][{f[1], g[2]}]
	,
	TestID->"SemanticEquivalence-20251120-NEST2"
]

Test[
	PatternMatcherMatchQ[{{x_}}][{{42}}]
	,
	MatchQ[{{x_}}][{{42}}]
	,
	TestID->"SemanticEquivalence-20251120-NEST3"
]


(*==============================================================================
	Complex Real-World Patterns
==============================================================================*)
Test[
	PatternMatcherMatchQ[
		{x_Integer, y_String, ___} /; x > 0
	][
		{5, "hello", 1, 2, 3}
	]
	,
	MatchQ[
		{x_Integer, y_String, ___} /; x > 0
	][
		{5, "hello", 1, 2, 3}
	]
	,
	TestID->"SemanticEquivalence-20251120-COMPLEX1"
]

Test[
	PatternMatcherMatchQ[
		f[x_Integer | x_Real, x_, ___]
	][
		f[5, 5, "extra"]
	]
	,
	MatchQ[
		f[x_Integer | x_Real, x_, ___]
	][
		f[5, 5, "extra"]
	]
	,
	TestID->"SemanticEquivalence-20251120-COMPLEX2"
]

Test[
	PatternMatcherMatchQ[
		{__, x_Integer /; x > 10}
	][
		{1, 2, 3, 15}
	]
	,
	MatchQ[
		{__, x_Integer /; x > 10}
	][
		{1, 2, 3, 15}
	]
	,
	TestID->"SemanticEquivalence-20251120-COMPLEX3"
]


TestStatePop[Global`contextState]


EndTestSection[]
