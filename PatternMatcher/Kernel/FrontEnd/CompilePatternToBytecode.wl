BeginPackage["DanielS`PatternMatcher`FrontEnd`CompilePatternToBytecode`"]


Begin["`Private`"]


Needs["DanielS`PatternMatcher`BackEnd`PatternBytecode`"]
Needs["DanielS`PatternMatcher`BackEnd`VirtualMachine`"]
Needs["DanielS`PatternMatcher`ErrorHandling`"]
Needs["DanielS`PatternMatcher`"] (* for CompilePatternToBytecode *)


(*=============================================================================
	CompilePatternToBytecode
=============================================================================*)

CompilePatternToBytecode::unsup =
	"The pattern expression `1` is currently not supported.";

SyntaxInformation[CompilePatternToBytecode] =
	{"ArgumentsPattern" -> {_, _.}};

CompilePatternToBytecode[pattExpr_, vm_?PatternMatcherVirtualMachineQ] :=
	Module[{res},
		res = vm["compilePattern", pattExpr];
		If[!PatternBytecodeQ[res],
			ThrowFailure["CompilePatternToBytecode", "Failed to compile pattern expression `1` to bytecode: `2`.", {pattExpr, res}]
		];
		res
	];

CompilePatternToBytecode[pattExpr_] :=
	CompilePatternToBytecode[pattExpr, CreatePatternMatcherVirtualMachine[]];


End[]


EndPackage[]
