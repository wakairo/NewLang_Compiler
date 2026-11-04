struct Prism { upper: u8, lower: u8 }

fn choose(n: usize) -> u8 {
    if (n == usize(6)) { u8(83) } else { u8(142) }
}

fn main() -> u8 {
    let original = Prism { lower: u8(197), upper: u8(29) };
    let before = original@upper;
    let result = loop (n = usize(6)) {
        if (n < usize(6)) {
            continue(n + usize(2));
        } else {
            break n;
        }
    };
    let copied = result;
    let selected = choose(copied);
    let old = loan_write(original@upper) { |w| replace(w, selected) };
    let updated = original@upper;
    let sibling = original@lower;
    old
}
