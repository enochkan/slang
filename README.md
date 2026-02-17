# slang

S-Lang, a Blazingly Fast Programming Language for Data Engineering

## Prerequisites

- CMake 3.14+
- LLVM (install via `brew install llvm` on macOS)
- A C++17 compiler

## Build

```bash
cmake -B build
cmake --build build
```

## Run

```bash
./build/slangcc examples/example.sl
clang output.o -o example && ./example
```

## Test

```bash
cd build && ctest --output-on-failure
```

## Syntax

```rust
fn fibonacci(n: i32) -> i32 {
    let mut a: i32 = 0;
    let mut b: i32 = 1;
    let mut i: i32 = 0;
    while i < n {
        let temp: i32 = b;
        b = a + b;
        a = temp;
        i = i + 1;
    }
    return a;
}

fn main() -> i32 {
    print(fibonacci(10));
    return 0;
}
```

**Types:** `i32`, `f64`, `bool`
**Variables:** `let x: i32 = 5;` / `let mut y: i32 = 10;`
**Control flow:** `if`/`else`, `while`
**Operators:** `+ - * /`, `== != < > <= >=`, `&& ||`, `!`
**Built-in:** `print(expr);`
**Comments:** `// line comments`