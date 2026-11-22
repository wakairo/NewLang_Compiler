fn scale(amount: usize) -> usize {
    amount / usize(3)
}

fn pick_band(use_base: bool, amount: usize) -> usize {
    let joined = if (use_base) {
        let broad = amount / usize(7);
        broad
    } else {
        let narrow = amount % usize(11);
        narrow
    };
    let carried = joined;
    carried
}

fn main() -> unit {
    let incoming = usize(93);
    let high = incoming < usize(100);
    let selected = pick_band(high, scale(incoming));
    let flag = selected < usize(3);
    let later = pick_band(flag, scale(usize(156)));
    let named = later;
    selected;
    named;
}
