// Control flow: if/else and while with multiple functions
fn abs(x: i32) -> i32 {
    if x < 0 {
        return -x;
    } else {
        return x;
    }
}

fn countdown(n: i32) -> i32 {
    let mut i: i32 = n;
    while i > 0 {
        print(i);
        i = i - 1;
    }
    return 0;
}

fn main() -> i32 {
    print(abs(-7));
    countdown(5);
    return 0;
}
