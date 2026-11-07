struct Dial { marker: u8, offset: u8 }

fn select_for_turn(turn: usize) -> u8 {
    if (turn == usize(1)) {
        return u8(172);
    } else {
        let ordinary = u8(46);
        let copy_of_ordinary = ordinary;
        copy_of_ordinary
    }
}

fn main() -> unit {
    let dial = Dial { offset: u8(96), marker: u8(6) };
    let marker_before = dial@marker;
    let second_old = loop (turn = usize(0)) {
        if (turn == usize(0)) {
            let old0 = loan_write(dial@offset) { |w|
                replace(w, select_for_turn(turn))
            };
            continue(usize(1));
        } else {
            let old1 = loan_write(dial@offset) { |w|
                replace(w, select_for_turn(turn))
            };
            break old1;
        }
    };
    let offset_after = dial@offset;
    let marker_after = dial@marker;
    return unit;
}
