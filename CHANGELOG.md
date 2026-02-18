# Changelog

## [Unreleased]

### Roadmap
- [ ] Add `string` type and string literals
- [ ] Add proper semantic analysis phase with user-facing error messages
- [ ] Add enums and `match` expressions
- [ ] Add SoA (Struct of Arrays) memory layout for structs (`#[columnar]`)
- [ ] Add region/arena allocators as a language primitive
- [ ] Add `comptime` compile-time evaluation
- [ ] Add SIMD vector types (`f64x4`, etc.)
- [ ] Add unchecked array access opt-in (`arr[i]!`)

---

## [0.4.0] - 2026-02-18

### Added
- **`struct` declarations** — named composite types with typed fields
  ```
  struct Vec2 { x: f64, y: f64, }
  ```
- **Scalar struct variables** — `let [mut] p: Point = Point { x: 1.0, y: 2.0 };`
- **Struct field read** — `p.x` in any expression context
- **Struct field write** — `p.x = val;` (mutable structs only)
- **SoA (Struct of Arrays) arrays** — `let [mut] points: [Vec2; N];`
  - Memory layout is `{ [N x f64], [N x f64] }` not `[N x {f64, f64}]`
  - All `x` values are stored contiguously → cache-friendly, auto-vectorisable loops
  - Zero-initialised by default; no explicit initialiser required
- **SoA field read** — `points[i].x` in any expression context
- **SoA field write** — `points[i].x = val;` (mutable arrays only)
- **New tokens**: `struct`, `.`
- **Struct registry** in codegen (`structDefs`, `llvmStructTypes`) keyed by struct name
- **`getSoAType()`** helper builds the SoA LLVM struct type for a given struct + size
- **Example program**: `examples/structs.sl` — scalar structs, SoA arrays, particle sim step

### Changed
- `Program` AST node now carries `structs` vector alongside `functions`
- `LetStmt` extended with two new constructors (struct, struct-array) and `structName` field
- `VarInfo` extended with `structName` for struct and struct-array symbol table entries
- `parseProgram()` now parses struct declarations first to populate `structNames` set,
  allowing `parsePrimary` to safely recognise struct init literals without ambiguity

### Architecture Note
- Struct declarations **must precede** function declarations in source files (enforced by parser)
- Struct init `Name { ... }` is only valid in let-statement initialiser position

---

## [0.3.0] - 2026-02-18

### Added
- **Arrays** — fixed-size stack-allocated arrays with type annotation `[elemType; N]`
  - Array literals: `let arr: [i32; 5] = [1, 2, 3, 4, 5];`
  - Immutable and mutable arrays (`let mut arr: [i32; 3] = [0, 0, 0];`)
  - Array index read: `arr[i]` (works in any expression context)
  - Array index write: `arr[i] = val;` (mutable arrays only)
  - LLVM codegen via `ArrayType::get` + `GEP` instructions
- **`for` loops with range syntax** — `for i in start..end { body }`
  - Loop variable is scoped to the loop body and immutable
  - Dynamic ranges supported (`for i in 0..n` where `n` is a variable)
  - Compiles to optimized condition/body/after basic block structure
- **New tokens**: `[`, `]`, `..`, `for`, `in`
- **Example program**: `examples/arrays.sl` — sum array, dot product with mutation

### Changed
- `LetStmt` AST node extended with `elemType` and `arraySize` fields for arrays
- `VarInfo` in codegen extended with `elemType` and `arraySize` for symbol table tracking
- Parser's `parsePrimary` now handles array literals and array index reads
- Parser's `parseAssignOrExprStmt` now handles array index assignment

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
