fn main() -> usize {
    loop (quantity = usize(7)) {
        if (quantity > usize(0)) {
            let next_quantity = quantity - usize(3);
            continue(next_quantity);
        } else {
            break quantity;
        }
    }
}
