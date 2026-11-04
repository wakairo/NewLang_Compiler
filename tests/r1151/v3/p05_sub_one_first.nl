struct Prism { upper: u8, lower: u8 }

fn choose(n: usize) -> u8 {
    if (n == usize(6)) { u8(83) } else { u8(142) }
}

fn main() -> unit {
    let original = Prism { lower: u8(227), upper: u8(109) };
    let before = original@upper;
    let result = loop (n = usize(8)) {
        if (n < usize(7)) {
            break n;
        } else {
            continue(n - usize(2));
        }
    };
    let copied = result;
    let selected = choose(copied);
    let old = loan_write(original@upper) { |w| replace(w, selected) };
    let updated = original@upper;
    let sibling = original@lower;
    let terminal_copy = updated;
    unit
}
