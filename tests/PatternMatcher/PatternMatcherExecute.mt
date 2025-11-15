If[FindFile["tests/CustomLoad.m"] =!= $Failed,
	Get["tests/CustomLoad.m"]
]


Needs["DanielS`PatternMatcher`Utilities`TestSuiteUtilities`"]

BeginTestSection[$CurrentTestSource]


Needs["DanielS`PatternMatcher`"]


Global`contextState = TestStatePush[]


(*==============================================================================
	Literals
==============================================================================*)
Test[
	PatternMatcherExecute[4, 4]["Result"]
	,
	True
	,
	TestID->"PatternMatcherExecute-20251024-E0O5N7"
]

Test[
	PatternMatcherExecute[4, 5]["Result"]
	,
	False
	,
	TestID->"PatternMatcherExecute-20251024-J3L2S6"
]

Test[
	PatternMatcherExecute["42", "42"]["Result"]
	,
	True
	,
	TestID->"PatternMatcherExecute-20251024-R2L0K9"
]

Test[
	PatternMatcherExecute["42", "43"]["Result"]
	,
	False
	,
	TestID->"PatternMatcherExecute-20251024-V2A8L7"
]

Test[
	PatternMatcherExecute[True, True]["Result"]
	,
	True
	,
	TestID->"PatternMatcherExecute-20251024-V8E3Z3"
]

Test[
	PatternMatcherExecute[True, False]["Result"]
	,
	False
	,
	TestID->"PatternMatcherExecute-20251024-X1X0G3"
]


(*==============================================================================
	Normal
==============================================================================*)
Test[
	PatternMatcherExecute[f[], f[]]["Result"]
	,
	True
	,
	TestID->"PatternMatcherExecute-20251024-N3W7Z3"
]

Test[
	PatternMatcherExecute[f[1, 2], f[1, 2]]["Result"]
	,
	True
	,
	TestID->"PatternMatcherExecute-20251024-U3S0Z6"
]

Test[
	PatternMatcherExecute[f[g[1], 2], f[g[1], 2]]["Result"]
	,
	True
	,
	TestID->"PatternMatcherExecute-20251024-B7W8P3"
]

Test[
	PatternMatcherExecute[f[g[1], 2], f[g[1], 3]]["Result"]
	,
	False
	,
	TestID->"PatternMatcherExecute-20251024-N2T6K3"
]

Test[
	PatternMatcherExecute[f[g[1], 2], h[g[1], 2]]["Result"]
	,
	False
	,
	TestID->"PatternMatcherExecute-20251024-J3X6F7"
]


(*==============================================================================
	Blank
==============================================================================*)
Test[
	PatternMatcherExecute[_, 5]["Result"]
	,
	True
	,
	TestID->"PatternMatcherExecute-20251024-J6I4L9"
]

Test[
	PatternMatcherExecute[_, "a string"]["Result"]
	,
	True
	,
	TestID->"PatternMatcherExecute-20251024-F5F1F3"
]

Test[
	PatternMatcherExecute[_, f[x, y]]["Result"]
	,
	True
	,
	TestID->"PatternMatcherExecute-20251114-B4J8Y9"
]

Test[
	PatternMatcherExecute[_Integer, 5]["Result"]
	,
	True
	,
	TestID->"PatternMatcherExecute-20251024-S3F5F7"
]

Test[
	PatternMatcherExecute[_Integer, "a string"]["Result"]
	,
	False
	,
	TestID->"PatternMatcherExecute-20251024-Y9Y8R4"
]

Test[
	PatternMatcherExecute[_Integer, Integer["not really"]]["Result"]
	,
	True
	,
	TestID->"PatternMatcherExecute-20251114-S7Y2D7"
]

Test[
	PatternMatcherExecute[f[_], f[5]]["Result"]
	,
	True
	,
	TestID->"PatternMatcherExecute-20251024-B4X6D7"
]

Test[
	PatternMatcherExecute[f[_], f["a string"]]["Result"]
	,
	True
	,
	TestID->"PatternMatcherExecute-20251024-K8V4H8"
]

Test[
	PatternMatcherExecute[f[_Integer], f[5]]["Result"]
	,
	True
	,
	TestID->"PatternMatcherExecute-20251024-N2B3O2"
]

Test[
	PatternMatcherExecute[f[_Integer], f["a string"]]["Result"]
	,
	False
	,
	TestID->"PatternMatcherExecute-20251024-P7H8A5"
]


(*==============================================================================
	Pattern
==============================================================================*)
Test[
	PatternMatcherExecute[x_, 5]
	,
	<|"Result" -> True, "CyclesExecuted" -> 8, "Bindings" -> <|"TestContext`x" -> 5|>|>
	,
	TestID->"PatternMatcherExecute-20251024-B6T7X1"
]

Test[
	PatternMatcherExecute[x_, "a string"]
	,
	<|"Result" -> True, "CyclesExecuted" -> 8, "Bindings" -> <|"TestContext`x" -> "a string"|>|>
	,
	TestID->"PatternMatcherExecute-20251024-V1W6B9"
]

Test[
	PatternMatcherExecute[x_Integer, 5]
	,
	<|"Result" -> True, "CyclesExecuted" -> 9, "Bindings" -> <|"TestContext`x" -> 5|>|>
	,
	TestID->"PatternMatcherExecute-20251024-V7E5C8"
]

Test[
	PatternMatcherExecute[x_Integer, "string"]
	,
	<|"Result" -> False, "CyclesExecuted" -> 6, "Bindings" -> <||>|>
	,
	TestID->"PatternMatcherExecute-20251024-R2L6F7"
]

Test[
	PatternMatcherExecute[f[x_], f[5]]["Result"]
	,
	True
	,
	TestID->"PatternMatcherExecute-20251024-T9Y3K9"
]

Test[
	PatternMatcherExecute[f[x_], f["string"]]["Result"]
	,
	True
	,
	TestID->"PatternMatcherExecute-20251024-R9C9G8"
]

Test[
	PatternMatcherExecute[f[x_Integer], f[5]]["Result"]
	,
	True
	,
	TestID->"PatternMatcherExecute-20251024-L8L1U7"
]

Test[
	PatternMatcherExecute[f[x_Integer], f["string"]]["Result"]
	,
	False
	,
	TestID->"PatternMatcherExecute-20251024-N9N7L9"
]

Test[
	PatternMatcherExecute[f[x_, y_], f[5, 5]]["Result"]
	,
	True
	,
	TestID->"PatternMatcherExecute-20251024-H1O1H0"
]

Test[
	PatternMatcherExecute[f[x_, y_], f[5, "string"]]["Result"]
	,
	True
	,
	TestID->"PatternMatcherExecute-20251024-C3O3I9"
]

Test[
	PatternMatcherExecute[f[x_, x_], f[5, 5]]["Result"]
	,
	True
	,
	TestID->"PatternMatcherExecute-20251024-Z3M9W3"
]

Test[
	PatternMatcherExecute[f[x_, x_], f[5, "string"]]["Result"]
	,
	False
	,
	TestID->"PatternMatcherExecute-20251024-D9V0X7"
]

Test[
	PatternMatcherExecute[f[x_, h[x_, x_]], f[5, h[5, 5]]]["Result"]
	,
	True
	,
	TestID->"PatternMatcherExecute-20251024-Q1Q9E5"
]

Test[
	(* FIXME *)
	PatternMatcherExecute[f[x_, x_[g]], f[5, 5[g]]]["Result"]
	,
	True
	,
	TestID->"PatternMatcherExecute-20251024-T3F2S6"
]


(*==============================================================================
	Alternatives
==============================================================================*)
Test[
	(* FIXME *)
	PatternMatcherExecute[_ | _, 5]["Result"]
	,
	True
	,
	TestID->"PatternMatcherExecute-20251024-K3S2G7"
]

Test[
	(* FIXME *)
	PatternMatcherExecute[_Real | _Integer, 5]["Result"]
	,
	True
	,
	TestID->"PatternMatcherExecute-20251024-B0M0W6"
]

Test[
	PatternMatcherExecute[f[_ | _], f[5]]["Result"]
	,
	True
	,
	TestID->"PatternMatcherExecute-20251024-Y9R6W0"
]

Test[
	(* FIXME *)
	PatternMatcherExecute[f[_Integer | _Real], f[5]]["Result"]
	,
	True
	,
	TestID->"PatternMatcherExecute-20251024-Y3E3Q6"
]

Test[
	(* FIXME *)
	PatternMatcherExecute[f[_Integer | _Real], f["string"]]["Result"]
	,
	False
	,
	TestID->"PatternMatcherExecute-20251024-P1Q7O6"
]


TestStatePop[Global`contextState]


EndTestSection[]
