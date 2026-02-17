// Fibonacci sequence using while loop
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
