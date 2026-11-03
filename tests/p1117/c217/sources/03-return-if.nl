fn sample_decision(gauge: usize) -> u8 {
    return if (gauge < usize(609)) { u8(161) } else { u8(227) };
}
fn main() -> unit {
    let a = sample_decision(usize(608));
    let b = sample_decision(usize(609));
    let c = sample_decision(usize(610));
    return unit;
}
