#pragma once

#include "VM/PatternBytecode.h"

#include "Expr.h"

#include <memory>

namespace PatternMatcher
{
std::shared_ptr<PatternBytecode> CompilePatternToBytecode(const Expr& pattern);
}; // namespace PatternMatcher