fn slots() -> usize {
    let columns = usize(37);
    let payload = columns * usize(29);
    let width = usize(31);
    payload / width
}

fn main() -> unit {
    let count = slots();
    let copy = count;
    copy;
    unit
}
