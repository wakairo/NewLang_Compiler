fn select(payload: usize, flag: bool) -> usize {
    let original = payload;
    let saved = if (flag) {
        original
    } else {
        original % usize(0)
    };
    saved
}
fn main() -> unit {
    let count = usize(18);
    let value = select(count, count < usize(28));
    unit
}
