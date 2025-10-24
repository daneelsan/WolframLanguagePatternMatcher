BeginPackage["DanielS`PatternMatcher`"];


PatternToMatchFunction::usage =
	"PatternToMatchFunction[patt] converts a pattern patt into a Function[\[Ellipsis]].";


PatternBytecode::usage =
	"PatternBytecode[\[Ellipsis]] represents bytecode for a virtual machine of a given pattern expression.";


PatternBytecodeQ::usage =
	"PatternBytecodeQ[x] returns True if x is a valid PatternBytecode[...] object, and False otherwise.";


CompilePatternToBytecode::usage =
	"CompilePatternToBytecode[patt] converts a pattern expression into bytecode to be run in a virtual machine.";


PatternMatcherVirtualMachine::usage =
	"PatternMatcherVirtualMachine[\[Ellipsis]] represents a virtual machine that executes pattern bytecode.";


CreatePatternMatcherVirtualMachine::usage =
	"CreatePatternMatcherVirtualMachine[pattObj] creates a virtual machine for the pattern matcher object pattObj$.";


PatternMatcherExecute::usage =
	"RunPatternMatcher[vm, expr] does pattern matching on the expr using the pattern matcher virtual machine.";


PatternMatcherStep::usage =
	"PatternMatcherStep[vm] runs one step of the pattern matcher virtual machine.";


ResetPatternMatcherVirtualMachine::usage =
	"ResetPatternMatcherVirtualMachine[vm] resets the state of the pattern matcher virtual machine.";


Begin["`Private`"];


Get["DanielS`PatternMatcher`AST`"]
Get["DanielS`PatternMatcher`BackEnd`"]
Get["DanielS`PatternMatcher`FrontEnd`"]


End[];


EndPackage[];
