BeginPackage["DanielS`PatternMatcher`Library`"]


$PatternMatcherLibrary::usage =
	"Path to the PatternMatcher Library.";

$PatternMatcherObjectFactory::usage =
	"Object factory for creating PatternMatcherLibrary objects.";

InitializePatternMatcherLibrary::usage =
	"Initialize the PatternMatcher library.";

InitializePatternMatcherLibraryInternal::usage =
	"Internal function to initialize the PatternMatcher library.";

UninitializePatternMatcherLibrary::usage =
	"Uninitialize the PatternMatcher library.";


Begin["`Private`"]


Needs["DanielS`PatternMatcher`Utilities`Logger`"]
Needs["DanielS`PatternMatcher`Utilities`LibraryLink`"]
Needs["DanielS`PatternMatcher`ErrorHandling`"] (* for ThrowFailure, CatchFailure *)

Needs["DataStructure`Utilities`"] (* for HandleDataStructureError *)


$PatternMatcherLibrary;

$PatternMatcherObjectFactory;

(* Whether the PatternMatcherLibrary has already been initialized or not. *)
$InitializedQ = False;


(* In theory, these could be obtained from the library, but it doesn't seem worth doing. *)
$ObjectFactoryClassName =
	"PatternMatcherLibrary`ObjectFactory";

$ObjectFactoryClass =
	Symbol[$ObjectFactoryClassName];


InitializePatternMatcherLibrary[opts:OptionsPattern[]] :=
	Module[{status},
		status = AbsoluteTiming[CatchFailure[InitializePatternMatcherLibraryInternal[opts]]];
		If[FailureQ[status[[2]]],
			status[[2]]
			,
			Success["PatternMatcherLibraryInitialized", <|
				"MessageTemplate" -> "The Wolfram compiler library was initialized successfully.",
				"LibraryPath" -> File[$PatternMatcherLibrary],
				"Timing" -> Quantity[status[[1]], "Seconds"]
			|>]
		]
	];


InitializePatternMatcherLibraryInternal[OptionsPattern[InitializePatternMatcherLibrary]] /; !TrueQ[$InitializedQ] :=
	Module[{libPath, factoryMethodsLibFun, factoryMethods, factory},
		libPath = SafeLibraryLoad["PatternMatcherLibrary"];
		factoryMethodsLibFun = SafeLibraryFunctionLoad[libPath, "PatternMatcherLibrary_ObjectFactoryMethods", LinkObject, LinkObject];
		factoryMethods = factoryMethodsLibFun[];
		If[!MatchQ[factoryMethods, {Repeated[Rule[_String, _Integer], 1]}],
			ThrowFailure["InitializePatternMatcherLibrary", "`1` is not a list of {\"methodName\", functionPointer} pairs.", {factoryMethods}]
		];
		(*
			HandleDataStructureError products nice error messages, except that the tag is DataStructure.
			We could revisit the tag for the error, but since this is an internal function it is not 
			very important.
		*)
		Compile`Class`InitializeClassMethods[$ObjectFactoryClassName, {}, factoryMethods, HandleDataStructureError];
		factory = Compile`Utilities`Class`Impl`CreateObject[$ObjectFactoryClass, {}];
		If[factory["_class"] =!= $ObjectFactoryClassName,
			ThrowFailure["InitializePatternMatcherLibrary", "Failed to create the PatternMatcherLibrary`ObjectFactory object: `1`.", {factory}]
		];

		$PatternMatcherLibrary = libPath;
		$PatternMatcherObjectFactory = factory;
		$InitializedQ = True;
	];


UninitializePatternMatcherLibrary[] /; TrueQ[$InitializedQ] :=
	Module[{res, libPath},
		res = Quiet[LibraryUnload[$PatternMatcherLibrary]];
		If[FailureQ[res],
			ThrowFailure["UninitializePatternMatcherLibrary", "Failed to unload the PatternMatcher library. Details: `1`", {ToString[LibraryLink`$LibraryError]}]
		];
		libPath = $PatternMatcherLibrary;
		ClearAll[$PatternMatcherLibrary];
		ClearAll[$PatternMatcherObjectFactory];
		(* TODO: Uninitialize MExprEnvironment, etc. *)
		$InitializedQ = False;
		Success["PatternMatcherLibraryUninitialized", <|
			"MessageTemplate" -> "The PatternMatcher library was uninitialized successfully.",
			"LibraryPath" -> File[libPath]
		|>]
	];

End[]

EndPackage[]
