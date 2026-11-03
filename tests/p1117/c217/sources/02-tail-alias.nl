fn decide(token: usize) -> u8 {
    let full_width = token;
    let gate = full_width == usize(769);
    let output = if (gate) { u8(143) } else { u8(218) };
    let forwarded = output;
    forwarded
}
fn main() -> unit {
    let a = decide(usize(1));
    let b = decide(usize(769));
    let c = decide(usize(770));
    return unit;
}
