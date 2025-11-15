(*
	OptimizePatternBytecode
	
Add a post-compilation optimization that removes:

- Empty blocks (BEGIN_BLOCK followed immediately by END_BLOCK with only jumps)
- Unreachable code (blocks never jumped to)
*)