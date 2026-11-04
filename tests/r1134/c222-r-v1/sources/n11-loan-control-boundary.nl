fn main() -> unit {
    let cell = u8(61);
    loop (quantity = usize(1)) {
        loan_write(cell) { |slot|
            continue(quantity);
        };
        break unit;
    }
}
