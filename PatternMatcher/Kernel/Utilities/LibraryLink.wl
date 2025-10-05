BeginPackage["DanielS`PatternMatcher`Utilities`LibraryLink`"]


SafeLibraryLoad::usage =
	"SafeLibraryLoad[lib] attempts to load a library and throws an error if it fails.";

SafeLibraryFunctionLoad::usage =
	"SafeLibraryFunctionLoad[libName, fname, fParams, retType] attempts to load a function from a library and throws an error if it fails."


Begin["`Private`"]


Needs["DanielS`PatternMatcher`ErrorHandling`"] (* for ThrowFailure *)


SafeLibraryLoad[lib_] :=
	Quiet @ Module[{libPath},
		libPath = LibraryLoad[lib];
		If[MatchQ[libPath, _?FailureQ | _LibraryLoad],
			ThrowFailure["SafeLibraryLoad", "Failed to load library `1`. Details: `2`.", {lib, ToString[LibraryLink`$LibraryError]}]
		];
		libPath
	];


SafeLibraryFunctionLoad[libName_?StringQ, fname_?StringQ, fParams_, retType_] :=
	Quiet @Module[{libFunction},
		libFunction = LibraryFunctionLoad[libName, fname, fParams, retType];
		If[MatchQ[libFunction, _?FailureQ | _LibraryFunctionLoad],
			ThrowFailure["SafeLibraryFunctionLoad", "Failed to load the function `1` from `2`. Details: `3`.", {fname, libName, ToString[LibraryLink`$LibraryError]}]
		];
		libFunction
	];


End[]

EndPackage[]
