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
    let left = usize(44);
    let right = usize(42);
    let primary = keep_second(left / usize(4), right % usize(13));
    let equal_values = keep_second(left / usize(4), spare(usize(50)));
    let named_left = rows(left);
    let named_right = spare(usize(50));
    let forwarded = keep_second(named_left, named_right);
    unit
}
