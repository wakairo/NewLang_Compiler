fn slots() -> usize {
    let payload = usize(3) * usize(6148914691236517205);
    payload / usize(3)
}

fn main() -> unit {
    let count = slots();
    let copy = count;
    copy;
    unit
}
