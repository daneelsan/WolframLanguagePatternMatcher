BeginPackage["DanielS`PatternMatcher`Utilities`Logger`"]


DiscardLog::usage =
	"Discard all log messages.";

FilterAcceptAll::usage =
	"Simple filter that does no filtering.";

FilterByFile::usage =
	"Filter that accepts messages matching a specific file name.";

FilterByFunction::usage =
	"Filter that accepts messages matching a specific function name.";

FilterByLevel::usage =
	"Filter that accepts messages matching a specific log level.";

FilterByLine::usage =
	"Filter that accepts messages matching a specific line number.";

FilterBySingleFeature::usage =
	"Filter that accepts messages matching a specific feature.";

FilterCustom::usage =
	"Filter that accepts messages matching a custom condition.";

FilterRejectAll::usage =
	"Simple filter that rejects all messages.";

FormattedLog::usage =
	"This is the selector for formatting log messages.";

LevelColorMapForLogs::usage =
	"Color mapping for different log levels in logs.";

LogFiltered::usage =
	"A symbol for filtered-out messages.";

LogFilterSelector::usage =
	"Selector for filtering log messages based on various criteria.";

LogHandler::usage =
	"Handler for processing and outputting log messages.";

TraceHandler::usage =
	"Handler for processing and outputting trace log messages.";

LogToAssociation::usage =
	"Convert log message components to an association.";

LogToGrid::usage =
	"Convert log message components to a TextGrid for styled display.";

LogToList::usage =
	"Convert log message components to a list.";

LogToRow::usage =
	"Convert log message components to a row.";

LogToShortString::usage =
	"Convert log message components to a short string.";

LogToString::usage =
	"Convert log message components to a string.";

StyledLogLevel::usage =
	"Styled representation of the log level.";

StyledLogMessageLocation::usage =
	"Styled representation of the log message location.";

StyledLogMessageText::usage =
	"Styled representation of the log message text.";

PrintLogFunctionSelector::usage =
	"Selector for choosing the print function for log messages.";

PrintLogToMessagesWindow::usage =
	"Print log messages to the messages window.";

PrintLogToNotebook::usage =
	"Print log messages to a notebook.";

PrintLogToSymbol::usage =
	"Append to a list and assign to given symbol. Good choice if you don't want to see the logs immediately, but want to store them for later analysis.";


Begin["`Private`"];

(************** Functions defining how to style different parts of a log message *************)

(* Colors associated with different log severities *)
LevelColorMapForLogs = <|
	"Debug" -> Darker[Green],
	"Warning" -> Orange,
	"Error" -> Red
|>;

(* Styled part of a message containing log level description *)
StyledLogLevel[logLevel_] :=
	Style["[" <> ToString @ logLevel <> "]", LevelColorMapForLogs[logLevel]];

(* Styled part of a message containing info on where the log was issued *)
StyledLogMessageLocation[file_, line_, fn_] :=
	Tooltip[Style["Line " <> ToString[line] <> " in " <> FileNameTake[file] <> ", function " <> fn, Darker[Gray]], file];

(* Styled part of a message containing the actual log text *)
StyledLogMessageText[args_List, size_:Inherited] :=
	Style[StringJoin @@ ToString /@ args, FontSize -> size];

(************* Functions defining how to format a log message *************)

(* Put all message parts in a list unstyled *)
LogToList[args___] := {args};

(* Put all message parts in Association *)
LogToAssociation[logLevel_, line_, file_, fn_, args___] :=
	Association["Level" -> logLevel, "Line" -> line, "File" -> file, "Function" -> fn, "Message" -> StyledLogMessageText[{args}]];

(* Combine all log parts to a String. No styling, contains a newline character. *)
LogToString[logLevel_, line_, file_, fn_, args___] :=
	"[" <> ToString @ logLevel <> "] In file " <> file <> ", line " <> ToString[line] <> ", function " <> fn <> ":\n" <> (StringJoin @@ ToString /@ {args});

(* Combine all log parts to a condensed String. No styling, single line (unless message text contains newlines). *)
LogToShortString[logLevel_, line_, file_, fn_, args___] :=
	"[" <> ToString @ logLevel <> "] " <> FileNameTake[file] <> ":" <> ToString[line] <> " (" <> fn <> "): " <> (StringJoin @@ ToString /@ {args});

(* Place fully styled log message in a TextGrid. Looks nice, good default choice for printing to the notebook. *)
LogToGrid[logLevel_, line_, file_, fn_, args___] :=
	TextGrid[{
		{StyledLogLevel[logLevel], StyledLogMessageLocation[file, line, fn]},
		{SpanFromAbove, StyledLogMessageText[{args}, 14]}
	}];

(* Fully styled, condensed log message in a Row. Good choice if you expect many log messages and want to see them all in the notebook. *)
LogToRow[logLevel_, line_, file_, fn_, args___] :=
	Row[{Style["(" <> FileNameTake[file] <> ":" <> ToString[line] <> ")", LevelColorMapForLogs[logLevel]], StyledLogMessageText[{args}]}];

(* This is a "selector" called by other functions below. Feel free to modify/Block this symbol, see examples. *)
If[TrueQ @ $Notebooks,
	FormattedLog := LogToGrid;
	,
	FormattedLog := LogToString
]


TraceToShortString[logLevel_, line_, file_, fn_, args___] :=
	StringJoin[ReleaseHold[Map[Function[Null, ToString[Unevaluated[##]], HoldAll], HoldComplete[args]]]];


(* Opcode metadata for intelligent formatting *)
$CategoriesToOpcodes = <|
	"DataMovement" -> {"MOVE", "LOAD_IMM"},
	"Introspection" -> {"GET_LENGTH", "GET_PART"},
	"Matching" -> {"MATCH_HEAD", "MATCH_LENGTH", "MATCH_MIN_LENGTH", "MATCH_LITERAL", "APPLY_TEST", "MATCH_SEQ_HEADS"},
	"Sequence" -> {"MAKE_SEQUENCE", "SPLIT_SEQ"},
	"Comparison" -> {"SAMEQ"},
	"Binding" -> {"BIND_VAR", "SAVE_BINDING"},
	"ControlFlow" -> {"JUMP", "BRANCH_FALSE", "HALT"},
	"Scope" -> {"BEGIN_BLOCK", "END_BLOCK", "EXPORT_BINDINGS"},
	"Backtracking" -> {"TRY", "RETRY", "TRUST", "CUT", "FAIL"},
	"Debug" -> {"DEBUG_PRINT"}
|>;


$OpcodesToCategories =
	Association[Flatten[KeyValueMap[Thread[#2 -> #1] &, $CategoriesToOpcodes]]];


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


(*
	Improved trace formatter with opcode awareness and structured data parsing
	TODO: Future improvements:
	- Add indentation for nested BEGIN_BLOCK/END_BLOCK scopes
	- Add tooltips on register references showing their values
	- Add instruction counter/cycle number display
	- Add collapsible sections for block scopes
	- Add diff highlighting for MOVE/BIND operations
	- Add performance metrics (time per opcode category)
	- Add export options (save trace to file, JSON format for analysis)
	- Add alternate line background colors for readability
*)
TraceToStyledRow[logLevel_, line_, file_, fn_, args___] := Module[
	{msg, color, icon, opcode, result, details, formattedDetails},
	
	(* Build message string *)
	msg = StringJoin[ReleaseHold[Map[Function[Null, ToString[Unevaluated[##]], HoldAll], HoldComplete[args]]]];
	
	(* Try to parse structured format: "OPCODE|RESULT|details..." *)
	{opcode, result, details} = StringSplit[msg, "|", 3];
	
	(* Format details with proper spacing *)
	formattedDetails = StringReplace[details, {
		(* Add space after register references: %e0==List -> %e0 == List *)
		RegularExpression["(%[eb]\\d+)(==|:=|<-)"] :> "$1 $2 ",
		(* Add space between register and keyword: %e0len -> %e0 len *)
		RegularExpression["(%[eb]\\d+)([a-z]+)"] :> "$1 $2",
		(* Add space between word and digit (but not register refs): saved0 -> saved 0 *)
		RegularExpression["(?<!%[eb])([a-z])(\\d)"] :> "$1 $2",
		(* Add space between number and word: 0bindings -> 0 bindings, 15cycles -> 15 cycles *)
		RegularExpression["(\\d+)([a-z]+)"] :> "$1 $2",
		(* Add space around equals sign *)
		"=" -> " = ",
		(* Add space before parentheses *)
		RegularExpression["(\\d)\\("] :> "$1 (",
		(* Add space after label references *)
		RegularExpression["L(\\d+)"] :> "L$1 ",
		(* Clean up multiple spaces *)
		RegularExpression["  +"] -> " "
	}] // StringTrim;
	
	(* Determine color based on result and opcode category *)
	color = Which[
		result == "SUCCESS", RGBColor[0, 0.65, 0],
		result == "FAILURE", RGBColor[0.85, 0, 0],
		result == "INVALID", RGBColor[0.85, 0.5, 0],
		result == "TERMINAL", RGBColor[0.7, 0, 0.7],
		result == "TRUE", RGBColor[0, 0.5, 0.5],
		result == "FALSE", RGBColor[0.5, 0.5, 0],
		result == "TAKEN", RGBColor[0.4, 0.4, 0.8],
		result == "SKIP", GrayLevel[0.6],
		True, GetOpcodeColor[opcode]
	];
	
	(* Choose icon based on result and opcode *)
	icon = Which[
		result == "SUCCESS", "\[CheckmarkedBox]",
		result == "FAILURE", "\[Times]",
		result == "INVALID", "\[WarningSign]",
		result == "TERMINAL", "\[FivePointedStar]",
		result == "TAKEN", "\[RightArrow]",
		result == "SKIP", "\[EmptyCircle]",
		result == "TRUE", "\[CheckmarkedBox]",
		result == "FALSE", "\[Times]",
		StringMatchQ[opcode, "JUMP" | "FAIL" | "BACKTRACK"], "\[RightArrow]",
		StringMatchQ[opcode, "MATCH_*"], "\[RightTriangle]",
		StringMatchQ[opcode, "GET_*"], "\[RightGuillemet]",
		StringMatchQ[opcode, "MAKE_*"], "\[CirclePlus]",
		StringMatchQ[opcode, "BIND_*"], "\[DownTeeArrow]",
		StringMatchQ[opcode, "BEGIN_*" | "END_*"], "\[VerticalEllipsis]",
		StringMatchQ[opcode, "TRY" | "RETRY" | "TRUST" | "CUT"], "\[Continuation]",
		True, "\[FilledSmallCircle]"
	];
	
	(* Format as styled row with color-coded components *)
	Row[{
		Style[icon <> " ", color, Bold],
		Style[opcode, color, Bold, FontFamily -> "Courier"],
		If[result =!= "INFO", 
			Style[" " <> result <> " ", color, Background -> Lighter[color, 0.9]],
			""
		],
		" ",
		Style[formattedDetails, color]
	}]
];

(* This is a "selector" called by other functions below. You can switch between formats *)
FormattedTrace := TraceToStyledRow; 


(************* Functions filtering log messages *************)

(* Define a symbol for filtered-out messages *)
LogFiltered = Missing["FilteredOut"];

(* Simple filter that does no filtering *)
FilterAcceptAll[args___] := args;

(* Filter that rejects everything *)
FilterRejectAll[___] := LogFiltered;

(* Meta function for defining filters that filter by a single element of a log: level, line, file name or function name *)
FilterBySingleFeature[featureIndex_][test_] := Sequence @@ If[TrueQ @ test[Slot[featureIndex]], {##}, {LogFiltered}]&;

(* Define single element filters *)
{FilterByLevel, FilterByLine, FilterByFile, FilterByFunction} = (FilterBySingleFeature /@ Range[4]);

(* Define custom filter - test function have access to all elements of the log *)
FilterCustom[test_] := Sequence @@ If[TrueQ @ test[##], {##}, {LogFiltered}]&;

(* This is a "selector" called by other functions below. Feel free to modify/Block this symbol, see examples. *)
LogFilterSelector := FilterAcceptAll;

(************* Functions defining where to place a log message *************)

(* Discard the log *)
DiscardLog[___] := Null;

(* Print to current notebook *)
PrintLogToNotebook[args___] :=
	Print @ FormattedLog[args];
PrintLogToNotebook[LogFiltered] := DiscardLog[];


PrintTraceToNotebook[args___] :=
	Print @ FormattedTrace[args];
PrintTraceToNotebook[LogFiltered] := DiscardLog[];


(* Print to Messages window. Remember that this window may be hidden by default. *)
PrintLogToMessagesWindow[args___] :=
	NotebookWrite[MessagesNotebook[], Cell[RawBoxes @ ToBoxes[FormattedLog[args]], "Output"]];
PrintLogToMessagesWindow[LogFiltered] := DiscardLog[];

(* Append to a list and assign to given symbol. Good choice if you don't want to see the logs immediately, but want to store them for later analysis. *)
Attributes[PrintLogToSymbol] = {HoldFirst};
PrintLogToSymbol[x_] := (
	If[Not @ ListQ @ x,
		x = {}
	];
	AppendTo[x, FormattedLog[##]];
)&;
PrintLogToSymbol[LogFiltered] := DiscardLog[];

(* This is a "selector" called by other functions below. Feel free to modify/Block this symbol, see examples. *)
PrintLogFunctionSelector := PrintLogToNotebook;

PrintTraceFunctionSelector := PrintTraceToNotebook;


(* This is a function the library will call from the C++ code. It all starts here. Feel free to modify/Block this symbol, see examples. *)
LogHandler := PrintLogFunctionSelector @* LogFilterSelector;

(* Callback for trace logs *)
TraceHandler := PrintTraceFunctionSelector @* LogFilterSelector;

End[];

EndPackage[]

(************* Examples of overriding default logger behavior *************)

(***
	Make logger format logs as Association and append to a list under a symbol TestLogSymbol:

		Logger`PrintLogFunctionSelector := Block[{Logger`FormattedLog = Logger`LogToAssociation},
			Logger`PrintLogToSymbol[TestLogSymbol][##]
		]&

	after you evaluate some library function the TestLogSymbol may be a list similar this:

		{
			<|
				"Level" -> "Debug",
				"Line" -> 17,
				"File" -> "main.cpp",
				"Function" -> "ReadData",
				"Message" -> Style["Library function entered with 4 arguments.", Automatic]
			|>,
			<|
				"Level" -> "Warning",
				"Line" -> 20,
				"File" -> "Utilities.cpp",
				"Function" -> "validateDimensions",
				"Message" -> Style["Dimensions are too large.", Automatic]
			|>,
			...
		}
***)

(***
	Log styled condensed logs to Messages window:

		Logger`PrintLogFunctionSelector := Block[{Logger`FormattedLog = Logger`LogToRow},
			Logger`PrintLogToMessagesWindow[##]
		]&
***)

(***
	Sow logs formatted as short Strings instead of printing:

		Logger`PrintLogFunctionSelector :=
			If[## =!= Logger`LogFiltered,
				Sow @ Logger`LogToShortString[##]
			]&;

	Remember that in this case library functions must be wrapped with Reap.

	You could theoretically write simply

		WolframCompiler`Logger`PrintLogFunctionSelector := Sow @* WolframCompiler`Logger`LogToShortString;

	But in this case, you are loosing the correct handling of filtered-out messages so it's only fine with the default "accept-all" filter.
***)
