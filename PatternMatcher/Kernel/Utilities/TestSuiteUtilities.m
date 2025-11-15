BeginPackage["DanielS`PatternMatcher`Utilities`TestSuiteUtilities`"]


CheckMemoryLeak::usage =
	"CheckMemoryLeak[expr] returns True if repeated evaluation of expr does not leak memory and the memory increase with each successive evaluation otherwise.";

TestStatePush::usage =
	"TestStatePush[] Sets $Context to a specified context, Lookup[Options[TestStatePush], \"Context\"] by default.";

TestStatePop::usage =
	"TestStatePop[state] Reverts $Context back to original state, clearing all symbols in state[\"Context\"].";


Begin["`Private`"]


(*==============================================================================
	Memory Leaks
==============================================================================*)

SetAttributes[CheckMemoryLeak, HoldAll]

CheckMemoryLeak[expr_, reps_: 10, tol_: 0] :=
	Module[{res, memory},
		Unprotect[$MessageList];
		$MessageList = {};
		ClearSystemCache[];
		res = Table[
			memory = MemoryInUse[];
			expr;
			$MessageList = {};
			ClearSystemCache[];
			MemoryInUse[] - memory
			,
			{reps}
		];
		Last[res] <= tol || res
	];


(*==============================================================================
	Utility for clearing all symbols created 
==============================================================================*)
Options[TestStatePush] = {
	"Context" -> "TestContext`"
};

TestStatePush[OptionsPattern[]] := 
	Module[
		{newContext, oldContext, oldContextPath, newContextPath},
		oldContextPath = $ContextPath;
		newContextPath = DeleteCases[$ContextPath, "Global`"];
		newContext = OptionValue["Context"];
		oldContext = $Context;
		$Context = newContext;
		$ContextPath = newContextPath;
		<|"OldContext" -> oldContext, "Context" -> $Context, "OldContextPath" -> oldContextPath|>
	];


SetAttributes[TestStatePop, HoldAll];

TestStatePop[state_] :=
	With[
		{toClear = state["Context"] <> "*"},
		(* Clear all symbols created within the state *)
		ClearAll[toClear];
		$Context = state["OldContext"];
		(* Restore context path *)
		$ContextPath = state["OldContextPath"];
		(* Clear the state symbol itself *)
		Clear[state];
	];


End[]


EndPackage[]
