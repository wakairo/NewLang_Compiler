fn residue_six(amount: usize) -> usize {
    let rest = amount % usize(0);
    let rest_copy = rest;
    rest_copy
}

fn main() -> unit {
    let cargo = usize(38);
    let quotient = split_seventeen(cargo);
    let residual = residue_six(cargo);
    let forwarded = residue_six(quotient);
    let quotient_copy = quotient;
    let residual_copy = residual;
    let forwarded_copy = forwarded;
    unit
}

fn split_seventeen(amount: usize) -> usize {
    let share = amount / usize(17);
    return share;
}
