If[FindFile["tests/CustomLoad.m"] =!= $Failed,
	Get["tests/CustomLoad.m"]
]


Needs["DanielS`PatternMatcher`Utilities`TestSuiteUtilities`"]


BeginTestSection[$CurrentTestSource]


Needs["DanielS`PatternMatcher`"]
Needs["DanielS`PatternMatcher`AST`"]


Global`contextState = TestStatePush[]


(*==============================================================================
	Integers
==============================================================================*)
Test[
	lit = ConstructMExpr[1];
	Head[lit]
	,
	PatternMatcherLibrary`AST`MExprLiteral
	,
	TestID->"MExprLiteral-20251018-S4U3C6"
]


Test[
	lit["toBoxes", StandardForm] // Head
	,
	InterpretationBox
	,
	TestID->"MExprLiteral-20251018-M6J7N5"
]


Test[
	lit["getHeldExpr"]
	,
	HoldComplete[1]
	,
	TestID->"MExprLiteral-20251018-N2Q5P6"
]


Test[
	litHead = lit["head"];
	{MExprSymbolQ[litHead], litHead["getSourceName"]}
	,
	{True, "Integer"}
	,
	TestID->"MExprLiteral-20251018-H6N3Q7"
]


Test[
	lit2 = ConstructMExpr[1];
	lit["getID"] < lit2["getID"]
	,
	True
	,
	TestID->"MExprLiteral-20251018-C7H3V2"
]

Test[
	lit["sameQ", lit2]
	,
	True
	,
	TestID->"MExprLiteral-20251018-I6H2X3"
]


(*==============================================================================
	TODO: Reals
==============================================================================*)


(*==============================================================================
	TODO: Strings
==============================================================================*)


(*
	Memory leaks
*)
Test[
	CheckMemoryLeak[lit]
	,
	True
	,
	TestID->"MExprLiteral-20251018-U8I8F7"
]


TestStatePop[Global`contextState]


EndTestSection[]
