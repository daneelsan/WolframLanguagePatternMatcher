BeginPackage["DanielS`PatternMatcherVM`LibraryLink`"];


InitializePatternMatcherVirtualMachineLibrary

$PatternMatcherVirtualMachineLibraryPath

$ObjectFactory


Begin["`Private`"]


Needs["CompileUtilities`Error`Exceptions`"]

Needs["DataStructure`Utilities`"]


$PatternMatcherVirtualMachineLibraryPath = Null;


InitializePatternMatcherVirtualMachineLibrary[] :=
	Module[{data},
		$PatternMatcherVirtualMachineLibraryPath = "/Users/daniels/Library/CloudStorage/OneDrive-Personal/projects/thesis/wolfram-vm/zig-out/lib/libPatternMatcherVirtualMachine.dylib";
		If[!FileQ[$PatternMatcherVirtualMachineLibraryPath],
			$PatternMatcherVirtualMachineLibraryPath = Null;
			Return[$Failed]
		];
		LibraryLoad[$PatternMatcherVirtualMachineLibraryPath];
		$ObjectFactory = LibraryFunctionLoad[
			$PatternMatcherVirtualMachineLibraryPath,
			"PatternMatcherVirtualMachineLibrary_ObjectFactory",
			LinkObject,
			LinkObject
		];
		data = $ObjectFactory[];
		If[!MatchQ[data, {Repeated[{_String, _Integer}, 1]}],
			Return[$Failed]
		];
		data = Apply[Rule, data, {1}];
		Compile`Class`InitializeClassMethods[
			"PatternMatcherVirtualMachine`ObjectFactory",
			{},
			data,
			HandleDataStructureError
		];
		Compile`Utilities`Class`Impl`CreateObject[PatternMatcherVirtualMachine`ObjectFactory, {}]
	];


End[]

EndPackage[]
