BeginTestSection[$CurrentTestSource]


$testRunTimeEnd = AbsoluteTime[]
$testRunMemoryEnd = MemoryInUse[]


Print["Stats ----------------------------------------------------------------"]
Print["Time to run tests: ", $testRunTimeEnd - $testRunTimeStart]
Print["Memory to run tests: ", $testRunMemoryEnd - $testRunMemoryStart]

Print["Test End -------------------------------------------------------------"]


EndTestSection[]