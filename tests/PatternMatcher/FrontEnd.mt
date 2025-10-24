If[FindFile["tests/CustomLoad.m"] =!= $Failed,
	Get["tests/CustomLoad.m"]
]


Needs["DanielS`PatternMatcher`Utilities`TestSuiteUtilities`"]

BeginTestSection[$CurrentTestSource]


Needs["DanielS`PatternMatcher`"]


Global`contextState = TestStatePush[]


(*==============================================================================
	Literal
==============================================================================*)
Test[
	bc1 = CompilePatternToBytecode[5];
	PatternBytecodeQ[bc1]
	,
	True
	,
	TestID->"CompilePatternToBytecode-20251022-E6L2K1"
]


(*==============================================================================
	Normal
==============================================================================*)
Test[
	bc2 = CompilePatternToBytecode[f[x_]];
	PatternBytecodeQ[bc2]
	,
	True
	,
	TestID->"CompilePatternToBytecode-20251022-Y3V6S8"
]


TestStatePop[Global`contextState]


EndTestSection[]
