fn slots() -> usize {
    let columns = usize(37);
    let payload = columns * usize(29);
    let used = payload;
    used % usize(31)
}

fn main() -> unit {
    let count = slots();
    let copy = count;
    copy;
    unit
}
