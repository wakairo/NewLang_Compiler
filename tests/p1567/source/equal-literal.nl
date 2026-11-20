fn count_from(input: usize) -> usize {
    let anchor = usize(29);
    let local = anchor;
    local / usize(6)
}

fn main() -> unit {
    let count = count_from(usize(29));
    unit
}
