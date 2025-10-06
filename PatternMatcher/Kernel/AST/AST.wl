BeginPackage["DanielS`PatternMatcher`AST`"]

$MExprEnvironment::usage =
	"$MExprEnvironment is a global MExpr environment.";

ConstructMExpr::usage =
	"ConstructMExpr[expr] constructs a MExpr from an expression expr.";

CreateMExprEnvironment::usage =
	"CreateMExprEnvironment[] creates a new MExpr environment.";


Begin["`Private`"]


Needs["DanielS`PatternMatcher`Library`"]

Needs["DanielS`PatternMatcher`ErrorHandling`"] (* for CatchFailure, ThrowFailure *)


(*=============================================================================
	$MExprEnvironment
=============================================================================*)

(* Lazily initialize the global MExpr environment. *)
$MExprEnvironment := $MExprEnvironment =
	CreateMExprEnvironment[];


(*=============================================================================
	CreateMExprEnvironment
=============================================================================*)

CreateMExprEnvironment[] :=
	Module[{mExprEnv},
		InitializePatternMatcherLibrary[];
		mExprEnv = $PatternMatcherObjectFactory["InstantiateObject", "MExprEnvironment"];
		If[Head[mExprEnv] =!= PatternMatcherLibrary`AST`MExprEnvironment,
			ThrowFailure["MExprEnvironment", "Failed to create the MExprEnvironment object: `1`.", {mExprEnv}]
		];
		mExprEnv
	];


(* TODO: How to force to release an ObjectInstance manually?
DeleteObject[MExprEnvironment[...]]
*)

(*=============================================================================
	ConstructMExpr
=============================================================================*)

SetAttributes[ConstructMExpr, HoldAllComplete];

ConstructMExpr[expr_] :=
	$MExprEnvironment["constructMExpr", expr];


(*=============================================================================
	MExprLiteral
=============================================================================*)

toMExprLiteralBoxes[mexpr_, fmt_] :=
	BoxForm`ArrangeSummaryBox[
		"MExprLiteral",
		mexpr,
		"",
		{
			BoxForm`SummaryItem[{"id: ", mexpr["getID"]}],
			BoxForm`SummaryItem[{"value: ", ToString[mexpr["toString"], InputForm]}]
		},
		{},
		fmt
	];


(*=============================================================================
	MExprSymbol
=============================================================================*)

toMExprSymbolBoxes[mexpr_, fmt_] :=
	BoxForm`ArrangeSummaryBox[
		"MExprSymbol",
		mexpr,
		"",
		{
			BoxForm`SummaryItem[{"id: ", mexpr["getID"]}],
			BoxForm`SummaryItem[{"value: ", mexpr["toString"]}]
		},
		{},
		fmt
	];


(*=============================================================================
	MExprNormal
=============================================================================*)

toMExprNormalBoxes[mexpr_, fmt_] :=
	BoxForm`ArrangeSummaryBox[
		"MExprNormal",
		mexpr,
		"",
		{
			BoxForm`SummaryItem[{"id: ", mexpr["getID"]}],
			BoxForm`SummaryItem[{"value: ", mexpr["toString"]}]
		},
		{},
		fmt
	];


End[]

EndPackage[]
