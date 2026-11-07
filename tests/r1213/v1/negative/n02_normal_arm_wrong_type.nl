struct Pulse { live: u8, sentinel: u8 }

fn select_for_pass(pass: usize) -> u8 {
    if (pass == usize(0)) {
        return u8(37);
    } else {
        let ordinary = u8(83);
        let copy_of_ordinary = ordinary;
        usize(83)
    }
}

fn main() -> unit {
    let cell = Pulse { live: u8(149), sentinel: u8(211) };
    let sentinel_before = cell@sentinel;
    let last_old = loop (pass = usize(0)) {
        if (pass == usize(0)) {
            let first_old = loan_write(cell@live) { |w|
                replace(w, select_for_pass(pass))
            };
            continue(usize(1));
        } else {
            let second_old = loan_write(cell@live) { |w|
                replace(w, select_for_pass(pass))
            };
            break second_old;
        }
    };
    let live_after = cell@live;
    let sentinel_after = cell@sentinel;
    return unit;
}
