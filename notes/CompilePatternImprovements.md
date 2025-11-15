/* ============================================================
   OPTIMIZATION 2: Peephole - Eliminate Dead Jumps
   ============================================================ */

// Add to PatternBytecode class:
void optimizeBytecode()
{
	// Pass 1: Remove JUMP_IF_FALSE with always-true conditions
	auto& instrs = instructions;
	for (size_t i = 0; i + 1 < instrs.size(); )
	{
		// Pattern: LOAD_IMM %b, 1 followed by JUMP_IF_FALSE %b, L
		if (instrs[i].opcode == Opcode::LOAD_IMM && 
		    instrs[i+1].opcode == Opcode::JUMP_IF_FALSE)
		{
			auto dstBool = std::get_if<BoolRegOp>(&instrs[i].ops[0]);
			auto immBool = std::get_if<ImmBool>(&instrs[i].ops[1]);
			auto jumpBool = std::get_if<BoolRegOp>(&instrs[i+1].ops[0]);
			
			if (dstBool && immBool && jumpBool && 
			    dstBool->v == jumpBool->v && immBool->v == true)
			{
				// This is a dead jump - remove both instructions
				instrs.erase(instrs.begin() + i, instrs.begin() + i + 2);
				continue; // Don't increment i
			}
		}
		++i;
	}
	
	// Pass 2: Remove unreachable code after unconditional jumps
	// (More complex - would need to track which labels are targets)
}

/* ============================================================
   OPTIMIZATION 3: Register Allocation Improvements
   ============================================================ */

// Current: Linear allocation wastes registers
// Better: Track register liveness and reuse dead registers

struct RegisterAllocator
{
	std::vector<bool> exprRegInUse;
	std::vector<bool> boolRegInUse;
	size_t maxExprReg = 1; // %e0 reserved
	size_t maxBoolReg = 1; // %b0 reserved
	
	ExprRegIndex allocExprReg()
	{
		// Find first free register
		for (size_t i = 1; i < exprRegInUse.size(); ++i)
		{
			if (!exprRegInUse[i])
			{
				exprRegInUse[i] = true;
				return i;
			}
		}
		// Need new register
		size_t newReg = exprRegInUse.size();
		exprRegInUse.push_back(true);
		maxExprReg = std::max(maxExprReg, newReg);
		return newReg;
	}
	
	void freeExprReg(ExprRegIndex r)
	{
		if (r > 0 && r < exprRegInUse.size())
			exprRegInUse[r] = false;
	}
	
	// Similar for bool registers...
};

/* ============================================================
   ISA IMPROVEMENT 1: Add MATCH_HEAD Instruction
   ============================================================ */

// Current pattern (4 instructions):
//   GET_HEAD %e1, %e0
//   LOAD_IMM %e2, Expr("f")
//   SAMEQ %b2, %e1, %e2
//   JUMP_IF_FALSE %b2, L

// Better (1 instruction):
enum class Opcode {
	// ...
	MATCH_HEAD, /*  A imm label    if head(%e0) != imm goto label  */
};

// Usage:
st.emit(Opcode::MATCH_HEAD, { OpExprReg(0), OpImm(headExpr), OpLabel(innerFail) });

/* ============================================================
   ISA IMPROVEMENT 2: Add MATCH_LENGTH Instruction
   ============================================================ */

// Current pattern (2 instructions):
//   TEST_LENGTH %b1, %e0, 2
//   JUMP_IF_FALSE %b1, L

// Better (1 instruction):
enum class Opcode {
	// ...
	MATCH_LENGTH, /*  A len label   if length(%e0) != len goto label */
};

// Usage:
st.emit(Opcode::MATCH_LENGTH, { OpExprReg(0), OpImm(argsLen), OpLabel(innerFail) });

/* ============================================================
   ISA IMPROVEMENT 3: Add MATCH_LITERAL Instruction
   ============================================================ */

// Current pattern (3 instructions):
//   LOAD_IMM %e1, Expr("x")
//   SAMEQ %b1, %e0, %e1
//   JUMP_IF_FALSE %b1, L

// Better (1 instruction):
enum class Opcode {
	// ...
	MATCH_LITERAL, /* A imm label   if %e0 != imm goto label       */
};

/* ============================================================
   ISA IMPROVEMENT 4: Add Scoped Binding Instructions
   ============================================================ */

// Problem: Current BEGIN_BLOCK/END_BLOCK work but are verbose
// Better: Make BIND_VAR implicitly create scope

enum class Opcode {
	// ...
	BIND_SCOPED,  /* varName A depth  Bind var in new scope at depth */
	UNBIND_SCOPE, /* depth            Pop scope at depth             */
};

// This lets the VM track scope depth more efficiently

/* ============================================================
   ISA IMPROVEMENT 5: Add Pattern-Specific Instructions
   ============================================================ */

enum class Opcode {
	// ...
	// For BlankSequence, BlankNullSequence:
	MATCH_SEQUENCE,     /* varName minLen maxLen    */
	
	// For Alternatives:
	BEGIN_ALTERNATIVE,  /* label1 label2 ...        */
	NEXT_ALTERNATIVE,   /* label                    */
	
	// For Conditional patterns:
	PATTERN_TEST_PRED,  /* A predicate              */
	
	// For repeated patterns with Repeated, RepeatedNull:
	BEGIN_REPEAT,       /* varName minRep maxRep    */
	END_REPEAT,         /* label                    */
};

/* ============================================================
   COMPILATION IMPROVEMENT 1: Constant Folding
   ============================================================ */

// If a pattern is known at compile time to always match or fail:
static void compilePatternRec(...)
{
	// Check for trivially true/false patterns
	if (isAlwaysTrue(mexpr))
	{
		if (isTopLevel)
			st.emit(Opcode::JUMP, { OpLabel(successLabel) });
		return;
	}
	
	if (isAlwaysFalse(mexpr))
	{
		st.emit(Opcode::JUMP, { OpLabel(failLabel) });
		return;
	}
	
	// ... normal compilation
}

/* ============================================================
   COMPILATION IMPROVEMENT 2: Common Subpattern Elimination
   ============================================================ */

// Pattern: f[x_, g[x_]] has repeated x_ tests
// Optimization: Cache the comparison result

// Track which patterns have been tested:
struct CompilerState
{
	// ...
	std::unordered_map<std::string, BoolRegIndex> testedComparisons;
	
	// When comparing repeated variable:
	auto cacheKey = lexName + "_vs_e0";
	auto it = testedComparisons.find(cacheKey);
	if (it != testedComparisons.end())
	{
		// Reuse cached comparison
		st.emit(Opcode::JUMP_IF_FALSE, { OpBoolReg(it->second), OpLabel(outerFail) });
	}
	else
	{
		// Perform and cache comparison
		BoolRegIndex b = st.allocBoolReg();
		st.emit(Opcode::SAMEQ, { OpBoolReg(b), OpExprReg(storedReg), OpExprReg(0) });
		testedComparisons[cacheKey] = b;
		st.emit(Opcode::JUMP_IF_FALSE, { OpBoolReg(b), OpLabel(outerFail) });
	}
};

/* ============================================================
   COMPILATION IMPROVEMENT 3: Pattern Specialization
   ============================================================ */

// Generate specialized matchers for common patterns:
// - f[x_, x_] → SpecializedDoubleMatch
// - f[_, _] → SpecializedLengthCheck
// - f[1, 2, 3] → SpecializedLiteralMatch

// This can be 10-100x faster than interpreted matching

/* ============================================================
   DEBUGGING IMPROVEMENT: Better Disassembly
   ============================================================ */

std::string PatternBytecode::toString() const
{
	std::stringstream ss;
	
	// Show control flow graph
	ss << "Control Flow:\n";
	ss << "  Entry: L" << /* entry label */ << "\n";
	ss << "  Success: L" << /* success label */ << "\n";
	ss << "  Failure: L" << /* fail label */ << "\n\n";
	
	// Show register usage summary
	ss << "Registers:\n";
	ss << "  Expr: " << exprRegCount << " used\n";
	ss << "  Bool: " << boolRegCount << " used\n\n";
	
	// Show bytecode with annotations
	for (size_t pc = 0; pc < instructions.size(); ++pc)
	{
		// Show label if this instruction is a target
		auto labelIt = std::find_if(labels.begin(), labels.end(),
			[pc](const auto& p) { return p.second == pc; });
		if (labelIt != labels.end())
		{
			ss << "L" << labelIt->first << ":\n";
		}
		
		// Show instruction with live registers
		ss << std::setw(4) << pc << "  " << instructionToString(instructions[pc]);
		
		// Show which registers are live after this instruction
		// ss << "  ; live: %e" << ... << " %b" << ...
		
		ss << "\n";
	}
	
	return ss.str();
}

/* ============================================================
   SUMMARY OF IMPROVEMENTS
   ============================================================ */

/*
IMMEDIATE (Easy wins):
1. ✅ Remove JUMP_IF_FALSE after LOAD_IMM true (saves 2 instrs per Blank[])
2. ✅ Add MATCH_HEAD, MATCH_LENGTH, MATCH_LITERAL (4x reduction per pattern)
3. ✅ Peephole optimizer pass (10-20% smaller bytecode)

SHORT TERM (Moderate effort):
4. Register reuse/liveness analysis (30-50% fewer registers)
5. Constant folding for trivial patterns
6. Better disassembly with CFG visualization

LONG TERM (High effort, high reward):
7. Pattern specialization / JIT compilation (10-100x speedup)
8. Common subpattern elimination
9. Advanced pattern-specific opcodes (MATCH_SEQUENCE, etc.)
10. Type-based optimizations (if type info available)

ESTIMATED IMPACT:
- Current: f[x_, x_] → 42 instructions, 7 regs, 6 bool regs
- With #1-3: f[x_, x_] → 18 instructions, 4 regs, 2 bool regs
- With #4-6: f[x_, x_] → 16 instructions, 3 regs, 1 bool reg
- With #7-9: Direct native code, ~5-10 CPU instructions

Your bytecode is now CORRECT and FUNCTIONAL. These are optimizations
to make it faster and more compact. Start with #1-3 for quick wins!
*/