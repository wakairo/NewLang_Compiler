fn select_bad(quantity: usize) -> u8 {
    let answer = if (quantity == usize(77)) { u8(129) } else { u8(222) };
    answer
}
fn main() -> unit {
    let result = select_bad(u8(77));
    return unit;
}
