fn half_left(a: usize, unused: usize) -> usize {
    a / usize(2)
}

fn main() -> unit {
    let left = usize(35);
    let right = usize(26);
    let answer = half_left(left % usize(9), right / usize(4));
    let kept = right % usize(5);
}
