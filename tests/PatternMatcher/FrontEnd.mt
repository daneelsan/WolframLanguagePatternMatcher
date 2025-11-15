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
	x_Integer
==============================================================================*)
Test[
	bc10 = CompilePatternToBytecode[x_Integer];
	PatternBytecodeQ[bc10]
	,
	True
	,
	TestID->"FrontEnd-20251114-C8Z5V6"
]

Test[
	ToString[bc10]
	,
	"
L0:
 0    BEGIN_BLOCK     Label[0]
 1    MATCH_HEAD      %e0, Expr[Integer], Label[3]
 2    MOVE            %e1, %e0
 3    BIND_VAR        Symbol[\"TestContext`x\"], %e1
 4    JUMP            Label[2]

L3:
 5    JUMP            Label[1]

L4:
 6    END_BLOCK       Label[0]

L1:
 7    DEBUG_PRINT     Expr[\"Pattern failed\"]
 8    LOAD_IMM        %b0, 0
 9    HALT            

L2:
10    DEBUG_PRINT     Expr[\"Pattern succeeded\"]
11    SAVE_BINDINGS   
12    LOAD_IMM        %b0, 1
13    HALT            

----------------------------------------
Expr registers: 2, Bool registers: 1
Lexical bindings:
  TestContext`x \[RightArrow] %e1
"
	,
	TestID->"FrontEnd-20251114-E7E4K9"
]


(*==============================================================================
	f[x_]
==============================================================================*)
Test[
	bc11 = CompilePatternToBytecode[f[x_]];
	PatternBytecodeQ[bc11]
	,
	True
	,
	TestID->"FrontEnd-20251114-U2T0U7"
]

Test[
	ToString[bc11]
	,
	"
L0:
 0    BEGIN_BLOCK     Label[0]

L3:
 1    BEGIN_BLOCK     Label[3]
 2    MATCH_LENGTH    %e0, 1, Label[4]
 3    MATCH_HEAD      %e0, Expr[f], Label[4]
 4    MOVE            %e1, %e0
 5    GET_PART        %e2, %e0, 1
 6    MOVE            %e0, %e2
 7    MOVE            %e3, %e0
 8    BIND_VAR        Symbol[\"TestContext`x\"], %e3
 9    JUMP            Label[6]

L5:
10    JUMP            Label[4]

L6:
11    MOVE            %e0, %e1
12    END_BLOCK       Label[3]
13    JUMP            Label[2]

L4:
14    JUMP            Label[1]

L7:
15    END_BLOCK       Label[0]

L1:
16    DEBUG_PRINT     Expr[\"Pattern failed\"]
17    LOAD_IMM        %b0, 0
18    HALT            

L2:
19    DEBUG_PRINT     Expr[\"Pattern succeeded\"]
20    SAVE_BINDINGS   
21    LOAD_IMM        %b0, 1
22    HALT            

----------------------------------------
Expr registers: 4, Bool registers: 1
Lexical bindings:
  TestContext`x \[RightArrow] %e3
"
	,
	TestID->"FrontEnd-20251114-Y8O6M2"
]


(*==============================================================================
	f[x_, y_]
==============================================================================*)
Test[
	bc12 = CompilePatternToBytecode[f[x_, y_]];
	PatternBytecodeQ[bc12]
	,
	True
	,
	TestID->"FrontEnd-20251114-C3K1M5"
]

Test[
	ToString[bc12]
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
 7    MOVE            %e3, %e0
 8    BIND_VAR        Symbol[\"TestContext`x\"], %e3
 9    JUMP            Label[6]

L5:
10    JUMP            Label[4]

L6:
11    MOVE            %e0, %e1
12    GET_PART        %e4, %e0, 2
13    MOVE            %e0, %e4
14    MOVE            %e5, %e0
15    BIND_VAR        Symbol[\"TestContext`y\"], %e5
16    JUMP            Label[8]

L7:
17    JUMP            Label[4]

L8:
18    MOVE            %e0, %e1
19    END_BLOCK       Label[3]
20    JUMP            Label[2]

L4:
21    JUMP            Label[1]

L9:
22    END_BLOCK       Label[0]

L1:
23    DEBUG_PRINT     Expr[\"Pattern failed\"]
24    LOAD_IMM        %b0, 0
25    HALT            

L2:
26    DEBUG_PRINT     Expr[\"Pattern succeeded\"]
27    SAVE_BINDINGS   
28    LOAD_IMM        %b0, 1
29    HALT            

----------------------------------------
Expr registers: 6, Bool registers: 1
Lexical bindings:
  TestContext`x \[RightArrow] %e3
  TestContext`y \[RightArrow] %e5
"
	,
	TestID->"FrontEnd-20251114-M7N0B3"
]


(*==============================================================================
	f[x_, 42, x_]
==============================================================================*)
Test[
	bc13 = CompilePatternToBytecode[f[x_, 42, x_]];
	PatternBytecodeQ[bc13]
	,
	True
	,
	TestID->"FrontEnd-20251114-J4P4Q5"
]

Test[
	ToString[bc13]
	,
	"
L0:
 0    BEGIN_BLOCK     Label[0]

L3:
 1    BEGIN_BLOCK     Label[3]
 2    MATCH_LENGTH    %e0, 3, Label[4]
 3    MATCH_HEAD      %e0, Expr[f], Label[4]
 4    MOVE            %e1, %e0
 5    GET_PART        %e2, %e0, 1
 6    MOVE            %e0, %e2
 7    MOVE            %e3, %e0
 8    BIND_VAR        Symbol[\"TestContext`x\"], %e3
 9    JUMP            Label[6]

L5:
10    JUMP            Label[4]

L6:
11    MOVE            %e0, %e1
12    GET_PART        %e4, %e0, 2
13    MOVE            %e0, %e4
14    MATCH_LITERAL   %e0, Expr[42], Label[4]
15    MOVE            %e0, %e1
16    GET_PART        %e5, %e0, 3
17    MOVE            %e0, %e5
18    SAMEQ           %b1, %e3, %e0
19    JUMP_IF_FALSE   %b1, Label[4]
20    MOVE            %e0, %e1
21    END_BLOCK       Label[3]
22    JUMP            Label[2]

L4:
23    JUMP            Label[1]

L7:
24    END_BLOCK       Label[0]

L1:
25    DEBUG_PRINT     Expr[\"Pattern failed\"]
26    LOAD_IMM        %b0, 0
27    HALT            

L2:
28    DEBUG_PRINT     Expr[\"Pattern succeeded\"]
29    SAVE_BINDINGS   
30    LOAD_IMM        %b0, 1
31    HALT            

----------------------------------------
Expr registers: 6, Bool registers: 2
Lexical bindings:
  TestContext`x \[RightArrow] %e3
"
	,
	TestID->"FrontEnd-20251114-M2P5I5"
]


(*==============================================================================
	f[x_Integer]
==============================================================================*)
Test[
	bc14 = CompilePatternToBytecode[f[x_Integer]];
	PatternBytecodeQ[bc14]
	,
	True
	,
	TestID->"FrontEnd-20251114-R9I3D4"
]

Test[
	ToString[bc14]
	,
	"
L0:
 0    BEGIN_BLOCK     Label[0]

L3:
 1    BEGIN_BLOCK     Label[3]
 2    MATCH_LENGTH    %e0, 1, Label[4]
 3    MATCH_HEAD      %e0, Expr[f], Label[4]
 4    MOVE            %e1, %e0
 5    GET_PART        %e2, %e0, 1
 6    MOVE            %e0, %e2
 7    MATCH_HEAD      %e0, Expr[Integer], Label[5]
 8    MOVE            %e3, %e0
 9    BIND_VAR        Symbol[\"TestContext`x\"], %e3
10    JUMP            Label[6]

L5:
11    JUMP            Label[4]

L6:
12    MOVE            %e0, %e1
13    END_BLOCK       Label[3]
14    JUMP            Label[2]

L4:
15    JUMP            Label[1]

L7:
16    END_BLOCK       Label[0]

L1:
17    DEBUG_PRINT     Expr[\"Pattern failed\"]
18    LOAD_IMM        %b0, 0
19    HALT            

L2:
20    DEBUG_PRINT     Expr[\"Pattern succeeded\"]
21    SAVE_BINDINGS   
22    LOAD_IMM        %b0, 1
23    HALT            

----------------------------------------
Expr registers: 4, Bool registers: 1
Lexical bindings:
  TestContext`x \[RightArrow] %e3
"
	,
	TestID->"FrontEnd-20251114-L8Z7J0"
]


(*==============================================================================
	f[x_Integer, x_]
==============================================================================*)
Test[
	bc15 = CompilePatternToBytecode[f[x_Integer, x_]];
	PatternBytecodeQ[bc15]
	,
	True
	,
	TestID->"FrontEnd-20251114-N9O4K1"
]

Test[
	ToString[bc15]
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
 7    MATCH_HEAD      %e0, Expr[Integer], Label[5]
 8    MOVE            %e3, %e0
 9    BIND_VAR        Symbol[\"TestContext`x\"], %e3
10    JUMP            Label[6]

L5:
11    JUMP            Label[4]

L6:
12    MOVE            %e0, %e1
13    GET_PART        %e4, %e0, 2
14    MOVE            %e0, %e4
15    SAMEQ           %b1, %e3, %e0
16    JUMP_IF_FALSE   %b1, Label[4]
17    MOVE            %e0, %e1
18    END_BLOCK       Label[3]
19    JUMP            Label[2]

L4:
20    JUMP            Label[1]

L7:
21    END_BLOCK       Label[0]

L1:
22    DEBUG_PRINT     Expr[\"Pattern failed\"]
23    LOAD_IMM        %b0, 0
24    HALT            

L2:
25    DEBUG_PRINT     Expr[\"Pattern succeeded\"]
26    SAVE_BINDINGS   
27    LOAD_IMM        %b0, 1
28    HALT            

----------------------------------------
Expr registers: 5, Bool registers: 2
Lexical bindings:
  TestContext`x \[RightArrow] %e3
"
	,
	TestID->"FrontEnd-20251114-C8L7R7"
]


(*==============================================================================
	f[x_Integer, x_Real]
==============================================================================*)
Test[
	bc16 = CompilePatternToBytecode[f[x_Integer, x_Real]];
	PatternBytecodeQ[bc16]
	,
	True
	,
	TestID->"FrontEnd-20251114-T1A4T3"
]

Test[
	ToString[bc16]
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
 7    MATCH_HEAD      %e0, Expr[Integer], Label[5]
 8    MOVE            %e3, %e0
 9    BIND_VAR        Symbol[\"TestContext`x\"], %e3
10    JUMP            Label[6]

L5:
11    JUMP            Label[4]

L6:
12    MOVE            %e0, %e1
13    GET_PART        %e4, %e0, 2
14    MOVE            %e0, %e4
15    SAMEQ           %b1, %e3, %e0
16    JUMP_IF_FALSE   %b1, Label[4]
17    MATCH_HEAD      %e0, Expr[Real], Label[4]
18    MOVE            %e0, %e1
19    END_BLOCK       Label[3]
20    JUMP            Label[2]

L4:
21    JUMP            Label[1]

L7:
22    END_BLOCK       Label[0]

L1:
23    DEBUG_PRINT     Expr[\"Pattern failed\"]
24    LOAD_IMM        %b0, 0
25    HALT            

L2:
26    DEBUG_PRINT     Expr[\"Pattern succeeded\"]
27    SAVE_BINDINGS   
28    LOAD_IMM        %b0, 1
29    HALT            

----------------------------------------
Expr registers: 5, Bool registers: 2
Lexical bindings:
  TestContext`x \[RightArrow] %e3
"
	,
	TestID->"FrontEnd-20251114-Y5S6J0"
]


(*==============================================================================
	f[x_, x_[g]]
==============================================================================*)
Test[
	bc17 = CompilePatternToBytecode[f[x_, x_[g]]];
	PatternBytecodeQ[bc17]
	,
	True
	,
	TestID->"FrontEnd-20251114-E8N0V3"
]

Test[
	ToString[bc17]
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
 7    MOVE            %e3, %e0
 8    BIND_VAR        Symbol[\"TestContext`x\"], %e3
 9    JUMP            Label[6]

L5:
10    JUMP            Label[4]

L6:
11    MOVE            %e0, %e1
12    GET_PART        %e4, %e0, 2
13    MOVE            %e0, %e4

L7:
14    BEGIN_BLOCK     Label[7]
15    MATCH_LENGTH    %e0, 1, Label[8]
16    MOVE            %e5, %e0
17    GET_PART        %e6, %e0, 0
18    MOVE            %e0, %e6
19    SAMEQ           %b1, %e3, %e0
20    JUMP_IF_FALSE   %b1, Label[8]
21    MOVE            %e0, %e5
22    GET_PART        %e7, %e0, 1
23    MOVE            %e0, %e7
24    MATCH_LITERAL   %e0, Expr[g], Label[8]
25    MOVE            %e0, %e5
26    END_BLOCK       Label[7]
27    JUMP            Label[9]

L8:
28    JUMP            Label[4]

L9:
29    MOVE            %e0, %e1
30    END_BLOCK       Label[3]
31    JUMP            Label[2]

L4:
32    JUMP            Label[1]

L10:
33    END_BLOCK       Label[0]

L1:
34    DEBUG_PRINT     Expr[\"Pattern failed\"]
35    LOAD_IMM        %b0, 0
36    HALT            

L2:
37    DEBUG_PRINT     Expr[\"Pattern succeeded\"]
38    SAVE_BINDINGS   
39    LOAD_IMM        %b0, 1
40    HALT            

----------------------------------------
Expr registers: 8, Bool registers: 2
Lexical bindings:
  TestContext`x \[RightArrow] %e3
"
	,
	TestID->"FrontEnd-20251114-P7E1O7"
]


(*==============================================================================
	_ | _
==============================================================================*)
Test[
	bc18 = CompilePatternToBytecode[_ | _];
	PatternBytecodeQ[bc18]
	,
	True
	,
	TestID->"FrontEnd-20251114-W7Q5B1"
]

Test[
	ToString[bc18]
	,
	"
L0:
 0    BEGIN_BLOCK     Label[0]
 1    TRY             Label[4]

L3:
 2    JUMP            Label[2]

L5:
 3    FAIL            

L4:
 4    TRUST           
 5    JUMP            Label[2]
 6    END_BLOCK       Label[0]

L1:
 7    DEBUG_PRINT     Expr[\"Pattern failed\"]
 8    LOAD_IMM        %b0, 0
 9    HALT            

L2:
10    DEBUG_PRINT     Expr[\"Pattern succeeded\"]
11    SAVE_BINDINGS   
12    LOAD_IMM        %b0, 1
13    HALT            

----------------------------------------
Expr registers: 1, Bool registers: 1
"
	,
	TestID->"FrontEnd-20251114-J1X2E6"
]


(*==============================================================================
	_Real | _Integer
==============================================================================*)
Test[
	bc19 = CompilePatternToBytecode[_Real | _Integer];
	PatternBytecodeQ[bc18]
	,
	True
	,
	TestID->"FrontEnd-20251114-I5H0J4"
]

Test[
	ToString[bc19]
	,
	"
L0:
 0    BEGIN_BLOCK     Label[0]
 1    TRY             Label[4]

L3:
 2    MATCH_HEAD      %e0, Expr[Real], Label[5]
 3    JUMP            Label[2]

L5:
 4    FAIL            

L4:
 5    TRUST           
 6    MATCH_HEAD      %e0, Expr[Integer], Label[1]
 7    JUMP            Label[2]
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
Expr registers: 1, Bool registers: 1
"
	,
	TestID->"FrontEnd-20251114-V7B0C0"
]


(*==============================================================================
	f[_Integer | _Real]
==============================================================================*)
Test[
	bc20 = CompilePatternToBytecode[f[_Integer | _Real]];
	PatternBytecodeQ[bc20]
	,
	True
	,
	TestID->"FrontEnd-20251114-L0B3W9"
]

Test[
	ToString[bc20]
	,
	"
L0:
 0    BEGIN_BLOCK     Label[0]

L3:
 1    BEGIN_BLOCK     Label[3]
 2    MATCH_LENGTH    %e0, 1, Label[4]
 3    MATCH_HEAD      %e0, Expr[f], Label[4]
 4    MOVE            %e1, %e0
 5    GET_PART        %e2, %e0, 1
 6    MOVE            %e0, %e2
 7    TRY             Label[6]

L5:
 8    MATCH_HEAD      %e0, Expr[Integer], Label[7]
 9    JUMP            Label[2]

L7:
10    FAIL            

L6:
11    TRUST           
12    MATCH_HEAD      %e0, Expr[Real], Label[4]
13    JUMP            Label[2]
14    MOVE            %e0, %e1
15    END_BLOCK       Label[3]
16    JUMP            Label[2]

L4:
17    JUMP            Label[1]

L8:
18    END_BLOCK       Label[0]

L1:
19    DEBUG_PRINT     Expr[\"Pattern failed\"]
20    LOAD_IMM        %b0, 0
21    HALT            

L2:
22    DEBUG_PRINT     Expr[\"Pattern succeeded\"]
23    SAVE_BINDINGS   
24    LOAD_IMM        %b0, 1
25    HALT            

----------------------------------------
Expr registers: 3, Bool registers: 1
"
	,
	TestID->"FrontEnd-20251114-L9M4V3"
]


(*==============================================================================
	f[x_Integer | x_Real]
==============================================================================*)
Test[
	bc21 = CompilePatternToBytecode[f[x_Integer | x_Real]];
	PatternBytecodeQ[bc21]
	,
	True
	,
	TestID->"FrontEnd-20251114-U1L9Q2"
]

Test[
	ToString[bc21]
	,
	"
L0:
 0    BEGIN_BLOCK     Label[0]

L3:
 1    BEGIN_BLOCK     Label[3]
 2    MATCH_LENGTH    %e0, 1, Label[4]
 3    MATCH_HEAD      %e0, Expr[f], Label[4]
 4    MOVE            %e1, %e0
 5    GET_PART        %e2, %e0, 1
 6    MOVE            %e0, %e2
 7    TRY             Label[6]

L5:
 8    MATCH_HEAD      %e0, Expr[Integer], Label[8]
 9    MOVE            %e3, %e0
10    BIND_VAR        Symbol[\"TestContext`x\"], %e3
11    JUMP            Label[2]

L8:
12    JUMP            Label[7]

L9:
13    FAIL            

L6:
14    TRUST           
15    MATCH_HEAD      %e0, Expr[Real], Label[10]
16    MOVE            %e4, %e0
17    BIND_VAR        Symbol[\"TestContext`x\"], %e4
18    JUMP            Label[2]

L10:
19    JUMP            Label[4]

L11:
20    MOVE            %e0, %e1
21    END_BLOCK       Label[3]
22    JUMP            Label[2]

L4:
23    JUMP            Label[1]

L12:
24    END_BLOCK       Label[0]

L1:
25    DEBUG_PRINT     Expr[\"Pattern failed\"]
26    LOAD_IMM        %b0, 0
27    HALT            

L2:
28    DEBUG_PRINT     Expr[\"Pattern succeeded\"]
29    SAVE_BINDINGS   
30    LOAD_IMM        %b0, 1
31    HALT            

----------------------------------------
Expr registers: 5, Bool registers: 1
Lexical bindings:
  TestContext`x \[RightArrow] %e4
"
	,
	TestID->"FrontEnd-20251114-F6L5P1"
]


TestStatePop[Global`contextState]


EndTestSection[]
