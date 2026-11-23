fn main() -> unit {
    let bound = usize(90);
    let flag = usize(63) < bound;
    let count = if (flag) { usize(23) } else { usize(29) };
    let bad = count * usize(11);
    unit
}
