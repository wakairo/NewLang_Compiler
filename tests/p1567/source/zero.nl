fn invalid(input: usize) -> usize {
    let alias = input;
    alias % usize(0)
}

fn main() -> unit {
    let value = invalid(usize(11));
    unit
}
