fn fixed(input: usize) -> usize {
    let constant = usize(29);
    constant / usize(4)
}

fn main() -> unit {
    let incoming = usize(87);
    let each = incoming / usize(3);
    let from_helper = fixed(each);
    unit
}
