BeginPackage["DanielS`PatternMatcher`ErrorHandling`"];


CatchFailure::usage =
	"CatchFailure[expr] catches thrown failures via ThrowFailure[...] that occurred within expr.";

ThrowFailure::usage =
	"ThrowFailure[subTag, msgTemplate, msgParameters, extra] construct a Failure object using the arguments and throws it.";

ThrowLibraryError::usage =
	"ThrowLibraryError[message] constructs and throws a Failure object with the given message string.";


Begin["`Private`"];


$PMCatchThrowTag =
	"$PMCatchThrowTag";


(*=============================================================================
	CatchFailure
=============================================================================*)

SetAttributes[CatchFailure, HoldFirst];

CatchFailure[expr_] :=
	Catch[expr, $PMCatchThrowTag];


(*=============================================================================
	ThrowFailure
=============================================================================*)

ThrowFailure[subTag_String, msgTemplate_, msgParameters_, extra:_Association:<||>] :=
	Throw[
		Failure[
			"PatternMatcher`" <> subTag,
			<|
				"MessageTemplate" -> msgTemplate,
				"MessageParameters" -> msgParameters,
				extra
			|>
		],
		$PMCatchThrowTag
	];


(*=============================================================================
	ThrowLibraryError
=============================================================================*)

ThrowLibraryError[message_String, msgParameters:_List:{}] :=
	ThrowFailure["LibraryError", message, msgParameters];


End[];


EndPackage[];
