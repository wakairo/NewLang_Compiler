fn main() -> unit {
    let seed = usize(53);
    let flag = usize(1) == usize(2);
    let value = select_quantity(seed, flag);
    let echoed = value;
    echoed;
    seed;
    unit
}

fn select_quantity(input: usize, take_quotient: bool) -> usize {
    let copied = input;
    let selected = if (take_quotient) {
        copied / usize(7)
    } else {
        copied % usize(7)
    };
    let result_copy = selected;
    result_copy
}
