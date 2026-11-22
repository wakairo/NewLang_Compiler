fn choose(payload: usize, flag: bool) -> usize {
    if (flag) {
        payload
    } else {
        usize(0)
    }
}
fn main() -> unit {
    let count = usize(18);
    let test = count < usize(28);
    let value = choose(test, count);
    unit
}
