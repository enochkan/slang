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

### Primitives & Variables
```rust
let x: i32 = 5;
let mut y: f64 = 3.14;
let flag: bool = true;
```

### Control Flow
```rust
if x > 0 { print(x); } else { print(0); }

while x < 10 { x = x + 1; }

for i in 0..10 { print(i); }
```

### Functions
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
```

### Arrays
```rust
let arr: [i32; 5] = [10, 20, 30, 40, 50];
let mut total: i32 = 0;
for i in 0..5 {
    total = total + arr[i];
}
arr[0] = 99;   // mutable array element write
```

### Structs & SoA Arrays
Struct arrays use **Struct of Arrays** memory layout automatically — all `x` values are
stored contiguously, enabling cache-friendly iteration and auto-vectorisation.

```rust
struct Vec2 {
    x: f64,
    y: f64,
}

fn main() -> i32 {
    // Scalar struct
    let p: Vec2 = Vec2 { x: 3.0, y: 4.0 };
    print(p.x);   // 3.000000

    // SoA array: layout is { [N x f64], [N x f64] } not [N x {f64, f64}]
    let mut points: [Vec2; 4];
    points[0].x = 1.0;  points[0].y = 10.0;
    points[1].x = 2.0;  points[1].y = 20.0;

    let mut sum: f64 = 0.0;
    for i in 0..4 {
        sum = sum + points[i].x;   // iterates xs[0..4] contiguously
    }
    print(sum);
    return 0;
}
```

**Primitive types:** `i32`, `f64`, `bool`
**Operators:** `+ - * /`, `== != < > <= >=`, `&& || !`
**Built-in:** `print(expr);`
**Comments:** `// line comments`