// Type demonstrations: i32, f64, bool
fn check_positive(x: i32) -> bool {
    return x > 0;
}

fn main() -> i32 {
    // Integer arithmetic
    let a: i32 = 10;
    let b: i32 = 3;
    print(a + b);
    print(a * b);

    // Float arithmetic
    let pi: f64 = 3.14;
    let r: f64 = 2.0;
    print(pi * r * r);

    // Boolean logic
    let result: bool = check_positive(5);
    print(result);

    return 0;
}
