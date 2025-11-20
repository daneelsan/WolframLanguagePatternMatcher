BeginPackage["DanielS`PatternMatcher`FrontEnd`PatternMatcherMatchQ`"]


Begin["`Private`"]


Needs["DanielS`PatternMatcher`BackEnd`VirtualMachine`"]
Needs["DanielS`PatternMatcher`ErrorHandling`"]
Needs["DanielS`PatternMatcher`"] (* for PatternMatcherMatchQ *)


SyntaxInformation[PatternMatcherMatchQ] =
	{"ArgumentsPattern" -> {_, _}};

Options[PatternMatcherMatchQ] = {
};

PatternMatcherMatchQ[args___] :=
	CatchFailure[iPatternMatcherMatchQ[args]];


iPatternMatcherMatchQ[vm_?PatternMatcherVirtualMachineQ, expr_] :=
	With[{},
		If[!vm["isInitialized"],
			ThrowFailure[
				"PatternMatcherMatchQ",
				"Cannot run the pattern matcher because the virtual machine is not initialized.", {vm}
			]
		];
		vm["reset"];
		vm["match", expr]
	];

iPatternMatcherMatchQ[patternExpr_, expr_] :=
	Module[{vm},
		vm = CreatePatternMatcherVirtualMachine[patternExpr];
		iPatternMatcherMatchQ[vm, expr]
	];

iPatternMatcherMatchQ[args___] :=
	ThrowFailure[
		"PatternMatcherMatchQ",
		"Unexpected arguments: `1`.",
		HoldCompleteForm[PatternMatcherMatchQ[args]]
	];


End[]


EndPackage[]
