fn advance_clamped(value: usize) -> usize {
    if (value < usize(9)) {
        value + usize(1)
    } else {
        value
    }
}

fn main() -> usize {
    loop (quantity = usize(3)) {
        let next_quantity = advance_clamped(quantity);
        if (quantity < usize(7)) {
            continue(next_quantity);
        } else {
            break quantity;
        }
    }
}
