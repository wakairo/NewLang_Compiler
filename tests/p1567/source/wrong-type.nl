fn invalid(input: u8) -> usize {
    let alias = input;
    alias / usize(6)
}

fn main() -> unit {
    let value = invalid(u8(29));
    unit
}
