fn slots() -> usize {
    let columns = usize(37);
    let payload = columns * usize(29);
    let used = payload / usize(31);
    let other = usize(34);
    other
}

fn main() -> unit {
    let count = slots();
    let copy = count;
    copy;
    unit
}
