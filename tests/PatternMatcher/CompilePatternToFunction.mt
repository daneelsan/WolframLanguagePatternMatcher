If[FindFile["tests/CustomLoad.m"] =!= $Failed,
	Get["tests/CustomLoad.m"]
]


Needs["DanielS`PatternMatcher`Utilities`TestSuiteUtilities`"]

BeginTestSection[$CurrentTestSource]


Needs["DanielS`PatternMatcher`"]


Global`contextState = TestStatePush[]


(*==============================================================================
	Blank
==============================================================================*)
Test[
	fun = CompilePatternToFunction[_]
	,
	Function[vm`expr$, True]
	,
	TestID->"CompilePatternToFunction-20251022-B6H7J8"
]

Test[
	fun[anyexpr]
	,
	True
	,
	TestID->"CompilePatternToFunction-20251022-N2H5B9"
]


Test[
	fun = CompilePatternToFunction[_String]
	,
	Function[vm`expr$, Head[vm`expr$] === String]
	,
	TestID->"CompilePatternToFunction-20251022-G5X5P2"
]

Test[
	fun /@ {"True", False}
	,
	{True, False}
	,
	TestID->"CompilePatternToFunction-20251022-R1H9U2"
]


(*==============================================================================
	Pattern
==============================================================================*)
Test[
	fun = CompilePatternToFunction[x_]
	,
	Function[vm`expr$, True]
	,
	TestID->"CompilePatternToFunction-20251022-B7N0D8"
]

Test[
	fun[Symbol]
	,
	True
	,
	TestID->"CompilePatternToFunction-20251022-Y1B2I9"
]


Test[
	fun = CompilePatternToFunction[x_Integer]
	,
	Function[vm`expr$, Head[vm`expr$] === Integer]
	,
	TestID->"CompilePatternToFunction-20251022-M1F2W6"
]

Test[
	fun /@ {42, "42", Integer["42"]}
	,
	{True, False, True}
	,
	TestID->"CompilePatternToFunction-20251022-G0L0J4"
]


(*==============================================================================
	PatternTest
==============================================================================*)
Test[
	fun = CompilePatternToFunction[_?StringQ]
	,
	Function[vm`expr$, True && TrueQ[StringQ[vm`expr$]]]
	,
	TestID->"CompilePatternToFunction-20251022-C2J4N2"
]

Test[
	fun /@ {String[5], "5"}
	,
	{False, True}
	,
	TestID->"CompilePatternToFunction-20251022-T9T7W3"
]


Test[
	fun = CompilePatternToFunction[x_?StringQ]
	,
	Function[vm`expr$, True && TrueQ[StringQ[vm`expr$]]]
	,
	TestID->"CompilePatternToFunction-20251022-N9R0J8"
]

Test[
	fun /@ {42, "42"}
	,
	{False, True}
	,
	TestID->"CompilePatternToFunction-20251022-V3F4T8"
]


Test[
	fun = CompilePatternToFunction[x_String?StringQ]
	,
	Function[vm`expr$, Head[vm`expr$] === String && TrueQ[StringQ[vm`expr$]]]
	,
	TestID->"CompilePatternToFunction-20251022-N3P3O0"
]

Test[
	fun /@ {String[5], "5"}
	,
	{False, True}
	,
	TestID->"CompilePatternToFunction-20251022-D2B9I6"
]


(*==============================================================================
	Except
==============================================================================*)
Test[
	fun = CompilePatternToFunction[Except[_]]
	,
	Function[vm`expr$, ! True]
	,
	TestID->"CompilePatternToFunction-20251022-A6Q9K7"
]

Test[
	fun /@ {a, 2, {}}
	,
	{False, False, False}
	,
	TestID->"CompilePatternToFunction-20251022-M3F2P0"
]


Test[
	fun = CompilePatternToFunction[Except[_String]]
	,
	Function[vm`expr$, ! Head[vm`expr$] === String]
	,
	TestID->"CompilePatternToFunction-20251022-R3Q3A5"
]

Test[
	fun /@ {1, "2"}
	,
	{True, False}
	,
	TestID->"CompilePatternToFunction-20251022-T2C8O8"
]


Test[
	fun = CompilePatternToFunction[Except[_String?StringQ]]
	,
	Function[vm`expr$, ! (Head[vm`expr$] === String && TrueQ[StringQ[vm`expr$]])]
	,
	TestID->"CompilePatternToFunction-20251022-O1F5K8"
]

Test[
	fun /@ {1, "2"}
	,
	{True, False}
	,
	TestID->"CompilePatternToFunction-20251022-R4R7V3"
]


Test[
	fun = CompilePatternToFunction[Except[_String, _Integer]]
	,
	Function[vm`expr$, ! Head[vm`expr$] === String && Head[vm`expr$] === Integer]
	,
	TestID->"CompilePatternToFunction-20251022-M8D1Y3"
]

Test[
	fun /@ {"1", 2, z}
	,
	{False, True, False}
	,
	TestID->"CompilePatternToFunction-20251022-B6U5X3"
]


(*==============================================================================
	Alternatives
==============================================================================*)
Test[
	fun = CompilePatternToFunction[_Integer | _String]
	,
	Function[vm`expr$, Head[vm`expr$] === Integer || Head[vm`expr$] === String]
	,
	TestID->"CompilePatternToFunction-20251022-O9F6D8"
]

Test[
	fun /@ {"1", 2, z}
	,
	{True, True, False}
	,
	TestID->"CompilePatternToFunction-20251022-W7T6V4"
]


Test[
	fun = CompilePatternToFunction[_Integer | _String?StringQ]
	,
	Function[vm`expr$, Head[vm`expr$] === Integer || (Head[vm`expr$] === String && TrueQ[StringQ[vm`expr$]])]
	,
	TestID->"CompilePatternToFunction-20251022-Z1E9M8"
]

Test[
	fun /@ {1, "2", z}
	,
	{True, True, False}
	,
	TestID->"CompilePatternToFunction-20251022-V7W3W1"
]


(*==============================================================================
	TODO: BlankSequence, BlankNullSequence, Repeated, RepeatedNull, PatternSequence
==============================================================================*)


(*==============================================================================
	Literal
==============================================================================*)
Test[
	fun = CompilePatternToFunction[5]
	,
	Function[vm`expr$, vm`expr$ === 5]
	,
	TestID->"CompilePatternToFunction-20251022-R8F6R5"
]

Test[
	fun /@ {5, 6}
	,
	{True, False}
	,
	TestID->"CompilePatternToFunction-20251022-E9K6Q7"
]


Test[
	fun = CompilePatternToFunction[f[x, y]]
	,
	Function[vm`expr$, vm`expr$ === f[x, y]]
	,
	TestID->"CompilePatternToFunction-20251022-G8Z5C7"
]

Test[
	fun /@ {f[x, y], f[x, x]}
	,
	{True, False}
	,
	TestID->"CompilePatternToFunction-20251022-D2J4C0"
]


(*==============================================================================
	Normal
==============================================================================*)
Test[
	fun = CompilePatternToFunction[f[x_]]
	,
	Function[vm`expr$,
		Module[{var1, var2},
			And[
				Length[vm`expr$] === 1
				,
				var1 = Head[vm`expr$];
				var1 === f
				,
				var2 = vm`expr$[[1]];
				True
			]
		]
	]
	,
	TestID->"CompilePatternToFunction-20251022-C8V4K7"
]

Test[
	fun /@ {f[1], f[1, 2]}
	,
	{True, False}
	,
	TestID->"CompilePatternToFunction-20251022-X5Q3G3"
]


Test[
	fun = CompilePatternToFunction[f[x_, x_]]
	,
	Function[vm`expr$,
		Module[{var1, var2, var3},
			And[
				Length[vm`expr$] === 2
				,
				var1 = Head[vm`expr$];
				var1 === f
				,
				var2 = vm`expr$[[1]];
				True
				,
				var3 = vm`expr$[[2]];
				var2 === var3
				,
				True
			]
		]
	]
	,
	TestID->"CompilePatternToFunction-20251022-N2X9J5"
]

Test[
	fun /@ {f[1, 1], f[1, 2]}
	,
	{True, False}
	,
	TestID->"CompilePatternToFunction-20251022-B3C1C0"
]


Test[
	fun = CompilePatternToFunction[f[x : (_Integer | _String)]]
	,
	Function[vm`expr$,
		Module[{var1, var2},
			And[
				Length[vm`expr$] === 1
				,
				var1 = Head[vm`expr$];
				var1 === f
				,
				Or[
					var2 = vm`expr$[[1]];
					Head[var2] === Integer
					,
					Head[var2] === String
				]
			]
		]
	]
	,
	TestID->"CompilePatternToFunction-20251022-T7L6Y5"
]

Test[
	fun /@ {f[1], f[g]}
	,
	{True, False}
	,
	TestID->"CompilePatternToFunction-20251022-P6F0F0"
]


Test[
	fun = CompilePatternToFunction[f[x : (_Integer | _String), x_]]
	,
	Function[vm`expr$,
		Module[{var1, var2, var3},
			And[
				Length[vm`expr$] === 2
				,
				var1 = Head[vm`expr$];
				var1 === f
				,
				Or[
					var2 = vm`expr$[[1]];
					Head[var2] === Integer
					,
					Head[var2] === String
				]
				,
				var3 = vm`expr$[[2]];
				var2 === var3
				,
				True
			]
		]
	]
	,
	TestID->"CompilePatternToFunction-20251022-W6R2R4"
]

Test[
	fun /@ {f[1, 1], f["1", "1"], f[1, 2]}
	,
	{True, True, False}
	,
	TestID->"CompilePatternToFunction-20251022-W2K6F0"
]


(*==============================================================================
	Options
==============================================================================*)
(*
	"ApplyOptimizations" -> False
*)
Test[
	CompilePatternToFunction[_, "ApplyOptimizations" -> False]
	,
	Function[vm`expr$, Module[{}, And[True]]]
	,
	TestID->"CompilePatternToFunction-20251022-E6L5U5"
]

Test[
	CompilePatternToFunction[_String, "ApplyOptimizations" -> False]
	,
	Function[vm`expr$, Module[{}, And[Head[vm`expr$] === String]]]
	,
	TestID->"CompilePatternToFunction-20251022-C7V6N3"
]


TestStatePop[Global`contextState]


EndTestSection[]
