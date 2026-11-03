fn select_equal(quantity: usize) -> u8 {
    let selected = quantity == usize(77);
    let answer = if (selected) { u8(129) } else { u8(222) };
    return answer;
}
fn main() -> unit {
    let matching = select_equal(usize(77));
    let smaller = select_equal(usize(76));
    let larger = select_equal(usize(78));
    return unit;
}
