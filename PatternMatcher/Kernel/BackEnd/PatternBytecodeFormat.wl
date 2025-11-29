BeginPackage["DanielS`PatternMatcher`BackEnd`PatternBytecodeFormat`"]


FormatPatternBytecodeDisassembly::usage =
	"FormatPatternBytecodeDisassembly[str] formats the bytecode disassembly string with colors and styling.";


Begin["`Private`"]


Needs["DanielS`PatternMatcher`Utilities`OpcodeInformation`"];


$OpcodeColors = <|
	"DataMovement" -> RGBColor[0.3, 0.3, 0.3],
	"Introspection" -> RGBColor[0.1, 0.4, 0.9],
	"Matching" -> RGBColor[0.2, 0.7, 0.2],
	"Sequence" -> RGBColor[0.6, 0.3, 0.9],
	"Comparison" -> RGBColor[0.8, 0.6, 0.1],
	"Binding" -> RGBColor[0.9, 0.4, 0.2],
	"ControlFlow" -> RGBColor[0.8, 0.2, 0.8],
	"Scope" -> RGBColor[0.4, 0.4, 0.4],
	"Backtracking" -> RGBColor[0.9, 0.3, 0.3],
	"Debug" -> RGBColor[0.5, 0.5, 0.5],
	"Unknown" -> GrayLevel[0.5]
|>;


GetOpcodeColor[opcode_String] := 
	Lookup[$OpcodeColors, $OpcodesToCategories[opcode], GrayLevel[0.5]];


(* Markup helper function - similar to CompileUtilities`Markup approach *)
markup[s_String, opts___?OptionQ] := Module[{},
	If[TrueQ[$Notebooks],
		(* Running in notebook frontend - use StyleBox *)
		"\!\(\*" <> ToString[StyleBox[s, opts], InputForm] <> "\)",
		(* Running in console or other context - return plain text *)
		s
	]
];


FormatPatternBytecodeDisassembly[str_String] :=
	Module[{formatted},
		formatted = str;
		
		(* Apply formatting rules - use named patterns to capture matched text *)
		(* Labels - L0:, L1:, etc - with newline *)
		formatted = StringReplace[formatted, 
			lbl : (StartOfLine ~~ "L" ~~ DigitCharacter.. ~~ ":" ~~ "\n") :> 
				markup[StringTrim[lbl], FontWeight -> Bold, FontColor -> RGBColor[0.0, 0.4, 0.6]] <> "\n"
		];
		
		(* Opcodes - use string pattern matching *)
		formatted = StringReplace[formatted,
			opcode : (WordBoundary ~~ Alternatives @@ Flatten[Values[$CategoriesToOpcodes]] ~~ WordBoundary) :> 
				markup[opcode, FontWeight -> Bold, FontColor -> GetOpcodeColor[opcode]]
		];
		
		(* Registers - %e0, %b1, etc *)
		formatted = StringReplace[formatted,
			reg : ("%" ~~ ("e" | "b") ~~ DigitCharacter..) :> 
				markup[reg, FontWeight -> Bold, FontColor -> RGBColor[0.2, 0.6, 0.3]]
		];
		
		(* Label references - Label[0] *)
		formatted = StringReplace[formatted,
			lbl : ("Label[" ~~ DigitCharacter.. ~~ "]") :> 
				markup[lbl, FontWeight -> Bold, FontColor -> RGBColor[0.0, 0.4, 0.6]]
		];
		
		(* Expr[...] literals - be careful not to match Expr registers *)
		formatted = StringReplace[formatted,
			expr : ("Expr[" ~~ Except["]"].. ~~ "]") :> 
				markup[expr, FontColor -> RGBColor[0.7, 0.3, 0.1]]
		];
		
		(* Arrows → *)
		formatted = StringReplace[formatted,
			"→" :> markup["→", FontColor -> RGBColor[0.5, 0.5, 0.5]]
		];
		
		(* Separators *)
		formatted = StringReplace[formatted,
			sep : (StartOfLine ~~ "=".. ~~ EndOfLine) :> markup[sep, FontColor -> GrayLevel[0.6]]
		];
		
		(* Statistics header *)
		formatted = StringReplace[formatted,
			"Statistics:" :> markup["Statistics:", FontWeight -> Bold, FontColor -> RGBColor[0.2, 0.2, 0.5]]
		];
		
		formatted = StringReplace[formatted,
			"Lexical bindings:" :> markup["Lexical bindings:", FontWeight -> Bold, FontColor -> RGBColor[0.2, 0.2, 0.5]]
		];

		(* Return formatted string *)
		Style[formatted, FontFamily -> "Courier", Background -> GrayLevel[0.98]]
	];


End[]

EndPackage[]
