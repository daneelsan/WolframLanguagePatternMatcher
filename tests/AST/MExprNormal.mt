If[FindFile["tests/CustomLoad.m"] =!= $Failed,
	Get["tests/CustomLoad.m"]
]


Needs["DanielS`PatternMatcher`Utilities`TestSuiteUtilities`"]


BeginTestSection[$CurrentTestSource]


Needs["DanielS`PatternMatcher`"]
Needs["DanielS`PatternMatcher`AST`"]


Global`contextState = TestStatePush[]


Test[
	normMExpr = ConstructMExpr[f[g][x, 1, "3"]];
	Head[normMExpr]
	,
	PatternMatcherLibrary`AST`MExprNormal
	,
	TestID->"MExprNormal-20251018-Y5H9T9"
]


Test[
	{normMExpr["literalQ"], normMExpr["normalQ"], normMExpr["symbolQ"]}
	,
	{False, True, False}
	,
	TestID->"MExprNormal-20251022-B9K1K7"
]


Test[
	normMExpr["toBoxes", StandardForm] // Head
	,
	InterpretationBox
	,
	TestID->"MExprNormal-20251018-H6U0O0"
]


Test[
	normMExpr["toHeldExpr"]
	,
	HoldComplete[f[g][x, 1, "3"]]
	,
	TestID->"MExprNormal-20251022-O7L2M1"
]


Test[
	normMExpr["length"]
	,
	3
	,
	TestID->"MExprNormal-20251018-X8L9D1"
]


Test[
	headMExpr = normMExpr["head"];
	{MExprNormalQ[headMExpr], headMExpr["toHeldExpr"]}
	,
	{True, HoldComplete[f[g]]}
	,
	TestID->"MExprNormal-20251018-E7M8T3"
]


TestMatch[
	normMExpr["arguments"]
	,
	{_?MExprSymbolQ, _?MExprLiteralQ, _?MExprLiteralQ}
	,
	TestID->"MExprNormal-20251022-J7L1R7"
]


Test[
	part1MExpr = normMExpr["part", 1];
	{MExprSymbolQ[part1MExpr], part1MExpr["getLexicalName"]}
	,
	{True, "TestContext`x"}
	,
	TestID->"MExprNormal-20251018-D6U7O1"
]


Test[
	part2MExpr = normMExpr["part", 2];
	{MExprLiteralQ[part2MExpr], part2MExpr["toHeldExpr"]}
	,
	{True, HoldComplete[1]}
	,
	TestID->"MExprNormal-20251018-P1N1C0"
]


Test[
	part3MExpr = normMExpr["part", 3];
	{MExprLiteralQ[part3MExpr], part3MExpr["toHeldExpr"]}
	,
	{True, HoldComplete["3"]}
	,
	TestID->"MExprNormal-20251018-W2F6R1"
]


Test[
	normMExpr2 = With[{e = normMExpr["toExpr"]}, ConstructMExpr[e]];
	normMExpr["sameQ", normMExpr2]
	,
	True
	,
	TestID->"MExprNormal-20251022-B7B4Q0"
]

Test[
	normMExpr2["part", 1]["updateName", "h"];
	normMExpr["sameQ", normMExpr2]
	,
	False
	,
	TestID->"MExprNormal-20251022-T4D1N4"
]


TestStatePop[Global`contextState]


EndTestSection[]
