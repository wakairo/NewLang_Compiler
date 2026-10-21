struct Compass {
    phase: u8,
    anchor: u8,
}

fn main() -> unit {
    let c = Compass { anchor: u8(23), phase: u8(74) };
    let old = if (usize(9) < usize(2)) {
        loan_write(c@phase) { |w| replace(w, u8(106)) }
    } else {
        c@phase
    };
    let current = c@phase;
    let sibling = c@anchor;
    unit
}
