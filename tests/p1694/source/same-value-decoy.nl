fn slots() -> usize {
    let columns = usize(37);
    let payload = columns * usize(29);
    let other = usize(1073);
    other / usize(31)
}

fn main() -> unit {
    let count = slots();
    let copy = count;
    copy;
    unit
}
