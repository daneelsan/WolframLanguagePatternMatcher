BeginPackage["DanielS`PatternMatcher`BackEnd`PatternBytecode`"]


CreatePatternBytecode::usage =
	"CreatePatternBytecode[assoc] creates the PatternBytecode[...] object.";


PatternBytecodeQ::usage =
	"PatternBytecodeQ[x] returns True if x is a valid PatternBytecode[...] object, and False otherwise.";


PatternBytecodeDissasembly::usage =
	"PatternBytecodeDissasembly[bytecodeObj] returns a dissasembly of the bytecodeObj.";


$PatternOpcodes::usage =
	"$PatternOpcodes returns an Association of all the pattern matcher opcodes.";


Begin["`Private`"]


Needs["DanielS`PatternMatcher`"]


(*=============================================================================
	PatternBytecodeDissasembly
=============================================================================*)

PatternBytecodeDissasembly[obj_?PatternBytecodeQ] :=
	Module[{bytecode = obj["Bytecode"], res},
		(* TODO: Indent when block scope *)
		res = Map[
			Replace[{
				{op_, {_, arg_}} :> StringJoin[op, " ", ToString[arg]],
				{op_} :> op
			}],
			bytecode
		];
		StringRiffle[res, "\n"]
	];

PatternBytecodeDissasembly[x_] :=
	$Failed;


(*=============================================================================
	CreatePatternBytecode
=============================================================================*)

CreatePatternBytecode[pattern_, assoc_Association] :=
	System`Private`SetValid @
		PatternBytecode[<|
			"Pattern" -> pattern,
			assoc
		|>];

CreatePatternBytecode[__] :=
	$Failed;


(*=============================================================================
	PatternBytecodeQ
=============================================================================*)

PatternBytecodeQ[x_PatternBytecode] :=
	System`Private`ValidQ[Unevaluated[x]];

PatternBytecodeQ[_] :=
	False;


(*=============================================================================
	PatternBytecode
=============================================================================*)

PatternBytecode /: (obj_PatternBytecode)["Properties"] /; PatternBytecodeQ[obj] :=
	{
		"Bytecode",
		"Pattern"
	};

PatternBytecode /: (obj:PatternBytecode[assoc_])[field_String] /; PatternBytecodeQ[obj] && KeyExistsQ[assoc, field] :=
	assoc[field];

PatternBytecode /: (obj:PatternBytecode[assoc_])["Part", n_Integer] /; PatternBytecodeQ[obj] :=
	assoc["Bytecode"][[n]];


PatternBytecode /: Length[obj_PatternBytecode] /; PatternBytecodeQ[obj] :=
	Length[obj["Bytecode"]];

PatternBytecode /: Normal[obj_PatternBytecode] /; PatternBytecodeQ[obj] :=
	obj["Pattern"];


PatternBytecode /: MakeBoxes[obj_PatternBytecode, fmt_] /; PatternBytecodeQ[obj] :=
	Module[{pattern, bytecode},
		pattern = obj["Pattern"];
		bytecode = obj["Bytecode"];
		BoxForm`ArrangeSummaryBox[
			"PatternBytecode",
			obj,
			None,
			{
				BoxForm`SummaryItem[{"Pattern: ", ClickToCopy[pattern]}]
			},
			{
				BoxForm`SummaryItem[{"Pattern: ", ClickToCopy[pattern]}],
				BoxForm`SummaryItem[{"Bytecode: ", ClickToCopy[PatternBytecodeDissasembly[obj], bytecode]}]
			}, 
			fmt,
			"CompleteReplacement" -> True
		]
	];


(*=============================================================================
	$PatternOpcodes
=============================================================================*)

$PatternOpcodes = <||>;

(*
	Core Stack/Logical Instructions
*)
$PatternOpcodes["PUSH_EXPR"] = <|
	"ShortName" -> "PUSH_EXPR",
	"Name" -> "Push Current Expression",
	"Mnemonic" -> "[] -> [expr]",
	"Pseudocode" -> "current_expr -> stack",
	"Class" -> "CoreOperations",
	"Description" -> "Pushes the expression being matched onto the operand stack.",
	"Symbol" -> Opcode$PushExpr
|>;

$PatternOpcodes["PUSH_TRUE"] = <|
	"ShortName" -> "PUSH_TRUE",
	"Name" -> "Push True",
	"Mnemonic" -> "[] -> [true]",
	"Pseudocode" -> "true -> stack",
	"Class" -> "CoreOperations",
	"Description" -> "Pushes Boolean True onto the stack.",
	"Symbol" -> Opcode$PushTrue
|>;

$PatternOpcodes["PUSH_FALSE"] = <|
	"ShortName" -> "PUSH_FALSE",
	"Name" -> "Push False",
	"Mnemonic" -> "[] -> [false]",
	"Pseudocode" -> "false -> stack",
	"Class" -> "CoreOperations",
	"Description" -> "Pushes Boolean False onto the stack.",
	"Symbol" -> Opcode$PushFalse
|>;

$PatternOpcodes["PUSH_SYMBOL"] = <|
	"ShortName" -> "PUSH_SYMBOL",
	"Name" -> "Push Symbol",
	"Mnemonic" -> "[] -> [sym]",
	"Pseudocode" -> "literal_symbol -> stack",
	"Class" -> "CoreOperations",
	"Description" -> "Pushes a literal symbol or constant onto the operand stack.",
	"Symbol" -> Opcode$PushSymbol
|>;

$PatternOpcodes["AND"] = <|
	"ShortName" -> "AND",
	"Name" -> "Logical AND",
	"Mnemonic" -> "[bool, bool] -> [bool]",
	"Pseudocode" -> "a && b -> stack",
	"Class" -> "CoreOperations",
	"Description" -> "Logical conjunction of top two stack values.",
	"Symbol" -> Opcode$And
|>;

$PatternOpcodes["OR"] = <|
	"ShortName" -> "OR",
	"Name" -> "Logical OR",
	"Mnemonic" -> "[bool, bool] -> [bool]",
	"Pseudocode" -> "a || b -> stack",
	"Class" -> "CoreOperations",
	"Description" -> "Logical disjunction of top two stack values.",
	"Symbol" -> Opcode$Or
|>;

$PatternOpcodes["NOT"] = <|
	"ShortName" -> "NOT",
	"Name" -> "Logical NOT",
	"Mnemonic" -> "[bool] -> [bool]",
	"Pseudocode" -> "!a -> stack",
	"Class" -> "CoreOperations",
	"Description" -> "Logical negation of top stack value.",
	"Symbol" -> Opcode$Not
|>;

$PatternOpcodes["SAMEQ"] = <|
	"ShortName" -> "SAMEQ",
	"Name" -> "SameQ",
	"Mnemonic" -> "[expr1, expr2] -> [bool]",
	"Pseudocode" -> "SameQ[a, b] -> stack",
	"Class" -> "CoreOperations",
	"Description" -> "Tests if top two expressions are structurally identical.",
	"Symbol" -> Opcode$SameQ
|>;

(*
	Expression Introspection
*)
$PatternOpcodes["GET_HEAD"] = <|
	"ShortName" -> "GET_HEAD",
	"Name" -> "Get Head",
	"Mnemonic" -> "[expr] -> [head]",
	"Pseudocode" -> "Head[expr] -> stack",
	"Class" -> "ExpressionInspection",
	"Description" -> "Extracts the head of the expression at the top of the stack.",
	"Symbol" -> Opcode$GetHead
|>;

$PatternOpcodes["GET_PART"] = <|
	"ShortName" -> "GET_PART",
	"Name" -> "Get Part",
	"Mnemonic" -> "[expr] -> [expr]",
	"Pseudocode" -> "Part[expr, i] -> stack",
	"Class" -> "ExpressionInspection",
	"Description" -> "Pushes the i-th part of the expression on the stack. Index i is an immediate argument.",
	"Symbol" -> Opcode$GetPart
|>;

$PatternOpcodes["GET_LENGTH"] = <|
	"ShortName" -> "GET_LENGTH",
	"Name" -> "Get Length",
	"Mnemonic" -> "[expr] -> [bool]",
	"Pseudocode" -> "Length[expr] == expected -> stack",
	"Class" -> "ExpressionInspection",
	"Description" -> "Pushes the length of the expression on the stack.",
	"Symbol" -> Opcode$GetLength
|>;

$PatternOpcodes["TEST_LENGTH"] = <|
	"ShortName" -> "TEST_LENGTH",
	"Name" -> "Test Length",
	"Mnemonic" -> "[expr] -> [bool]",
	"Pseudocode" -> "Length[expr] == expected -> stack",
	"Class" -> "ExpressionInspection",
	"Description" -> "Pushes True if the expression has the specified number of arguments.",
	"Symbol" -> Opcode$TestLength
|>;

$PatternOpcodes["TEST_HEAD"] = <|
	"ShortName" -> "TEST_HEAD",
	"Name" -> "Test Head",
	"Mnemonic" -> "[expr] -> [bool]",
	"Pseudocode" -> "SameQ[Head[expr], expected] -> stack",
	"Class" -> "ExpressionInspection",
	"Description" -> "Pushes True if the head of the expression matches the given expected symbol.",
	"Symbol" -> Opcode$TestHead
|>;

(*
	Pattern Matcher Primitives
*)
$PatternOpcodes["BLANK"] = <|
	"ShortName" -> "BLANK",
	"Name" -> "Blank Match",
	"Mnemonic" -> "[expr] -> [bool]",
	"Pseudocode" -> "match any expr -> true",
	"Class" -> "PatternPrimitives",
	"Description" -> "Matches any expression. Always pushes True.",
	"Symbol" -> Opcode$Blank
|>;

$PatternOpcodes["PATTERN_TEST"] = <|
	"ShortName" -> "PATTERN_TEST",
	"Name" -> "Pattern Test",
	"Mnemonic" -> "[expr] -> [bool]",
	"Pseudocode" -> "test[expr] -> stack",
	"Class" -> "PatternPrimitives",
	"Description" -> "Applies a predicate test function to the top of the stack. Assumes test is pure and side-effect-free.",
	"Symbol" -> Opcode$PatternTest
|>;

$PatternOpcodes["EXCEPT"] = <|
	"ShortName" -> "EXCEPT",
	"Name" -> "Except",
	"Mnemonic" -> "[expr] -> [bool]",
	"Pseudocode" -> "!(match notPatt) && match patt -> stack",
	"Class" -> "PatternPrimitives",
	"Description" -> "Matches expressions not matching a given subpattern.",
	"Symbol" -> Opcode$Except
|>;

$PatternOpcodes["ALTERNATIVES"] = <|
	"ShortName" -> "ALTERNATIVES",
	"Name" -> "Pattern Alternatives",
	"Mnemonic" -> "[expr] -> [bool]",
	"Pseudocode" -> "match[p1] || match[p2] || ...",
	"Class" -> "PatternPrimitives",
	"Description" -> "Represents a logical OR of alternative pattern branches.",
	"Symbol" -> Opcode$Alternatives
|>;

$PatternOpcodes["ALT_BRANCH"] = <|
	"ShortName" -> "ALT_BRANCH",
	"Name" -> "Branch to Alternatives",
	"Mnemonic" -> "[] -> [bool]",
	"Pseudocode" -> "for each alternative branch: run and short-circuit if true",
	"Class" -> "ControlFlow",
	"Description" -> "Executes a set of alternative pattern-matching bytecode branches. Used to implement Alternatives.",
	"Symbol" -> Opcode$AltBranch
|>;

(*
	Control Flow
*)
$PatternOpcodes["BEGIN_BLOCK"] = <|
	"ShortName" -> "BEGIN_BLOCK",
	"Name" -> "Begin Block",
	"Mnemonic" -> "[] -> []",
	"Pseudocode" -> "Start scoped pattern block",
	"Class" -> "ControlFlow",
	"Description" -> "Begins a scoped matching block. May push a new variable environment.",
	"Symbol" -> Opcode$BeginBlock
|>;

$PatternOpcodes["END_BLOCK"] = <|
	"ShortName" -> "END_BLOCK",
	"Name" -> "End Block",
	"Mnemonic" -> "[] -> []",
	"Pseudocode" -> "End scoped pattern block",
	"Class" -> "ControlFlow",
	"Description" -> "Ends a scoped matching block. Pops variable environment if needed.",
	"Symbol" -> Opcode$EndBlock
|>;

$PatternOpcodes["BRANCH"] = <|
	"ShortName" -> "BRANCH",
	"Name" -> "Branch if True",
	"Mnemonic" -> "[bool] -> []",
	"Pseudocode" -> "if pop() then jump to label",
	"Class" -> "ControlFlow",
	"Description" -> "Conditional jump if top-of-stack is true. Label/offset is an immediate operand.",
	"Symbol" -> Opcode$Branch
|>;

$PatternOpcodes["LOOP"] = <|
	"ShortName" -> "LOOP",
	"Name" -> "Loop",
	"Mnemonic" -> "[] -> []",
	"Pseudocode" -> "repeat from label while condition",
	"Class" -> "ControlFlow",
	"Description" -> "Looping construct. Behavior depends on loop context and condition.",
	"Symbol" -> Opcode$Loop
|>;

(*
	Variable Handling
*)
$PatternOpcodes["BIND_VAR"] = <|
	"ShortName" -> "BIND_VAR",
	"Name" -> "Bind Variable",
	"Mnemonic" -> "[expr] -> []",
	"Pseudocode" -> "bind top-of-stack to var name",
	"Class" -> "VariableManagement",
	"Description" -> "Binds the top of the stack to a named pattern variable.",
	"Symbol" -> Opcode$BindVar
|>;

$PatternOpcodes["GET_VAR"] = <|
	"ShortName" -> "GET_VAR",
	"Name" -> "Get Variable",
	"Mnemonic" -> "[] -> [expr]",
	"Pseudocode" -> "lookup var -> stack",
	"Class" -> "VariableManagement",
	"Description" -> "Pushes the value associated with a bound variable name onto the stack.",
	"Symbol" -> Opcode$GetVar
|>;

$PatternOpcodes["TEST_VAR"] = <|
	"ShortName" -> "TEST_VAR",
	"Name" -> "Test Variable Equality",
	"Mnemonic" -> "[expr] -> [bool]",
	"Pseudocode" -> "stack_top == bound[var] -> stack",
	"Class" -> "VariableManagement",
	"Description" -> "Compares top of stack to existing variable binding. For repeated pattern names.",
	"Symbol" -> Opcode$TestVar
|>;


End[]


EndPackage[]
