fn increase(incoming: usize) -> usize {
    let divided = incoming / usize(3);
    divided + usize(2)
}

fn main() -> unit {
    let result = increase(usize(92));
    unit
}
