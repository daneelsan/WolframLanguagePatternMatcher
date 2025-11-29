BeginPackage["DanielS`PatternMatcher`FrontEnd`PatternMatcherReplace`"]


Begin["`Private`"]


Needs["DanielS`PatternMatcher`BackEnd`VirtualMachine`"]
Needs["DanielS`PatternMatcher`ErrorHandling`"]
Needs["DanielS`PatternMatcher`AST`"]
Needs["DanielS`PatternMatcher`"] (* for PatternMatcherReplace *)


SyntaxInformation[PatternMatcherReplace] =
	{"ArgumentsPattern" -> {_, _.}};

Options[PatternMatcherReplace] = {
};

PatternMatcherReplace[arg1_, arg2_] :=
	CatchFailure[iPatternMatcherReplace[arg1, arg2]];

PatternMatcherReplace[arg_][expr_] :=
	CatchFailure[iPatternMatcherReplace[arg][expr]];

PatternMatcherReplace[arg1_, arg2_, arg3__] :=
	CatchFailure @ ThrowFailure[
		"PatternMatcherReplace",
		"Unexpected arguments: `1`.",
		HoldCompleteForm[PatternMatcherReplace[arg1, arg2, arg3]]
	];


iPatternMatcherReplace[(r : Rule | RuleDelayed)[vm_?PatternMatcherVirtualMachineQ, rhs_]][expr_] :=
	Module[{res, binds, bindsMExpr, patternVars, resMExpr},
		If[!vm["isInitialized"],
			ThrowFailure[
				"PatternMatcherReplace",
				"Cannot run the pattern matcher because the virtual machine is not initialized.", {vm}
			]
		];
		vm["reset"];
		res = vm["match", expr];
		If[res,
			binds = vm["getResultBindings"];
			patternVars = extractPatternVariables[vm["getBytecode"]["getPattern"]["toHeldExpr"]];
			bindsMExpr = getBindingsMExpr[patternVars, binds];
			resMExpr = replaceBindingsInMExpr[rhs, bindsMExpr];
			resMExpr["toExpr"]
			,
			expr
		]
	];

iPatternMatcherReplace[(r : Rule | RuleDelayed)[lhs_, rhs_]][expr_] :=
	Module[{vm},
		vm = CreatePatternMatcherVirtualMachine[lhs];
		iPatternMatcherReplace[r[vm, rhs]][expr]
	];

iPatternMatcherReplace[arg1_, arg2_] :=
	iPatternMatcherReplace[arg2][arg1];


(* Extract all pattern variable names from a pattern expression *)
extractPatternVariables[patt_] :=
	DeleteDuplicates @
	Cases[patt, HoldPattern[Verbatim[Pattern][s_Symbol, _]] :> Context[s] <> SymbolName[s], {0, Infinity}];


getBindingsMExpr[patternVars_List, bindings_Association] :=
	KeyValueMap[
		Function[{k, v},
			With[{s = Symbol[k]}, ConstructMExpr[s = v]]
		],
		ConstructMExpr /@ Join[
			AssociationThread[patternVars, Unevaluated[Sequence[]]],
			bindings
		]
	];


SetAttributes[replaceBindingsInMExpr, HoldFirst];
replaceBindingsInMExpr[rhs_, bindsMExpr_List] :=
	ConstructMExpr[With[bindsMExpr, rhs]];


End[]


EndPackage[]
