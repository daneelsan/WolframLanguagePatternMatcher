# Pattern Matcher VM Compilation: Comprehensive Notes

## 1. Introduction

This document describes the compilation strategy, architecture, and rationale behind the Wolfram Language Pattern Matcher VM project. It is intended as a reference for thesis writing and technical papers, providing a deep understanding of how high-level patterns are transformed into efficient, debuggable bytecode for a custom virtual machine.

---

## 2. Compilation Pipeline Overview

### 2.1. Input Representation

- **Pattern Expression**: The input is a Wolfram expression representing a pattern, e.g., `f[x_, 1]`, `f[_Integer | _String]`.
- **AST Construction**: The pattern is parsed into an abstract syntax tree (AST) using classes like `MExpr`, `MExprNormal`, and `MExprSymbol`. This enables recursive traversal and semantic analysis.

### 2.2. Compiler State Management

- **CompilerState**: Central structure for managing compilation.
	- Tracks available registers (`nextExprReg`, `nextBoolReg`).
	- Generates unique labels for control flow (`nextLabel`).
	- Maintains lexical variable mapping (`lexical`) for repeated variable detection.
	- Manages nested block scopes (`blockStack`) for temporaries and variable lifetimes.

### 2.3. Register Allocation

- **Expression Registers (`%eN`)**: Store subexpressions and intermediate results.
- **Boolean Registers (`%bN`)**: Store boolean results of tests and matches.
- **Register 0 (`%e0`, `%b0`)**: Reserved for input expression and final match result.

### 2.4. Label Management

- **Labels**: Used for control flow (jumps, alternatives, failure/success handlers).
- **Label Binding**: Each label is bound to an instruction index in the bytecode for fast lookup.

---

## 3. Bytecode Generation

### 3.1. Instruction Emission

- **emit(opcode, operands)**: Adds an instruction to the bytecode stream.
- **BEGIN_BLOCK / END_BLOCK**: Mark lexical scopes for variable bindings and temporaries. Blocks are used to manage lifetimes and unwind on failure.

### 3.2. Pattern Constructs and Compilation Strategies

#### 3.2.1. Literal Match

- **Strategy**: Load literal into a register, compare with input using `SAMEQ`, store result in a boolean register.
- **Jump on Failure**: If match fails, jump to failure label using `JUMP_IF_FALSE`.

#### 3.2.2. Blank Patterns

- **Blank[]**: Matches any expression; always succeeds.
- **Blank[f]**: Checks head of input using `MATCH_HEAD`; jumps to failure if not matched.

#### 3.2.3. Pattern Variables

- **First Occurrence**: Bind variable to input, store in lexical map, create a block for scope. Uses `BIND_VAR` and block management.
- **Repeated Occurrence**: Compare input with previously bound value using `SAMEQ` and `JUMP_IF_FALSE`.

#### 3.2.4. Alternatives (`|`)

- **TRY/RETRY/TRUST**: Implements backtracking for alternatives.
	- `TRY`: Creates a choice point for the first alternative.
	- `RETRY`: On failure, backtracks and tries the next alternative.
	- `TRUST`: Last alternative, removes choice point.
- **Label Management**: Each alternative is assigned a label for control flow.

#### 3.2.5. Normal Expressions

- **Head and Arguments**: Checks head and argument length, then recursively matches each argument.
- **Failure Handler**: On failure, unwinds block and jumps to outer failure label.
- **Temporary Registers**: Used for storing intermediate results and argument values.

### 3.3. Control Flow

- **JUMP / JUMP_IF_FALSE**: Used for branching on match results and propagating failure.
- **HALT**: Stops execution; used in success/failure blocks.

### 3.4. Backtracking Support

- **Choice Points**: Created for alternatives; store VM state for restoration.
- **Trail Stack**: Records variable bindings for undoing on backtrack.
- **CUT**: Removes choice points up to current frame (commits to a branch).
- **TRAIL_BIND**: Ensures variable bindings are reversible.

---

## 4. Optimization Passes

- **Dead Jump Removal**: Eliminates jumps with always-true conditions (e.g., after loading a constant true value).
- **Unreachable Code Removal**: (Planned) Removes code after unconditional jumps for efficiency.

---

## 5. Example Compilation Flow

### Example: `f[x_, 1]`

1. **Check Head**: `MATCH_HEAD %e0, f, failLabel`
2. **Check Length**: `TEST_LENGTH %b1, %e0, 2`; `JUMP_IF_FALSE %b1, failLabel`
3. **Match Arguments**:
	 - First argument: Bind `x_` to part 1, create block, store in lexical map.
	 - Second argument: Compare part 2 with literal `1`.
4. **Success/Failure Blocks**: On success, set `%b0 = true`; on failure, set `%b0 = false`.

### Example: `f[_Integer | _String]`

1. **TRY**: Create choice point for alternative.
2. **First Alternative**: Match `f[_Integer]`.
3. **RETRY**: On failure, backtrack and try `f[_String]`.
4. **TRUST**: Last alternative, remove choice point.

---

## 6. Design Rationale and Inspirations

- **WAM Inspiration**: The architecture borrows from the Warren Abstract Machine, using choice points, trail stack, and explicit control flow for efficient backtracking and pattern matching.
- **Extensibility**: The bytecode and compiler are designed to support advanced pattern features (e.g., Repeated, Alternatives, custom tests).
- **Debuggability**: Bytecode includes debug print instructions and block markers for tracing execution.
- **Separation of Concerns**: Compilation, execution, and optimization are modular, making the system maintainable and extensible.

---

## 7. Benefits and Tradeoffs

- **Performance**: Register allocation and direct bytecode execution minimize overhead and maximize speed.
- **Correctness**: Lexical scoping and trail management ensure correct variable binding and backtracking.
- **Maintainability**: Clear separation of compilation phases and modular opcode definitions.
- **Extensibility**: Easy to add new pattern features and optimizations.

---

## 8. Further Reading and References

- **Warren Abstract Machine (WAM)**: [A. S. Warren, "An Abstract Machine for Prolog Compilation and Execution"](https://en.wikipedia.org/wiki/Warren_Abstract_Machine)
- **Pattern Matching in Wolfram Language**: [Wolfram Documentation](https://reference.wolfram.com/language/guide/Patterns.html)
- **Prolog Compilation Techniques**: See classic texts on logic programming and pattern matching.

---

## 9. Suggested Diagrams and Visuals

- **Compilation Flowchart**: From pattern AST to bytecode emission.
- **Register Allocation Table**: Example mapping of pattern variables to registers.
- **Bytecode Example**: Annotated listing for a sample pattern.
- **Choice Point Stack**: Illustration of backtracking and state restoration.

---

## 10. Conclusion

The compilation strategy in this project provides a robust, extensible, and efficient foundation for symbolic pattern matching. By leveraging ideas from logic programming and VM design, it enables advanced features like backtracking, lexical scoping, and alternatives, while remaining maintainable and open to future enhancements. These notes should serve as a comprehensive reference for further research, thesis writing, and technical documentation.
