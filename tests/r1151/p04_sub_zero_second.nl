struct Prism { upper: u8, lower: u8 }

fn choose(n: usize) -> u8 {
    if (n == usize(6)) { u8(83) } else { u8(142) }
}

fn main() -> u8 {
    let original = Prism { lower: u8(211), upper: u8(61) };
    let before = original@lower;
    let result = loop (n = usize(9)) {
        if (n > usize(9)) {
            continue(n - usize(2));
        } else {
            break n;
        }
    };
    let copied = result;
    let selected = choose(copied);
    let old = loan_write(original@lower) { |w| replace(w, selected) };
    let updated = original@lower;
    let sibling = original@upper;
    sibling
}
