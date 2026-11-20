fn bad_capacity(rows: usize) -> usize {
    rows / usize(18446744073709551616)
}

fn main() -> unit {
    let result = bad_capacity(usize(35));
    unit
}
