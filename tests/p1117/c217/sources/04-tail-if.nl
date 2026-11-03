fn final_choice(key: usize) -> u8 {
    if (key == usize(1027)) { u8(146) } else { u8(223) }
}
fn main() -> unit {
    let a = final_choice(usize(1));
    let b = final_choice(usize(1027));
    let c = final_choice(usize(1028));
    return unit;
}
