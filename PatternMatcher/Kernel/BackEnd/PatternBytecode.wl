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
				BoxForm`SummaryItem[{"Number of Expr registers: ", exprRegisterCount}],
				BoxForm`SummaryItem[{"Number of Bool registers: ", boolRegisterCount}]
			},
			{
				BoxForm`SummaryItem[{"Disassembly: ", ClickToCopy[disassembly]}]
			},
			fmt
		]
	];


End[]


EndPackage[]
