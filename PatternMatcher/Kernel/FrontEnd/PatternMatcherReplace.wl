BeginPackage["DanielS`PatternMatcher`FrontEnd`PatternMatcherReplace`"]


Begin["`Private`"]


Needs["DanielS`PatternMatcher`BackEnd`VirtualMachine`"]
Needs["DanielS`PatternMatcher`ErrorHandling`"]
Needs["DanielS`PatternMatcher`AST`"]
Needs["DanielS`PatternMatcher`"] (* for PatternMatcherReplace *)


SyntaxInformation[PatternMatcherReplace] =
	{"ArgumentsPattern" -> {_, _}};

Options[PatternMatcherReplace] = {
};

PatternMatcherReplace[args___] :=
	CatchFailure[iPatternMatcherReplace[args]];


iPatternMatcherReplace[(r : Rule | RuleDelayed)[vm_?PatternMatcherVirtualMachineQ, rhs_], expr_] :=
	Module[{res, binds, bindsMExpr, resMExpr},
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
			bindsMExpr = getBindingsMExpr[binds];
			resMExpr = replaceBindingsInMExpr[rhs, bindsMExpr];
			resMExpr["toExpr"]
			,
			expr  (* No match - return unchanged *)
		]
	];

iPatternMatcherReplace[(r : Rule | RuleDelayed)[lhs_, rhs_], expr_] :=
	Module[{vm},
		vm = CreatePatternMatcherVirtualMachine[lhs];
		iPatternMatcherReplace[r[vm, rhs], expr]
	];

iPatternMatcherReplace[args___] :=
	ThrowFailure[
		"PatternMatcherReplace",
		"Unexpected arguments: `1`.",
		HoldCompleteForm[PatternMatcherReplace[args]]
	];


getBindingsMExpr[bindings_Association] :=
	KeyValueMap[
		Function[{k, v},
			With[{s = Symbol[k]}, ConstructMExpr[s = v]]
		],
		ConstructMExpr /@ bindings
	];


SetAttributes[replaceBindingsInMExpr, HoldFirst];
replaceBindingsInMExpr[rhs_, bindsMExpr_List] :=
	ConstructMExpr[With[bindsMExpr, rhs]]


End[]


EndPackage[]
