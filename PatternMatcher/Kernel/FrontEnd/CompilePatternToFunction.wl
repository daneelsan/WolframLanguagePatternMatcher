BeginPackage["DanielS`PatternMatcher`FrontEnd`CompilePatternToFunction`"];


Begin["`Private`"];


Needs["DanielS`PatternMatcher`"] (* for PatternToFunction *)
Needs["DanielS`PatternMatcher`ErrorHandling`"]
Needs["DanielS`PatternMatcher`AST`"]
Needs["DanielS`PatternMatcher`AST`MExprUtilities`"]


(*=============================================================================
	PatternToFunction
=============================================================================*)
(*
	TODO: Consider renaming to PatternToMatchQFunction.
*)

Options[CompilePatternToFunction] = {
	"ApplyOptimizations" -> True
}

CompilePatternToFunction::unsup =
	"The pattern expression `1` is currently not supported.";

CompilePatternToFunction[patt_, opts:OptionsPattern[]] :=
	CatchFailure @ Module[{state},
		state = <|
			"VariableCounter" -> CreateDataStructure["Counter", 1],
			"Input" :> patt,
			"CurrentVariable" -> CreateDataStructure["Value", vm`expr$],
			"TestsList" -> CreateDataStructure["DynamicArray"],
			"TestsStack" -> CreateDataStructure["Stack"],
			"Evaluations" -> CreateDataStructure["DynamicArray"],
			"BoundVariables" -> CreateDataStructure["OrderedHashSet"],
			"BoundVariablesMap" -> CreateDataStructure["HashTable"],
			"AssignedVariablesStack" -> CreateDataStructure["Stack"],
			"ApplyOptimizations" -> TrueQ[OptionValue["ApplyOptimizations"]]
		|>;
		visitPattern[state, ConstructMExpr[patt]];
		With[{
				moduleVars = getAssignmentVariables[state],
				moduleBody = Echo[getFullCondition[state]]
			},
			If[state["ApplyOptimizations"] && moduleVars["length"] === 0,
				ConstructMExpr @ Function[vm`expr$, moduleBody]
				,
				ConstructMExpr @ Function[vm`expr$, Module[moduleVars, moduleBody]]
			]
		]["toExpr"]
	]


(*======================================
	visitPattern
======================================*)

visitPattern[state_, mexpr_] :=
	Which[
		MExprIsLiteral[mexpr],
			processLiteral[state, mexpr]
		,
		mexpr["normalQ"],
			Which[
				MExprIsPattern[mexpr],
					processPattern[state, mexpr]
				,
				MExprIsBlank[mexpr],
					processBlank[state, mexpr]
				,
				MExprIsPatternTest[mexpr],
					processPatternTest[state, mexpr]
				,
				MExprIsExcept[mexpr],
					processExcept[state, mexpr]
				,
				MExprIsAlternatives[mexpr],
					processAlternatives[state, mexpr]
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
					processNormal[state, mexpr]
			]
		,
		True,
			ThrowFailure[
				"CompilePatternToFunction",
				"Cannot compile the expression `1`.",
				{mexpr["toHeldFormExpr"]},
				<|"Input" -> mexpr["toHeldExpr"]|>
			]
	];


throwUnsupportedPattern[mexpr_] := (
	Message[CompilePatternToFunction::unsup, mexpr["toHeldFormExpr"]];
	ThrowFailure[
		"CompilePatternToFunction",
		CompilePatternToFunction::unsup,
		{mexpr["toHeldFormExpr"]},
		<|"Input" -> mexpr["toHeldExpr"]|>
	]
);


(*======================================
	processPattern
======================================*)
(*
	`Pattern[sym, patt]`
	Pattern needs to be aware of repeated names, in which case a SameQ test needs to be added.
	If the first instance then bind the name to the state["CurrentVariable"].
*)
processPattern[state_, mexpr_] :=
	Module[{sym, patt},
		sym = mexpr["part", 1];
		patt = mexpr["part", 2];
		processPatternWork[state, sym, state["CurrentVariable"]["Get"]];
		visitPattern[state, patt];
	]


processPatternWork[state_, boundSym_, currentExpr_] :=
	Module[{lexicalName, symData},
		lexicalName = boundSym["getLexicalName"];
		state["BoundVariables"]["Insert", lexicalName];
		symData = state["BoundVariablesMap"]["Lookup", lexicalName, Null &];
		If[symData === Null,
			state["BoundVariablesMap"]["Insert", lexicalName -> <|"Symbol" -> boundSym, "Expression" -> currentExpr|>];
			(*state["variables"]["appendTo", lexicalName];*)
			,
			addSameQTest[state, symData["Expression"], currentExpr]
		]
	]


(*======================================
	processBlank
======================================*)
(*
	`Blank[]` OR `Blank[f]`
*)
processBlank[state_, mexpr_] :=
	If[mexpr["length"] === 0,
		addTest[state, ConstructMExpr[True]]
		,
		With[{var = state["CurrentVariable"]["Get"], head = mexpr["part", 1]},
			addTest[state, ConstructMExpr[Head[var] === head]]
		]
	]


(*======================================
	processPatternTest
======================================*)
(*
	`PatternTest[patt, test]`
	TODO: What about something like `__?(TrueQ[And @@ {##}] &)`
*)
processPatternTest[state_, mexpr_] :=
	With[{
		patt = mexpr["part", 1],
		test = mexpr["part", 2],
		var = state["CurrentVariable"]["Get"]
	},
		visitPattern[state, patt];
		addTest[state, ConstructMExpr[TrueQ[test[var]]]]
	]


(*======================================
	processExcept
======================================*)
(*
	`Except[notPatt]` or `Except[notPatt, patt]`
*)
processExcept[state_, mexpr_] :=
	Module[{notPatt, patt},
		notPatt = mexpr["part", 1];
		pushTests[state];
		visitPattern[state, notPatt];
		popTests[state, Not, {getFullCondition[state]}];
		If[mexpr["length"] === 2,
			patt = mexpr["part", 2];
			visitPattern[state, patt]
		]
	]


(*======================================
	processAlternatives
======================================*)
(*
	`patt1 | patt2 | ...`
*)
processAlternatives[state_, mexpr_] :=
	Module[{altArgs},
		pushTests[state];
		altArgs =
			Map[
				(visitPattern[state, #]; getFullCondition[state]) &,
				mexpr["arguments"]
			];
		popTests[state, Or, altArgs];
	]


(*======================================
	processLiteral
======================================*)
processLiteral[state_, mexpr_] :=
	addSameQTest[state, state["CurrentVariable"]["Get"], mexpr]


(*======================================
	processNormal
======================================*)
(*
	head[arg1, arg2, ...]
*)
processNormal[state_, mexpr_] :=
	(
		addLengthTest[state, mexpr["length"]];
		addStateElement[state, 0, mexpr["head"]];
		MapIndexed[
			addStateElement[state, #2[[1]], #1] &,
			mexpr["arguments"]
		]
	)


(*======================================
	addStateElement
======================================*)
(*
	addStateElement digs into the inside of an element.
	It creates a new variable, which is set to the head or the part depending whether index is 0 or not.
*)
addStateElement[state_, index_, pattMExpr_] :=
	With[{newVar = newVariable[state], currentVar = state["CurrentVariable"]["Get"]},
		state["Evaluations"]["Append",
			If[index === 0,
				ConstructMExpr[newVar = Head[currentVar]]
				,
				ConstructMExpr[newVar = Part[currentVar, index]]
			]
		];
		state["CurrentVariable"]["Set", newVar];
		visitPattern[state, pattMExpr];
		state["CurrentVariable"]["Set", currentVar];
	];


(*======================================
	getFullCondition
======================================*)
getFullCondition[state_] :=
	Module[{condList},
		condList = Function[{tests},
			If[Length[tests] === 1, tests[[1]], Apply[ConstructMExpr[CompoundExpression[##]] &][tests]]
		] /@ state["TestsList"]["Elements"];
		state["TestsList"]["DropAll"];
		If[state["ApplyOptimizations"] && Length[condList] === 1,
			condList[[1]]
			,
			Apply[ConstructMExpr[And[##]] &][condList]
		]
	]


(*======================================
	addLengthTest
======================================*)
addLengthTest[state_, len_] :=
	With[{var = state["CurrentVariable"]["Get"]},
		addTest[state, ConstructMExpr[Length[var] === len]]
	]


(*======================================
	addSameQTest
======================================*)
addSameQTest[state_, lhs_, rhs_] :=
	addTest[state, ConstructMExpr[lhs === rhs]]


(*======================================
	addTest
======================================*)
addTest[state_, test_] :=
	Module[{evals},
		evals = Flatten[{state["Evaluations"]["Elements"], test}];
		state["TestsList"]["Append", evals];
		state["Evaluations"]["DropAll"];
	]


(*======================================
	pushTests
======================================*)
pushTests[state_] :=
	(
		state["TestsStack"]["Push", state["TestsList"]["Elements"]];
		state["TestsList"]["DropAll"]
	)


(*======================================
	popTests
======================================*)
popTests[state_, head_, args_List] :=
	Module[{currentTest, oldTests},
		oldTests = state["TestsStack"]["Pop"];
		Assert[state["TestsList"]["Length"] === 0];
		state["TestsList"]["JoinBack", oldTests];
		currentTest = Apply[ConstructMExpr[head[##]] &, args];
		state["TestsList"]["Append", currentTest];
	]


(*======================================
	getAssignmentVariables
======================================*)
getAssignmentVariables[state_] :=
	With[{vars = state["AssignedVariablesStack"]["Elements"][[All, "Variable"]]},
		ConstructMExpr[vars]
	]


(*======================================
	newVariable
======================================*)
newVariable[state_] :=
	With[{var = makeVariable[state]},
		addAssignment[state, var, Null];
		var
	]


(*======================================
	addAssignment
======================================*)
(*
	Add an assignment, this will show up in the Module of the outer function.
	A value of Null means there is no top-level assignment.
*)
addAssignment[state_, var_, val_] :=
	state["AssignedVariablesStack"]["Push", <|"Variable" -> var, "Value" -> val|>]


(*======================================
	makeVariable
======================================*)
makeVariable[state_] :=
	With[{s = Symbol["var" <> ToString[state["VariableCounter"]["Increment"]]]},
		ConstructMExpr[s]
	]


End[];


EndPackage[];
