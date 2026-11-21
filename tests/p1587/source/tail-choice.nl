fn main() -> unit {
    let at = choose(usize(42));
    let away = choose(usize(68));
    let count = at / usize(2);
    let retained = forward(away);
    unit
}

fn choose(amount: usize) -> usize {
    let matches = amount == usize(42);
    let picked = matches;
    if (picked) {
        let saved = amount;
        saved
    } else {
        usize(91)
    }
}

fn forward(amount: usize) -> usize {
    let spare = amount % usize(10);
    spare
}
