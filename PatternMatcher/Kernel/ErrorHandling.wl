BeginPackage["DanielS`PatternMatcher`ErrorHandling`"];


CatchFailure::usage =
	"CatchFailure[expr] catches thrown failures via ThrowFailure[...] that occurred within expr.";

ThrowFailure::usage =
	"ThrowFailure[subTag, msgTemplate, msgParameters, extra] construct a Failure object using the arguments and throws it.";


Begin["`Private`"];


$PMVMCatchThrowTag =
	"$PMVMCatchThrowTag";


(*=============================================================================
	CatchFailure
=============================================================================*)

SetAttributes[CatchFailure, HoldFirst];

CatchFailure[expr_] :=
	Catch[expr, $PMVMCatchThrowTag];


(*=============================================================================
	ThrowFailure
=============================================================================*)

SetAttributes[ThrowFailure, HoldFirst];

ThrowFailure[subTag_String, msgTemplate_, msgParameters_, extra:_Association:<||>] :=
	Throw[
		Failure[
			"PatternMatcherVirtualMachine`" <> subTag,
			<|
				"MessageTemplate" -> msgTemplate,
				"MessageParameters" -> msgParameters,
				extra
			|>
		],
		$PMVMCatchThrowTag
	];



End[];


EndPackage[];
