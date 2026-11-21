fn rows(input: usize) -> usize {
    input / usize(4)
}

fn spare(input: usize) -> usize {
    input % usize(13)
}

fn keep_second(ignored: usize, chosen: usize) -> usize {
    let stored = chosen;
    return stored;
}

fn main() -> unit {
    let primary = keep_second(rows(usize(44)), spare(usize(42)));
    let secondary = keep_second(spare(usize(50)), rows(usize(28)));
    let equal_values = keep_second(rows(usize(44)), spare(usize(50)));
    unit
}
