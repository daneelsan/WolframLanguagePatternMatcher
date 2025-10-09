#pragma once

#include "ClassSupport.h"
#include "Expr.h"

namespace PatternMatcher
{
extern Expr VirtualMachineExpr();

class VirtualMachine
{
private:
	bool initialized = false;

	size_t pc = 0; // program counter
	size_t cycles = 0; // number of cycles executed
	bool halted = false;

public:
	VirtualMachine();
	~VirtualMachine();

	size_t getCycles() const { return cycles; };
	size_t getPC() const { return pc; };
	bool isHalted() const { return halted; };

	void initialize();
	void shutdown();

	/// @brief Reset the virtual machine to its initial state.
	void reset();

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