fn main() -> unit {
    let cell = u8(61);
    let escaped = loan_write(cell) { |slot|
        loop (quantity = usize(2)) {
            if (quantity > usize(0)) {
                let next_quantity = quantity - usize(1);
                continue(next_quantity);
            } else {
                break slot;
            }
        }
    };
    unit
}
