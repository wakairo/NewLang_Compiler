struct Prism { upper: u8, lower: u8 }

fn choose(n: usize) -> u8 {
    if (n == usize(6)) { u8(83) } else { u8(142) }
}

fn main() -> u8 {
    let original = Prism { lower: u8(199), upper: u8(31) };
    let result = loop (n = usize(4)) {
        if (usize(1)) {
            continue(n + usize(3));
        } else {
            break n;
        }
    };
    let copied = result;
    let selected = choose(copied);
    let old = loan_write(original@upper) { |w| replace(w, selected) };
    original@upper
}
