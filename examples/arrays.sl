// Demonstrates: array literals, for loops, array indexing, array mutation

fn sum_array() -> i32 {
    let arr: [i32; 5] = [10, 20, 30, 40, 50];
    let mut total: i32 = 0;
    for i in 0..5 {
        total = total + arr[i];
    }
    return total;
}

fn dot_product() -> i32 {
    let mut a: [i32; 4] = [1, 2, 3, 4];
    let b: [i32; 4] = [4, 3, 2, 1];
    let mut result: i32 = 0;
    // mutate a[0] to test array assignment
    a[0] = 10;
    for i in 0..4 {
        result = result + a[i] * b[i];
    }
    return result;
}

fn main() -> i32 {
    print(sum_array());    // 150
    print(dot_product());  // 10*4 + 2*3 + 3*2 + 4*1 = 40+6+6+4 = 56
    return 0;
}
