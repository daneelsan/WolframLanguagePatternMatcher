If[FindFile["tests/CustomLoad.m"] =!= $Failed,
	Get["tests/CustomLoad.m"]
]


Needs["DanielS`PatternMatcher`Utilities`TestSuiteUtilities`"]


BeginTestSection[$CurrentTestSource]


Needs["DanielS`PatternMatcher`"]
Needs["DanielS`PatternMatcher`AST`"]


Global`contextState = TestStatePush[]


(*==============================================================================
	System` symbol
==============================================================================*)

Test[
	sym = ConstructMExpr[List];
	Head[sym]
	,
	PatternMatcherLibrary`AST`MExprSymbol
	,
	TestID->"MExprSymbol-20251018-O4I4L1"
]


Test[
	sym["toBoxes", StandardForm] // Head
	,
	InterpretationBox
	,
	TestID->"MExprSymbol-20251018-A2C1H5"
]


Test[
	sym["getHeldExpr"]
	,
	HoldComplete[List]
	,
	TestID->"MExprSymbol-20251018-S6V3D7"
]


Test[
	sym["getSourceName"]
	,
	"List"
	,
	TestID->"MExprSymbol-20251018-F3C2V6"
]


Test[
	sym["getLexicalName"]
	,
	"System`List"
	,
	TestID->"MExprSymbol-20251018-Q2O6C4"
]


Test[
	sym["getContext"]
	,
	"System`"
	,
	TestID->"MExprSymbol-20251018-L2N5G9"
]


Test[
	sym["isSystemProtected"]
	,
	True
	,
	TestID->"MExprSymbol-20251018-O7O2S4"
]


Test[
	otherSym = ConstructMExpr[List];
	sym["getID"] === otherSym["getID"]
	,
	True
	,
	TestID->"MExprSymbol-20251018-I8E7R0"
]

Test[
	sym["sameQ", otherSym]
	,
	True
	,
	TestID->"MExprSymbol-20251018-G0C7G2"
]


Test[
	symHead = sym["getHead"];
	{MExprSymbolQ[symHead], symHead["getSourceName"]}
	,
	{True, "Symbol"}
	,
	TestID->"MExprSymbol-20251018-Y1O4G5"
]


(*==============================================================================
	Non-System` symbol
==============================================================================*)

Test[
	symPriv = ConstructMExpr[TestContext`x];
	symPriv // MExprSymbolQ
	,
	True
	,
	TestID->"MExprSymbol-20251018-S4F7L8"
]


Test[
	symPriv["getHeldExpr"]
	,
	HoldComplete[TestContext`x]
	,
	TestID->"MExprSymbol-20251018-R1G1B8"
]


Test[
	symPriv["getSourceName"]
	,
	"x"
	,
	TestID->"MExprSymbol-20251018-J0A1Q0"
]


Test[
	symPriv["getLexicalName"]
	,
	"TestContext`x"
	,
	TestID->"MExprSymbol-20251018-K9X2D6"
]


Test[
	symPriv["getContext"]
	,
	"TestContext`"
	,
	TestID->"MExprSymbol-20251018-I9B0M4"
]


Test[
	symPriv["isSystemProtected"]
	,
	False
	,
	TestID->"MExprSymbol-20251018-H4D3W1"
]


Test[
	symPriv2 = ConstructMExpr[TestContext`x];
	symPriv["getID"] < symPriv2["getID"]
	,
	True
	,
	TestID->"MExprSymbol-20251018-B8V9C1"
]

Test[
	symPriv["sameQ", symPriv2]
	,
	True
	,
	TestID->"MExprSymbol-20251018-K2W9E1"
]


TestStatePop[Global`contextState]


EndTestSection[]
