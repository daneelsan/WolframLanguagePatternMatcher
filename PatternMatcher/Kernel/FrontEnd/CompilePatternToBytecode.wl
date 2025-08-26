BeginPackage["DanielS`PatternMatcher`FrontEnd`CompilePatternToBytecode`"]


Begin["`Private`"]


Needs["DanielS`PatternMatcher`BackEnd`PatternBytecode`"] (* for CreatePatternBytecode *)
Needs["DanielS`PatternMatcher`ErrorHandling`"]
Needs["DanielS`PatternMatcher`MExprUtilities`"]
Needs["DanielS`PatternMatcher`"] (* for CompilePatternToBytecode *)

Needs["CompileAST`Create`Construct`"] (* for CreateMExpr *)


(*=============================================================================
	CompilePatternToBytecode
=============================================================================*)

CompilePatternToBytecode::unsup =
	"The pattern expression `1` is currently not supported.";

CompilePatternToBytecode[patt_] :=
	CatchFailure @
	Module[{state},
		state = <|
			(* accumulator for bytecode instructions *)
			"Bytecode" -> CreateDataStructure["Stack"],
			(* tracks bound variables for SameQ checks *)
			"LexicalEnvironment" -> CreateDataStructure["HashTable"],
			(* TODO: future support for expression scoping/indexing *)
			"CurrentExpr" -> 0,
			(* TODO: future variable numbering *)
			"VarCounter" -> 1
		|>;
		(*emit[state, "PUSH_EXPR"];*)
		compilePattern[state, CreateMExpr[patt]];
		state["LexicalEnvironment"] = Normal /@ Normal[state["LexicalEnvironment"]];
		state["Bytecode"] = Normal[state["Bytecode"]];
		CreatePatternBytecode[patt, state]
	];


compilePattern[state_, mexpr_] :=
	Which[
		MExprIsLiteral[mexpr],
			emitLiteralMatch[state, mexpr]
		,
		mexpr["normalQ"],
			Which[
				MExprIsPattern[mexpr],
					compilePatternPattern[state, mexpr]
				,
				MExprIsBlank[mexpr],
					compilePatternBlank[state, mexpr]
				,
				MExprIsPatternTest[mexpr],
					compilePatternTest[state, mexpr]
				,
				MExprIsAlternatives[mexpr],
					compilePatternAlternatives[state, mexpr]
				,
				MExprIsExcept[mexpr],
					compilePatternExcept[state, mexpr]
				,
				MExprIsCondition[mexpr],
					throwUnsupportedPattern[mexpr]
				,
				MExprIsRepeated[mexpr],
					throwUnsupportedPattern[mexpr]
				,
				MExprIsRepeatedNull[mexpr],
					throwUnsupportedPattern[mexpr]
				,
				MExprIsBlankSequence[mexpr],
					throwUnsupportedPattern[mexpr]
				,
				MExprIsBlankNullSequence[mexpr],
					throwUnsupportedPattern[mexpr]
				,
				MExprIsPatternSequence[mexpr],
					throwUnsupportedPattern[mexpr]
				,
				True,
					compilePatternNormal[state, mexpr]
			]
		,
		True,
			throwUnsupportedPattern[mexpr]
	];


throwUnsupportedPattern[mexpr_] := (
	Message[CompilePatternToBytecode::unsup, mexpr["HoldFormExpression"]];
	ThrowFailure[
		"CompilePatternToBytecode",
		CompilePatternToBytecode::unsup,
		{mexpr["HoldFormExpression"]},
		<|"Input" -> mexpr["toHeldExpression"]|>
	]
);


emit[state_, instr_, args___] := 
	state["Bytecode"]["Push", {instr, args}];


updateLexicalEnv[lexicalEnv_, lexicalName_String] :=
	If[lexicalEnv["KeyExistsQ", lexicalName],
		lexicalEnv["Lookup", lexicalName]["Increment"];
		True
		,
		lexicalEnv["Insert", lexicalName -> CreateDataStructure["Counter", 1]];
		False
	];


(* --- Core compilers --- *)

emitLiteralMatch[state_, mexpr_] := (
	emit[state, "PUSH_EXPR", {"expr", mexpr}];
	emit[state, "SAMEQ"]
);


(*
	`Blank[]` OR `Blank[f]`
*)
compilePatternBlank[state_, mexpr_] := (
	emit[state, "BLANK"];
	If[mexpr["length"] === 1,
		emit[state, "GET_HEAD"];
		emit[state, "PUSH_SYMBOL", {"sym", mexpr["part", 1]}];
		emit[state, "SAMEQ"]
	]
);


(*
	`Pattern[sym, patt]`
	Pattern needs to be aware of repeated names, in which case a SameQ test needs to be added.
	If the first instance then bind the name to the state["CurrentVariable"].
*)
compilePatternPattern[state_, mexpr_] :=
	Module[{lexicalName, patt},
		lexicalName = mexpr["part", 1]["lexicalName"];
		patt = mexpr["part", 2];
		If[updateLexicalEnv[state["LexicalEnvironment"], lexicalName],
			(* repeated variable *)
			emit[state, "GET_VAR", {"var", lexicalName}];
			emit[state, "SAMEQ"];
			,
			(*
				TODO: [Optimization] Add a pass that get rids of all the sym_
				that are only used once in the pattern.
			*)
			emit[state, "BIND_VAR", {"var", lexicalName}];
			compilePattern[state, patt]
		]
	];


(*
	`PatternTest[patt, test]`
	TODO: What about something like `__?(TrueQ[And @@ {##}] &)`
*)
compilePatternTest[state_, mexpr_] :=
	Module[{subpatt, test},
		subpatt = mexpr["part", 1];
		test = mexpr["part", 2];
		compilePattern[state, subpatt];
		emit[state, "PATTERN_TEST", {"expr", test}]
	];


(*
	`Except[notPatt]` or `Except[notPatt, patt]`
*)
compilePatternExcept[state_, mexpr_] :=
	Module[{notPatt, maybePatt},
		notPatt = mexpr["part", 1];
		emit[state, "BEGIN_BLOCK"];
		compilePattern[state, notPatt];
		emit[state, "NOT"];
		If[mexpr["length"] == 2,
			maybePatt = mexpr["part", 2];
			compilePattern[state, maybePatt]
		];
		emit[state, "END_BLOCK"]
	];


(*
	`patt1 | patt2 | ...`
*)
compilePatternAlternatives[state_, mexpr_] :=
	Module[{alts},
		alts = mexpr["arguments"];
		(* Begin a branching region: all branches try to match the expr *)
		emit[state, "BEGIN_BLOCK"];
		Do[
			compilePattern[state, alts[[i]]]; (* compile each alternative subpattern *)
			If[i < Length[alts],
				emit[state, "ALT_BRANCH"] (* jump to next alt if current fails *)
			]
			,
			{i, Length[alts]}
		];
		(* Close the branching region *)
		emit[state, "END_BLOCK"];
	];


(*
	head[arg1, arg2, ...]
*)
compilePatternNormal[state_, mexpr_] :=
	Module[{head, args, argsLen, i},
		head = mexpr["head"];
		args = mexpr["arguments"];
		argsLen = Length[args];
		(* Test length *)
		emit[state, "TEST_LENGTH", {"int", argsLen}];
		emit[state, "CHECK_ABORT"];
		(* Test head *)
		emit[state, "GET_HEAD"];
		(* TODO: Head could be a pattern it itself *)
		emit[state, "PUSH_EXPR", {"expr", head}];
		emit[state, "SAMEQ"];
		emit[state, "CHECK_ABORT"];
		Do[
			emit[state, "GET_PART", {"int", i}];
			compilePattern[state, args[[i]]]
			,
			{i, argsLen}
		]
	];


End[]


EndPackage[]
