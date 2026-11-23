fn slots() -> usize {
    let payload = usize(37) * usize(0);
    payload / usize(31)
}

fn main() -> unit {
    let count = slots();
    let copy = count;
    copy;
    unit
}
