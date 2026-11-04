struct Prism { upper: u8, lower: u8 }

fn choose(n: usize) -> u8 {
    if (n == usize(6)) { u8(83) } else { u8(142) }
}

fn main() -> u8 {
    let original = Prism { lower: u8(193), upper: u8(37) };
    let before = original@lower;
    let result = loop (n = usize(4)) {
        if (n < usize(7)) {
            continue(n + usize(3));
        } else {
            break n;
        }
    };
    let copied = result;
    let selected = choose(copied);
    let old = loan_write(original@lower) { |w| replace(w, selected) };
    let updated = original@lower;
    let sibling = original@upper;
    updated
}
