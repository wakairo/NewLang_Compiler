struct Beacon {
    witness: u8,
    impulse: u8,
}

fn main() -> unit {
    let b = Beacon { impulse: u8(19), witness: u8(61) };
    let old = if (true) {
        loan_write(b@impulse) { |w| replace(w, true) }
    } else {
        loan_write(b@impulse) { |w| replace(w, u8(83)) }
    };
    let current = b@impulse;
    let sibling = b@witness;
    unit
}
