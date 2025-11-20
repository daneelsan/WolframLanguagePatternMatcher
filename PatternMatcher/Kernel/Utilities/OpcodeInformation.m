BeginPackage["DanielS`PatternMatcher`Utilities`OpcodeInformation`"]


$CategoriesToOpcodes::usage =
    "$CategoriesToOpcodes is an Association that maps opcode categories to lists of opcodes in those categories.";

$OpcodesToCategories::usage =
    "$OpcodesToCategories is an Association that maps opcodes to their corresponding categories.";


Begin["`Private`"]


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


End[]


EndPackage[]
