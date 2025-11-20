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
TestMatch[
	PatternMatcherExecute[x_, 5]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`x" -> 5|>|>
	,
	TestID->"PatternMatcherExecute-20251024-B6T7X1"
]

TestMatch[
	PatternMatcherExecute[x_, "a string"]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`x" -> "a string"|>|>
	,
	TestID->"PatternMatcherExecute-20251024-V1W6B9"
]

TestMatch[
	PatternMatcherExecute[x_Integer, 5]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`x" -> 5|>|>
	,
	TestID->"PatternMatcherExecute-20251024-V7E5C8"
]

TestMatch[
	PatternMatcherExecute[x_Integer, "string"]
	,
	<|"Result" -> False, "CyclesExecuted" -> _, "Bindings" -> <||>|>
	,
	TestID->"PatternMatcherExecute-20251024-R2L6F7"
]

TestMatch[
	PatternMatcherExecute[f[x_], f[5]]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`x" -> 5|>|>
	,
	TestID->"PatternMatcherExecute-20251024-T9Y3K9"
]

TestMatch[
	PatternMatcherExecute[f[x_], f["string"]]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`x" -> "string"|>|>
	,
	TestID->"PatternMatcherExecute-20251024-R9C9G8"
]

TestMatch[
	PatternMatcherExecute[f[x_Integer], f[5]]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`x" -> 5|>|>
	,
	TestID->"PatternMatcherExecute-20251024-L8L1U7"
]

TestMatch[
	PatternMatcherExecute[f[x_Integer], f["string"]]
	,
	<|"Result" -> False, "CyclesExecuted" -> _, "Bindings" -> <||>|>
	,
	TestID->"PatternMatcherExecute-20251024-N9N7L9"
]

TestMatch[
	PatternMatcherExecute[f[x_, y_], f[5, 5]]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`y" -> 5, "TestContext`x" -> 5|>|>
	,
	TestID->"PatternMatcherExecute-20251024-H1O1H0"
]

TestMatch[
	PatternMatcherExecute[f[x_, y_], f[5, "string"]]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`y" -> "string", "TestContext`x" -> 5|>|>
	,
	TestID->"PatternMatcherExecute-20251024-C3O3I9"
]

TestMatch[
	PatternMatcherExecute[f[x_, x_], f[5, 5]]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`x" -> 5|>|>
	,
	TestID->"PatternMatcherExecute-20251024-Z3M9W3"
]

TestMatch[
	PatternMatcherExecute[f[x_, x_Integer], f[5, 5]]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`x" -> 5|>|>
	,
	TestID->"PatternMatcherExecute-20251114-X8X6E9"
]

TestMatch[
	PatternMatcherExecute[f[x_, x_Integer], f[5, 4]]
	,
	<|"Result" -> False, "CyclesExecuted" -> _, "Bindings" -> <||>|>
	,
	TestID->"PatternMatcherExecute-20251114-B9B5J8"
]

TestMatch[
	PatternMatcherExecute[f[x_, x_Integer], f[5, 5.]]
	,
	<|"Result" -> False, "CyclesExecuted" -> _, "Bindings" -> <||>|>
	,
	TestID->"PatternMatcherExecute-20251114-N2Q9A5"
]

TestMatch[
	PatternMatcherExecute[f[x_Real, x_Integer], f[5, 5]]
	,
	<|"Result" -> False, "CyclesExecuted" -> _, "Bindings" -> <||>|>
	,
	TestID->"PatternMatcherExecute-20251114-D9X3S4"
]

TestMatch[
	PatternMatcherExecute[f[x_, x_], f[5, "string"]]
	,
	<|"Result" -> False, "CyclesExecuted" -> _, "Bindings" -> <||>|>
	,
	TestID->"PatternMatcherExecute-20251024-D9V0X7"
]

TestMatch[
	PatternMatcherExecute[f[x_, h[x_, x_]], f[5, h[5, 5]]]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`x" -> 5|>|>
	,
	TestID->"PatternMatcherExecute-20251024-Q1Q9E5"
]

TestMatch[
	(* FIXME *)
	PatternMatcherExecute[f[x_, x_[g]], f[5, 5[g]]]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`x" -> 5|>|>
	,
	TestID->"PatternMatcherExecute-20251024-T3F2S6"
]


(*==============================================================================
	PatternTest
==============================================================================*)
TestMatch[
	PatternMatcherExecute[_?IntegerQ, 5]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <||>|>
	,
	TestID->"PatternMatcherExecute-20251115-N4Y8K5"
]

TestMatch[
	PatternMatcherExecute[_?IntegerQ, Integer["not an integer"]]
	,
	<|"Result" -> False, "CyclesExecuted" -> _, "Bindings" -> <||>|>
	,
	TestID->"PatternMatcherExecute-20251115-B5H9O8"
]

TestMatch[
	PatternMatcherExecute[x_Integer?EvenQ, 2]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`x" -> 2|>|>
	,
	TestID->"PatternMatcherExecute-20251115-F4P7S3"
]

TestMatch[
	PatternMatcherExecute[x_Integer?EvenQ, 3]
	,
	<|"Result" -> False, "CyclesExecuted" -> _, "Bindings" -> <||>|>
	,
	TestID->"PatternMatcherExecute-20251115-Q2R3G1"
]

TestMatch[
	PatternMatcherExecute[f[x_Integer?EvenQ], f[4]]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`x" -> 4|>|>
	,
	TestID->"PatternMatcherExecute-20251115-S4U9B8"
]

TestMatch[
	PatternMatcherExecute[f[x_Integer?EvenQ], f[3]]
	,
	<|"Result" -> False, "CyclesExecuted" -> _, "Bindings" -> <||>|>
	,
	TestID->"PatternMatcherExecute-20251115-K8P6E1"
]

TestMatch[
	PatternMatcherExecute[f[x_Integer?EvenQ, y_], f[4, 5]]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`y" -> 5, "TestContext`x" -> 4|>|>
	,
	TestID->"PatternMatcherExecute-20251115-Y7U1M0"
]

TestMatch[
	PatternMatcherExecute[f[x_Integer?EvenQ, y_], f[3, 5]]
	,
	<|"Result" -> False, "CyclesExecuted" -> _, "Bindings" -> <||>|>
	,
	TestID->"PatternMatcherExecute-20251115-J8J2M4"
]

TestMatch[
	PatternMatcherExecute[f[x_Integer?EvenQ, x_], f[4, 4]]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`x" -> 4|>|>
	,
	TestID->"PatternMatcherExecute-20251115-Q5A8B9"
]

TestMatch[
	PatternMatcherExecute[f[x_Integer?EvenQ, x_], f[4, 5]]
	,
	<|"Result" -> False, "CyclesExecuted" -> _, "Bindings" -> <||>|>
	,
	TestID->"PatternMatcherExecute-20251115-L1H4V7"
]

TestMatch[
	PatternMatcherExecute[f[x_Integer?EvenQ, x_], f[3, 3]]
	,
	<|"Result" -> False, "CyclesExecuted" -> _, "Bindings" -> <||>|>
	,
	TestID->"PatternMatcherExecute-20251115-P7C4H6"
]

TestMatch[
	PatternMatcherExecute[{x_, y_}?OrderedQ, {1, 2}]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`y" -> 2, "TestContext`x" -> 1|>|>
	,
	TestID->"PatternMatcherExecute-20251115-W9M8P7"
]

TestMatch[
	PatternMatcherExecute[{x_, y_}?OrderedQ, {2, 1}]
	,
	<|"Result" -> False, "CyclesExecuted" -> _, "Bindings" -> <||>|>
	,
	TestID->"PatternMatcherExecute-20251115-F0G6S9"
]


(*==============================================================================
	Sequence Patterns (BlankSequence __ and BlankNullSequence ___)
==============================================================================*)

(* Standalone BlankSequence __ *)
Test[
	PatternMatcherExecute[__, {1, 2, 3}]["Result"]
	,
	True
	,
	TestID->"PatternMatcherExecute-20251119-SEQ001"
]

Test[
	PatternMatcherExecute[__, {1}]["Result"]
	,
	True
	,
	TestID->"PatternMatcherExecute-20251119-SEQ002"
]

Test[
	PatternMatcherExecute[__, {}]["Result"]
	,
	True
	,
	TestID->"PatternMatcherExecute-20251119-SEQ003"
]

(* Standalone BlankNullSequence ___ *)
Test[
	PatternMatcherExecute[___, {1, 2, 3}]["Result"]
	,
	True
	,
	TestID->"PatternMatcherExecute-20251119-SEQ004"
]

Test[
	PatternMatcherExecute[___, {}]["Result"]
	,
	True
	,
	TestID->"PatternMatcherExecute-20251119-SEQ005"
]

(* Trailing sequences: {__, last} *)
Test[
	PatternMatcherExecute[{__, 3}, {1, 2, 3}]["Result"]
	,
	True
	,
	TestID->"PatternMatcherExecute-20251119-SEQ010"
]

Test[
	PatternMatcherExecute[{__, 3}, {3}]["Result"]
	,
	False
	,
	TestID->"PatternMatcherExecute-20251119-SEQ011"
]

Test[
	PatternMatcherExecute[{__, 3}, {}]["Result"]
	,
	False
	,
	TestID->"PatternMatcherExecute-20251119-SEQ012"
]

Test[
	PatternMatcherExecute[{___, 3}, {}]["Result"]
	,
	False
	,
	TestID->"PatternMatcherExecute-20251119-SEQ013"
]

Test[
	PatternMatcherExecute[{__, 4}, {1, 2, 3}]["Result"]
	,
	False
	,
	TestID->"PatternMatcherExecute-20251119-SEQ014"
]

(* Named sequence binding *)
TestMatch[
	PatternMatcherExecute[a__, {1, 2, 3}]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`a" -> {1, 2, 3}|>|>
	,
	TestID->"PatternMatcherExecute-20251119-SEQ015"
]

Test[
	PatternMatcherExecute[{a__}, {1, 2, 3}]["Result"]
	,
	True
	,
	TestID->"PatternMatcherExecute-20251119-SEQ016-REMOVED"
]

TestMatch[
	PatternMatcherExecute[{a__, 3}, {1, 2, 3}]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`a" -> Sequence[1, 2]|>|>
	,
	TestID->"PatternMatcherExecute-20251119-SEQ017"
]

Test[
	PatternMatcherExecute[{a__, 3}, {3}]["Result"]
	,
	False
	,
	TestID->"PatternMatcherExecute-20251119-SEQ018"
]

Test[
	PatternMatcherExecute[{___, 3}, {3}]["Result"]
	,
	True
	,
	TestID->"PatternMatcherExecute-20251119-SEQ018B"
]

(* Leading patterns: {first, __} *)
Test[
	PatternMatcherExecute[{1, __}, {1, 2, 3}]["Result"]
	,
	True
	,
	TestID->"PatternMatcherExecute-20251119-SEQ019"
]

Test[
	PatternMatcherExecute[{1, __}, {1}]["Result"]
	,
	False
	,
	TestID->"PatternMatcherExecute-20251119-SEQ020"
]

Test[
	PatternMatcherExecute[{1, ___}, {1}]["Result"]
	,
	True
	,
	TestID->"PatternMatcherExecute-20251119-SEQ021"
]

TestMatch[
	PatternMatcherExecute[{x_, y__}, {1, 2, 3, 4}]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`y" -> Sequence[2, 3, 4], "TestContext`x" -> 1|>|>
	,
	TestID->"PatternMatcherExecute-20251119-SEQ022"
]

TestMatch[
	PatternMatcherExecute[{x_, y__}, {1, 2}]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`y" -> Sequence[2], "TestContext`x" -> 1|>|>
	,
	TestID->"PatternMatcherExecute-20251119-SEQ023"
]

(* Typed sequences in patterns *)
Test[
	PatternMatcherExecute[{__Integer, 4}, {1, 2, 3, 4}]["Result"]
	,
	True
	,
	TestID->"PatternMatcherExecute-20251119-SEQ024"
]

Test[
	PatternMatcherExecute[{__Integer, 4}, {1, 2.5, 3, 4}]["Result"]
	,
	False
	,
	TestID->"PatternMatcherExecute-20251119-SEQ025"
]

TestMatch[
	PatternMatcherExecute[{_, __Integer}, {a, 1, 2, 3}]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <||>|>
	,
	TestID->"PatternMatcherExecute-20251119-SEQ026"
]

Test[
	PatternMatcherExecute[{__Integer, _Symbol}, {1, 2, 3, x}]["Result"]
	,
	True
	,
	TestID->"PatternMatcherExecute-20251119-SEQ027"
]

(* Named typed sequences *)
TestMatch[
	PatternMatcherExecute[{x_, a__Integer}, {0, 1, 2, 3}]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`a" -> Sequence[1, 2, 3], "TestContext`x" -> 0|>|>
	,
	TestID->"PatternMatcherExecute-20251119-SEQ028"
]

TestMatch[
	PatternMatcherExecute[{a__Symbol, x_Integer}, {a, b, c, 42}]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`x" -> 42, "TestContext`a" -> Sequence[a, b, c]|>|>
	,
	TestID->"PatternMatcherExecute-20251119-SEQ029"
]

(* BlankNullSequence with bindings *)
TestMatch[
	PatternMatcherExecute[{a___, 3}, {3}]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`a" -> Sequence[]|>|>
	,
	TestID->"PatternMatcherExecute-20251119-SEQ030"
]

TestMatch[
	PatternMatcherExecute[{a___, 3}, {1, 2, 3}]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`a" -> Sequence[1, 2]|>|>
	,
	TestID->"PatternMatcherExecute-20251119-SEQ031"
]

(* Multiple patterns with BlankNullSequence *)
TestMatch[
	PatternMatcherExecute[{x_, a___, y_}, {1, 2}]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`y" -> 2, "TestContext`a" -> Sequence[], "TestContext`x" -> 1|>|>
	,
	TestID->"PatternMatcherExecute-20251119-SEQ031B"
]

TestMatch[
	PatternMatcherExecute[{x_, a___, y_}, {1, 2, 3, 4}]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`y" -> 4, "TestContext`a" -> Sequence[2, 3], "TestContext`x" -> 1|>|>
	,
	TestID->"PatternMatcherExecute-20251119-SEQ031C"
]

(* Edge cases *)
Test[
	PatternMatcherExecute[{__}, {}]["Result"]
	,
	False
	,
	TestID->"PatternMatcherExecute-20251119-SEQ032"
]

Test[
	PatternMatcherExecute[{___}, {}]["Result"]
	,
	True
	,
	TestID->"PatternMatcherExecute-20251119-SEQ033"
]

Test[
	PatternMatcherExecute[{1, 2, __}, {1, 2, 3, 4}]["Result"]
	,
	True
	,
	TestID->"PatternMatcherExecute-20251119-SEQ034"
]

Test[
	PatternMatcherExecute[{__, 3, 4}, {1, 2, 3, 4}]["Result"]
	,
	True
	,
	TestID->"PatternMatcherExecute-20251119-SEQ035"
]

TestMatch[
	PatternMatcherExecute[{a_, b__, c_}, {1, 2, 3, 4}]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`c" -> 4, "TestContext`b" -> Sequence[2, 3], "TestContext`a" -> 1|>|>
	,
	TestID->"PatternMatcherExecute-20251119-SEQ036"
]

TestMatch[
	PatternMatcherExecute[{a_, b__, c_}, {1, 2, 3}]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`c" -> 3, "TestContext`b" -> Sequence[2], "TestContext`a" -> 1|>|>
	,
	TestID->"PatternMatcherExecute-20251119-SEQ037"
]

(* Nested sequences in expressions *)
Test[
	PatternMatcherExecute[f[__], f[1, 2, 3]]["Result"]
	,
	True
	,
	TestID->"PatternMatcherExecute-20251119-SEQ038"
]

Test[
	PatternMatcherExecute[f[__Integer], f[1, 2, 3]]["Result"]
	,
	True
	,
	TestID->"PatternMatcherExecute-20251119-SEQ039"
]

Test[
	PatternMatcherExecute[f[x_, __], f[1, 2, 3]]["Result"]
	,
	True
	,
	TestID->"PatternMatcherExecute-20251119-SEQ040"
]

(* Sequences in other heads *)
TestMatch[
	PatternMatcherExecute[f[a__, 3], f[1, 2, 3]]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`a" -> Sequence[1, 2]|>|>
	,
	TestID->"PatternMatcherExecute-20251119-SEQ041"
]

TestMatch[
	PatternMatcherExecute[g[x_, y__], g[1, 2, 3, 4]]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`y" -> Sequence[2, 3, 4], "TestContext`x" -> 1|>|>
	,
	TestID->"PatternMatcherExecute-20251119-SEQ042"
]

(* Minimum length edge cases *)
Test[
	PatternMatcherExecute[{a_, b__}, {1}]["Result"]
	,
	False
	,
	TestID->"PatternMatcherExecute-20251119-SEQ043"
]

Test[
	PatternMatcherExecute[{a_, b__}, {1, 2}]["Result"]
	,
	True
	,
	TestID->"PatternMatcherExecute-20251119-SEQ044"
]

TestMatch[
	PatternMatcherExecute[{a_, b___, c_}, {1, 2}]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`c" -> 2, "TestContext`b" -> Sequence[], "TestContext`a" -> 1|>|>
	,
	TestID->"PatternMatcherExecute-20251119-SEQ045"
]

(* Leading sequences with bindings *)
TestMatch[
	PatternMatcherExecute[{a__, x_, y_}, {1, 2, 3, 4}]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`y" -> 4, "TestContext`x" -> 3, "TestContext`a" -> Sequence[1, 2]|>|>
	,
	TestID->"PatternMatcherExecute-20251119-SEQ046"
]

TestMatch[
	PatternMatcherExecute[{a__, x_}, {1, 2, 3}]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`x" -> 3, "TestContext`a" -> Sequence[1, 2]|>|>
	,
	TestID->"PatternMatcherExecute-20251119-SEQ047"
]

(* Single element sequences *)
TestMatch[
	PatternMatcherExecute[{a__}, {1}]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`a" -> Sequence[1]|>|>
	,
	TestID->"PatternMatcherExecute-20251119-SEQ048"
]

(* Standalone typed sequence: should check expression head, not parts *)
Test[
	PatternMatcherExecute[x___Integer, f[1, 2, 3]]["Result"]
	,
	False
	,
	TestID->"PatternMatcherExecute-20251119-SEQ049"
]

TestMatch[
	PatternMatcherExecute[x___Integer, 5]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`x" -> 5|>|>
	,
	TestID->"PatternMatcherExecute-20251119-SEQ050"
]

Test[
	PatternMatcherExecute[x__Real, 3.14]["Result"]
	,
	True
	,
	TestID->"PatternMatcherExecute-20251119-SEQ051"
]

Test[
	PatternMatcherExecute[x__Real, 5]["Result"]
	,
	False
	,
	TestID->"PatternMatcherExecute-20251119-SEQ052"
]


(*==============================================================================
	Alternatives
==============================================================================*)
TestMatch[
	PatternMatcherExecute[_ | _, 5]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <||>|>
	,
	TestID->"PatternMatcherExecute-20251024-K3S2G7"
]

TestMatch[
	PatternMatcherExecute[_Real | _Integer, 5]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <||>|>
	,
	TestID->"PatternMatcherExecute-20251024-B0M0W6"
]

TestMatch[
	PatternMatcherExecute[f[_ | _], f[5]]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <||>|>
	,
	TestID->"PatternMatcherExecute-20251024-Y9R6W0"
]


(*
	f[_Integer | _Real]
*)
TestMatch[
	PatternMatcherExecute[f[_Integer | _Real], f[5]]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <||>|>
	,
	TestID->"PatternMatcherExecute-20251024-Y3E3Q6"
]

TestMatch[
	PatternMatcherExecute[f[_Integer | _Real], f["string"]]
	,
	<|"Result" -> False, "CyclesExecuted" -> _, "Bindings" -> <||>|>
	,
	TestID->"PatternMatcherExecute-20251024-P1Q7O6"
]


TestMatch[
	PatternMatcherExecute[f[_Integer | x], f[x]]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <||>|>
	,
	TestID->"PatternMatcherExecute-20251114-P9I8K4"
]


(*
	f[x_Integer | x_Real]
*)
TestMatch[
	PatternMatcherExecute[f[x_Integer | x_Real], f[5]]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`x" -> 5|>|>
	,
	TestID->"PatternMatcherExecute-20251114-A7K4O4"
]

TestMatch[
	PatternMatcherExecute[f[x_Integer | x_Real], f[5.5]]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`x" -> 5.5|>|>
	,
	TestID->"PatternMatcherExecute-20251114-X8J5D4"
]


(*
	_Integer?EvenQ | _Real
*)
TestMatch[
	PatternMatcherExecute[_Integer?EvenQ | _Real, 3.5]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <||>|>
	,
	TestID->"PatternMatcherExecute-20251115-H6L7D3"
]

TestMatch[
	PatternMatcherExecute[_Integer?EvenQ | _Real, 4]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <||>|>
	,
	TestID->"PatternMatcherExecute-20251115-B8J2M3"
]


(*
	x_Integer | x_Real
*)
TestMatch[
	PatternMatcherExecute[x_Integer | x_Real, 5]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`x" -> 5|>|>
	,
	TestID->"PatternMatcherExecute-20251115-W2A7R3"
]

TestMatch[
	PatternMatcherExecute[x_Integer | x_Real, 5.5]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`x" -> 5.5|>|>
	,
	TestID->"PatternMatcherExecute-20251115-P8U2G2"
]

TestMatch[
	PatternMatcherExecute[f[x_Integer | x_Real, x_], f[3.5, 3.5]]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`x" -> 3.5|>|>
	,
	TestID->"PatternMatcherExecute-20251115-Q2N3W2"
]

TestMatch[
	PatternMatcherExecute[f[x_Integer | x_Real, x_], f[3.5, 5]]
	,
	<|"Result" -> False, "CyclesExecuted" -> _, "Bindings" -> <||>|>
	,
	TestID->"PatternMatcherExecute-20251115-O1D4L0"
]


(*
	x_Integer | y_Real | z_String
*)
TestMatch[
	PatternMatcherExecute[x_Integer | y_Real | z_String, 5]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`x" -> 5|>|>
	,
	TestID->"PatternMatcherExecute-20251115-F0H4V4"
]

TestMatch[
	PatternMatcherExecute[x_Integer | y_Real | z_String, 5.5]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`y" -> 5.5|>|>
	,
	TestID->"PatternMatcherExecute-20251115-J4W5W5"
]

TestMatch[
	PatternMatcherExecute[x_Integer | y_Real | z_String, "a string"]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`z" -> "a string"|>|>
	,
	TestID->"PatternMatcherExecute-20251115-F4M1Q2"
]

TestMatch[
	PatternMatcherExecute[x_Integer | y_Real | z_String, notastring]
	,
	<|"Result" -> False, "CyclesExecuted" -> _, "Bindings" -> <||>|>
	,
	TestID->"PatternMatcherExecute-20251115-E2O4N3"
]


(*==============================================================================
	Condition Patterns (pattern /; test)
==============================================================================*)

(* Basic Condition with Blank *)
TestMatch[
	PatternMatcherExecute[x_ /; x > 0, 5]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`x" -> 5|>|>
	,
	TestID->"PatternMatcherExecute-20251120-C1A0B0"
]

TestMatch[
	PatternMatcherExecute[x_ /; x > 0, -5]
	,
	<|"Result" -> False, "CyclesExecuted" -> _, "Bindings" -> <||>|>
	,
	TestID->"PatternMatcherExecute-20251120-C2A0B0"
]

TestMatch[
	PatternMatcherExecute[x_ /; x > 0, 0]
	,
	<|"Result" -> False, "CyclesExecuted" -> _, "Bindings" -> <||>|>
	,
	TestID->"PatternMatcherExecute-20251120-C3A0B0"
]


(* Condition with Typed Blank *)
TestMatch[
	PatternMatcherExecute[x_Integer /; x > 0, 5]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`x" -> 5|>|>
	,
	TestID->"PatternMatcherExecute-20251120-C4A0B0"
]

TestMatch[
	PatternMatcherExecute[x_Integer /; x > 0, -5]
	,
	<|"Result" -> False, "CyclesExecuted" -> _, "Bindings" -> <||>|>
	,
	TestID->"PatternMatcherExecute-20251120-C5A0B0"
]

TestMatch[
	PatternMatcherExecute[x_Integer /; x > 0, 5.5]
	,
	<|"Result" -> False, "CyclesExecuted" -> _, "Bindings" -> <||>|>
	,
	TestID->"PatternMatcherExecute-20251120-C6A0B0"
]


(* Condition with Multiple Variables *)
TestMatch[
	PatternMatcherExecute[{x_, y_} /; x < y, {1, 2}]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`y" -> 2, "TestContext`x" -> 1|>|>
	,
	TestID->"PatternMatcherExecute-20251120-C7A0B0"
]

TestMatch[
	PatternMatcherExecute[{x_, y_} /; x < y, {2, 1}]
	,
	<|"Result" -> False, "CyclesExecuted" -> _, "Bindings" -> <||>|>
	,
	TestID->"PatternMatcherExecute-20251120-C8A0B0"
]

TestMatch[
	PatternMatcherExecute[{x_, y_} /; x < y, {1, 1}]
	,
	<|"Result" -> False, "CyclesExecuted" -> _, "Bindings" -> <||>|>
	,
	TestID->"PatternMatcherExecute-20251120-C9A0B0"
]


(* Condition with Function Call *)
TestMatch[
	PatternMatcherExecute[x_Integer /; EvenQ[x], 4]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`x" -> 4|>|>
	,
	TestID->"PatternMatcherExecute-20251120-C10A0B0"
]

TestMatch[
	PatternMatcherExecute[x_Integer /; EvenQ[x], 5]
	,
	<|"Result" -> False, "CyclesExecuted" -> _, "Bindings" -> <||>|>
	,
	TestID->"PatternMatcherExecute-20251120-C11A0B0"
]

TestMatch[
	PatternMatcherExecute[x_Integer /; PrimeQ[x], 7]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`x" -> 7|>|>
	,
	TestID->"PatternMatcherExecute-20251120-C12A0B0"
]

TestMatch[
	PatternMatcherExecute[x_Integer /; PrimeQ[x], 8]
	,
	<|"Result" -> False, "CyclesExecuted" -> _, "Bindings" -> <||>|>
	,
	TestID->"PatternMatcherExecute-20251120-C13A0B0"
]


(* Condition with Complex Expression *)
TestMatch[
	PatternMatcherExecute[x_Integer /; Mod[x, 3] == 0, 9]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`x" -> 9|>|>
	,
	TestID->"PatternMatcherExecute-20251120-C14A0B0"
]

TestMatch[
	PatternMatcherExecute[x_Integer /; Mod[x, 3] == 0, 10]
	,
	<|"Result" -> False, "CyclesExecuted" -> _, "Bindings" -> <||>|>
	,
	TestID->"PatternMatcherExecute-20251120-C15A0B0"
]


(* Condition with Nested Pattern *)
TestMatch[
	PatternMatcherExecute[f[x_Integer /; x > 0], f[5]]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`x" -> 5|>|>
	,
	TestID->"PatternMatcherExecute-20251120-C16A0B0"
]

TestMatch[
	PatternMatcherExecute[f[x_Integer /; x > 0], f[-5]]
	,
	<|"Result" -> False, "CyclesExecuted" -> _, "Bindings" -> <||>|>
	,
	TestID->"PatternMatcherExecute-20251120-C17A0B0"
]

TestMatch[
	PatternMatcherExecute[f[x_Integer /; x > 0], f[5.5]]
	,
	<|"Result" -> False, "CyclesExecuted" -> _, "Bindings" -> <||>|>
	,
	TestID->"PatternMatcherExecute-20251120-C18A0B0"
]


(* Multiple Conditions in Same Pattern *)
TestMatch[
	PatternMatcherExecute[{x_Integer /; x > 0, y_Integer /; y > 0}, {3, 5}]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`y" -> 5, "TestContext`x" -> 3|>|>
	,
	TestID->"PatternMatcherExecute-20251120-C19A0B0"
]

TestMatch[
	PatternMatcherExecute[{x_Integer /; x > 0, y_Integer /; y > 0}, {3, -5}]
	,
	<|"Result" -> False, "CyclesExecuted" -> _, "Bindings" -> <||>|>
	,
	TestID->"PatternMatcherExecute-20251120-C20A0B0"
]

TestMatch[
	PatternMatcherExecute[{x_Integer /; x > 0, y_Integer /; y > 0}, {-3, 5}]
	,
	<|"Result" -> False, "CyclesExecuted" -> _, "Bindings" -> <||>|>
	,
	TestID->"PatternMatcherExecute-20251120-C21A0B0"
]


(* Condition That Evaluates to Non-Boolean (should fail) *)
TestMatch[
	PatternMatcherExecute[x_ /; x, 5]
	,
	<|"Result" -> False, "CyclesExecuted" -> _, "Bindings" -> <||>|>
	,
	TestID->"PatternMatcherExecute-20251120-C22A0B0"
]

TestMatch[
	PatternMatcherExecute[x_ /; "string", 5]
	,
	<|"Result" -> False, "CyclesExecuted" -> _, "Bindings" -> <||>|>
	,
	TestID->"PatternMatcherExecute-20251120-C23A0B0"
]


(* Condition with String Operations *)
TestMatch[
	PatternMatcherExecute[s_String /; StringLength[s] > 3, "hello"]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`s" -> "hello"|>|>
	,
	TestID->"PatternMatcherExecute-20251120-C24A0B0"
]

TestMatch[
	PatternMatcherExecute[s_String /; StringLength[s] > 3, "hi"]
	,
	<|"Result" -> False, "CyclesExecuted" -> _, "Bindings" -> <||>|>
	,
	TestID->"PatternMatcherExecute-20251120-C25A0B0"
]


(* Condition with List Operations *)
TestMatch[
	PatternMatcherExecute[{x_, y_, z_} /; x + y == z, {3, 4, 7}]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`z" -> 7, "TestContext`y" -> 4, "TestContext`x" -> 3|>|>
	,
	TestID->"PatternMatcherExecute-20251120-C26A0B0"
]

TestMatch[
	PatternMatcherExecute[{x_, y_, z_} /; x + y == z, {3, 4, 8}]
	,
	<|"Result" -> False, "CyclesExecuted" -> _, "Bindings" -> <||>|>
	,
	TestID->"PatternMatcherExecute-20251120-C27A0B0"
]


(* Condition with Range Check *)
TestMatch[
	test[x_] := 0 <= x <= 100;
	PatternMatcherExecute[x_Integer /; test[x], 50]
	,
	<|"Result" -> True, "CyclesExecuted" -> _, "Bindings" -> <|"TestContext`x" -> 50|>|>
	,
	TestID->"PatternMatcherExecute-20251120-C28A0B0"
]

TestMatch[
	PatternMatcherExecute[x_Integer /; test[x], 101]
	,
	<|"Result" -> False, "CyclesExecuted" -> _, "Bindings" -> <||>|>
	,
	TestID->"PatternMatcherExecute-20251120-C29A0B0"
]

TestMatch[
	PatternMatcherExecute[x_Integer /; test[x], -1]
	,
	<|"Result" -> False, "CyclesExecuted" -> _, "Bindings" -> <||>|>
	,
	TestID->"PatternMatcherExecute-20251120-C30A0B0"
]


TestStatePop[Global`contextState]


EndTestSection[]
