#pragma once

#include "VM/PatternBytecode.h"

#include "ClassSupport.h"
#include "Expr.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <unordered_map>

namespace PatternMatcher
{
/*===========================================================================
VirtualMachine: Pattern Matching Virtual Machine

A register-based VM that executes pattern matching bytecode with backtracking
support. The VM implements a subset of the Warren Abstract Machine (WAM)
design used in Prolog, adapted for Mathematica-style pattern matching.

Architecture:
- Register-based execution model (not stack-based)
- Expression registers (%e0, %e1, ...) for Expr values
- Boolean registers (%b0, %b1, ...) for comparison results
- Frame stack for lexical scoping and variable bindings
- Choice point stack for backtracking (alternatives)
- Trail for undoing variable bindings on backtrack

Execution Model:
1. Load bytecode via initialize()
2. Set input expression in %e0
3. Execute instructions via match() or step()
4. Result in %b0, bindings in resultFrame

Example:
  VirtualMachine vm;
  auto bytecode = CompilePatternToBytecode(pattern);
  vm.initialize(bytecode);
  bool matched = vm.match(inputExpr);
  auto bindings = vm.getResultBindings();
===========================================================================*/

extern Expr VirtualMachineExpr();

class VirtualMachine
{
public:
	VirtualMachine();
	~VirtualMachine();

	//=========================================================================
	// Frame: Lexical Scope for Variable Bindings
	//=========================================================================

	/// @brief Represents a call frame for variable bindings
	///
	/// Each frame corresponds to a lexical scope (created by BEGIN_BLOCK).
	/// Bindings are stored as name -> value mappings.
	/// Frames can be nested (frame stack), and bindings can be merged
	/// from child frames to parent frames on successful pattern match.
	struct Frame
	{
		using Bindings = std::unordered_map<std::string, Expr>;
		Bindings bindings; ///< Variable name -> bound value

		/// Bind or update a variable in this frame
		void bindVariable(const std::string& name, const Expr& value)
		{
			bindings.insert_or_assign(name, value);
			return;
		}

		/// Check if variable is bound in this frame
		bool hasVariable(const std::string& name) const { return bindings.find(name) != bindings.end(); }

		/// Get a variable binding (nullopt if not found)
		std::optional<Expr> getVariable(const std::string& name) const
		{
			auto it = bindings.find(name);
			if (it != bindings.end())
				return it->second;
			return std::nullopt;
		}

		/// Clear all bindings in this frame
		void reset() { bindings.clear(); }
	};

	//=========================================================================
	// ChoicePoint: Saved State for Backtracking
	//=========================================================================

	/// @brief Represents a saved state for backtracking (alternatives)
	///
	/// When TRY creates a choice point, it saves:
	/// - All registers (for restoration on backtrack)
	/// - Frame depth (to pop inner frames)
	/// - Trail position (to undo variable bindings)
	/// - Next alternative label (where to jump on FAIL)
	///
	/// Choice points implement non-deterministic choice in patterns like:
	///   p1 | p2 | p3
	///
	/// If p1 fails, FAIL triggers backtrack:
	/// - Restore registers from choice point
	/// - Pop frames to saved depth
	/// - Unwind trail to undo bindings
	/// - Jump to next alternative (p2)
	struct ChoicePoint
	{
		size_t returnPC; ///< PC when choice point was created (for debugging)
		size_t nextAlternative; ///< Label to jump to on backtrack
		std::vector<Expr> savedExprRegs; ///< Snapshot of expression registers
		std::vector<bool> savedBoolRegs; ///< Snapshot of boolean registers
		size_t trailMark; ///< Trail size to restore to
		size_t frameMark; ///< Frame stack depth to restore to

		ChoicePoint(size_t returnPC_, size_t nextAlt_, const std::vector<Expr>& exprRegs_,
					const std::vector<bool>& boolRegs_, size_t trailMark_, size_t frameMark_)
			: returnPC(returnPC_)
			, nextAlternative(nextAlt_)
			, savedExprRegs(exprRegs_)
			, savedBoolRegs(boolRegs_)
			, trailMark(trailMark_)
			, frameMark(frameMark_)
		{
		}
	};

	//=========================================================================
	// TrailEntry: Undo Log for Variable Bindings
	//=========================================================================

	/// @brief Trail entry for undoing variable bindings on backtrack
	///
	/// The trail records every variable binding made while choice points exist.
	/// On backtrack, we unwind the trail to the choice point's mark,
	/// effectively undoing all bindings made in the failed alternative.
	///
	/// Example: In pattern (x_Integer | x_Real) matching 4.2:
	/// 1. TRY creates choice point (trailMark=0)
	/// 2. First alt binds x -> 4.2 (trail=[{x, frame=1}])
	/// 3. _Integer fails, FAIL unwinds trail (unbinds x)
	/// 4. Second alt binds x -> 4.2 again
	/// 5. _Real matches -> success
	struct TrailEntry
	{
		std::string varName; ///< Variable to unbind
		size_t frameIndex; ///< Which frame the binding is in

		TrailEntry(std::string name, size_t frame)
			: varName(std::move(name))
			, frameIndex(frame)
		{
		}
	};

	//=========================================================================
	// State Accessors (Read-Only)
	//=========================================================================

	/// Get the number of executed instruction cycles
	size_t getCycles() const { return cycles; }

	/// Get the current program counter (instruction index)
	size_t getPC() const { return pc; }

	/// Get the loaded bytecode (if any)
	std::optional<std::shared_ptr<PatternBytecode>> getBytecode() const { return bytecode; }

	/// Check if VM has halted (HALT instruction or error)
	bool isHalted() const { return halted; }

	/// Check if VM has been initialized with bytecode
	bool isInitialized() const { return initialized; }

	/// Get the final result bindings after successful match
	/// @return Map of variable name -> bound value
	/// @note Only valid after match() returns true and EXPORT_BINDINGS executed
	const Frame::Bindings& getResultBindings() const { return resultFrame.bindings; }

	/// Check if there are active choice points (for backtracking)
	bool hasChoicePoints() const { return !choiceStack.empty(); }

	//=========================================================================
	// Lifecycle Management
	//=========================================================================

	/// @brief Initialize VM with compiled bytecode
	/// @param bytecode_ The compiled pattern bytecode to execute
	/// @note Must be called before match() or step()
	void initialize(const std::shared_ptr<PatternBytecode>& bytecode_);

	/// @brief Shutdown VM and release resources
	/// @note Clears bytecode, registers, and all runtime state
	void shutdown();

	/// @brief Reset VM to initial state (keeps bytecode loaded)
	/// @note Resets PC, registers, frames, choice stack, trail
	/// @note Useful for matching against multiple inputs with same pattern
	void reset();

	//=========================================================================
	// Execution
	//=========================================================================

	/// @brief Execute pattern match against input expression
	/// @param input The expression to match against
	/// @return true if pattern matches, false otherwise
	/// @note Executes until HALT or error
	/// @note Bindings available via getResultBindings() on success
	bool match(Expr input);

	/// @brief Execute a single instruction (for debugging/tracing)
	/// @return false if halted or error, true otherwise
	bool step();

	/// @brief Get the current match result from %b0
	/// @return The boolean value in register %b0
	bool currentBoolResult();

	//=========================================================================
	// Internal Operations (used by instruction implementations)
	//=========================================================================

	/// @brief Jump to a label (unconditional or on failure)
	/// @param label The label to jump to
	/// @param isFailure true if this is a failure jump (sets unwinding flag)
	void jump(LabelOp label, bool isFailure);

	/// @brief Save bindings from current frame to target frame
	/// @param frame The frame to save bindings into
	/// @note Used by END_BLOCK and EXPORT_BINDINGS
	void saveBindings(Frame& frame);

	//=========================================================================
	// Backtracking Operations
	//=========================================================================

	/// @brief Create a choice point for backtracking (TRY instruction)
	/// @param nextAlternative Label to jump to on backtrack
	/// @note Saves current state: registers, frames, trail
	void createChoicePoint(size_t nextAlternative);

	/// @brief Backtrack to most recent choice point (FAIL instruction)
	/// @return false if no choice points exist (permanent failure)
	/// @note Restores state and jumps to next alternative
	/// @note Does NOT pop the choice point (RETRY/TRUST handle that)
	bool backtrack();

	/// @brief Remove all choice points (CUT instruction)
	/// @note Commits to current choice, prevents backtracking
	void commit();

	//=========================================================================
	// Trail Operations (for undoing bindings)
	//=========================================================================

	/// @brief Bind a variable with trail support
	/// @param varName Variable name to bind
	/// @param value Value to bind to
	/// @note Creates trail entry if choice points exist
	void trailBind(const std::string& varName, const Expr& value);

	/// @brief Unwind trail to a previous mark (undo bindings)
	/// @param mark Trail size to restore to
	/// @note Called by backtrack() to undo failed alternative's bindings
	void unwindTrail(size_t mark);

	//=========================================================================
	// Embedded Object Interface (for Wolfram Language)
	//=========================================================================

	/// Initialize embedded methods for VirtualMachine class
	void initializeEmbedMethods(const char* embedName);

private:
	//=========================================================================
	// VM State
	//=========================================================================

	/// VM lifecycle
	bool initialized = false; ///< Has bytecode been loaded?
	bool halted = false; ///< Has execution stopped?

	/// Execution flags
	bool backtracking = false; ///< Currently backtracking?
	bool unwindingFailure = false; ///< Unwinding after failure?

	/// Execution state
	size_t pc = 0; ///< Program counter (current instruction index)
	size_t cycles = 0; ///< Number of instructions executed

	/// Loaded bytecode
	std::optional<std::shared_ptr<PatternBytecode>> bytecode = std::nullopt;

	//=========================================================================
	// Runtime State
	//=========================================================================

	/// Lexical scope stack (BEGIN_BLOCK/END_BLOCK)
	std::vector<Frame> frames;

	/// Register file
	std::vector<Expr> exprRegs; ///< Expression registers (%e0, %e1, ...)
	std::vector<bool> boolRegs; ///< Boolean registers (%b0, %b1, ...)

	/// Result frame (for EXPORT_BINDINGS)
	Frame resultFrame;

	//=========================================================================
	// Backtracking State
	//=========================================================================

	/// Choice point stack (for alternatives)
	std::vector<ChoicePoint> choiceStack;

	/// Trail (undo log for variable bindings)
	std::vector<TrailEntry> trail;
};

template <>
inline const char* EmbedName<VirtualMachine>()
{
	return "PatternMatcherLibrary`VM`VirtualMachine";
}
}; // namespace PatternMatcher