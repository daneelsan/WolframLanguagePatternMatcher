BeginPackage["DanielS`PatternMatcher`AST`"]


$MExprEnvironment::usage =
	"$MExprEnvironment is a global MExpr environment.";

ConstructMExpr::usage =
	"ConstructMExpr[expr] constructs a MExpr from an expression expr.";

CreateMExprEnvironment::usage =
	"CreateMExprEnvironment[] creates a new MExpr environment.";

MExprQ::usage =
	"MExprQ[mexpr] returns True if mexpr is a MExpr object.";

MExprSymbolQ::usage =
	"MExprSymbolQ[mexpr] returns True if mexpr is a MExpr symbol.";

MExprLiteralQ::usage =
	"MExprLiteralQ[mexpr] returns True if mexpr is a MExpr literal.";

MExprNormalQ::usage =
	"MExprNormalQ[mexpr] returns True if mexpr is a MExpr normal.";


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
	MExpr predicates
=============================================================================*)
MExprQ[mexpr_] :=
	MExprSymbolQ[mexpr] || MExprLiteralQ[mexpr] || MExprNormalQ[mexpr];


MExprSymbolQ[mexpr_?Compile`Utilities`Class`Impl`ObjectInstanceQ] :=
	Head[mexpr] === PatternMatcherLibrary`AST`MExprSymbol;

MExprSymbolQ[_] :=
	False;


MExprLiteralQ[mexpr_?Compile`Utilities`Class`Impl`ObjectInstanceQ] :=
	Head[mexpr] === PatternMatcherLibrary`AST`MExprLiteral;

MExprLiteralQ[_] :=
	False;


MExprNormalQ[mexpr_?Compile`Utilities`Class`Impl`ObjectInstanceQ] :=
	Head[mexpr] === PatternMatcherLibrary`AST`MExprNormal;

MExprNormalQ[_] :=
	False;


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
			BoxForm`SummaryItem[{"value: ", ReleaseHold[mexpr["getHeldExpr"]]}]
		},
		{},
		fmt
	];


(*=============================================================================
	MExprSymbol
=============================================================================*)

toMExprSymbolBoxes[mexpr_, fmt_] :=
	Module[{isSystemProtected},
		isSystemProtected = mexpr["isSystemProtected"];
		BoxForm`ArrangeSummaryBox[
			"MExprSymbol",
			mexpr,
			"",
			Flatten @ {
				BoxForm`SummaryItem[{"id: ", mexpr["getID"]}],
				BoxForm`SummaryItem[{"source name: ", mexpr["getSourceName"]}],
				BoxForm`SummaryItem[{"context: ", mexpr["getContext"]}]
			},
			{
				BoxForm`SummaryItem[{"cached: ", isSystemProtected}],
				BoxForm`SummaryItem[{"lexical name: ", mexpr["getLexicalName"]}]
			},
			fmt
		]
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
			BoxForm`SummaryItem[{"held expr: ", mexpr["getHeldExpr"]}]
		},
		{},
		fmt
	];


End[]

EndPackage[]
