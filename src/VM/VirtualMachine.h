#pragma once

#include "VM/PatternBytecode.h"

#include "ClassSupport.h"
#include "Expr.h"

#include <memory>
#include <optional>

namespace PatternMatcher
{
extern Expr VirtualMachineExpr();

class VirtualMachine
{
private:
	bool initialized = false;
	bool halted = false;

	size_t pc = 0; // program counter
	size_t cycles = 0; // number of cycles executed

	// Current bytecode
	std::optional<std::shared_ptr<PatternBytecode>> bytecode = std::nullopt;

	// runtime binding frames for pattern variables
	// each frame maps var name -> Expr value
	std::vector<std::unordered_map<std::string, Expr>> frames;

	// Registers
	std::vector<Expr> exprRegs;
	std::vector<bool> boolRegs;

public:
	VirtualMachine();
	~VirtualMachine();

	// --- State ---
	size_t getCycles() const { return cycles; };
	size_t getPC() const { return pc; };
	std::optional<std::shared_ptr<PatternBytecode>> getBytecode() const { return bytecode; };
	bool isHalted() const { return halted; };
	bool isInitialized() const { return initialized; };

	// --- Lifecycle ---
	void initialize(const std::shared_ptr<PatternBytecode>& bytecode_);
	void shutdown();
	void reset();

	// --- Execution ---
	bool match(Expr input); // executes until HALT or return
	bool step(); // executes one instruction
	bool currentBoolResult(); // returns %b0 (final match result)

	/// @brief Initializes the embedded methods for the VirtualMachine class.
	/// @param embedName The name to use for embedding.
	void initializeEmbedMethods(const char* embedName);
};

template <>
inline const char* EmbedName<VirtualMachine>()
{
	return "PatternMatcherLibrary`VM`VirtualMachine";
}
}; // namespace PatternMatcher