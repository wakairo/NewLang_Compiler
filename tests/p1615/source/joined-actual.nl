fn carry_right(other: usize, payload: usize) -> usize {
    let taken = payload;
    taken
}

fn relay(stock: usize, reserve: usize) -> usize {
    let selected = if (stock < usize(20)) {
        stock
    } else {
        reserve
    };
    let tracked = selected;
    carry_right(reserve % usize(11), tracked)
}

fn main() -> unit {
    let total = relay(usize(19), usize(33));
    let backup = relay(usize(29), usize(41));
    let residue = backup % usize(10);
    unit
}
