fn main() -> unit {
    let a = usize(33);
    let b = usize(7);
    let first = smaller_piece(a, b);
    let second = smaller_piece(b, a);
    let kept = a / usize(1);
}

fn smaller_piece(a: usize, b: usize) -> usize {
    let left = a % usize(7);
    let right = b / usize(3);
    if (left < right) {
        left
    } else {
        right
    }
}
