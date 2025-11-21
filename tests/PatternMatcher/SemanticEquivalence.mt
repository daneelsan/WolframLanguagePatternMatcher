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
	PatternMatcherReplace[x_ :> f[x]][42]
	,
	Replace[x_ :> f[x]][42]
	,
	TestID->"SemanticEquivalence-20251120-N1"
]

Test[
	PatternMatcherReplace[x_Integer :> f[x]][42]
	,
	Replace[x_Integer :> f[x]][42]
	,
	TestID->"SemanticEquivalence-20251120-N2"
]


(*==============================================================================
	Compound Patterns
==============================================================================*)
Test[
	PatternMatcherReplace[f[x_, y_] :> {x, y}][f[1, 2]]
	,
	Replace[f[x_, y_] :> {x, y}][f[1, 2]]
	,
	TestID->"SemanticEquivalence-20251120-C1"
]

Test[
	PatternMatcherReplace[f[x_, y_] :> {x, y}][f[1, 2, 3]]
	,
	Replace[f[x_, y_] :> {x, y}][f[1, 2, 3]]
	,
	TestID->"SemanticEquivalence-20251120-C2"
]

Test[
	PatternMatcherReplace[{x_, y_} :> f[x, y]][{1, 2}]
	,
	Replace[{x_, y_} :> f[x, y]][{1, 2}]
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
	PatternMatcherReplace[f[x_, __] :> g[x]][f[1, 2, 3]]
	,
	Replace[f[x_, __] :> g[x]][f[1, 2, 3]]
	,
	TestID->"SemanticEquivalence-20251120-MIX1"
]

Test[
	PatternMatcherReplace[f[__, y_] :> g[y]][f[1, 2, 3]]
	,
	Replace[f[__, y_] :> g[y]][f[1, 2, 3]]
	,
	TestID->"SemanticEquivalence-20251120-MIX2"
]

Test[
	PatternMatcherReplace[f[x_, ___, y_] :> g[x, y]][f[1, 2]]
	,
	Replace[f[x_, ___, y_] :> g[x, y]][f[1, 2]]
	,
	TestID->"SemanticEquivalence-20251120-MIX3"
]

Test[
	PatternMatcherReplace[f[x_, ___, y_] :> g[x, y]][f[1, 2, 3, 4]]
	,
	Replace[f[x_, ___, y_] :> g[x, y]][f[1, 2, 3, 4]]
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
	PatternMatcherReplace[f[x_Integer | x_Real] :> {x}][f[5]]
	,
	Replace[f[x_Integer | x_Real] :> {x}][f[5]]
	,
	TestID->"SemanticEquivalence-20251120-ALT4"
]

Test[
	PatternMatcherReplace[f[x_Integer | x_Real] :> {x}][f[5.5]]
	,
	Replace[f[x_Integer | x_Real] :> {x}][f[5.5]]
	,
	TestID->"SemanticEquivalence-20251120-ALT5"
]

Test[
	PatternMatcherReplace[f[x_Integer | x_Real, x_] :> {x}][f[2, 2]]
	,
	Replace[f[x_Integer | x_Real, x_] :> {x}][f[2, 2]]
	,
	TestID->"SemanticEquivalence-20251120-ALT6"
]

Test[
	PatternMatcherReplace[f[x_Integer | x_Real, x_] :> {x}][f[2.5, 2.5]]
	,
	Replace[f[x_Integer | x_Real, x_] :> {x}][f[2.5, 2.5]]
	,
	TestID->"SemanticEquivalence-20251120-ALT7"
]

Test[
	PatternMatcherReplace[f[x_Integer | x_Real, x_] :> {x}][f[2, 2.5]]
	,
	Replace[f[x_Integer | x_Real, x_] :> {x}][f[2, 2.5]]
	,
	TestID->"SemanticEquivalence-20251120-ALT8"
]

Test[
	PatternMatcherReplace[f[x_Integer | y_Real, x_] :> {x, y}][f[2, 2]]
	,
	Replace[f[x_Integer | y_Real, x_] :> {x, y}][f[2, 2]]
	,
	TestID->"SemanticEquivalence-20251120-ALT9"
]

Test[
	PatternMatcherReplace[f[x_Integer | y_Real, x_] :> {x, y}][f[2.5, 2.5]]
	,
	Replace[f[x_Integer | y_Real, x_] :> {x, y}][f[2.5, 2.5]]
	,
	TestID->"SemanticEquivalence-20251120-ALT10"
]

Test[
	PatternMatcherReplace[f[x_Integer | y_Real, x_] :> {x, y}][f[2.5, 2]]
	,
	Replace[f[x_Integer | y_Real, x_] :> {x, y}][f[2.5, 2]]
	,
	TestID->"SemanticEquivalence-20251120-ALT11"
]


(*==============================================================================
	Condition (/;)
==============================================================================*)
Test[
	PatternMatcherReplace[x_ /; x > 0 :> f[x]][5]
	,
	Replace[x_ /; x > 0 :> f[x]][5]
	,
	TestID->"SemanticEquivalence-20251120-COND1"
]

Test[
	PatternMatcherReplace[x_ /; x > 0 :> f[x]][-5]
	,
	Replace[x_ /; x > 0 :> f[x]][-5]
	,
	TestID->"SemanticEquivalence-20251120-COND2"
]

Test[
	PatternMatcherReplace[x_Integer /; EvenQ[x] :> g[x]][4]
	,
	Replace[x_Integer /; EvenQ[x] :> g[x]][4]
	,
	TestID->"SemanticEquivalence-20251120-COND3"
]

Test[
	PatternMatcherReplace[x_Integer /; EvenQ[x] :> g[x]][5]
	,
	Replace[x_Integer /; EvenQ[x] :> g[x]][5]
	,
	TestID->"SemanticEquivalence-20251120-COND4"
]

Test[
	PatternMatcherReplace[{x_, y_} /; x < y :> f[x, y]][{1, 2}]
	,
	Replace[{x_, y_} /; x < y :> f[x, y]][{1, 2}]
	,
	TestID->"SemanticEquivalence-20251120-COND5"
]

Test[
	PatternMatcherReplace[{x_, y_} /; x < y :> f[x, y]][{2, 1}]
	,
	Replace[{x_, y_} /; x < y :> f[x, y]][{2, 1}]
	,
	TestID->"SemanticEquivalence-20251120-COND6"
]


(*==============================================================================
	Nested Patterns
==============================================================================*)
Test[
	PatternMatcherReplace[f[g[x_]] :> {x}][f[g[42]]]
	,
	Replace[f[g[x_]] :> {x}][f[g[42]]]
	,
	TestID->"SemanticEquivalence-20251120-NEST1"
]

Test[
	PatternMatcherReplace[{f[x_], g[y_]} :> h[x, y]][{f[1], g[2]}]
	,
	Replace[{f[x_], g[y_]} :> h[x, y]][{f[1], g[2]}]
	,
	TestID->"SemanticEquivalence-20251120-NEST2"
]

Test[
	PatternMatcherReplace[{{x_}} :> f[x]][{{42}}]
	,
	Replace[{{x_}} :> f[x]][{{42}}]
	,
	TestID->"SemanticEquivalence-20251120-NEST3"
]


(*==============================================================================
	Complex Real-World Patterns
==============================================================================*)
Test[
	PatternMatcherReplace[
		{x_Integer, y_String, ___} /; x > 0 :> f[x, y]
	][
		{5, "hello", 1, 2, 3}
	]
	,
	Replace[
		{x_Integer, y_String, ___} /; x > 0 :> f[x, y]
	][
		{5, "hello", 1, 2, 3}
	]
	,
	TestID->"SemanticEquivalence-20251120-COMPLEX1"
]

Test[
	PatternMatcherReplace[
		f[x_Integer | x_Real, x_, ___] :> {x}
	][
		f[5, 5, "extra"]
	]
	,
	Replace[
		f[x_Integer | x_Real, x_, ___] :> {x}
	][
		f[5, 5, "extra"]
	]
	,
	TestID->"SemanticEquivalence-20251120-COMPLEX2"
]

Test[
	PatternMatcherReplace[
		{__, x_Integer /; x > 10} :> f[x]
	][
		{1, 2, 3, 15}
	]
	,
	Replace[
		{__, x_Integer /; x > 10} :> f[x]
	][
		{1, 2, 3, 15}
	]
	,
	TestID->"SemanticEquivalence-20251120-COMPLEX3"
]


(*==============================================================================
	Alternative Variable Binding Tests
==============================================================================*)

(* Test that unbound variables are dropped in replacement, not left as symbols *)
VerificationTest[
	PatternMatcherReplace[f[x_Integer | y_Real, x_] :> g[x, y]][f[2, 2]]
	,
	Replace[f[x_Integer | y_Real, x_] :> g[x, y]][f[2, 2]]
	,
	TestID->"SemanticEquivalence-20251120-ALT-UNBOUND1"
]

VerificationTest[
	PatternMatcherReplace[f[x_Integer | y_Real, x_] :> g[x, y]][f[2.5, 2.5]]
	,
	Replace[f[x_Integer | y_Real, x_] :> g[x, y]][f[2.5, 2.5]]
	,
	TestID->"SemanticEquivalence-20251120-ALT-UNBOUND2"
]

(* Test alternatives with repeated variables *)
VerificationTest[
	PatternMatcherReplace[f[x_Integer | x_Real, x_] :> g[x]][f[2, 2]]
	,
	Replace[f[x_Integer | x_Real, x_] :> g[x]][f[2, 2]]
	,
	TestID->"SemanticEquivalence-20251120-ALT-REPEATED1"
]

VerificationTest[
	PatternMatcherReplace[f[x_Integer | x_Real, x_] :> g[x]][f[2.5, 2.5]]
	,
	Replace[f[x_Integer | x_Real, x_] :> g[x]][f[2.5, 2.5]]
	,
	TestID->"SemanticEquivalence-20251120-ALT-REPEATED2"
]

(* Test that unbound variables in lists remain as symbols (different behavior than functions) *)
VerificationTest[
	PatternMatcherReplace[f[x_Integer | y_Real, x_] :> {x, y}][f[2, 2]]
	,
	Replace[f[x_Integer | y_Real, x_] :> {x, y}][f[2, 2]]
	,
	TestID->"SemanticEquivalence-20251120-ALT-LIST1"
]

VerificationTest[
	PatternMatcherReplace[f[x_Integer | y_Real, x_] :> {x, y}][f[2.5, 2.5]]
	,
	Replace[f[x_Integer | y_Real, x_] :> {x, y}][f[2.5, 2.5]]
	,
	TestID->"SemanticEquivalence-20251120-ALT-LIST2"
]

(* Test complex alternatives with multiple variables *)
VerificationTest[
	PatternMatcherMatchQ[f[2, 2], f[x_Integer | y_Real, x_]]
	,
	MatchQ[f[2, 2], f[x_Integer | y_Real, x_]]
	,
	TestID->"SemanticEquivalence-20251120-ALT-COMPLEX1"
]

VerificationTest[
	PatternMatcherMatchQ[f[2.5, 2.5], f[x_Integer | y_Real, x_]]
	,
	MatchQ[f[2.5, 2.5], f[x_Integer | y_Real, x_]]
	,
	TestID->"SemanticEquivalence-20251120-ALT-COMPLEX2"
]

VerificationTest[
	PatternMatcherMatchQ[f[2, 2.5], f[x_Integer | y_Real, x_]]
	,
	MatchQ[f[2, 2.5], f[x_Integer | y_Real, x_]]
	,
	TestID->"SemanticEquivalence-20251120-ALT-COMPLEX3"
]

(* Test alternatives with conditions *)
VerificationTest[
	PatternMatcherMatchQ[f[3, 3], f[x_Integer /; x > 0 | y_Real /; y < 0, x_]]
	,
	MatchQ[f[3, 3], f[x_Integer /; x > 0 | y_Real /; y < 0, x_]]
	,
	TestID->"SemanticEquivalence-20251120-ALT-COND1"
]

VerificationTest[
	PatternMatcherMatchQ[f[-2.5, -2.5], f[x_Integer /; x > 0 | y_Real /; y < 0, x_]]
	,
	MatchQ[f[-2.5, -2.5], f[x_Integer /; x > 0 | y_Real /; y < 0, x_]]
	,
	TestID->"SemanticEquivalence-20251120-ALT-COND2"
]

(* Test nested alternatives *)
VerificationTest[
	PatternMatcherMatchQ[g[f[2]], g[x_Integer | f[y_Integer]]]
	,
	MatchQ[g[f[2]], g[x_Integer | f[y_Integer]]]
	,
	TestID->"SemanticEquivalence-20251120-ALT-NESTED1"
]

VerificationTest[
	PatternMatcherMatchQ[g[5], g[x_Integer | f[y_Integer]]]
	,
	MatchQ[g[5], g[x_Integer | f[y_Integer]]]
	,
	TestID->"SemanticEquivalence-20251120-ALT-NESTED2"
]

(* Test alternatives with sequences *)
VerificationTest[
	PatternMatcherMatchQ[f[1, 2, 3], f[x_Integer, ___Integer | y_String, z_]]
	,
	MatchQ[f[1, 2, 3], f[x_Integer, ___Integer | y_String, z_]]
	,
	TestID->"SemanticEquivalence-20251120-ALT-SEQ1"
]

VerificationTest[
	PatternMatcherMatchQ[f[1, "hello", 3], f[x_Integer, ___Integer | y_String, z_]]
	,
	MatchQ[f[1, "hello", 3], f[x_Integer, ___Integer | y_String, z_]]
	,
	TestID->"SemanticEquivalence-20251120-ALT-SEQ2"
]

(* Test alternatives with no matches *)
VerificationTest[
	PatternMatcherMatchQ[f["hello", 2], f[x_Integer | y_Real, x_]]
	,
	MatchQ[f["hello", 2], f[x_Integer | y_Real, x_]]
	,
	TestID->"SemanticEquivalence-20251120-ALT-NOMATCH1"
]

VerificationTest[
	PatternMatcherMatchQ[f[2, "hello"], f[x_Integer | y_Real, x_]]
	,
	MatchQ[f[2, "hello"], f[x_Integer | y_Real, x_]]
	,
	TestID->"SemanticEquivalence-20251120-ALT-NOMATCH2"
]

(* Test three-way alternatives *)
VerificationTest[
	PatternMatcherMatchQ[f[2], f[x_Integer | y_Real | z_String]]
	,
	MatchQ[f[2], f[x_Integer | y_Real | z_String]]
	,
	TestID->"SemanticEquivalence-20251120-ALT-THREE1"
]

VerificationTest[
	PatternMatcherMatchQ[f[2.5], f[x_Integer | y_Real | z_String]]
	,
	MatchQ[f[2.5], f[x_Integer | y_Real | z_String]]
	,
	TestID->"SemanticEquivalence-20251120-ALT-THREE2"
]

VerificationTest[
	PatternMatcherMatchQ[f["hello"], f[x_Integer | y_Real | z_String]]
	,
	MatchQ[f["hello"], f[x_Integer | y_Real | z_String]]
	,
	TestID->"SemanticEquivalence-20251120-ALT-THREE3"
]

(* Test replacement with non-pattern symbols *)
VerificationTest[
	PatternMatcherReplace[f[x_Integer] :> g[x, nonPatternSymbol]][f[5]]
	,
	Replace[f[x_Integer] :> g[x, nonPatternSymbol]][f[5]]
	,
	TestID->"SemanticEquivalence-20251120-NONPATTERN1"
]

(* Test empty sequence type matching (regression test for MATCH_SEQ_HEADS bug) *)
VerificationTest[
	PatternMatcherMatchQ[f[], f[___Integer]]
	,
	MatchQ[f[], f[___Integer]]
	,
	TestID->"SemanticEquivalence-20251120-EMPTYSEQ1"
]

VerificationTest[
	PatternMatcherMatchQ[f[], f[__Integer]]
	,
	MatchQ[f[], f[__Integer]]
	,
	TestID->"SemanticEquivalence-20251120-EMPTYSEQ2"
]

(* Test LOAD_VAR synchronization (regression test for alternative variable tracking) *)
VerificationTest[
	PatternMatcherMatchQ[f[5, 5, "extra"], f[x_Integer | x_Real, x_, ___]]
	,
	MatchQ[f[5, 5, "extra"], f[x_Integer | x_Real, x_, ___]]
	,
	TestID->"SemanticEquivalence-20251120-LOADVAR1"
]

VerificationTest[
	PatternMatcherMatchQ[f[5.5, 5.5, "extra"], f[x_Integer | x_Real, x_, ___]]
	,
	MatchQ[f[5.5, 5.5, "extra"], f[x_Integer | x_Real, x_, ___]]
	,
	TestID->"SemanticEquivalence-20251120-LOADVAR2"
]


TestStatePop[Global`contextState]


EndTestSection[]
