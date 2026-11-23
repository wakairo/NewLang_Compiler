fn slots() -> usize {
    let columns = usize(37);
    let payload = columns * usize(29);
    let left = payload % usize(31);
    let copy = left;
    copy
}

fn main() -> unit {
    let count = slots();
    let copy = count;
    copy;
    unit
}
