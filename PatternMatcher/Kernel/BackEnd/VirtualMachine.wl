BeginPackage["DanielS`PatternMatcher`BackEnd`VirtualMachine`"]


Begin["`Private`"]


Needs["DanielS`PatternMatcher`BackEnd`PatternBytecode`"]
Needs["DanielS`PatternMatcher`ErrorHandling`"]
Needs["DanielS`PatternMatcher`"]

Needs["CompileAST`Create`Construct`"] (* for CreateMExpr *)


(*=============================================================================
	ResetPatternMatcherVirtualMachine
=============================================================================*)
ResetPatternMatcherVirtualMachine[vm_?PatternMatcherVirtualMachineQ] :=
	Module[{},
		vm["CurrentExpression"]["Set", CreateMExpr[Undefined]];
		vm["ProgramCounter"]["Set", 1];
		vm["Stack"]["DropAll"];
		vm["BoundVariables"]["KeyDropAll"];

		vm["Halted"]["Set", False];
		vm["Cycles"]["Set", 0];
		vm
	];


(*=============================================================================
	CreatePatternMatcherVirtualMachine
=============================================================================*)

CreatePatternMatcherVirtualMachine[bytecode_?PatternBytecodeQ] :=
	System`Private`SetValid @
		PatternMatcherVirtualMachine[<|
			"PatternBytecode" -> bytecode,

			"CurrentExpression" -> CreateDataStructure["Value", CreateMExpr[Undefined]],
			"ProgramCounter" -> CreateDataStructure["Counter", 1],
			"Stack" -> CreateDataStructure["Stack"],
			"BoundVariables" -> CreateDataStructure["HashTable"],

			"Halted" -> CreateDataStructure["Value", False],
			"Cycles" -> CreateDataStructure["Counter", 0]
		|>];

CreatePatternMatcherVirtualMachine[__] :=
	$Failed;


(*=============================================================================
	PatternMatcherVirtualMachineQ
=============================================================================*)

PatternMatcherVirtualMachineQ[x_PatternMatcherVirtualMachine] :=
	System`Private`ValidQ[Unevaluated[x]];

PatternMatcherVirtualMachineQ[_] :=
	False;


(*=============================================================================
	PatternMatcherVirtualMachine
=============================================================================*)

PatternMatcherVirtualMachine /: (obj_PatternMatcherVirtualMachine)["Properties"] /; PatternMatcherVirtualMachineQ[obj] :=
	{
		"BoundVariables",
		"CurrentExpression",
		"Cycles",
		"Halted",
		"ProgramCounter",
		"PatternBytecode",
		"Snapshot",
		"Stack"
	};


PatternMatcherVirtualMachine /: (obj:PatternMatcherVirtualMachine[assoc_])["BoundVariables"] /; PatternMatcherVirtualMachineQ[obj] :=
	assoc["BoundVariables"];


PatternMatcherVirtualMachine /: (obj:PatternMatcherVirtualMachine[assoc_])["CurrentExpression"] /; PatternMatcherVirtualMachineQ[obj] :=
	assoc["CurrentExpression"];


PatternMatcherVirtualMachine /: (obj:PatternMatcherVirtualMachine[assoc_])["Cycles"] /; PatternMatcherVirtualMachineQ[obj] :=
	assoc["Cycles"];


PatternMatcherVirtualMachine /: (obj:PatternMatcherVirtualMachine[assoc_])["Halted"] /; PatternMatcherVirtualMachineQ[obj] :=
	assoc["Halted"];


PatternMatcherVirtualMachine /: (obj:PatternMatcherVirtualMachine[assoc_])["PatternBytecode"] /; PatternMatcherVirtualMachineQ[obj] :=
	assoc["PatternBytecode"];


PatternMatcherVirtualMachine /: (obj:PatternMatcherVirtualMachine[assoc_])["ProgramCounter"] /; PatternMatcherVirtualMachineQ[obj] :=
	assoc["ProgramCounter"];


PatternMatcherVirtualMachine /: (obj:PatternMatcherVirtualMachine[assoc_])["Snapshot"] /; PatternMatcherVirtualMachineQ[obj] :=
	<|
		"PatternBytecode" -> assoc["PatternBytecode"],

		"CurrentExpression" -> ToString[assoc["CurrentExpression"]["Get"]],
		"ProgramCounter" -> assoc["ProgramCounter"]["Get"],
		"Stack" -> ToString /@ Normal[assoc["Stack"]],
		"BoundVariables" -> Normal[assoc["BoundVariables"]],

		"Halted" -> assoc["Halted"]["Get"],
		"Cycles" -> assoc["Cycles"]["Get"]
	|>;


PatternMatcherVirtualMachine /: (obj:PatternMatcherVirtualMachine[assoc_])["Stack"] /; PatternMatcherVirtualMachineQ[obj] :=
	assoc["Stack"];


PatternMatcherVirtualMachine /: (obj:PatternMatcherVirtualMachine[assoc_])[field_String] /; PatternMatcherVirtualMachineQ[obj] && KeyExistsQ[assoc, field] :=
	assoc[field];


PatternMatcherVirtualMachine /: Normal[obj:PatternMatcherVirtualMachine[assoc_]] /; PatternMatcherVirtualMachineQ[obj] :=
	assoc;


PatternMatcherVirtualMachine /: MakeBoxes[obj_PatternMatcherVirtualMachine, fmt_] /; PatternMatcherVirtualMachineQ[obj] :=
	Module[{halted, cycles, pc, bytecode, stack, boundVars},
		halted = obj["Halted"]["Get"];
		cycles = obj["Cycles"]["Get"];
		pc = obj["ProgramCounter"]["Get"];
		bytecode = obj["PatternBytecode"];
		stack = Normal[obj["Stack"]];
		boundVars = Normal[obj["BoundVariables"]];
		BoxForm`ArrangeSummaryBox[
			"PatternMatcherVirtualMachine",
			obj,
			None,
			{
				BoxForm`SummaryItem[{"Halted: ", halted}],
				BoxForm`SummaryItem[{"Program counter: ", pc}]
			},
			{
				BoxForm`SummaryItem[{"Stack: ", stack}],
				BoxForm`SummaryItem[{"Pattern bytecode: ", bytecode}],
				BoxForm`SummaryItem[{"Bound variables: ", boundVars}],
				BoxForm`SummaryItem[{"Cycles: ", cycles}]
			}, 
			fmt
		]
	];


(*=============================================================================
	PatternMatcherMatch
=============================================================================*)
Options[PatternMatcherMatch] = {
	"Logger" -> None,
	MaxIterations -> Infinity
};

PatternMatcherMatch[vm_?PatternMatcherVirtualMachineQ, expr_, opts:OptionsPattern[]] :=
	Module[{logger, maxIters, i = 1, stepOpts},
		vm["CurrentExpression"]["Set", CreateMExpr[expr]];
		maxIters = OptionValue[PatternMatcherMatch, {opts}, MaxIterations];
		logger = OptionValue[PatternMatcherMatch, {opts}, "Logger"];
		If[logger === None,
			logger = Function[Null, Null, HoldAllComplete]
		];
		stepOpts = <|"Logger" -> logger|>;
		While[!vm["Halted"]["Get"] && i < maxIters,
			PatternMatcherStep[vm, stepOpts];
			i += 1;
		];
		vm
	];


(*=============================================================================
	PatternMatcherStep
=============================================================================*)

PatternMatcherStep::halted =
	"The virtual machine has already halted.";

PatternMatcherStep::unsup =
	"The instruction `1` is not supported.";

PatternMatcherStep[vm_?PatternMatcherVirtualMachineQ, opts:_Association:<||>] :=
	CatchFailure @
	Module[{logger, opcode, args},
		logger = Lookup[opts, "Logger", Function[Null, Null, HoldAllComplete]];
		Which[
			vm["Halted"]["Get"],
				Message[PatternMatcherStep::halted];
				vm
			,
			vm["ProgramCounter"]["Get"] > Length[vm["PatternBytecode"]],
				logger[vm["Snapshot"], "End of program."];
				vm["Halted"]["Set", True];
				vm
			,
			True,
				{opcode, args} = fetch[vm];
				logger[vm["Snapshot"], "Executing: ", {opcode, args}];
				execute[opcode, args][vm];
				vm["Cycles"]["Increment"];
				vm
		]
	];


fetch[vm_] :=
	Module[{data},
		data = vm["PatternBytecode"]["Part", vm["ProgramCounter"]["Increment"]];
		normalizeInstruction[data]
	];


normalizeInstruction[{instr_, args_}] :=
	{instr, args};

normalizeInstruction[{instr_}] :=
	{instr, {}};


(*
	Core Stack/Logical Instructions
*)
execute["PUSH_CURRENT_EXPR", {}][vm_] :=
	push[vm, vm["CurrentExpression"]["Get"]];

execute["PUSH_EXPR", {"expr", mexpr_}][vm_] :=
	push[vm, mexpr];

execute["PUSH_SYMBOL", {"sym", sym_}][vm_] :=
	push[vm, sym];

execute["PUSH_TRUE", {}][vm_] :=
	push[vm, True];

execute["PUSH_FALSE", {}][vm_] :=
	push[vm, False];

execute["AND", {}][vm_] :=
	push[vm, And[pop[vm], pop[vm]]];

execute["OR", {}][vm_] :=
	push[vm, Or[pop[vm], pop[vm]]];

execute["NOT", {}][vm_] :=
	push[vm, Not[pop[vm]]];

execute["SAMEQ", {}][vm_] :=
	Module[{a, b},
		a = pop[vm];
		b = pop[vm];
		(* reverse order, as stack is LIFO *)
		push[vm, b["sameQ", a]]
	];

(*
	Expression Introspection
*)
execute["GET_HEAD", {}][vm_] :=
	push[vm, vm["CurrentExpression"]["Get"]["head"]];

execute["GET_PART", {"int", n_}][vm_] :=
	push[vm, vm["CurrentExpression"]["Get"]["part", n]];

execute["GET_LENGTH", {}][vm_] :=
	push[vm, vm["CurrentExpression"]["Get"]["length"]];

(* NOTE: Could be GET_LENGTH + INT_SAMEQ *)
execute["TEST_LENGTH", {"int", n_}][vm_] :=
	Module[{expr = vm["CurrentExpression"]["Get"]},
		push[vm, expr["length"] === n]
	];

(* NOTE: Could be GET_HEAD + MATCHQ expr *)
(* TODO: Only handles symbols, not any expr. *)
execute["TEST_HEAD", {"sym", sym_}][vm_] :=
	Module[{expr = vm["CurrentExpression"]["Get"]},
		push[vm, expr["head"] === sym]
	];

(*
	Pattern Matcher Primitives
*)
execute["BLANK", {}][vm_] := 
	With[{expr = pop[vm]},
		push[vm, True];
	];

execute["PATTERN_TEST", {"expr", testFn_}][vm_] :=
	With[{expr = vm["CurrentExpression"]["Get"]},
		expr = pop[vm];
		push[vm, TrueQ[CreateMExpr[testFn[expr]]["toExpression"]]];
	];

(*
	Control Flow
*)
execute["CHECK_ABORT", {}][vm_] := 
	With[{res = pop[vm]},
		If[!TrueQ[res],
			vm["Halted"]["Set", True];
			push[vm, False]
		]
	];


execute[opcode_, args___][vm_] :=
	ThrowFailure[
		"PatternMatcherStep",
		PatternMatcherStep::unsup,
		{HoldForm[execute[opcode, args]]},
		"Input" -> HoldComplete[execute[opcode, args]]
	];


(*
	Stack operations
*)
push[vm_, val_] :=
	vm["Stack"]["Push", val];


pop[vm_] :=
	vm["Stack"]["Pop"];


End[]


EndPackage[]
