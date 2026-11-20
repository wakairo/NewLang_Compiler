fn bad_phase(rows: usize) -> usize {
    rows % usize(0)
}

fn main() -> unit {
    let phase = bad_phase(usize(35));
    unit
}
