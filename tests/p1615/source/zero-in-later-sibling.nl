fn pieces(input: usize) -> usize {
    input / usize(4)
}

fn first_of(first: usize, ignored: usize) -> usize {
    first
}

fn main() -> unit {
    let source = usize(59);
    let total = first_of(pieces(usize(44)), source / usize(0));
    unit
}
