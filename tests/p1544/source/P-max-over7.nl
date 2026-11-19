// P1543 own FIRST: independent Unknown formal, computed quotient, caller quantity use.
fn quotient_once(amount: usize) -> usize {
    amount / usize(7)
}

fn main() -> unit {
    let groups = quotient_once(usize(18446744073709551615));
    let successor = groups + usize(1);
    let retained_quotient = groups;
    unit
}
