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
 4    DEBUG_PRINT     Expr[\"Pattern failed\"]
 5    LOAD_IMM        %b0, 0
 6    HALT            

L2:
 7    DEBUG_PRINT     Expr[\"Pattern succeeded\"]
 8    SAVE_BINDINGS   
 9    LOAD_IMM        %b0, 1
10    HALT            

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
 4    DEBUG_PRINT     Expr[\"Pattern failed\"]
 5    LOAD_IMM        %b0, 0
 6    HALT            

L2:
 7    DEBUG_PRINT     Expr[\"Pattern succeeded\"]
 8    SAVE_BINDINGS   
 9    LOAD_IMM        %b0, 1
10    HALT            

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
 4    DEBUG_PRINT     Expr[\"Pattern failed\"]
 5    LOAD_IMM        %b0, 0
 6    HALT            

L2:
 7    DEBUG_PRINT     Expr[\"Pattern succeeded\"]
 8    SAVE_BINDINGS   
 9    LOAD_IMM        %b0, 1
10    HALT            

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
	bc4 = CompilePatternToBytecode[f[]];
	PatternBytecodeQ[bc4]
	,
	True
	,
	TestID->"FrontEnd-20251114-L3C1Q6"
]

Test[
	ToString[bc4]
	,
	"
L0:
 0    BEGIN_BLOCK     Label[0]

L3:
 1    BEGIN_BLOCK     Label[3]
 2    MATCH_LENGTH    %e0, 0, Label[4]
 3    MATCH_HEAD      %e0, Expr[f], Label[4]
 4    MOVE            %e1, %e0
 5    END_BLOCK       Label[3]
 6    JUMP            Label[2]

L4:
 7    JUMP            Label[1]

L5:
 8    END_BLOCK       Label[0]

L1:
 9    DEBUG_PRINT     Expr[\"Pattern failed\"]
10    LOAD_IMM        %b0, 0
11    HALT            

L2:
12    DEBUG_PRINT     Expr[\"Pattern succeeded\"]
13    SAVE_BINDINGS   
14    LOAD_IMM        %b0, 1
15    HALT            

----------------------------------------
Expr registers: 2, Bool registers: 1
"
	,
	TestID->"FrontEnd-20251114-L9P2M7"
]


Test[
	bc5 = CompilePatternToBytecode[{x, 1, "a"}];
	PatternBytecodeQ[bc5]
	,
	True
	,
	TestID->"FrontEnd-20251114-N4W1G0"
]

Test[
	ToString[bc5]
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
19    JUMP            Label[1]

L5:
20    END_BLOCK       Label[0]

L1:
21    DEBUG_PRINT     Expr[\"Pattern failed\"]
22    LOAD_IMM        %b0, 0
23    HALT            

L2:
24    DEBUG_PRINT     Expr[\"Pattern succeeded\"]
25    SAVE_BINDINGS   
26    LOAD_IMM        %b0, 1
27    HALT            

----------------------------------------
Expr registers: 5, Bool registers: 1
"
	,
	TestID->"FrontEnd-20251114-A2M8M4"
]


Test[
	bc6 = CompilePatternToBytecode[f[g[1], 2]];
	PatternBytecodeQ[bc6]
	,
	True
	,
	TestID->"FrontEnd-20251114-P6O0B3"
]

Test[
	ToString[bc6]
	,
	"
L0:
 0    BEGIN_BLOCK     Label[0]

L3:
 1    BEGIN_BLOCK     Label[3]
 2    MATCH_LENGTH    %e0, 2, Label[4]
 3    MATCH_HEAD      %e0, Expr[f], Label[4]
 4    MOVE            %e1, %e0
 5    GET_PART        %e2, %e0, 1
 6    MOVE            %e0, %e2

L5:
 7    BEGIN_BLOCK     Label[5]
 8    MATCH_LENGTH    %e0, 1, Label[6]
 9    MATCH_HEAD      %e0, Expr[g], Label[6]
10    MOVE            %e3, %e0
11    GET_PART        %e4, %e0, 1
12    MOVE            %e0, %e4
13    MATCH_LITERAL   %e0, Expr[1], Label[6]
14    MOVE            %e0, %e3
15    END_BLOCK       Label[5]
16    JUMP            Label[7]

L6:
17    JUMP            Label[4]

L7:
18    MOVE            %e0, %e1
19    GET_PART        %e5, %e0, 2
20    MOVE            %e0, %e5
21    MATCH_LITERAL   %e0, Expr[2], Label[4]
22    MOVE            %e0, %e1
23    END_BLOCK       Label[3]
24    JUMP            Label[2]

L4:
25    JUMP            Label[1]

L8:
26    END_BLOCK       Label[0]

L1:
27    DEBUG_PRINT     Expr[\"Pattern failed\"]
28    LOAD_IMM        %b0, 0
29    HALT            

L2:
30    DEBUG_PRINT     Expr[\"Pattern succeeded\"]
31    SAVE_BINDINGS   
32    LOAD_IMM        %b0, 1
33    HALT            

----------------------------------------
Expr registers: 6, Bool registers: 1
"
	,
	TestID->"FrontEnd-20251114-Z6C2B8"
]


(*==============================================================================
	Blank
==============================================================================*)
Test[
	bc7 = CompilePatternToBytecode[_];
	PatternBytecodeQ[bc7]
	,
	True
	,
	TestID->"FrontEnd-20251114-E2W2W1"
]

Test[
	ToString[bc7]
	,
	"
L0:
0    BEGIN_BLOCK     Label[0]
1    JUMP            Label[2]
2    END_BLOCK       Label[0]

L1:
3    DEBUG_PRINT     Expr[\"Pattern failed\"]
4    LOAD_IMM        %b0, 0
5    HALT            

L2:
6    DEBUG_PRINT     Expr[\"Pattern succeeded\"]
7    SAVE_BINDINGS   
8    LOAD_IMM        %b0, 1
9    HALT            

----------------------------------------
Expr registers: 1, Bool registers: 1
"
	,
	TestID->"FrontEnd-20251114-H9R8D4"
]


Test[
	bc8 = CompilePatternToBytecode[_Integer];
	PatternBytecodeQ[bc8]
	,
	True
	,
	TestID->"FrontEnd-20251114-V8D5Z1"
]

Test[
	ToString[bc8]
	,
	"
L0:
 0    BEGIN_BLOCK     Label[0]
 1    MATCH_HEAD      %e0, Expr[Integer], Label[1]
 2    JUMP            Label[2]
 3    END_BLOCK       Label[0]

L1:
 4    DEBUG_PRINT     Expr[\"Pattern failed\"]
 5    LOAD_IMM        %b0, 0
 6    HALT            

L2:
 7    DEBUG_PRINT     Expr[\"Pattern succeeded\"]
 8    SAVE_BINDINGS   
 9    LOAD_IMM        %b0, 1
10    HALT            

----------------------------------------
Expr registers: 1, Bool registers: 1
"
	,
	TestID->"FrontEnd-20251114-T8R6E8"
]


(*==============================================================================
	x_
==============================================================================*)
Test[
	bc9 = CompilePatternToBytecode[x_];
	PatternBytecodeQ[bc9]
	,
	True
	,
	TestID->"FrontEnd-20251114-I0U0X1"
]

Test[
	ToString[bc9]
	,
	"
L0:
 0    BEGIN_BLOCK     Label[0]
 1    MOVE            %e1, %e0
 2    BIND_VAR        Symbol[\"TestContext`x\"], %e1
 3    JUMP            Label[2]

L3:
 4    JUMP            Label[1]

L4:
 5    END_BLOCK       Label[0]

L1:
 6    DEBUG_PRINT     Expr[\"Pattern failed\"]
 7    LOAD_IMM        %b0, 0
 8    HALT            

L2:
 9    DEBUG_PRINT     Expr[\"Pattern succeeded\"]
10    SAVE_BINDINGS   
11    LOAD_IMM        %b0, 1
12    HALT            

----------------------------------------
Expr registers: 2, Bool registers: 1
Lexical bindings:
  TestContext`x \[RightArrow] %e1
"
	,
	TestID->"FrontEnd-20251114-E2K3V0"
]


(*==============================================================================
	Normal
==============================================================================*)
Test[
	bc9 = CompilePatternToBytecode[f[x_]];
	PatternBytecodeQ[bc9]
	,
	True
	,
	TestID->"FrontEnd-20251022-Y3V6S8"
]


TestStatePop[Global`contextState]


EndTestSection[]
