If[FindFile["tests/CustomLoad.m"] =!= $Failed,
	Get["tests/CustomLoad.m"]
]


Needs["DanielS`PatternMatcher`Utilities`TestSuiteUtilities`"]

BeginTestSection[$CurrentTestSource]


Needs["DanielS`PatternMatcher`"]
Needs["DanielS`PatternMatcher`BackEnd`VirtualMachine`"]


Global`contextState = TestStatePush[]


(*==============================================================================
	CreatePatternMatcherVirtualMachine[]
==============================================================================*)
Test[
	vm = CreatePatternMatcherVirtualMachine[];
	PatternMatcherVirtualMachineQ[vm]
	,
	True
	,
	TestID->"PatternMatcherVirtualMachine-20251022-O0Q3F5"
]


Test[
	vm["toBoxes", StandardForm] // Head
	,
	InterpretationBox
	,
	TestID->"PatternMatcherVirtualMachine-20251022-I1U1O3"
]


Test[
	vm["isInitialized"]
	,
	False
	,
	TestID->"PatternMatcherVirtualMachine-20251022-R7D8C8"
]

Test[
	vm["isHalted"]
	,
	False
	,
	TestID->"PatternMatcherVirtualMachine-20251022-O3O2R0"
]

Test[
	vm["getCycles"]
	,
	0
	,
	TestID->"PatternMatcherVirtualMachine-20251022-Q2C4S3"
]

Test[
	vm["getPC"]
	,
	0
	,
	TestID->"PatternMatcherVirtualMachine-20251022-Y0E1B2"
]

Test[
	vm["getBytecode"]
	,
	None
	,
	TestID->"PatternMatcherVirtualMachine-20251022-Z5P4I2"
]

(*==============================================================================
	CreatePatternMatcherVirtualMachine[patt]
==============================================================================*)
Test[
	vm1 = CreatePatternMatcherVirtualMachine[f[x_, y_, x_]];
	PatternMatcherVirtualMachineQ[vm1]
	,
	True
	,
	TestID->"PatternMatcherVirtualMachine-20251022-O3J2J7"
]

Test[
	vm1["isInitialized"]
	,
	True
	,
	TestID->"PatternMatcherVirtualMachine-20251022-A5K1H9"
]

Test[
	vm1["isHalted"]
	,
	False
	,
	TestID->"PatternMatcherVirtualMachine-20251022-F6G0H8"
]

Test[
	vm1["getCycles"]
	,
	0
	,
	TestID->"PatternMatcherVirtualMachine-20251022-B7X3G8"
]

Test[
	vm1["getPC"]
	,
	0
	,
	TestID->"PatternMatcherVirtualMachine-20251022-P3R7Q7"
]

Test[
	PatternBytecodeQ[vm1["getBytecode"]]
	,
	True
	,
	TestID->"PatternMatcherVirtualMachine-20251022-U6N1G3"
]


Test[
	ResetPatternMatcherVirtualMachine[vm1];
	vm1["match", f[1, 2, 1]]
	,
	True
	,
	TestID->"PatternMatcherVirtualMachine-20251022-O6G8S4"
]


Test[
	ResetPatternMatcherVirtualMachine[vm1];
	vm1["match", f[1, 2, 3]]
	,
	False
	,
	TestID->"PatternMatcherVirtualMachine-20251022-W8O4O3"
]


TestStatePop[Global`contextState]


EndTestSection[]
