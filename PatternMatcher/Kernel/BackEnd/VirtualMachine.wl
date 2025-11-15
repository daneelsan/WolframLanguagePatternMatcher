BeginPackage["DanielS`PatternMatcher`BackEnd`VirtualMachine`"]


PatternMatcherVirtualMachineQ::usage =
	"PatternMatcherVirtualMachineQ[x] returns True if x is a valid PatternMatcherVirtualMachine[...] object, and False otherwise.";


Begin["`Private`"]


Needs["DanielS`PatternMatcher`Library`"]
Needs["DanielS`PatternMatcher`"]


(*=============================================================================
	ResetPatternMatcherVirtualMachine
=============================================================================*)
ResetPatternMatcherVirtualMachine[vm_?PatternMatcherVirtualMachineQ] :=
	Module[{},
		vm["reset"];
		vm
	];


(*=============================================================================
	CreatePatternMatcherVirtualMachine
=============================================================================*)

CreatePatternMatcherVirtualMachine[] :=
	Module[{vm},
		InitializePatternMatcherLibrary[];
		vm = $PatternMatcherObjectFactory["InstantiateObject", "VirtualMachine"];
		If[!PatternMatcherVirtualMachineQ[vm],
			ThrowFailure["VirtualMachine", "Failed to create the VirtualMachine object: `1`.", {vm}]
		];
		vm
	];

CreatePatternMatcherVirtualMachine[pattExpr_] :=
	Module[{vm, patt},
		vm = CreatePatternMatcherVirtualMachine[];
		patt = CompilePatternToBytecode[pattExpr, vm];
		vm["initialize", patt];
		vm
	];


CreatePatternMatcherVirtualMachine[__] :=
	$Failed;


(*=============================================================================
	PatternMatcherVirtualMachineQ
=============================================================================*)

PatternMatcherVirtualMachineQ[x_?Compile`Utilities`Class`Impl`ObjectInstanceQ] :=
	x["_class"] === "PatternMatcherLibrary`VM`VirtualMachine";

PatternMatcherVirtualMachineQ[_] :=
	False;


(*=============================================================================
	PatternMatcherVirtualMachine
=============================================================================*)

toBoxes[obj_, fmt_] :=
	Module[{initialized, halted, cycles, pc},
		initialized = obj["isInitialized"];
		halted = obj["isHalted"];
		cycles = obj["getCycles"];
		pc = obj["getPC"];
		BoxForm`ArrangeSummaryBox[
			"PatternMatcherVirtualMachine",
			obj,
			None,
			{
				BoxForm`SummaryItem[{"Initialized: ", obj["isInitialized"]}],
				BoxForm`SummaryItem[{"Halted: ", halted}],
				BoxForm`SummaryItem[{"Program counter: ", pc}],
				BoxForm`SummaryItem[{"Cycles: ", cycles}]
			},
			{
				If[initialized,
					BoxForm`SummaryItem[{"Current bytecode: ", obj["getBytecode"]}],
					Nothing
				]
				(*BoxForm`SummaryItem[{"Stack: ", stack}],
				BoxForm`SummaryItem[{"Pattern bytecode: ", bytecode}],
				BoxForm`SummaryItem[{"Bound variables: ", boundVars}],*)
			}, 
			fmt
		]
	];


(*=============================================================================
	PatternMatcherMatch
=============================================================================*)

(*=============================================================================
	PatternMatcherStep
=============================================================================*)


End[]


EndPackage[]
