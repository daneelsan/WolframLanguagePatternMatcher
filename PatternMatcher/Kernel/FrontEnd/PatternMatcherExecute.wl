BeginPackage["DanielS`PatternMatcher`FrontEnd`PatternMatcherExecute`"]


Begin["`Private`"]


Needs["DanielS`PatternMatcher`BackEnd`VirtualMachine`"]
Needs["DanielS`PatternMatcher`ErrorHandling`"]
Needs["DanielS`PatternMatcher`"] (* for PatternMatcherExecute *)


(*=============================================================================
	CompilePatternToBytecode
=============================================================================*)

Options[PatternMatcherExecute] = {
};

PatternMatcherExecute[args___] :=
	CatchFailure[iPatternMatcherExecute[args]];


iPatternMatcherExecute[vm_?PatternMatcherVirtualMachineQ, expr_] :=
	Module[{res},
		If[!vm["isInitialized"],
			ThrowFailure[
				"PatternMatcherExecute",
				"Cannot run the pattern matcher because the virtual machine is not initialized.", {vm}
			]
		];
		vm["reset"];
		res = vm["match", expr];
		(* TODO: Add bound expressions, stats, etc. *)
		<|
			"Result" -> res,
			"CyclesExecuted" -> vm["getCycles"]
		|>
	];

iPatternMatcherExecute[patternExpr_, expr_] :=
	Module[{vm},
		vm = CreatePatternMatcherVirtualMachine[patternExpr];
		iPatternMatcherExecute[vm, expr]
	];

iPatternMatcherExecute[args___] :=
	ThrowFailure[
		"PatternMatcherExecute",
		"Unexpected arguments: `1`.",
		HoldCompleteForm[PatternMatcherExecute[args]]
	];


End[]


EndPackage[]
