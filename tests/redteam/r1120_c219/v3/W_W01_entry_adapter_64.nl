fn r1120_width_gate(observed: usize) -> bool {
    observed < usize(4294967296)
}

fn main() -> unit {
    let sample_0 = r1120_width_gate(usize(4294967295));
    let sample_1 = r1120_width_gate(usize(4294967296));
    let sample_2 = r1120_width_gate(usize(18446744073709551615));
    unit
}
