fn choose(payload: usize, flag: bool) -> usize {
    if (flag) {
        payload
    } else {
        usize(0)
    }
}
fn main() -> unit {
    let value = choose(usize(18), usize(1));
    unit
}
