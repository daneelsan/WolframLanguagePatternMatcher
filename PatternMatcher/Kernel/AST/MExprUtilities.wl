BeginPackage["DanielS`PatternMatcher`MExprUtilities`"];


MExprIsBlank::usage =
	"MExprIsAlternatives[mexpr] returns True if mexpr is a Blank MExpr.";

MExprIsPattern::usage =
	"MExprIsAlternatives[mexpr] returns True if mexpr is a Pattern MExpr.";

MExprIsPatternTest::usage =
	"MExprIsAlternatives[mexpr] returns True if mexpr is a PatternTest MExpr.";

MExprIsCondition::usage =
	"MExprIsAlternatives[mexpr] returns True if mexpr is a Condition MExpr.";

MExprIsExcept::usage =
	"MExprIsAlternatives[mexpr] returns True if mexpr is an Except MExpr.";

MExprIsAlternatives::usage =
	"MExprIsAlternatives[mexpr] returns True if mexpr is an Alternatives MExpr.";

MExprIsRepeated::usage =
	"MExprIsRepeated[mexpr] returns True if mexpr is a Repeated MExpr.";

MExprIsRepeatedNull::usage =
	"MExprIsRepeatedNull[mexpr] returns True if mexpr is a RepeatedNull MExpr.";

MExprIsBlankSequence::usage =
	"MMExprIsBlankSequence[mexpr] returns True if mexpr is a BlankSequence MExpr.";

MExprIsBlankNullSequence::usage =
	"MExprIsBlankNullSequence[mexpr] returns True if mexpr is a BlankNullSequence MExpr.";

MExprIsPatternSequence::usage =
	"MExprIsPatternSequence[mexpr] returns True if mexpr is a PatternSequence MExpr.";

MExprIsLiteral::usage =
	"MExprIsLiteral[mexpr] returns True if mexpr is a literal MExpr.";


Begin["`Private`"];


Needs["DanielS`PatternMatcher`AST`"]


MExprIsBlank[patt_] :=
	patt["hasHead", Blank] && patt["length"] <= 1;


MExprIsPattern[patt_] :=
	patt["hasHead", Pattern] && patt["length"] == 2 && patt["part", 1]["symbolQ"];


MExprIsPatternTest[patt_] :=
	patt["hasHead", PatternTest] && patt["length"] == 2;


MExprIsCondition[patt_] :=
	patt["hasHead", Condition] && patt["length"] == 2;


MExprIsExcept[patt_] :=
	patt["hasHead", Except] && Between[patt["length"], {1, 2}];


MExprIsAlternatives[patt_] :=
	patt["hasHead", Alternatives] && patt["length"] >= 1;


MExprIsRepeated[patt_] :=
	patt["hasHead", Repeated] && Between[patt["length"], {1, 2}];


MExprIsRepeatedNull[patt_] :=
	patt["hasHead", RepeatedNull] && Between[patt["length"], {1, 2}];


MExprIsBlankSequence[patt_] :=
	patt["hasHead", BlankSequence] && patt["length"] <= 1;


MExprIsBlankNullSequence[patt_] :=
	patt["hasHead", BlankNullSequence] && patt["length"] <= 1;


MExprIsPatternSequence[patt_] :=
	patt["hasHead", PatternSequence];


MExprIsLiteral[patt_] :=
	Internal`PatternFreeQ[patt["getHeldExpr"]];


End[];


EndPackage[];
