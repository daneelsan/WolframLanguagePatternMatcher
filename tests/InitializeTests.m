If[FindFile["tests/CustomLoad.m"] =!= $Failed,
	Get["tests/CustomLoad.m"]
]


Print["$ProcessID: ", $ProcessID]

Print["$Version: ", $Version]


$testRunMemoryStart = MemoryInUse[]

$testRunTimeStart = AbsoluteTime[]

Print["Test Start -----------------------------------------------------------"]
