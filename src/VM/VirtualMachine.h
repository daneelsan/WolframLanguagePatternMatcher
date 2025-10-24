#pragma once

#include "VM/PatternBytecode.h"

#include "ClassSupport.h"
#include "Expr.h"

#include <memory>
#include <optional>
#include <vector>
#include <unordered_map>

namespace PatternMatcher
{
extern Expr VirtualMachineExpr();

class VirtualMachine
{
public:
	VirtualMachine();
	~VirtualMachine();

	/// @brief Represents a saved state for backtracking
	struct ChoicePoint
	{
		size_t pc; // Program counter to resume at
		size_t nextAlternative; // Label for next alternative
		std::vector<Expr> savedExprRegs; // Saved expression registers
		std::vector<bool> savedBoolRegs; // Saved boolean registers
		size_t trailMark; // Trail position to restore to
		size_t frameMark; // Frame depth to restore to

		ChoicePoint(size_t pc_, size_t nextAlt_, const std::vector<Expr>& exprRegs_, const std::vector<bool>& boolRegs_,
					size_t trailMark_, size_t frameMark_)
			: pc(pc_)
			, nextAlternative(nextAlt_)
			, savedExprRegs(exprRegs_)
			, savedBoolRegs(boolRegs_)
			, trailMark(trailMark_)
			, frameMark(frameMark_)
		{
		}
	};

	/// @brief Trail entry for undoing variable bindings
	struct TrailEntry
	{
		std::string varName;
		size_t frameIndex; // Which frame the binding was in

		TrailEntry(std::string name, size_t frame)
			: varName(std::move(name))
			, frameIndex(frame)
		{
		}
	};

	// --- State ---
	size_t getCycles() const { return cycles; };
	size_t getPC() const { return pc; };
	std::optional<std::shared_ptr<PatternBytecode>> getBytecode() const { return bytecode; };
	bool isHalted() const { return halted; };
	bool isInitialized() const { return initialized; };
	const std::unordered_map<std::string, Expr>& getResultBindings() const { return resultBindings; }

	// --- Lifecycle ---
	void initialize(const std::shared_ptr<PatternBytecode>& bytecode_);
	void shutdown();
	void reset();

	// Backtracking methods
	void createChoicePoint(size_t nextAlternative);
	bool backtrack(); // Returns false if no more choice points
	void commit(); // Remove choice points (for optimization/cuts)
	bool hasChoicePoints() const { return !choiceStack.empty(); }

	// Trail operations
	void trailBind(const std::string& varName, const Expr& value);
	void unwindTrail(size_t mark);

	// --- Execution ---
	bool match(Expr input); // executes until HALT or return
	bool step(); // executes one instruction
	bool currentBoolResult(); // returns %b0 (final match result)

	/// @brief Initializes the embedded methods for the VirtualMachine class.
	/// @param embedName The name to use for embedding.
	void initializeEmbedMethods(const char* embedName);

private:
	bool initialized = false;
	bool halted = false;
	bool backtracking = false;

	size_t pc = 0; // program counter
	size_t cycles = 0; // number of cycles executed

	// Current bytecode
	std::optional<std::shared_ptr<PatternBytecode>> bytecode = std::nullopt;

	// runtime binding frames for pattern variables
	// each frame maps var name -> Expr value
	std::vector<std::unordered_map<std::string, Expr>> frames;
	std::vector<Expr> exprRegs;
	std::vector<bool> boolRegs;

	std::unordered_map<std::string, Expr> resultBindings;

	// Backtracking support
	std::vector<ChoicePoint> choiceStack; // Stack of choice points
	std::vector<TrailEntry> trail; // Trail of variable bindings to undo
};

template <>
inline const char* EmbedName<VirtualMachine>()
{
	return "PatternMatcherLibrary`VM`VirtualMachine";
}
}; // namespace PatternMatcher