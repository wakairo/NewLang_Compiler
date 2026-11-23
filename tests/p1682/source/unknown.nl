fn measure_grid() -> usize {
    let bound = usize(90);
    let flag = usize(63) < bound;
    let count = if (flag) { usize(23) } else { usize(29) };
    count * usize(11)
}

fn main() -> unit {
    let count = measure_grid();
    let copy = count;
    copy;
    unit
}
