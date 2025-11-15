If[FindFile["tests/CustomLoad.m"] =!= $Failed,
	Get["tests/CustomLoad.m"]
]


Needs["DanielS`PatternMatcher`Utilities`TestSuiteUtilities`"]


BeginTestSection[$CurrentTestSource]


Needs["DanielS`PatternMatcher`"]
Needs["DanielS`PatternMatcher`AST`"]
Needs["DanielS`PatternMatcher`AST`MExprUtilities`"]


Global`contextState = TestStatePush[]


mexpr1 = ConstructMExpr[_?ListQ];

mexpr2 = ConstructMExpr[_List?ListQ];

mexpr3 = ConstructMExpr[x_?ListQ];

mexpr4 = ConstructMExpr[x_List?ListQ];

mexpr6 = ConstructMExpr[f[x_] /; ListQ[x]];

mexpr7 = ConstructMExpr[Except[_]];

mexpr8 = ConstructMExpr[Except[0, _Integer]];

mexpr9 = ConstructMExpr[x_Integer | x_Real];

mexpr10 = ConstructMExpr[f[_] ..];

mexpr11 = ConstructMExpr[Repeated[_Integer, {2, 3}]];

mexpr12 = ConstructMExpr[f[_] ...];

mexpr13 = ConstructMExpr[RepeatedNull[_Integer, {2, 3}]];

mexpr14 = ConstructMExpr[__];

mexpr15 = ConstructMExpr[__Real];

mexpr16 = ConstructMExpr[___];

mexpr17 = ConstructMExpr[___Real];

mexpr18 = ConstructMExpr[PatternSequence[x_, y_]];

mexpr19 = ConstructMExpr[f[x, y]];

mexprs = {
	mexpr1,
	mexpr2,
	mexpr3,
	mexpr4,
	mexpr6,
	mexpr7,
	mexpr8,
	mexpr9,
	mexpr10,
	mexpr11,
	mexpr12,
	mexpr13,
	mexpr14,
	mexpr15,
	mexpr16,
	mexpr17,
	mexpr18,
	mexpr19
};

Test[
	AllTrue[mexprs, MExprQ]
	,
	True
	,
	TestID->"MExprUtilities-20251022-D8T9C7"
]


(*
	MExprIsBlank
*)
Test[
	MExprIsBlank[mexpr1["part", 1]]
	,
	True
	,
	TestID->"MExprUtilities-20251022-J0O7J3"
]

Test[
	MExprIsBlank[mexpr2["part", 1]]
	,
	True
	,
	TestID->"MExprUtilities-20251022-B3B1Y1"
]

Test[
	MExprIsBlank[mexpr2]
	,
	False
	,
	TestID->"MExprUtilities-20251022-A2P7S6"
]


(*
	MExprIsPattern
*)
Test[
	MExprIsPattern[mexpr3["part", 1]]
	,
	True
	,
	TestID->"MExprUtilities-20251022-I5W9Y2"
]

Test[
	MExprIsPattern[mexpr4["part", 1]]
	,
	True
	,
	TestID->"MExprUtilities-20251022-D1Q4V7"
]

Test[
	MExprIsPattern[mexpr4]
	,
	False
	,
	TestID->"MExprUtilities-20251022-R6V9I2"
]


(*
	MExprIsPatternTest
*)
Test[
	MExprIsPatternTest[mexpr1]
	,
	True
	,
	TestID->"MExprUtilities-20251022-J3X0I5"
]

Test[
	MExprIsPatternTest[mexpr2]
	,
	True
	,
	TestID->"MExprUtilities-20251022-N6V0B6"
]

Test[
	MExprIsPatternTest[mexpr3]
	,
	True
	,
	TestID->"MExprUtilities-20251022-P1Q3S8"
]

Test[
	MExprIsPatternTest[mexpr4]
	,
	True
	,
	TestID->"MExprUtilities-20251022-O3P8Z6"
]

Test[
	MExprIsPatternTest[mexpr6]
	,
	False
	,
	TestID->"MExprUtilities-20251022-N7A0R4"
]


(*
	MExprIsCondition
*)
Test[
	MExprIsCondition[mexpr6]
	,
	True
	,
	TestID->"MExprUtilities-20251022-Z1L2E6"
]

Test[
	MExprIsCondition[mexpr4]
	,
	False
	,
	TestID->"MExprUtilities-20251022-O4W5R5"
]


(*
	MExprIsExcept
*)
Test[
	MExprIsExcept[mexpr7]
	,
	True
	,
	TestID->"MExprUtilities-20251022-U7D2P8"
]

Test[
	MExprIsExcept[mexpr8]
	,
	True
	,
	TestID->"MExprUtilities-20251022-V8P0I4"
]

Test[
	MExprIsExcept[mexpr6]
	,
	False
	,
	TestID->"MExprUtilities-20251022-C4N8B9"
]


(*
	MExprIsAlternatives
*)
Test[
	MExprIsAlternatives[mexpr9]
	,
	True
	,
	TestID->"MExprUtilities-20251022-J8N6H3"
]

Test[
	MExprIsAlternatives[mexpr8]
	,
	False
	,
	TestID->"MExprUtilities-20251022-C9G2D5"
]


(*
	MExprIsRepeated
*)
Test[
	MExprIsRepeated[mexpr10]
	,
	True
	,
	TestID->"MExprUtilities-20251022-A3F9W3"
]

Test[
	MExprIsRepeated[mexpr11]
	,
	True
	,
	TestID->"MExprUtilities-20251022-K3C3F7"
]

Test[
	MExprIsRepeated[mexpr9]
	,
	False
	,
	TestID->"MExprUtilities-20251022-T0W3K2"
]


(*
	MExprIsRepeatedNull
*)
Test[
	MExprIsRepeatedNull[mexpr12]
	,
	True
	,
	TestID->"MExprUtilities-20251022-S6F4Q7"
]

Test[
	MExprIsRepeatedNull[mexpr13]
	,
	True
	,
	TestID->"MExprUtilities-20251022-D1K0Y8"
]

Test[
	MExprIsRepeatedNull[mexpr11]
	,
	False
	,
	TestID->"MExprUtilities-20251022-E3V6N7"
]


(*
	MExprIsBlankSequence
*)
Test[
	MExprIsBlankSequence[mexpr14]
	,
	True
	,
	TestID->"MExprUtilities-20251022-M7L2Q9"
]

Test[
	MExprIsBlankSequence[mexpr15]
	,
	True
	,
	TestID->"MExprUtilities-20251022-I3P8P0"
]

Test[
	MExprIsBlankSequence[mexpr13]
	,
	False
	,
	TestID->"MExprUtilities-20251022-R9I4I4"
]


(*
	MExprIsBlankNullSequence
*)
Test[
	MExprIsBlankNullSequence[mexpr16]
	,
	True
	,
	TestID->"MExprUtilities-20251022-Y3Y6M5"
]

Test[
	MExprIsBlankNullSequence[mexpr17]
	,
	True
	,
	TestID->"MExprUtilities-20251022-C7E0R2"
]

Test[
	MExprIsBlankNullSequence[mexpr15]
	,
	False
	,
	TestID->"MExprUtilities-20251022-N1S1A0"
]


(*
	MExprIsPatternSequence
*)
Test[
	MExprIsPatternSequence[mexpr18]
	,
	True
	,
	TestID->"MExprUtilities-20251022-F3N9M6"
]

Test[
	MExprIsPatternSequence[mexpr17]
	,
	False
	,
	TestID->"MExprUtilities-20251022-L2V3Q6"
]


(*
	MExprIsLiteral
*)
Test[
	MExprIsLiteral[mexpr19]
	,
	True
	,
	TestID->"MExprUtilities-20251022-A2M1C9"
]

Test[
	MExprIsLiteral[mexpr18]
	,
	False
	,
	TestID->"MExprUtilities-20251022-T2P6B4"
]


TestStatePop[Global`contextState]


EndTestSection[]
