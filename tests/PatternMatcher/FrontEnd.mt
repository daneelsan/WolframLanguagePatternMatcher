If[FindFile["tests/CustomLoad.m"] =!= $Failed,
	Get["tests/CustomLoad.m"]
]


Needs["DanielS`PatternMatcher`Utilities`TestSuiteUtilities`"]

BeginTestSection[$CurrentTestSource]


Needs["DanielS`PatternMatcher`"]


Global`contextState = TestStatePush[]


(*==============================================================================
	Integer
==============================================================================*)
Test[
	bc1 = CompilePatternToBytecode[5];
	PatternBytecodeQ[bc1]
	,
	True
	,
	TestID->"CompilePatternToBytecode-20251022-E6L2K1"
]

Test[
	ToString[bc1]
	,
	"
L0:
 0    BEGIN_BLOCK     Label[0]
 1    MATCH_LITERAL   %e0, Expr[5], Label[1]
 2    JUMP            Label[2]
 3    END_BLOCK       Label[0]

L1:
 4    BEGIN_BLOCK     Label[1]
 5    DEBUG_PRINT     Expr[\"Pattern failed\"]
 6    LOAD_IMM        %b0, 0
 7    HALT            
 8    END_BLOCK       Label[1]

L2:
 9    BEGIN_BLOCK     Label[2]
10    DEBUG_PRINT     Expr[\"Pattern succeeded\"]
11    SAVE_BINDINGS   
12    LOAD_IMM        %b0, 1
13    HALT            
14    END_BLOCK       Label[2]

----------------------------------------
Expr registers: 1, Bool registers: 1
"
	,
	TestID->"FrontEnd-20251114-F0T9I0"
]


(*==============================================================================
	String
==============================================================================*)
Test[
	bc2 = CompilePatternToBytecode["foo"];
	PatternBytecodeQ[bc2]
	,
	True
	,
	TestID->"FrontEnd-20251114-P9R1J8"
]

Test[
	ToString[bc2]
	,
	"
L0:
 0    BEGIN_BLOCK     Label[0]
 1    MATCH_LITERAL   %e0, Expr[\"foo\"], Label[1]
 2    JUMP            Label[2]
 3    END_BLOCK       Label[0]

L1:
 4    BEGIN_BLOCK     Label[1]
 5    DEBUG_PRINT     Expr[\"Pattern failed\"]
 6    LOAD_IMM        %b0, 0
 7    HALT            
 8    END_BLOCK       Label[1]

L2:
 9    BEGIN_BLOCK     Label[2]
10    DEBUG_PRINT     Expr[\"Pattern succeeded\"]
11    SAVE_BINDINGS   
12    LOAD_IMM        %b0, 1
13    HALT            
14    END_BLOCK       Label[2]

----------------------------------------
Expr registers: 1, Bool registers: 1
"
	,
	TestID->"FrontEnd-20251114-H4J5A5"
]


(*==============================================================================
	Symbol
==============================================================================*)
(*
	NOTE: It doesn't matter if the symbol has a own-value/down-value,
	CompilePatternToBytecode is not HoldFirst.
*)
Test[
	bc3 = CompilePatternToBytecode[bar];
	PatternBytecodeQ[bc3]
	,
	True
	,
	TestID->"FrontEnd-20251114-T7X3L8"
]

Test[
	ToString[bc3]
	,
	"
L0:
 0    BEGIN_BLOCK     Label[0]
 1    MATCH_LITERAL   %e0, Expr[bar], Label[1]
 2    JUMP            Label[2]
 3    END_BLOCK       Label[0]

L1:
 4    BEGIN_BLOCK     Label[1]
 5    DEBUG_PRINT     Expr[\"Pattern failed\"]
 6    LOAD_IMM        %b0, 0
 7    HALT            
 8    END_BLOCK       Label[1]

L2:
 9    BEGIN_BLOCK     Label[2]
10    DEBUG_PRINT     Expr[\"Pattern succeeded\"]
11    SAVE_BINDINGS   
12    LOAD_IMM        %b0, 1
13    HALT            
14    END_BLOCK       Label[2]

----------------------------------------
Expr registers: 1, Bool registers: 1
"
	,
	TestID->"FrontEnd-20251114-Z2C8N3"
]


(*==============================================================================
	Normal (no patterns)
==============================================================================*)
Test[
	bc4 = CompilePatternToBytecode[{x, 1, "a"}];
	PatternBytecodeQ[bc4]
	,
	True
	,
	TestID->"FrontEnd-20251114-N4W1G0"
]

Test[
	ToString[bc4]
	,
	"
L0:
 0    BEGIN_BLOCK     Label[0]

L3:
 1    BEGIN_BLOCK     Label[3]
 2    MATCH_LENGTH    %e0, 3, Label[4]
 3    MATCH_HEAD      %e0, Expr[List], Label[4]
 4    MOVE            %e1, %e0
 5    GET_PART        %e2, %e0, 1
 6    MOVE            %e0, %e2
 7    MATCH_LITERAL   %e0, Expr[x], Label[4]
 8    MOVE            %e0, %e1
 9    GET_PART        %e3, %e0, 2
10    MOVE            %e0, %e3
11    MATCH_LITERAL   %e0, Expr[1], Label[4]
12    MOVE            %e0, %e1
13    GET_PART        %e4, %e0, 3
14    MOVE            %e0, %e4
15    MATCH_LITERAL   %e0, Expr[\"a\"], Label[4]
16    MOVE            %e0, %e1
17    END_BLOCK       Label[3]
18    JUMP            Label[2]

L4:
19    END_BLOCK       Label[3]
20    JUMP            Label[1]

L5:
21    END_BLOCK       Label[0]

L1:
22    BEGIN_BLOCK     Label[1]
23    DEBUG_PRINT     Expr[\"Pattern failed\"]
24    LOAD_IMM        %b0, 0
25    HALT            
26    END_BLOCK       Label[1]

L2:
27    BEGIN_BLOCK     Label[2]
28    DEBUG_PRINT     Expr[\"Pattern succeeded\"]
29    SAVE_BINDINGS   
30    LOAD_IMM        %b0, 1
31    HALT            
32    END_BLOCK       Label[2]

----------------------------------------
Expr registers: 5, Bool registers: 1
"
	,
	TestID->"FrontEnd-20251114-A2M8M4"
]


(*==============================================================================
	Blank
==============================================================================*)
Test[
	bc4 = CompilePatternToBytecode[_];
	PatternBytecodeQ[bc4]
	,
	True
	,
	TestID->"FrontEnd-20251114-E2W2W1"
]

Test[
	ToString[bc4]
	,
	"
L0:
 0    BEGIN_BLOCK     Label[0]
 1    JUMP            Label[2]
 2    END_BLOCK       Label[0]

L1:
 3    BEGIN_BLOCK     Label[1]
 4    DEBUG_PRINT     Expr[\"Pattern failed\"]
 5    LOAD_IMM        %b0, 0
 6    HALT            
 7    END_BLOCK       Label[1]

L2:
 8    BEGIN_BLOCK     Label[2]
 9    DEBUG_PRINT     Expr[\"Pattern succeeded\"]
10    SAVE_BINDINGS   
11    LOAD_IMM        %b0, 1
12    HALT            
13    END_BLOCK       Label[2]

----------------------------------------
Expr registers: 1, Bool registers: 1
"
	,
	TestID->"FrontEnd-20251114-H9R8D4"
]


Test[
	bc5 = CompilePatternToBytecode[_Integer];
	PatternBytecodeQ[bc5]
	,
	True
	,
	TestID->"FrontEnd-20251114-V8D5Z1"
]

Test[
	ToString[bc5]
	,
	"
L0:
 0    BEGIN_BLOCK     Label[0]
 1    MATCH_HEAD      %e0, Expr[Integer], Label[1]
 2    JUMP            Label[2]
 3    END_BLOCK       Label[0]

L1:
 4    BEGIN_BLOCK     Label[1]
 5    DEBUG_PRINT     Expr[\"Pattern failed\"]
 6    LOAD_IMM        %b0, 0
 7    HALT            
 8    END_BLOCK       Label[1]

L2:
 9    BEGIN_BLOCK     Label[2]
10    DEBUG_PRINT     Expr[\"Pattern succeeded\"]
11    SAVE_BINDINGS   
12    LOAD_IMM        %b0, 1
13    HALT            
14    END_BLOCK       Label[2]

----------------------------------------
Expr registers: 1, Bool registers: 1
"
	,
	TestID->"FrontEnd-20251114-T8R6E8"
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
