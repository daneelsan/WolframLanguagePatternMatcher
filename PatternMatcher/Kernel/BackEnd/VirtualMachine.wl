BeginPackage["DanielS`PatternMatcher`BackEnd`VirtualMachine`"]


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
		initPatternVirtualMachine[];
		vm
	];

CreatePatternMatcherVirtualMachine[__] :=
	$Failed;


$toBoxesInitialized = False;

initPatternVirtualMachine[] /; !TrueQ[$toBoxesInitialized] := (
	Compile`Class`SetClassMethods["PatternMatcherLibrary`VM`VirtualMachine", {}, {"toBoxes" -> toBoxes}];
	$toBoxesInitialized = True;
);


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
	Module[{halted, cycles, pc},
		halted = obj["isHalted"];
		cycles = obj["getCycles"];
		pc = obj["getPC"];
		BoxForm`ArrangeSummaryBox[
			"PatternMatcherVirtualMachine",
			obj,
			None,
			{
				BoxForm`SummaryItem[{"Halted: ", halted}],
				BoxForm`SummaryItem[{"Program counter: ", pc}]
			},
			{
				(*BoxForm`SummaryItem[{"Stack: ", stack}],
				BoxForm`SummaryItem[{"Pattern bytecode: ", bytecode}],
				BoxForm`SummaryItem[{"Bound variables: ", boundVars}],*)
				BoxForm`SummaryItem[{"Cycles: ", cycles}]
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
