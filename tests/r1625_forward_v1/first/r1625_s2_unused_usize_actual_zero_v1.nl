fn main() -> unit {
    let flag = usize(1) == usize(2);
    let value = ignore_quantity(flag, usize(9) / usize(0));
    value;
    flag;
    unit
}

fn ignore_quantity(take_quotient: bool, ignored: usize) -> usize {
    let copied = usize(29);
    let selected = if (take_quotient) {
        copied / usize(7)
    } else {
        copied % usize(7)
    };
    selected
}
