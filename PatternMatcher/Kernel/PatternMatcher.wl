Unprotect["DanielS`PatternMatcher`*"];
ClearAll["DanielS`PatternMatcher`*"];
ClearAll["DanielS`PatternMatcher`*`*"];


BeginPackage["DanielS`PatternMatcher`"];


CompilePatternToFunction::usage =
	"CompilePatternToFunction[patt] converts a pattern patt into a Function[\[Ellipsis]].";


PatternBytecode::usage =
	"PatternBytecode[\[Ellipsis]] represents bytecode for a virtual machine of a given pattern expression.";


CompilePatternToBytecode::usage =
	"CompilePatternToBytecode[patt] converts a pattern expression into bytecode to be run in a virtual machine.";


PatternMatcherVirtualMachine::usage =
	"PatternMatcherVirtualMachine[\[Ellipsis]] represents a virtual machine that executes pattern bytecode.";


CreatePatternMatcherVirtualMachine::usage =
	"CreatePatternMatcherVirtualMachine[pattObj] creates a virtual machine for the pattern matcher object pattObj$.";


PatternMatcherMatch::usage =
	"PatternMatcherMatch[vm, expr] does pattern matching on the expr using the pattern matcher virtual machine.";


PatternMatcherStep::usage =
	"PatternMatcherStep[vm] runs one step of the pattern matcher virtual machine.";


ResetPatternMatcherVirtualMachine::usage =
	"ResetPatternMatcherVirtualMachine[vm] resets the state of the pattern matcher virtual machine.";


Begin["`Private`"];


(*
	TODO: Remove this hack. CreateMExpr should be independent of the compiler.
*)
(*FunctionCompile[Function[1]];*)


Needs["DanielS`PatternMatcher`FrontEnd`"]
Needs["DanielS`PatternMatcher`BackEnd`"]


End[];


EndPackage[];


SetAttributes[#, {Protected, ReadProtected}] & /@ Evaluate @ Names["DanielS`PatternMatcher`*"];
