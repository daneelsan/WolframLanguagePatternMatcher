If[FindFile["tests/CustomLoad.m"] =!= $Failed,
	Get["tests/CustomLoad.m"]
]


Needs["DanielS`PatternMatcher`Utilities`TestSuiteUtilities`"]


BeginTestSection[$CurrentTestSource]


Needs["DanielS`PatternMatcher`"]
Needs["DanielS`PatternMatcher`AST`"]


Global`contextState = TestStatePush[]


Test[
	$MExprEnvironment // Head
	,
	PatternMatcherLibrary`AST`MExprEnvironment
	,
	TestID->"MExprEnvironment-20251018-A1F7N7"
]

Test[
	ConstructMExpr[f[x, y]] // MExprNormalQ
	,
	True
	,
	TestID->"MExprEnvironment-20251018-J6Q4O6"
]

(*
	Memory leaks
*)
Test[
	CheckMemoryLeak[$MExprEnvironment]
	,
	True
	,
	TestID->"MExprEnvironment-20251018-G5G4Q9"
]


TestStatePop[Global`contextState]


EndTestSection[]
