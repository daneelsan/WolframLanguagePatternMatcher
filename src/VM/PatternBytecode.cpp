#include "VM/PatternBytecode.h"
#include "VM/Opcode.h"

#include "AST/MExpr.h"
#include "AST/MExprPatternTools.h"

#include "Embeddable.h"
#include "Expr.h"
#include "Logger.h"

#include <iomanip>
#include <initializer_list>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace PatternMatcher
{
std::string PatternBytecode::toString() const
{
	std::ostringstream out;

	auto formatLabel = [&](Label L) {
		auto it = labelMap.find(L);
		if (it != labelMap.end())
			out << "\nL" << L << ":\n";
	};

	out << std::left;
	size_t width = 18; // align operands roughly

	for (size_t i = 0; i < _instrs.size(); ++i)
	{
		// Check if this instruction is the start of a label
		for (auto& kv : labelMap)
		{
			if (kv.second == i)
			{
				out << "\nL" << kv.first << ":\n";
				break;
			}
		}

		// Print opcode and operands
		out << std::setw(4) << i << "  " << std::setw(width) << opcodeName(_instrs[i].opcode);

		if (!_instrs[i].ops.empty())
		{
			bool first = true;
			for (auto& op : _instrs[i].ops)
			{
				if (!first)
					out << ", ";
				out << operandToString(op);
				first = false;
			}
		}
		out << "\n";
	}

	out << "\n----------------------------------------\n";
	out << "Expr registers: " << exprRegisterCount << ", Bool registers: " << boolRegisterCount << "\n";

	if (!lexicalMap.empty())
	{
		out << "Lexical bindings:\n";
		for (auto& p : lexicalMap)
			out << "  " << std::setw(12) << p.first << " → %e" << p.second << "\n";
	}

	return out.str();
}

std::optional<size_t> PatternBytecode::resolveLabel(Label L) const
{
	auto it = labelMap.find(L);
	if (it != labelMap.end())
		return it->second;
	throw std::nullopt;
}

namespace PatternBytecodeInterface
{
	Expr getBoolRegisterCount(std::shared_ptr<PatternBytecode> bytecode)
	{
		return Expr(static_cast<mint>(bytecode->getBoolRegisterCount()));
	}
	Expr getExprRegisterCount(std::shared_ptr<PatternBytecode> bytecode)
	{
		return Expr(static_cast<mint>(bytecode->getExprRegisterCount()));
	}
	Expr getPattern(std::shared_ptr<PatternBytecode> bytecode)
	{
		return MExpr::toExpr(bytecode->getPattern());
	}
	Expr length(std::shared_ptr<PatternBytecode> bytecode)
	{
		return Expr(static_cast<mint>(bytecode->length()));
	}
	Expr toBoxes(Expr objExpr, Expr fmt)
	{
		return Expr::construct("DanielS`PatternMatcher`BackEnd`PatternBytecode`Private`toBoxes", objExpr, fmt);
	}
	Expr toString(std::shared_ptr<PatternBytecode> bytecode)
	{
		return Expr(bytecode->toString());
	}
}; // namespace PatternBytecodeInterface

void PatternBytecode::initializeEmbedMethods(const char* embedName)
{
	RegisterMethod<std::shared_ptr<PatternBytecode>, PatternBytecodeInterface::getBoolRegisterCount>(
		embedName, "getBoolRegisterCount");
	RegisterMethod<std::shared_ptr<PatternBytecode>, PatternBytecodeInterface::getExprRegisterCount>(
		embedName, "getExprRegisterCount");
	RegisterMethod<std::shared_ptr<PatternBytecode>, PatternBytecodeInterface::getPattern>(embedName, "getPattern");
	RegisterMethod<std::shared_ptr<PatternBytecode>, PatternBytecodeInterface::length>(embedName, "length");
	RegisterMethod<std::shared_ptr<PatternBytecode>, PatternBytecodeInterface::toBoxes>(embedName, "toBoxes");
	RegisterMethod<std::shared_ptr<PatternBytecode>, PatternBytecodeInterface::toString>(embedName, "toString");
}
}; // namespace PatternMatcher