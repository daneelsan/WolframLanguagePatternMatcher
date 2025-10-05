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


(* This is a function the library will call from the C++ code. It all starts here. Feel free to modify/Block this symbol, see examples. *)
LogHandler := PrintLogFunctionSelector @* LogFilterSelector;

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
