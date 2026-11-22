fn unsupported_return_operation(total: usize, take_early: bool) -> usize {
    let selected = if (take_early) {
        let incremented = usize(8) + usize(1);
        return incremented;
    } else {
        let residue = total % usize(9);
        residue
    };
    selected
}

fn main() -> unit {
    let total = usize(86);
    let early = total < usize(100);
    let ordinary = total < usize(30);
    let first = unsupported_return_operation(total, early);
    let second = unsupported_return_operation(total, ordinary);
    first;
    second;
}
