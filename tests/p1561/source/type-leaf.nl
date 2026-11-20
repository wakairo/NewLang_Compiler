fn bad_shares(points: u8) -> usize {
    points / usize(5)
}

fn main() -> unit {
    let result = bad_shares(u8(29));
    unit
}
