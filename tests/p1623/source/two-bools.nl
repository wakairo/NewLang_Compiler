fn two_flags(left: bool, right: bool) -> usize {
    if (left) {
        usize(11)
    } else {
        usize(7)
    }
}
fn main() -> unit {
    let count = usize(17);
    let value = two_flags(count < usize(36), count < usize(3));
    unit
}
