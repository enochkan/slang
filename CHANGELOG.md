# Changelog

## [Unreleased]

### Roadmap
- [ ] Add `string` type and string literals
- [ ] Add `for` loops and ranges (`for i in 0..10`)
- [ ] Add structs
- [ ] Add arrays and slices
- [ ] Add proper semantic analysis phase with user-facing error messages
- [ ] Add enums and `match` expressions
- [ ] Add SoA (Struct of Arrays) memory layout for structs (`#[columnar]`)
- [ ] Add region/arena allocators as a language primitive
- [ ] Add `comptime` compile-time evaluation
- [ ] Add SIMD vector types (`f64x4`, etc.)
- [ ] Add unchecked array access opt-in (`arr[i]!`)

---

## [0.2.0] - 2026-02-17

### Added
- **O3 optimization pipeline** via LLVM's new PassManager (`PassBuilder`)
  - Full O3 pass pipeline: inlining, dead code elimination, scalar optimizations
  - Loop vectorization enabled
  - SLP (superword-level parallelism) vectorization enabled
- **Native CPU detection** — compiler now targets the host machine's actual CPU
  (Apple Silicon, Intel with AVX2/AVX512, etc.) instead of `"generic"`
  - Enables hardware-specific instruction sets (NEON, AVX2, AVX512) automatically
- **`CodeGenOptLevel::Aggressive`** on the target machine for O3-level machine code emission

### Changed
- `emitObjectFile()` refactored to accept a shared `TargetMachine` instead of
  creating its own, eliminating redundant target setup
- `generate()` now owns the full compilation pipeline: target setup → optimization → emission

### Performance
- Constant folding across function call boundaries at compile time
  (e.g. `fibonacci(10)` is fully evaluated to `55` and inlined into the call site)
- Generated binaries are now performance-competitive with Rust and C++ at `-O3`

---

## [0.1.0] - Initial Release

### Added
- **Lexer** — tokenizes `.sl` source files with line/column tracking
- **Parser** — recursive descent parser with precedence climbing for expressions
- **AST** — typed node hierarchy for expressions and statements
- **Code generator** — LLVM IR backend producing `.ll` and `.o` output files
- **Type system** — `i32`, `f64`, `bool`, `void` primitive types with implicit coercion
- **Immutable-by-default variables** — `let x: i32 = 5;` / `let mut y: i32 = 10;`
- **Control flow** — `if` / `else`, `while` loops, `return`
- **Functions** — typed parameters, explicit return types, recursive calls
- **Operators** — arithmetic (`+`, `-`, `*`, `/`), comparison, logical (`&&`, `||`, `!`)
- **Built-in `print`** — outputs `i32`, `f64`, and `bool` values to stdout
- **Line comments** — `// comment`
- **GoogleTest suite** — 32 tests covering lexer, parser, and codegen
- **Example programs** — `fibonacci.sl`, `control_flow.sl`, `types.sl`, `example.sl`
- **CMake build system** with LLVM integration
