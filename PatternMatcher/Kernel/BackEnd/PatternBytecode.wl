BeginPackage["DanielS`PatternMatcher`BackEnd`PatternBytecode`"]


$PatternOpcodes::usage =
	"$PatternOpcodes returns an Association of all the pattern matcher opcodes.";


Begin["`Private`"]


Needs["DanielS`PatternMatcher`"]
Needs["DanielS`PatternMatcher`BackEnd`PatternBytecodeFormat`"]


(*=============================================================================
	PatternBytecodeDisassemble (formatted with colors)
=============================================================================*)

PatternBytecodeDisassemble[obj_?PatternBytecodeQ] :=
	FormatPatternBytecodeDisassembly[obj["disassemble"]];

PatternBytecodeDisassemble[x_] :=
	$Failed;


(*=============================================================================
	PatternBytecodeInformation (statistics as Association)
=============================================================================*)

PatternBytecodeInformation[obj_?PatternBytecodeQ] :=
	<|
		"InstructionCount" -> obj["getInstructionCount"],
		"LabelCount" -> obj["getLabelCount"],
		"ExprRegisterCount" -> obj["getExprRegisterCount"],
		"BoolRegisterCount" -> obj["getBoolRegisterCount"],
		"BlockCount" -> obj["getBlockCount"],
		"MaxBlockDepth" -> obj["getMaxBlockDepth"],
		"JumpCount" -> obj["getJumpCount"],
		"BacktrackPointCount" -> obj["getBacktrackPointCount"],
		"LexicalBindings" -> obj["getLexicalBindings"]
	|>;

PatternBytecodeInformation[x_] :=
	$Failed;


(*=============================================================================
	PatternBytecodeQ
=============================================================================*)

PatternBytecodeQ[x_?Compile`Utilities`Class`Impl`ObjectInstanceQ] :=
	x["_class"] === "PatternMatcherLibrary`VM`PatternBytecode";

PatternBytecodeQ[_] :=
	False;


(*=============================================================================
	PatternBytecode
=============================================================================*)

toBoxes[obj_, fmt_] :=
	Module[{pattern, disassembly, exprRegisterCount, boolRegisterCount},
		pattern = obj["getPattern"];
		disassembly = obj["toString"];
		exprRegisterCount = obj["getExprRegisterCount"];
		boolRegisterCount = obj["getBoolRegisterCount"];
		BoxForm`ArrangeSummaryBox[
			"PatternBytecode",
			obj,
			None,
			{
				BoxForm`SummaryItem[{"Pattern: ", ClickToCopy[pattern["toHeldFormExpr"], pattern["toHeldExpr"]]}],
				BoxForm`SummaryItem[{"Expr registers count: ", exprRegisterCount}],
				BoxForm`SummaryItem[{"Bool registers count: ", boolRegisterCount}],
				BoxForm`SummaryItem[{"Instruction count: ", obj["getInstructionCount"]}]
			},
			{
				BoxForm`SummaryItem[{"Block count: ", obj["getBlockCount"]}],
				BoxForm`SummaryItem[{"Backtrack point count: ", obj["getBacktrackPointCount"]}]
			},
			fmt
		]
	];


End[]


EndPackage[]
