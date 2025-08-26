#pragma once

#include "WolframLanguageRuntimeV1SDK.h"

#include <memory>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

class PatternMatcher
{
public:
    // VM state
    struct VMState
    {
        wlr_expr current_expr;
        std::stack<wlr_expr> stack;
        std::unordered_map<std::string, wlr_expr> bound_vars;
        size_t pc = 0;
        bool halt = false;

        // For alternatives
        std::stack<bool> alt_stack;
        std::vector<bool> current_alts;
    };

    // Instruction types
    enum class OpCode
    {
        PUSH_EXPR,
        PUSH_TRUE,
        PUSH_FALSE,
        PUSH_SYMBOL,
        AND,
        OR,
        NOT,
        SAMEQ,
        GET_HEAD,
        GET_PART,
        TEST_HEAD,
        TEST_LENGTH,
        BLANK,
        PATTERN,
        PATTERN_TEST,
        EXCEPT,
        ALTERNATIVES,
        ALT_BRANCH,
        BEGIN_BLOCK,
        END_BLOCK,
        BRANCH,
        BRANCH_FALSE,
        LOOP,
        BIND_VAR,
        GET_VAR,
        TEST_VAR,
        LABEL // For branching targets
    };

    struct Instruction
    {
        OpCode op;
        std::string symbol_arg;
        int int_arg;
    };

    PatternMatcher();
    ~PatternMatcher();

    // Compile a pattern to bytecode
    std::vector<Instruction> compilePattern(wlr_expr pattern);

    // Execute bytecode on an expression
    bool execute(const std::vector<Instruction> &bytecode, wlr_expr expr);

private:
    // Helper functions
    void push(VMState &state, wlr_expr expr);
    wlr_expr pop(VMState &state);
    void releaseAll(VMState &state);

    // Instruction implementations
    void executePushExpr(VMState &state);
    void executePushTrue(VMState &state);
    void executePushFalse(VMState &state);
    void executePushSymbol(VMState &state, const std::string &symbol);
    void executeAnd(VMState &state);
    void executeOr(VMState &state);
    void executeNot(VMState &state);
    void executeSameQ(VMState &state);
    void executeGetHead(VMState &state);
    void executeGetPart(VMState &state, int index);
    void executeTestHead(VMState &state, const std::string &head);
    void executeTestLength(VMState &state, int length);
    void executeBlank(VMState &state);
    void executePattern(VMState &state, const std::string &symbol);
    void executePatternTest(VMState &state, const std::string &test);
    void executeExcept(VMState &state);
    void executeAlternatives(VMState &state, int count);
    void executeAltBranch(VMState &state);
    void executeBindVar(VMState &state, const std::string &symbol);
    void executeGetVar(VMState &state, const std::string &symbol);
    void executeTestVar(VMState &state, const std::string &symbol);
    void executeBranch(VMState &state, int offset);
    void executeBranchFalse(VMState &state, int offset);
};
