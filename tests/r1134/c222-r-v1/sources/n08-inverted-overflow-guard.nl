fn inverted_guard(seed: usize) -> usize {
    loop (quantity = seed) {
        if (quantity > usize(6)) {
            let next_quantity = quantity + usize(2);
            continue(next_quantity);
        } else {
            break quantity;
        }
    }
}
