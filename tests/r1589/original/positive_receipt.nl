fn third(value: usize) -> usize {
    value / usize(3)
}

fn echo(value: usize) -> usize {
    let kept = value;
    kept
}

fn bucket(amount: usize) -> usize {
    if (amount == usize(0)) {
        let exactzero = usize(6) - usize(6);
        exactzero
    } else {
        third(amount)
    }
}

fn receipt(amount: usize) -> usize {
    let joined = bucket(amount);
    let copied = joined;
    let quotient = copied / usize(4);
    echo(quotient % usize(9))
}

fn main() -> unit {
    let zero = receipt(usize(0));
    let ordinary = receipt(usize(84));
    let collision = receipt(usize(108));
    unit
}
